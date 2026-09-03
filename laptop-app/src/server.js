import express from 'express';
import http from 'node:http';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { SerialLink } from './serial.js';
import { HandyClient } from './handyClient.js';
import { ZoneRandomizer } from './zoneRandomizer.js';
import { loadConfig, saveConfig } from './config.js';
import { createApiRouter } from './api.js';
import { createWebSocketServer } from './websocket.js';
import { createState } from './state.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = process.env.PORT || 3000;
// Same build artifact the ESP32's LittleFS image uses (webapp/'s `yarn build`
// copies its output here) - one source of truth, no separate frontend build.
const DATA_DIR = path.join(__dirname, '..', '..', 'data');

const state = createState();
const config = loadConfig();

const serial = new SerialLink();
const handy = new HandyClient();
handy.setKeys(config.handy.connectionKey, config.handy.appKey);
state.ble.handy.configured = handy.isConfigured();

const zoneRandomizer = new ZoneRandomizer(handy);
zoneRandomizer.setZoneConfig(config.handy.zone);

const app = express();
app.use(
  '/api',
  createApiRouter({
    state,
    serial,
    handy,
    getConfig: loadConfig,
    setConfig: saveConfig,
    zoneRandomizer,
  }),
);
app.use(express.static(DATA_DIR));

const httpServer = http.createServer(app);
const ws = createWebSocketServer(httpServer, state);

function broadcastBleStatus() {
  ws.broadcast('ble_status', { ...state.ble, wifi: { rssi: state.wifi.rssi } });
}

function broadcastArousalStatus() {
  ws.broadcast('arousal_status', state.arousal);
}

serial.on('connected', async () => {
  serial.send({ type: 'get_config' });

  if (handy.isConfigured()) {
    const connected = await handy.begin();
    state.ble.handy.connected = connected;
    serial.send({ type: 'handy_status', connected });
    broadcastBleStatus();
  }
});

serial.on('disconnected', () => {
  broadcastBleStatus();
});

let lastArousalLog = 0;
const AROUSAL_LOG_INTERVAL_MS = 1000;

serial.on('message', async (msg) => {
  if (msg.type === 'arousal_status') {
    const { type, ...rest } = msg;
    Object.assign(state.arousal, rest);
    broadcastArousalStatus();

    // Throttled visibility into raw telemetry - useful for confirming the
    // sensor pipeline end-to-end without going through the browser.
    if (Date.now() - lastArousalLog > AROUSAL_LOG_INTERVAL_MS) {
      lastArousalLog = Date.now();
      console.log(
        `[arousal] active=${state.arousal.active} pressure=${state.arousal.pressure} arousal%=${state.arousal.arousalPercent} state=${state.arousal.state}`,
      );
    }
    return;
  }

  if (msg.type === 'arousal_config') {
    const { type, ...rest } = msg;
    state.arousalConfig = rest;
    return;
  }

  if (msg.type === 'handy_vibrate') {
    // Full-resolution raw speed (0-255), not the 0-20 level scale used
    // elsewhere - see SerialDeviceOutput.h's class comment on the firmware side.
    const ok = await handy.setVibrationLevel(msg.speed);
    const nowConnected = handy.isConnected();
    if (state.ble.handy.connected !== nowConnected) {
      state.ble.handy.connected = nowConnected;
      serial.send({ type: 'handy_status', connected: nowConnected });
      broadcastBleStatus();
    }
    if (!ok) {
      console.log(`[handy] failed to apply speed ${msg.speed}`);
    }
  }
});

// Periodically reassure the firmware the link (and Handy) is alive, so
// SerialDeviceOutput::isConnectedState() stays accurate even between
// vibration commands.
setInterval(() => {
  if (serial.isOpen()) {
    serial.send({ type: 'handy_status', connected: handy.isConnected() });
  }
}, 5000);

httpServer.listen(PORT, () => {
  console.log(`NogasmLink laptop app running at http://localhost:${PORT}`);
  console.log('Looking for the ESP32 over USB serial...');
});

serial.connect();
