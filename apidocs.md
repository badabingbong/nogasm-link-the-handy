Point your AI to this file to start the vibe coding. NB! This doc is AI-generated and not verified.

TODO: Make a skill.md for API v3

## Overview

The Handy API v2 is the REST API for **The Handy** by Ohdoki — a connected automatic stroker. The API allows you to: 

- Control device motion (speed, position, stroke zone)
- Synchronize and play scripts (FunScript/CSV)
- Read device info and status
- Manage firmware updates
- Synchronize time with the HandyFeeling server for coordinated playback

**Base URL (production):** `https://www.handyfeeling.com/api/handy/v2`

---

## Authentication

All device-specific endpoints require:

```
X-Connection-Key: <your_connection_key>
```

The connection key is the unique identifier for the device found in the Handy app or printed on the device. It is a short alphanumeric string (e.g., `A1B2C3`).

**Exception:** `/servertime` does NOT require a connection key.

---

## Server Environments

| Environment | Base URL |
| --- | --- |
| Production | `https://www.handyfeeling.com/api/handy/v2` |
| Staging | `https://staging.handyfeeling.com/api/handy/v2` |
| Local dev | `http://localhost:3000/api/handy/v2` |

Use the env var `HANDY_API_BASE_URL` in your apps to switch environments.

---

## Device Modes

The Handy operates in exclusive modes. Only one motion subsystem is active at a time.

| Mode Value | Name | Description |
| --- | --- | --- |
| `0` | HAMP | Automatic oscillation — set velocity, run at constant speed |
| `1` | HSSP | Script playback — synchronized script player |
| `2` | HDSP | Direct position control — send position commands manually |
| `3` | MAINTENANCE | Firmware update mode |
| `4` | HBSP | Bluetooth script player |

**Switching modes:** `PUT /mode` with body `{"mode": <int>}`. Device will reject commands from another mode unless you switch first.

---

## Rate Limiting

All API responses include rate limit headers:

| Header | Description |
| --- | --- |
| `X-RateLimit-Limit` | Requests allowed per window |
| `X-RateLimit-Remaining` | Remaining requests in current window |
| `X-RateLimit-Reset` | Unix timestamp when the window resets |

---

## Endpoints by Tag

### BASE — Core device control

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/mode` | Get current mode |
| `PUT` | `/mode` | Set mode — body: `{mode: 0\|1\|2\|3\|4}` |
| `GET` | `/connected` | Check if device is connected and reachable |
| `GET` | `/info` | Get device info (model, firmware version, hardware revision) |
| `GET` | `/settings` | Get extended device settings |
| `GET` | `/status` | Get device status (current mode, errors) |

---

### HAMP — Handy Automatic Motion Player

Simple oscillation mode. Set a velocity and the device runs continuously.

| Method | Path | Description |
| --- | --- | --- |
| `PUT` | `/hamp/start` | Start HAMP motion |
| `PUT` | `/hamp/stop` | Stop HAMP motion |
| `GET` | `/hamp/velocity` | Get current HAMP velocity (%) |
| `PUT` | `/hamp/velocity` | Set HAMP velocity — body: `{velocity: 0-100}` |
| `GET` | `/hamp/state` | Get current HAMP state |

**Velocity:** 0–100 (percentage of maximum speed). Full-stroke top speed ~350mm/s in practice.

---

### HDSP — Handy Direct Stroke Player

Real-time position control. Send position targets with velocity or time constraints.

| Method | Path | Description |
| --- | --- | --- |
| `PUT` | `/hdsp/xava` | Absolute position + absolute velocity |
| `PUT` | `/hdsp/xpva` | Percentage position + absolute velocity |
| `PUT` | `/hdsp/xpvp` | Percentage position + percentage velocity |
| `PUT` | `/hdsp/xat` | Absolute position + time (duration ms) |
| `PUT` | `/hdsp/xpt` | Percentage position + time (duration ms) |

**Request body fields:**

- `position` — target position (0–100% or mm depending on endpoint)
- `velocity` — target velocity (mm/s or % depending on endpoint)
- `duration` — time in milliseconds to reach position (xat/xpt only)
- `stopOnTarget` (optional bool) — stop motion when target is reached
- `immediateResponse` (optional bool) — return immediately before motion completes

HDSP is ideal for custom real-time interactive applications, game integrations, and live control.

---

### HSSP — Handy Script Sync Player

Synchronized script playback. Requires time synchronization (HSTP) before use.

| Method | Path | Description |
| --- | --- | --- |
| `PUT` | `/hssp/setup` | Upload script URL to device — body: `{url, sha256?}` |
| `PUT` | `/hssp/play` | Start playback — body: `{estimatedServerTime, startTime, playbackRate?}` |
| `PUT` | `/hssp/stop` | Stop playback |
| `GET` | `/hssp/state` | Get current HSSP state |
| `GET` | `/hssp/loop` | Get loop setting |
| `PUT` | `/hssp/loop` | Set loop — body: `{activated: bool}` |

**HSSP Play body:**

- `estimatedServerTime` — current estimated server time in ms (clientTime + offset)
- `startTime` — server time at which playback should begin
- `playbackRate` (optional) — 1.0 = normal, 0.5 = half speed, 2.0 = double

**Script URL requirements:**

- Must be publicly accessible HTTPS URL
- Handy device fetches the script itself
- `sha256` optional checksum for integrity verification

---

### HSTP — Handy Server Time Protocol

Time synchronization between the Handy device and HandyFeeling server clock.

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/hstp/time` | Get current device time |
| `GET` | `/hstp/offset` | Get current HSTP offset |
| `PUT` | `/hstp/offset` | Set HSTP offset — body: `{offset: ms}` |
| `GET` | `/hstp/rtd` | Get round-trip delay (RTD) measurement |
| `GET` | `/hstp/sync` | Trigger device sync with server clock |

