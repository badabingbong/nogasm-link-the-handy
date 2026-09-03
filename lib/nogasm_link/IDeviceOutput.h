#ifndef IDEVICE_OUTPUT_H
#define IDEVICE_OUTPUT_H

#include <Arduino.h>

/**
 * Abstract interface for device output backends.
 * ArousalManager programs against this interface, so BLE (Lovense)
 * and WiFi REST (The Handy) backends are interchangeable.
 *
 * Vibration speed is passed at ArousalManager's full internal resolution
 * (0-255, matching SPEED_VIB_MAX), NOT the 20-level scale used elsewhere in
 * the UI/config. Each backend maps that down to its own native resolution -
 * NogasmBLEManager quantizes to Lovense's real 20-level BLE protocol, while
 * HandyOutput/SerialDeviceOutput pass it through to Handy's native 0-100%
 * scale at (near-)full resolution, since Handy has no reason to be limited
 * to 20 steps the way a Lovense toy's own protocol is.
 */
class IDeviceOutput
{
 public:
  virtual ~IDeviceOutput() = default;

  /**
   * Set the vibration / stroke speed.
   * @param speed  0 = off, 255 = full speed (matches SPEED_VIB_MAX) - see class comment
   * @return true if the command was sent successfully
   */
  virtual bool setVibrationLevel(uint8_t speed) = 0;

  /**
   * @return true when the backend is ready to accept commands
   */
  virtual bool isConnectedState() const = 0;

  /**
   * Called from the main loop — used for polling / state-machine work.
   * Implementations that don't need this can leave it empty.
   */
  virtual void update()
  {
  }
};

#endif  // IDEVICE_OUTPUT_H
