// Port of lib/handy_output/HandyOutput.cpp's HAMP state machine to JS, since
// Handy control now happens from here (this laptop has real internet) rather
// than the ESP32, which only talks to us over USB serial.
//
// Base URL and auth confirmed against the real Handy API v2 docs (see
// apidocs.md at the repo root) - this used to point at a nonexistent
// "handy-rest/v3" path, which is why every request failed regardless of the
// connection key. Auth is X-Connection-Key only; there is no Application Key
// in this API (the optional X-Api-Key header below is harmless dead capacity).
//
// IMPORTANT: this API returns HTTP 200 even on failure, with the real error
// embedded in the JSON body as {"error": {code, name, message, connected}} -
// confirmed by hitting it directly with a bogus key. Every request here must
// check the body, not just the status code, or failures look like success.

const HANDY_API_BASE = 'https://www.handyfeeling.com/api/handy/v2';
const CONNECT_TIMEOUT_MS = 8000;

// Per apidocs.md's Error Handling table (for transport-level failures - DNS,
// timeout, or an actual non-2xx status, as opposed to the 200-with-error-body
// case handled separately via response.error).
function describeStatus(status) {
  switch (status) {
    case -1:
      return "could not reach handyfeeling.com - check this computer's internet connection";
    case 400:
      return 'bad request (HTTP 400)';
    case 401:
      return 'invalid connection key (HTTP 401)';
    case 404:
      return 'device not found - is the Handy powered on and connected to WiFi/the Handy app? (HTTP 404)';
    case 429:
      return "rate limited by Handy's API - try again shortly (HTTP 429)";
    case 500:
      return 'Handy server error (HTTP 500)';
    case 503:
      return 'device offline or unavailable (HTTP 503)';
    default:
      return `unexpected response (HTTP ${status})`;
  }
}

export class HandyClient {
  constructor() {
    this.connectionKey = '';
    this.appKey = ''; // optional - see HandyOutput.h's class comment for why
    this.hampModeActive = false;
    this.hampStarted = false;
    this.connected = false;
    this.lastPercent = 255; // last velocity % actually sent; 255 = sentinel "never set"
    this.lastError = null; // human-readable reason for the last failure, if any
  }

  setKeys(connectionKey, appKey) {
    this.connectionKey = connectionKey || '';
    this.appKey = appKey || '';
    this.hampModeActive = false;
    this.hampStarted = false;
    this.connected = false;
    this.lastPercent = 255;
  }

  isConfigured() {
    return this.connectionKey.length > 0;
  }

  isConnected() {
    return this.connected;
  }

