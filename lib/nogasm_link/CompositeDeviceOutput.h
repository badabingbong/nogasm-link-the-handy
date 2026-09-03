#ifndef COMPOSITE_DEVICE_OUTPUT_H
#define COMPOSITE_DEVICE_OUTPUT_H

#include "IDeviceOutput.h"

/**
 * Fans a single vibration level out to two IDeviceOutput backends at once (e.g.
 * BLE/Lovense + WiFi REST/Handy), so both can be driven together by the same
 * ArousalManager, which only ever talks to one IDeviceOutput.
 *
 * Each backend is only sent a command while it individually reports itself
 * connected, mirroring the gating ArousalManager already does for a single
 * IDeviceOutput (see ArousalManager::updateVibrationLevel).
 */
class CompositeDeviceOutput : public IDeviceOutput
{
 public:
  CompositeDeviceOutput(IDeviceOutput& primary, IDeviceOutput& secondary) : _primary(primary), _secondary(secondary)
  {
  }

  bool setVibrationLevel(uint8_t speed) override;

  bool isConnectedState() const override
  {
    return _primary.isConnectedState() || _secondary.isConnectedState();
  }

  // Note: update() is intentionally NOT overridden here. Both backends already
  // have their own update() called directly from the main loop (NogasmBLEManager
  // does far more there than device output, and HandyOutput's retry logic is
  // driven the same way) - forwarding it here too would run them twice per tick.

 private:
  IDeviceOutput& _primary;
  IDeviceOutput& _secondary;
};

#endif  // COMPOSITE_DEVICE_OUTPUT_H
