#include "PressureSensor.h"
#include <Util.h>

// HX710B's constructor takes (dataPin, clockPin) - our constructor takes
// (sckPin, outPin), i.e. (clock, data) - swapped relative to the library.
// Passing them straight through drove the wrong physical pin as the clock.
PressureSensor::PressureSensor(const uint8_t sckPin, const uint8_t outPin, const uint8_t samples) : _sckPin(sckPin), _outPin(outPin), _hx710(outPin, sckPin), _pressureRA(samples)
{
}

void PressureSensor::begin()
{
  _hx710.begin();

  // Initialize running average
  _pressureRA.clear();

  Util::logDebug("Starting HX710B pressure sensor, SCK:%d OUT:%d samples:%d", _sckPin, _outPin, _pressureRA.getSize());
}

int PressureSensor::readRawPressure()
{
  // Read 24-bit signed value from HX710B
  const long rawValue = _hx710.read();

  // Apply zero-point offset
  const float calibrated = (static_cast<float>(rawValue) - _offset) * _scale;

  // Map from practical HX710B range to 0-4095 (same range the rest of the code expects)
  // Clamp negative (pressure below zero) to 0
  const long mapped = constrain(static_cast<long>(calibrated * 4095.0f / static_cast<float>(MAX_PRESSURE_LIMIT)), 0L, 4095L);

  // TEMP DIAGNOSTIC: throttled raw/pre-clamp visibility for tracking down a
  // reading stuck at 0 - if `raw` never changes, it's a wiring/hardware issue;
  // if `raw` changes but `calibrated` goes negative when pressure increases,
  // the sign convention for this sensor is inverted relative to what
  // calibrateZero() assumes. Remove once confirmed.
  static unsigned long lastLogMs = 0;
  if (millis() - lastLogMs > 1000)
  {
    lastLogMs = millis();
    Util::logInfo("PressureSensor: raw=%ld offset=%.1f calibrated=%.1f mapped=%ld", rawValue, _offset, calibrated, mapped);
  }

  _lastValue = static_cast<int>(mapped);
  return _lastValue;
}

float PressureSensor::readSmoothedPressure()
{
  const int pressure = readRawPressure();
  _pressureRA.addValue(static_cast<float>(pressure));
  _lastValueSmoothed = _pressureRA.getAverage();
  return _lastValueSmoothed;
}

bool PressureSensor::isReady() const
{
  return _pressureRA.bufferIsFull();
}

void PressureSensor::calibrateZero(uint16_t samples)
{
  float sum = 0.0;

  _pressureRA.clear();

  // Wait for HX710B to be ready before starting calibration
  for (uint16_t i = 0; i < samples; i++)
  {
    sum += static_cast<float>(_hx710.read());
    delay(30);  // HX710B @ 40 SPS → ~25 ms per sample; 30 ms gives headroom
  }

  _offset = sum / static_cast<float>(samples);
  Util::logDebug("HX710B zero calibrated to offset: %.2f (raw counts)", _offset);
}