  // Returns { status, body, ok } - ok is true only when the transport
  // succeeded (2xx) AND the body carries no {"error": ...}.
  async _request(method, path, requestBody) {
    const headers = {
      Accept: 'application/json',
      'Content-Type': 'application/json',
      'X-Connection-Key': this.connectionKey,
    };
    if (this.appKey) {
      headers['X-Api-Key'] = this.appKey;
    }

    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), CONNECT_TIMEOUT_MS);
    try {
      const res = await fetch(`${HANDY_API_BASE}${path}`, {
        method,
        headers,
        body: requestBody !== undefined ? JSON.stringify(requestBody) : undefined,
        signal: controller.signal,
      });

      let body = null;
      try {
        body = await res.json();
      } catch {
        // no/invalid JSON body - fine for endpoints with empty responses
      }

      const ok = res.status >= 200 && res.status < 300 && !body?.error;
      return { status: res.status, body, ok };
    } catch {
      return { status: -1, body: null, ok: false };
    } finally {
      clearTimeout(timeout);
    }
  }

  // Human-readable reason for a failed request, preferring the API's own
  // error message when the request transported fine but the body said no.
  _explain({ status, body }) {
    if (body?.error) {
      return `${body.error.name || 'error'}: ${body.error.message || 'unknown'}`;
    }
    return describeStatus(status);
  }

  // PUT /slide - sets the physical stroke range as a percentage of full travel
  // (0-100, min < max). Applies across all modes, independent of HAMP setup -
  // see apidocs.md's SLIDE section.
  async setSlideZone(min, max) {
    if (!this.isConfigured()) {
      return false;
    }
    const result = await this._request('PUT', '/slide', { min, max });
    if (!result.ok) {
      this.lastError = this._explain(result);
      console.log(`[handy] failed to set stroke zone [${min}, ${max}]: ${this.lastError}`);
    }
    return result.ok;
  }

  // Stops HAMP and resets its started/mode-active flags, so the next
  // setVibrationLevel() call renegotiates from scratch (setHampMode +
  // startHamp) instead of assuming HAMP is still running - used before a
  // stroke zone change, since HAMP may only read the zone when it (re)starts.
  async stopAndResetHamp() {
    if (this.hampStarted) {
      await this._stopHamp();
    }
    this.hampStarted = false;
    this.hampModeActive = false;
    this.lastPercent = 0;
  }

  // PUT /slide - sets the physical stroke range as a percentage of full travel
  // (0-100, min < max). Applies across all modes, independent of HAMP setup -
  // see apidocs.md's SLIDE section.
  async setSlideZone(min, max) {
    if (!this.isConfigured()) {
      return false;
    }
    const result = await this._request('PUT', '/slide', { min, max });
    if (!result.ok) {
      this.lastError = this._explain(result);
      console.log(`[handy] failed to set stroke zone [${min}, ${max}]: ${this.lastError}`);
    }
    return result.ok;
  }

  // Verify connectivity and switch to HAMP mode. Call after setKeys().
  async begin() {
    if (!this.isConfigured()) {
      return false;
    }

    const result = await this._request('GET', '/connected');

    let error = null;
    if (!result.ok) {
      error = this._explain(result);
    } else if (result.body?.connected !== true) {
      // Transport succeeded and the body had no {"error": ...}, but the device
      // itself reports not connected - most likely an invalid connection key,
      // or the Handy is powered off / not linked to the app right now.
      error = 'invalid connection key, or the Handy is not powered on and linked to the app';
    }

    if (error) {
      this.lastError = error;
      console.log(`[handy] not connected: ${this.lastError}`);
      this.connected = false;
      return false;
    }

    this.lastError = null;
    this.connected = true;
    return this._setHampMode();
  }

  async _setHampMode() {
    const result = await this._request('PUT', '/mode', { mode: 0 });
    this.hampModeActive = result.ok;
    if (!this.hampModeActive) {
      this.lastError = this._explain(result);
      console.log(`[handy] failed to set HAMP mode: ${this.lastError}`);
    }
    return this.hampModeActive;
  }

  async _startHamp() {
    const result = await this._request('PUT', '/hamp/start', {});
    if (result.ok) {
      this.hampStarted = true;
      this.lastPercent = 0;
      return true;
    }
    this.lastError = this._explain(result);
    console.log(`[handy] failed to start HAMP: ${this.lastError}`);
    return false;
  }

  async _stopHamp() {
    const result = await this._request('PUT', '/hamp/stop', {});
    return result.ok;
  }

  async _applyVelocity(percent) {
    const result = await this._request('PUT', '/hamp/velocity', { velocity: percent });
    if (result.ok) {
      this.lastPercent = percent;
      return true;
    }

    // A body error (e.g. device dropped out of HAMP state) means we should
    // renegotiate mode/start next time rather than assume it's still active.
    this.hampModeActive = false;
    this.hampStarted = false;
    this.lastError = this._explain(result);
    console.log(`[handy] setVelocity failed: ${this.lastError}`);
    return false;
  }

  // speed: 0-255, ArousalManager's full-resolution raw speed (see
  // SerialDeviceOutput.h's class comment on the firmware side) - mapped
  // directly to Handy's native 0-100% velocity, no 20-level quantization.
  async setVibrationLevel(speed) {
    if (!this.isConfigured()) {
      return false;
    }

    // Handle "stop" as a direct best-effort call up front, before any mode/start
    // negotiation - if hampStarted/hampModeActive are stale-false (e.g. reset by
    // the zone randomizer's out-of-band stopAndResetHamp()), going through the
    // normal negotiation path below would actually *start* HAMP motion before
    // realizing there was nothing to do, instead of just stopping. A stop must
    // never be skipped just because our cache already thinks we're at 0.
    if (speed === 0) {
      const ok = await this._stopHamp();
      this.hampStarted = false;
      this.lastPercent = 0;
      return ok;
    }

    if (!this.hampModeActive && !(await this._setHampMode())) {
      return false;
    }
    if (!this.hampStarted && !(await this._startHamp())) {
      return false;
    }

    const velocityPercent = Math.max(0, Math.min(100, Math.round((speed / 255) * 100)));
    if (velocityPercent === this.lastPercent) {
      return true; // nothing to do
    }

    return this._applyVelocity(velocityPercent);
  }
}
