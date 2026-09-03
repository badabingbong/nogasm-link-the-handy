#include "CompositeDeviceOutput.h"

bool CompositeDeviceOutput::setVibrationLevel(const uint8_t speed)
{
  bool sent = false;

  if (_primary.isConnectedState())
  {
    sent = _primary.setVibrationLevel(speed) || sent;
  }

  if (_secondary.isConnectedState())
  {
    sent = _secondary.setVibrationLevel(speed) || sent;
  }

  return sent;
}
