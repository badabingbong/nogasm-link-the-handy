import express from 'express';

const CONFIG_WAIT_TIMEOUT_MS = 1000;
const CONFIG_WAIT_POLL_MS = 50;

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Express router replicating the REST contract lib/nogasm_http/NogasmHttp.cpp
 * used to serve over WiFi, so webapp/ works unmodified against this server.
 * BLE-specific routes are harmless stubs (no Lovense support in USB mode -
 * see main.cpp's ENABLE_BLE); arousal + Handy routes are real, backed by the
 * serial link to the ESP32 and the direct Handy REST client respectively.
 */
export function createApiRouter({ state, serial, handy, getConfig, setConfig, zoneRandomizer }) {
  const router = express.Router();
  router.use(express.json());

  // --- status / BLE stubs ---------------------------------------------------

  router.get('/status', (req, res) => {
    res.json({ wifi: state.wifi, ble: state.ble });
  });

  router.get('/devices', (req, res) => res.json([]));

  router.post('/scan', (req, res) => {
    res.json({ success: true, message: 'BLE is not used in USB serial mode' });
  });

  router.post('/disconnect', (req, res) => {
    res.json({ success: true });
  });

  router.get('/config', (req, res) => {
    res.json({
      connection: { scanDuration: 15000, connectionTimeout: 15000 },
      device: { defaultVibrationLevel: 0 },
      ui: { autoConnect: true, autoReconnect: true },
    });
  });

  router.post('/config', (req, res) => {
    res.json({ success: true, message: 'Nothing to change in USB serial mode' });
  });

  router.post('/reset-wifi', (req, res) => {
    res.json({ success: true, message: 'WiFi is not used in USB serial mode' });
  });

  // --- Handy -----------------------------------------------------------------

  router.get('/handy/config', (req, res) => {
    const config = getConfig();
    res.json({
      connectionKey: config.handy.connectionKey,
      appKey: config.handy.appKey,
      configured: handy.isConfigured(),
      connected: handy.isConnected(),
      zone: config.handy.zone,
    });
  });

  router.post('/handy/config', async (req, res) => {
    const { connectionKey = '', appKey = '', zone } = req.body || {};

    const config = getConfig();
    config.handy.connectionKey = connectionKey;
    config.handy.appKey = appKey;
    if (zone) {
      config.handy.zone = { ...config.handy.zone, ...zone };
    }
    setConfig(config);

    handy.setKeys(connectionKey, appKey);

    let connected = false;
    if (handy.isConfigured()) {
      connected = await handy.begin();
    }

    state.ble.handy.configured = handy.isConfigured();
    state.ble.handy.connected = connected;
    serial.send({ type: 'handy_status', connected });

    zoneRandomizer.setZoneConfig(config.handy.zone);

    res.json({
      success: true,
      connected,
      message: connected ? 'Handy connected' : `Saved, but could not reach the Handy - ${handy.lastError || 'unknown error'}`,
    });
  });

  // --- Arousal (real, over serial) --------------------------------------------

  router.get('/arousal/status', (req, res) => res.json(state.arousal));

  router.post('/arousal/state', (req, res) => {
    const sent = serial.send({ type: 'arousal_state', ...req.body });
    res.json({ success: sent, message: sent ? undefined : 'ESP32 not connected over serial' });
  });

  router.post('/arousal/sensitivity', (req, res) => {
    const sent = serial.send({ type: 'arousal_sensitivity', ...req.body });
    res.json({ success: sent, message: sent ? undefined : 'ESP32 not connected over serial' });
  });

  router.get('/arousal/config', async (req, res) => {
    if (!state.arousalConfig) {
      serial.send({ type: 'get_config' });
      const deadline = Date.now() + CONFIG_WAIT_TIMEOUT_MS;
      while (!state.arousalConfig && Date.now() < deadline) {
        await sleep(CONFIG_WAIT_POLL_MS);
      }
    }
    res.json(state.arousalConfig || {});
  });

  router.post('/arousal/config', (req, res) => {
    const sent = serial.send({ type: 'arousal_config', ...req.body });
    res.json({
      success: sent,
      message: sent ? 'Configuration sent to device' : 'ESP32 not connected over serial',
    });
  });

  return router;
}
