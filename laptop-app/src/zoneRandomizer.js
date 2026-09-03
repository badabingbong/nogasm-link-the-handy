// Periodically picks a random sub-zone within a configured outer stroke range
// and applies it via HandyClient.setSlideZone(), so the physically-used range
// shifts around over time instead of staying fixed. Independent of the
// arousal/serial pipeline - purely a Handy stroke-range effect.
//
// Each shift briefly stops HAMP first (pauseSeconds) before setting the new
// zone - HAMP may only read the stroke zone when it (re)starts, so changing
// it while continuously running could otherwise go unnoticed by the device.
// Movement resumes naturally the next time the normal arousal pipeline sends
// a vibrate command - this module doesn't own starting/stopping movement.

const MIN_INTERVAL_SECONDS = 3;
const DEFAULT_PAUSE_SECONDS = 1;

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function pickRandomSubZone(outerMin, outerMax, subZoneWidth) {
  const range = Math.max(0, outerMax - outerMin);
  const width = Math.min(Math.max(0, subZoneWidth), range);
  const maxStart = outerMax - width;
  const start = outerMin + Math.random() * Math.max(0, maxStart - outerMin);
  return { min: Math.round(start), max: Math.round(start + width) };
}

export class ZoneRandomizer {
  constructor(handyClient) {
    this.handy = handyClient;
    this.zoneConfig = null;
    this.timer = null;
  }

  // zoneConfig: { enabled, outerMin, outerMax, subZoneWidth, intervalSeconds, pauseSeconds }
  setZoneConfig(zoneConfig) {
    this.zoneConfig = zoneConfig;
    this._restart();
  }

  _restart() {
    if (this.timer) {
      clearTimeout(this.timer);
      this.timer = null;
    }

    if (!this.zoneConfig?.enabled || !this.handy.isConfigured()) {
      return;
    }

    this._tick();
  }

  async _tick() {
    if (!this.zoneConfig?.enabled) {
      return;
    }

    const { outerMin, outerMax, subZoneWidth, intervalSeconds, pauseSeconds } = this.zoneConfig;
    const zone = pickRandomSubZone(outerMin, outerMax, subZoneWidth);

    const pauseMs = Math.max(0, pauseSeconds ?? DEFAULT_PAUSE_SECONDS) * 1000;
    if (pauseMs > 0) {
      await this.handy.stopAndResetHamp();
      await sleep(pauseMs);
    }

    await this.handy.setSlideZone(zone.min, zone.max);

    const delayMs = Math.max(MIN_INTERVAL_SECONDS, intervalSeconds || MIN_INTERVAL_SECONDS) * 1000;
    this.timer = setTimeout(() => this._tick(), delayMs);
  }

  stop() {
    if (this.timer) {
      clearTimeout(this.timer);
      this.timer = null;
    }
  }
}
