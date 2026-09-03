import { WebSocketServer } from 'ws';

const PING_INTERVAL_MS = 15000;

/**
 * Mirrors the /ws contract lib/nogasm_http/NogasmHttp.cpp used to serve over
 * WiFi (ble_status / arousal_status messages) so webapp/'s WebSocketService.js
 * works unmodified.
 */
export function createWebSocketServer(httpServer, state) {
  const wss = new WebSocketServer({ server: httpServer, path: '/ws' });

  function broadcast(type, payload) {
    const message = JSON.stringify({ type, ...payload });
    for (const client of wss.clients) {
      if (client.readyState === client.OPEN) {
        client.send(message);
      }
    }
  }

  wss.on('connection', (ws) => {
    ws.isAlive = true;
    ws.on('pong', () => {
      ws.isAlive = true;
    });

    // Send an immediate snapshot so the UI doesn't wait for the next tick.
    ws.send(JSON.stringify({ type: 'ble_status', ...state.ble, wifi: { rssi: state.wifi.rssi } }));
    ws.send(JSON.stringify({ type: 'arousal_status', ...state.arousal }));
  });

  const pingTimer = setInterval(() => {
    for (const client of wss.clients) {
      if (!client.isAlive) {
        client.terminate();
        continue;
      }
      client.isAlive = false;
      client.ping();
    }
  }, PING_INTERVAL_MS);

  wss.on('close', () => clearInterval(pingTimer));

  return { broadcast };
}
