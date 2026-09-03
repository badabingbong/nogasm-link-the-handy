#ifndef HANDY_OUTPUT_H
#define HANDY_OUTPUT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "IDeviceOutput.h"

// Confirmed against the current Handy API v2 docs (see apidocs.md at the repo root) -
// this used to point at a nonexistent "handy-rest/v3" path, which is why every
// request failed regardless of the connection key.
#define HANDY_API_BASE "https://www.handyfeeling.com/api/handy/v2"
#define HANDY_CONNECT_TIMEOUT_MS 8000

/**
 * IDeviceOutput implementation for The Handy via the Handyfeeling REST API v2.
 *
 * Required credentials (stored in NogasmConfig / set via web UI):
 *   - Connection Key: shown on The Handy's own display (short alphanumeric code) - required,
 *     and the ONLY credential the API needs (see apidocs.md - Authentication).
 *   - Application Key: does not exist in this API. The optional X-Api-Key header below is
 *     harmless dead capability kept only in case a future API version wants it.
 *
 * The Handy must be:
 *   1. Connected to the same WiFi network (or to the internet generally)
 *   2. In HAMP mode  ← this class handles that automatically on first use
 *
 * Vibration is received as ArousalManager's full-resolution 0-255 raw speed
 * (see IDeviceOutput.h's class comment) and mapped directly to Handy's native
 * 0-100% velocity - no artificial 20-level quantization, since Handy's API
 * genuinely supports the finer resolution.
 *
 * IMPORTANT: this API returns HTTP 200 even on failure, with the real error
 * embedded in the JSON body as {"error": {code, name, message, connected}} -
 * confirmed by hitting it directly with a bogus key. Every request checks the
 * body via HandyApiResult, not just the HTTP status, or failures look like success.
 */
struct HandyApiResult
{
  int status = -1;
  bool ok = false;             // 2xx status AND no "error" object in the body
  bool connectedField = false; // parsed "connected" field, when the endpoint returns one
  String errorMessage;         // human-readable reason, populated when !ok
};

class HandyOutput : public IDeviceOutput
{
 public:
  HandyOutput(const String& connectionKey, const String& appKey);

  // IDeviceOutput
  bool setVibrationLevel(uint8_t speed) override;
  // True once /info has been reached successfully. Deliberately does NOT require
  // HAMP mode/start to already be active - those are lazily negotiated on the
  // first setVibrationLevel() call, which only happens once a caller sees this
  // return true (see ArousalManager::updateVibrationLevel).
  bool isConnectedState() const override;
  void update() override;

  // Call once at startup (or after WiFi connects) to verify connectivity
  // and put the device into HAMP mode.
  bool begin();

  // Re-run begin() if settings changed
  void setKeys(const String& connectionKey, const String& appKey);

  // Application Key is optional - see class comment above.
  bool isConfigured() const
  {
    return _connectionKey.length() > 0;
  }

  // Human-readable reason for the last failure, if any - populated by begin().
  const String& lastError() const
  {
    return _lastError;
  }

 private:
  String _connectionKey;
  String _appKey;

  bool _hampModeActive = false;  // has the device been set to HAMP mode yet?
  bool _hampStarted = false;     // has HAMP playback been started?
  bool _connected = false;       // last known connectivity status
  uint8_t _lastPercent = 255;    // last velocity % actually sent; 255 = sentinel "never set"
  String _lastError;

  unsigned long _lastRetryMs = 0;
  static constexpr unsigned long RETRY_INTERVAL_MS = 30000;

  // Send a PUT/GET to the Handy API and parse the response body for a
  // {"error": ...} object - the API reports failures in the body, not the
  // HTTP status (see class comment above).
  HandyApiResult apiPut(const String& path, const String& jsonBody);
  HandyApiResult apiGet(const String& path);
  static HandyApiResult parseResponse(int status, const String& body);

  bool setHampMode();                   // set device mode = 0 (HAMP)
  bool startHamp();                     // start HAMP playback (required before velocity)
  bool applyVelocity(uint8_t percent);  // PUT /hamp/velocity
  bool stopHamp();

  void setupClient(HTTPClient& http, const String& path) const;
};

#endif  // HANDY_OUTPUT_H
