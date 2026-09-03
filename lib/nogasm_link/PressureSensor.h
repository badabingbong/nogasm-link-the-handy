#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>
#include <RunningAverage.h>
#include <HX710AB.h>

#define RA_DEFAULT_SAMPLES 15
// HX710B is 24-bit signed; practical positive range after zeroing is ~8M counts
#define MAX_PRESSURE_LIMIT 8388607L

class PressureSensor
{
 public:
  explicit PressureSensor(uint8_t sckPin, uint8_t outPin, uint8_t samples = 15);

  void begin();
  void calibrateZero(uint16_t samples = RA_DEFAULT_SAMPLES);

  int readRawPressure();         // HX710B reading mapped to 0-4095 range
  float readSmoothedPressure();  // Smoothed reading using RunningAverage
  bool isReady() const;

  /**
   * @return the max pressure limit for this sensor, taking into account the zero offset
   */
  unsigned int getMaxPressureLimitRaw() const
  {
    return 4095;  // We always report in the 0-4095 normalised range
  }

  // the last value that was read
  int getLastRawPressure() const
  {
    return _lastValue;
  }

  // the last smoothed value that was read
  float getLastSmoothedPressure() const
  {
    return _lastValueSmoothed;
  }

  void setOffset(const float offset)
  {
    _offset = offset;
  }
  void setScale(const float scale)
  {
    _scale = scale;
  }

 private:
  uint8_t _sckPin;
  uint8_t _outPin;
  // Was declared as the HX710AB base class, whose fetch() is a placeholder that
  // always returns a constant 1/0 without touching any pins - meaning every
  // reading through it was identical, calibrateZero() "zeroed" that constant,
  // and every subsequent reading computed to exactly 0. HX710B is the real
  // implementation that actually bit-bangs the 24-bit read.
  HX710B _hx710;
  int _lastValue = 0;              // the last value that was read (0-4095 normalised)
  float _lastValueSmoothed = 0.0;  // the last smoothed value that was read
  float _offset = 0.0;             // Zero-point offset (raw HX710B counts)
  float _scale = 1.0;              // Scale for calibration
  RunningAverage _pressureRA;
};

#endif