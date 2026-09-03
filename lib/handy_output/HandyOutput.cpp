#include "HandyOutput.h"
#include <Util.h>

HandyOutput::HandyOutput(const String& connectionKey, const String& appKey) : _connectionKey(connectionKey), _appKey(appKey)
{
}

void HandyOutput::setKeys(const String& connectionKey, const String& appKey)
{
  _connectionKey = connectionKey;
  _appKey = appKey;
  _hampModeActive = false;
  _hampStarted = false;
  _connected = false;
  _lastPercent = 255;
}

// ---------------------------------------------------------------------------
// IDeviceOutput interface
// ---------------------------------------------------------------------------

bool HandyOutput::isConnectedState() const
{
  // Deliberately doesn't require _hampModeActive/_hampStarted: those are lazily
  // negotiated inside setVibrationLevel() the first time it's called, and callers
  // (e.g. ArousalManager) only call setVibrationLevel() once this returns true -
  // requiring them here would mean it can never become true.
  return _connected;
}

bool HandyOutput::setVibrationLevel(const uint8_t speed)
{
  if (!isConfigured())
  {
    return false;
  }

  // Handle "stop" as a direct best-effort call up front, before any mode/start
  // negotiation - if the cached _hampStarted/_hampModeActive flags are stale
  // (e.g. the device was stopped by something else out-of-band), going through
  // the normal negotiation path below would actually *start* HAMP motion
  // before realizing there was nothing to do, instead of just stopping. A
  // stop must never be skipped just because our cache already thinks we're at 0.
  if (speed == 0)
  {
    const bool ok = stopHamp();
    _hampStarted = false;
    _lastPercent = 0;
    return ok;
  }

  // Lazily initialise HAMP mode if needed
  if (!_hampModeActive)
  {
    if (!setHampMode())
      return false;
  }
  if (!_hampStarted)
  {
    if (!startHamp())
      return false;
  }

  // Map 0-255 (ArousalManager's full-resolution raw speed) directly to 0-100
  // (Handy velocity %) - no 20-level quantization, see class comment in HandyOutput.h.
  const uint8_t velocityPercent = static_cast<uint8_t>(constrain(map(speed, 0, 255, 0, 100), 0, 100));

  if (velocityPercent == _lastPercent)
  {
    return true;  // nothing to do
  }

  return applyVelocity(velocityPercent);
}

void HandyOutput::update()
{
  // Periodically retry initialisation if not connected
  if (!isConnectedState() && isConfigured())
  {
    if (millis() - _lastRetryMs > RETRY_INTERVAL_MS)
    {
      _lastRetryMs = millis();
      begin();
    }
  }
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

bool HandyOutput::begin()
{
  if (!isConfigured())
  {
    Util::logInfo("HandyOutput: no credentials configured — skipping init");
    return false;
  }

  Util::logInfo("HandyOutput: checking connectivity...");

  // GET /connected returns {"connected": bool} - purpose-built for this check,
  // and correctly reflects a wrong key / offline device via that field rather
  // than an HTTP error status (see class comment in HandyOutput.h).
  const HandyApiResult result = apiGet("/connected");
  if (!result.ok || !result.connectedField)
  {
    _lastError = result.errorMessage;
    Util::logInfo("HandyOutput: not connected — %s", _lastError.c_str());
    _connected = false;
    return false;
  }

  _lastError = "";
  _connected = true;
  Util::logInfo("HandyOutput: device reachable, switching to HAMP mode...");

  return setHampMode();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void HandyOutput::setupClient(HTTPClient& http, const String& path) const
{
  http.begin(String(HANDY_API_BASE) + path);
  http.setConnectTimeout(HANDY_CONNECT_TIMEOUT_MS);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Connection-Key", _connectionKey);
  if (_appKey.length() > 0)
  {
    http.addHeader("X-Api-Key", _appKey);
  }
}

HandyApiResult HandyOutput::parseResponse(const int status, const String& body)
{
  HandyApiResult result;
  result.status = status;

  JsonDocument doc;
  const bool parsed = !body.isEmpty() && !deserializeJson(doc, body);

  if (parsed && doc["error"].is<JsonObject>())
  {
    const char* name = doc["error"]["name"] | "error";
    const char* message = doc["error"]["message"] | "unknown";
    result.ok = false;
    result.errorMessage = String(name) + ": " + String(message);
    return result;
  }

  if (status < 200 || status >= 300)
  {
    result.ok = false;
    result.errorMessage = "HTTP " + String(status);
    return result;
  }

  result.ok = true;
  if (parsed && !doc["connected"].isNull())
  {
    result.connectedField = doc["connected"].as<bool>();
  }
  return result;
}

HandyApiResult HandyOutput::apiPut(const String& path, const String& jsonBody)
{
  HTTPClient http;
  setupClient(http, path);
  const int code = http.PUT(jsonBody);
  const String responseBody = code > 0 ? http.getString() : "";
  http.end();
  return parseResponse(code, responseBody);
}

HandyApiResult HandyOutput::apiGet(const String& path)
{
  HTTPClient http;
  setupClient(http, path);
  const int code = http.GET();
  const String responseBody = code > 0 ? http.getString() : "";
  http.end();
  return parseResponse(code, responseBody);
}

bool HandyOutput::setHampMode()
{
  // PUT /mode   body: {"mode": 0}   (0 = HAMP)
  const HandyApiResult result = apiPut("/mode", "{\"mode\":0}");
  _hampModeActive = result.ok;
  if (!_hampModeActive)
  {
    _lastError = result.errorMessage;
    Util::logInfo("HandyOutput: failed to set HAMP mode — %s", _lastError.c_str());
  }
  return _hampModeActive;
}

bool HandyOutput::startHamp()
{
  // PUT /hamp/start
  const HandyApiResult result = apiPut("/hamp/start", "{}");
  if (result.ok)
  {
    _hampStarted = true;
    _lastPercent = 0;
    return true;
  }
  _lastError = result.errorMessage;
  Util::logInfo("HandyOutput: failed to start HAMP — %s", _lastError.c_str());
  return false;
}

bool HandyOutput::stopHamp()
{
  // PUT /hamp/stop
  return apiPut("/hamp/stop", "{}").ok;
}

bool HandyOutput::applyVelocity(const uint8_t percent)
{
  // PUT /hamp/velocity   body: {"velocity": <0-100>}
  const String body = "{\"velocity\":" + String(percent) + "}";
  const HandyApiResult result = apiPut("/hamp/velocity", body);
  if (result.ok)
  {
    _lastPercent = percent;
    Util::logDebug("HandyOutput: velocity=%d%%", percent);
    return true;
  }

  // A body error (e.g. device dropped out of HAMP state) means we should
  // renegotiate mode/start next time rather than assume it's still active.
  _hampModeActive = false;
  _hampStarted = false;
  _lastError = result.errorMessage;
  Util::logInfo("HandyOutput: setVelocity failed — %s", _lastError.c_str());
  return false;
}