---

### SLIDE — Stroke Zone Control

Define the slide's min and max positions (the physical stroke zone).

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/slide` | Get current slide settings |
| `PUT` | `/slide` | Set slide zone — body: `{min?: 0-100, max?: 0-100}` |
| `GET` | `/slide/position/absolute` | Get absolute slide position in mm |
- `min` / `max` values are percentages of total travel (0–100)
- `min` must be < `max`
- Default: min=0, max=100 (full travel)

---

### MAINTENANCE — Firmware Management

| Method | Path | Description |
| --- | --- | --- |
| `PUT` | `/maintenance/restart` | Restart the device |
| `PUT` | `/maintenance/update/perform` | Trigger firmware update — body: `{url, sha256?}` |
| `GET` | `/maintenance/update/status` | Get firmware update status |

**FirmwareStatus enum:**

| Value | Name | Description |
| --- | --- | --- |
| `0` | UP_TO_DATE | Device is on latest firmware |
| `1` | UPDATE_AVAILABLE | Update is available but not required |
| `2` | UPDATE_REQUIRED | Update must be performed |

---

### OTA — Over-The-Air Updates

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/ota/latest` | Get latest firmware — query: `?model=<model>&branch=<branch>` |

---

### TIMESYNC — Server Time

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/servertime` | Get current HandyFeeling server time in ms (no auth required) |

---

## Time Synchronization Concept

HSSP requires precise time alignment between the client, Handy device, and HandyFeeling server.

### Algorithm

1. Sample `/servertime` repeatedly (default 30 samples)
2. For each sample:
    - `t0` = local time before request
    - `serverTime` = response value
    - `t1` = local time after request
    - `RTD` = `t1 - t0`
    - `offset` = `serverTime - (t0 + RTD/2)`
3. Average the offsets, discarding outliers
4. Store as `clientServerOffset`

### Usage

```tsx
const estimatedServerTime = Date.now() + clientServerOffset;
// Use this when calling hssp/play
```

### HSTP Sync

Alternatively, use `GET /hstp/sync` to have the device itself synchronize its internal clock with the HandyFeeling server. Then you only need to pass `estimatedServerTime` accurately in play commands.

---

## Script Formats

### FunScript (JSON)

The standard format used across the ecosystem:

```json
{
  "actions": [
    {"at": 0, "pos": 0},
    {"at": 1000, "pos": 100},
    {"at": 2000, "pos": 0}
  ]
}
```

- `at` — timestamp in milliseconds
- `pos` — position 0–100 (0 = bottom, 100 = top)

### CSV

```
# title=My Script
# duration=120000
ms,pos
0,0
1000,100
2000,0
```

### Script Tokens

Internal format used for direct device delivery. Same position/time encoding as FunScript but encoded for the device's binary RPC layer. Used with Bluetooth / HBSP mode.

---

## Device Specifications

| Spec | Value |
| --- | --- |
| Travel (Handy 2) | 110mm |
| Travel (Handy 1.0) | 108mm |
| End buffer (FW3+) | 7mm |
| FW capped travel | 96mm |
| Gearing | 12mm per rotation |
| Encoder resolution | 12 per round (~2/3mm) |
| Motor max speed | 12,000 RPM (1,600mm/s) |
| Motor with gear max | ~700mm/s |
| Max output speed | 400mm/s (capped by firmware) |
| Practical top speed (full stroke) | ~350mm/s |
| Min speed | 32mm/s (capped by firmware) |
| Material (main) | ABS |
| Material (slider/motor holder) | POM |
| Material (sleeve) | TPU |
| Dimensions | 70 × 232mm |
| Weight (device) | 578g |
| Weight (boxed) | 1.35kg |
| WiFi | 802.11b/g/n |
| Bluetooth | 4.2 |
| Storage | 8MB (~2.5MB available) |

---

## Bluetooth Control (FW4+)

The Handy supports Bluetooth Low Energy (BLE) for direct control without internet connectivity.

| Property | Value |
| --- | --- |
| BLE Service UUID | `77834d26-40f7-11ee-be56-0242ac120002` |
| TX Characteristic | `77835032-40f7-11ee-be56-0242ac120002` |
| RX Characteristic | `77835410-40f7-11ee-be56-0242ac120002` |
| Advertising name | `ohd_hwX_UID` (X = hardware version, UID = device UID) |
| Protocol | Protobuf + custom RPC layer |

Bluetooth mode (HBSP = mode 4) enables direct script delivery and control over BLE without routing through HandyFeeling servers.

---

## Channel API Concept

The Channel API is the abstraction layer that routes commands from the API to the physical device. When you send a command to `/hamp/start`, it:

1. Arrives at HandyFeeling servers
2. Is routed to the device via its persistent connection channel (WebSocket / MQTT)
3. Device executes the command and returns a response

This means all API calls are effectively RPC over the device's cloud channel. Latency includes: client → server → device channel → device → back.

For latency-sensitive applications, use HDSP with `immediateResponse: true` or HBSP (Bluetooth) for direct control.

---

## Code Generation (OpenAPI)

The full OpenAPI 3.0.3 spec is available at:

```
<https://www.handyfeeling.com/api/handy/v2/spec>
```

Generate a typed TypeScript client:

```bash
npx openapi --exportSchemas true \\
  --input <https://www.handyfeeling.com/api/handy/v2/spec> \\
  --output src/_HANDYAPI \\
  --name HandyApi
