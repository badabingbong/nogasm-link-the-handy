#ifndef SERIAL_DEVICE_OUTPUT_H
#define SERIAL_DEVICE_OUTPUT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "IDeviceOutput.h"

/**
 * IDeviceOutput implementation that relays vibration commands over the USB
 * serial connection to a companion app (see laptop-app/) instead of talking
 * to a device directly. The companion app owns the actual Handy REST API
 * calls, since it has its own internet connection - this device never needs
 * WiFi for Handy control.
 *
 * Protocol (see laptop-app/README.md and SerialLink for the full reference):
 *   ESP32 -> host: {"type":"handy_vibrate","speed":0-255}\n   (full-resolution
 *                   raw speed, NOT the 0-20 level scale used elsewhere - see
 *                   IDeviceOutput.h's class comment)
 *   host -> ESP32: {"type":"handy_status","connected":bool}\n   (heartbeat)
 *
 * isConnectedState() reflects the last "handy_status" heartbeat from the
 * host, defaulting to false so ArousalManager doesn't bother sending until
 * the host confirms it can actually reach Handy.
 *
 * Note: writes to Serial assume nothing else concurrently writes to it from
 * another FreeRTOS task (e.g. NimBLE's host task doing its own logging) -
 * safe as long as ENABLE_BLE is off in main.cpp. If BLE is ever re-enabled
 * alongside this, Serial access would need its own mutex.
 */
class SerialDeviceOutput : public IDeviceOutput
{
 public:
  bool setVibrationLevel(const uint8_t speed) override
  {
    JsonDocument doc;
    doc["type"] = "handy_vibrate";
    doc["speed"] = speed;
    serializeJson(doc, Serial);
    Serial.println();
    return true;
  }

  bool isConnectedState() const override
  {
    return _hostConnected;
  }

  // Called by SerialLink when a "handy_status" message arrives from the host.
  void setHostConnected(const bool connected)
  {
    _hostConnected = connected;
  }

 private:
  bool _hostConnected = false;
};

#endif  // SERIAL_DEVICE_OUTPUT_H
