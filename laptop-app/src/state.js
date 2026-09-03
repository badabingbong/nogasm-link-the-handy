// In-memory state mirroring the shapes NogasmHttp used to serve over WiFi
// (see lib/nogasm_http/NogasmHttp.cpp's generate*Json methods) so the existing
// webapp/ frontend works completely unmodified against this server.
export function createState() {
  return {
    // BLE isn't used in USB serial mode (see main.cpp's ENABLE_BLE) - these
    // are stable stand-ins so the frontend's WiFi/Bluetooth metrics don't
    // error out, they just won't mean much.
    wifi: {
      connected: true,
      ssid: 'N/A (USB serial mode)',
      ip: '127.0.0.1',
      hostname: 'nogasmlink-laptop-app',
      rssi: 0,
    },
    ble: {
      scanning: false,
      connected: false,
      state: 0,
      stateString: 'IDLE',
      device: null,
      handy: { configured: false, connected: false },
    },
    arousal: {
      active: false,
      arousalPercent: 0,
      pressure: 0,
      limit: 0,
      limitExceededCounter: 0,
      sensitivity: 0,
      currentSessionDuration: 0,
      clenchThreshold: 0,
      lastClenchDuration: 0,
      state: 'IDLE',
    },
    // Populated once the ESP32 replies to a "get_config" request over serial.
    arousalConfig: null,
  };
}
