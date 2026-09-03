#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include <functional>
#include "ArousalManager.h"

// Encoder is not present on M5 Atom Lite.
// This stub preserves the interface so main.cpp compiles unchanged.
// Arousal start/stop is controlled via the web UI.

#define ENCODER_MIN 0
#define ENCODER_MAX 255

class EncoderManager
{
 public:
  explicit EncoderManager(ArousalManager& arousalManager) : _arousalManager(arousalManager)
  {
  }

  void begin()
  {
  }  // no-op
  void update()
  {
  }  // no-op

  void setEncoderValue(int /*value*/, int /*inMin*/, int /*inMax*/)
  {
  }

  void onEncoderValueChanged(std::function<void(int)> callback)
  {
    _valueChangedCallback = callback;
  }

  void onButtonPressed(std::function<void()> callback)
  {
    _buttonPressedCallback = callback;
  }

 private:
  ArousalManager& _arousalManager;
  std::function<void(int)> _valueChangedCallback = nullptr;
  std::function<void()> _buttonPressedCallback = nullptr;
};

#endif