```

Or use `openapi-typescript`:

```bash
npx openapi-typescript <https://www.handyfeeling.com/api/handy/v2/spec> -o src/handy-api.d.ts
```

---

## Quick Start Example

```tsx
const API = '<https://www.handyfeeling.com/api/handy/v2>';
const KEY = 'YOUR_CONNECTION_KEY';
const headers = { 'X-Connection-Key': KEY, 'Content-Type': 'application/json' };

// 1. Switch to HAMP mode
await fetch(`${API}/mode`, { method: 'PUT', headers, body: JSON.stringify({ mode: 0 }) });

// 2. Start at 50% speed
await fetch(`${API}/hamp/velocity`, { method: 'PUT', headers, body: JSON.stringify({ velocity: 50 }) });
await fetch(`${API}/hamp/start`, { method: 'PUT', headers });

// 3. Stop
await fetch(`${API}/hamp/stop`, { method: 'PUT', headers });
```

---

## Error Handling

| HTTP Status | Meaning |
| --- | --- |
| `200` | Success |
| `400` | Bad request (invalid params) |
| `401` | Missing or invalid connection key |
| `404` | Device not found / not connected |
| `429` | Rate limit exceeded |
| `500` | Server error |
| `503` | Device offline or unavailable |

Always check `X-RateLimit-Remaining` and back off when approaching 0.

---