import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { EventEmitter } from 'node:events';

// USB-UART bridge chips commonly used on ESP32 dev boards (including M5Stack's).
const KNOWN_VID_PID = [
  { vendorId: '10c4', productId: 'ea60' }, // Silicon Labs CP210x
  { vendorId: '1a86', productId: '7523' }, // CH340
  { vendorId: '1a86', productId: '55d4' }, // CH9102
  { vendorId: '0403', productId: '6001' }, // FTDI FT232
];

const RECONNECT_DELAY_MS = 3000;

/**
 * Talks newline-delimited JSON to the ESP32 over USB serial - see
 * lib/nogasm_serial/SerialLink.h on the firmware side for the protocol.
 * Emits 'connected', 'disconnected', and 'message' (parsed JSON object).
 * Any line that isn't valid JSON is treated as a plain firmware log line.
 */
export class SerialLink extends EventEmitter {
  constructor() {
    super();
    this.port = null;
    this.parser = null;
    this.reconnectTimer = null;
  }

  async findPort() {
    if (process.env.SERIAL_PORT) {
      return process.env.SERIAL_PORT;
    }

    const ports = await SerialPort.list();
    const match = ports.find((p) =>
      KNOWN_VID_PID.some(
        (known) => p.vendorId?.toLowerCase() === known.vendorId && p.productId?.toLowerCase() === known.productId,
      ),
    );
    return match ? match.path : null;
  }

  async connect() {
    const portPath = await this.findPort();
    if (!portPath) {
      console.log('[serial] No ESP32 found - retrying in 3s. Set the SERIAL_PORT env var to override auto-detect.');
      this._scheduleReconnect();
      return;
    }

    console.log(`[serial] Connecting to ${portPath}...`);
    this.port = new SerialPort({ path: portPath, baudRate: 115200 }, (err) => {
      if (err) {
        console.log(`[serial] Failed to open ${portPath}: ${err.message}`);
        this._scheduleReconnect();
      }
    });

    this.parser = this.port.pipe(new ReadlineParser({ delimiter: '\n' }));
    this.parser.on('data', (line) => this._handleLine(line));

    this.port.on('open', () => {
      console.log(`[serial] Connected to ${portPath}`);
      this.emit('connected');
    });

    this.port.on('close', () => {
      console.log('[serial] Disconnected - retrying in 3s');
      this.emit('disconnected');
      this._scheduleReconnect();
    });

    this.port.on('error', (err) => {
      console.log(`[serial] Error: ${err.message}`);
    });
  }

  _scheduleReconnect() {
    if (this.reconnectTimer) {
      return;
    }
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, RECONNECT_DELAY_MS);
  }

  _handleLine(line) {
    const trimmed = line.trim();
    if (!trimmed) {
      return;
    }

    let msg;
    try {
      msg = JSON.parse(trimmed);
    } catch {
      // Not JSON - a plain Util::log*() line sharing the same UART. Echo it
      // for visibility rather than silently dropping it.
      console.log(`[esp32] ${trimmed}`);
      return;
    }

    this.emit('message', msg);
  }

  isOpen() {
    return !!this.port?.isOpen;
  }

  send(obj) {
    if (!this.isOpen()) {
      return false;
    }
    this.port.write(`${JSON.stringify(obj)}\n`);
    return true;
  }
}
