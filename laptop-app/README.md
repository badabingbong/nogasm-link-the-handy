# NogasmLink Laptop App

Local companion app that replaces the ESP32-hosted WiFi dashboard. Instead of the
device joining your WiFi and hosting its own web server (`NogasmHttp`, disabled by
default now - see `ENABLE_WIFI_WEB_SERVER` in `src/main.cpp`), this app:

- Talks to the ESP32 over its existing **USB serial** connection (the same one
  used for flashing/monitoring) - no WiFi, no captive portal, no WPA3 issues.
- Hosts the dashboard **locally** at `http://localhost:3000`, serving the exact
  same build the ESP32's filesystem image used to (`../data`, produced by
  `webapp`'s `yarn build`).
- Makes the actual **Handy** REST API calls itself, since this laptop has a
  normal internet connection - the ESP32 never needs one for Handy control.

The ESP32 still does the real-time work: reading the pressure sensor and running
the arousal/edging algorithm. It just tells this app "set velocity to X" over
serial instead of calling Handy's API directly, and this app relays that.

## Setup

```bash
cd laptop-app
npm install
```

Handy credentials are entered through the web UI (Configuration Settings → The
Handy) exactly like before - they're saved to `config.json` here (gitignored),
not on the ESP32.

## Running

**Close any other program that has the ESP32's serial port open first** -
`pio device monitor`, the Arduino IDE's Serial Monitor, `screen`, etc. Only one
process can hold a serial port at a time, and this app will just keep retrying
with "Resource busy" errors until it's free.

```bash
npm start
```

Then open **http://localhost:3000**.

By default the app auto-detects the ESP32 by scanning connected serial devices
for common USB-UART bridge chip IDs (CP210x, CH340, CH9102, FTDI). If that
doesn't find it (or finds the wrong one, e.g. multiple boards attached), set:

```bash
SERIAL_PORT=/dev/tty.usbserial-XXXX npm start
```

(list candidates with `ls /dev/tty.*` on macOS/Linux, or check Device Manager on
Windows for the COM port).

Change the local port with `PORT=8080 npm start` if 3000 is taken.

## Protocol

Newline-delimited JSON, one object per line, in both directions over the same
115200-baud serial connection already used for firmware logging. Any line that
fails to parse as JSON is treated as a plain log line and just printed to this
app's console (prefixed `[esp32]`) - so `Util::logInfo`/`logDebug` output and
the structured protocol happily share one UART.

See `lib/nogasm_serial/SerialLink.h`/`.cpp` on the firmware side for the
authoritative implementation. Summary:

**ESP32 → laptop:**
| type | fields | meaning |
|---|---|---|
| `arousal_status` | active, arousalPercent, pressure, limit, limitExceededCounter, sensitivity, currentSessionDuration, clenchThreshold, lastClenchDuration, state | periodic telemetry (100ms while active, 1s idle) |
| `arousal_config` | (full `ArousalConfig` shape) | reply to a `get_config` request |
| `handy_vibrate` | `level` (0-20) | ArousalManager decided to change speed - relay to Handy |

**Laptop → ESP32:**
| type | fields | meaning |
|---|---|---|
| `get_config` | - | request the current `arousal_config` |
| `arousal_state` | `active` (bool) or `reset` (bool) | start/stop a session, or reset+recalibrate |
| `arousal_sensitivity` | `sensitivity` (0-255) | update sensitivity |
| `arousal_config` | any subset of `ArousalConfig` fields | update + persist config on the device |
| `handy_status` | `connected` (bool) | heartbeat - tells the ESP32 whether Handy is currently reachable |

## Reverting to WiFi mode

Nothing was deleted - `NogasmHttp`, `WiFiManager`, and the ESP32-side `HandyOutput`
are all still in the firmware, just unwired. Flip `ENABLE_WIFI_WEB_SERVER` back to
`true` in `src/main.cpp`, reflash, and the ESP32 goes back to hosting its own WiFi
dashboard (including its own direct Handy-over-WiFi support) - this app becomes
unnecessary in that mode.
