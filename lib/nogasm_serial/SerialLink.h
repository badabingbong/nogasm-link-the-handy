#ifndef SERIAL_LINK_H
#define SERIAL_LINK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "ArousalManager.h"
#include "NogasmConfig.h"
#include "SerialDeviceOutput.h"

#define SERIAL_LINK_BUFFER_SIZE 512
#define SERIAL_LINK_STATUS_ACTIVE_MS 100
#define SERIAL_LINK_STATUS_IDLE_MS 1000

/**
 * Serial (USB) companion protocol - lets a laptop app (see laptop-app/) drive
 * this device's arousal control and receive telemetry without any WiFi
 * involvement. See laptop-app/README.md for the full message reference.
 *
 * Framing: newline-delimited JSON, one object per line, in both directions on
 * the same Serial connection already used for logging. Any line that fails to
 * parse as JSON is assumed to be a plain log line and is silently ignored
 * here - the host side does the same, so Util::log*() output and the
 * protocol happily share one UART.
 */
class SerialLink
{
 public:
  SerialLink(ArousalManager& arousalManager, NogasmConfig& config, SerialDeviceOutput& deviceOutput)
      : _arousalManager(arousalManager), _config(config), _deviceOutput(deviceOutput)
  {
  }

  void update();

 private:
  ArousalManager& _arousalManager;
  NogasmConfig& _config;
  SerialDeviceOutput& _deviceOutput;

  char _buffer[SERIAL_LINK_BUFFER_SIZE] = {0};
  size_t _bufferLen = 0;

  unsigned long _lastStatusUpdate = 0;

  void readIncoming();
  void handleLine(const char* line, size_t len);
  void handleMessage(JsonDocument& doc);

  void sendArousalStatus();
  void sendArousalConfig();
};

#endif  // SERIAL_LINK_H
