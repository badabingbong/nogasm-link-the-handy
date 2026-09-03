#include "RGBManager.h"
#include <Util.h>
#include <math.h>

#define RGB_UPDATE_PERIOD 20  // ms

RGBManager::RGBManager(const uint8_t neoPixelPin, const uint8_t numLeds) : _pin(neoPixelPin), _numLeds(numLeds)
{
  initStateConfigs();
}

void RGBManager::initStateConfigs()
{
  const LEDStateConfig configs[] = {
    // State,                      R,   G,   B,   Animation Type,       Duration, Continuous
    {LEDState::OFF,                  0,   0,   0,   AnimationType::NONE,  0,    false, "Off"            }, // Black (OFF)
    {LEDState::WIFI_SETUP_REQUIRED,  255, 255, 255, AnimationType::FLASH, 500,  true,  "Setup required" }, // Pure White flashing
    {LEDState::WIFI_CONNECTING,      255, 255, 255, AnimationType::PULSE, 2000, true,  "WiFi connecting"}, // Pure white pulsing
    {LEDState::WIFI_CONNECTED,       255, 255, 255, AnimationType::NONE,  0,    false, "Wifi connected" }, // Pure white
    {LEDState::SCANNING,             0,   0,   255, AnimationType::PULSE, 2000, true,  "BLE scanning"   }, // Blue pulsing
    {LEDState::CONNECTING,           255, 165, 0,   AnimationType::PULSE, 2000, true,  "BLE connecting" }, // Orange pulsing
    {LEDState::CONNECTED,            0,   0,   255, AnimationType::NONE,  0,    false, "BLE connected"  }, // Blue solid
    {LEDState::BLE_ERROR,            255, 0,   40,  AnimationType::FLASH, 300,  true,  "BLE Error"      }, // Bright red flashing
    {LEDState::PRESSURE_ERROR,       0,   255, 40,  AnimationType::FLASH, 300,  true,  "Pressure Error" }, // Bright green flashing
    {LEDState::AROUSAL_ACTIVE,       220, 0,   220, AnimationType::PULSE, 2000, true,  "Session active" }, // Magenta pulsing
    {LEDState::AROUSAL_ALLOW_ORGASM, 220, 0,   220, AnimationType::FLASH, 2000, true,  "Orgasm allowed" }  // Magenta flashing
  };

  for (const auto& config : configs)
  {
    _stateConfigs[config.state] = config;
  }
}

void RGBManager::begin()
{
  // SK6812 on M5 Atom Lite is GRB order
  FastLED.addLeds<SK6812, 27, GRB>(_leds, ATOM_LITE_LED_COUNT);
  FastLED.setBrightness(_brightness);
  setLEDState(LEDState::OFF);
  Util::logDebug("RGB LED Manager (SK6812 NeoPixel) initialized on pin %d", _pin);
}

void RGBManager::update()
{
  const unsigned long currentTime = millis();
  if (!Util::hasTimeExpired(RGB_UPDATE_PERIOD, _lastUpdateTime))
  {
    return;
  }

  _lastUpdateTime = currentTime;
  if (_isAnimating)
  {
    updateAnimation();
    if (_isContinuous && Util::hasTimeExpired(_animationDuration, _animationStartTime))
    {
      restartCurrentAnimation();
    }
  }

  updateLED();
}

void RGBManager::setLEDState(const LEDState state)
{
  if (_currentState == state)
  {
    return;
  }

  _currentState = state;

  const auto configIt = _stateConfigs.find(state);
  if (configIt != _stateConfigs.end())
  {
    applyStateConfig(configIt->second);
    Util::logDebug("LED State::%s", configIt->second.description.c_str());
  }
  else
  {
    // Fallback to OFF if state isn't defined
    const auto offConfigIt = _stateConfigs.find(LEDState::OFF);
    if (offConfigIt != _stateConfigs.end())
    {
      applyStateConfig(offConfigIt->second);
    }
    else
    {
      setColor(0, 0, 0);
    }
  }
}

void RGBManager::applyStateConfig(const LEDStateConfig& config)
{
  _targetRed = config.red;
  _targetGreen = config.green;
  _targetBlue = config.blue;
  _currentAnimation = config.animation;
  _animationDuration = config.duration;
  _isContinuous = config.continuous;

  // Reset animation state
  _isAnimating = (_currentAnimation != AnimationType::NONE);
  _animationStartTime = millis();

  if (!_isAnimating)
  {
    _red = _targetRed;
    _green = _targetGreen;
    _blue = _targetBlue;
  }
}

void RGBManager::restartCurrentAnimation()
{
  _animationStartTime = millis();
}

void RGBManager::setColor(const uint8_t red, const uint8_t green, const uint8_t blue)
{
  _red = red;
  _green = green;
  _blue = blue;
  _isAnimating = false;
  _currentAnimation = AnimationType::NONE;

  updateLED();
}

void RGBManager::setBrightness(const uint8_t brightness)
{
  _brightness = brightness;
  FastLED.setBrightness(brightness);
  updateLED();
}

void RGBManager::updateAnimation()
{
  const unsigned long currentTime = millis();
  const unsigned long elapsed = currentTime - _animationStartTime;
  const float progress = static_cast<float>(elapsed % _animationDuration) / _animationDuration;

  switch (_currentAnimation)
  {
    case AnimationType::FLASH:
    {
      const bool isOn = (elapsed / (_animationDuration / 2)) % 2 == 0;
      if (isOn)
      {
        _red = _targetRed;
        _green = _targetGreen;
        _blue = _targetBlue;
      }
      else
      {
        _red = 0;
        _green = 0;
        _blue = 0;
      }
      break;
    }

    case AnimationType::PULSE:
    {
      const float brightness = (sin(progress * 2.0 * PI) + 1.0) / 2.0;
      _red = static_cast<uint8_t>(_targetRed * brightness);
      _green = static_cast<uint8_t>(_targetGreen * brightness);
      _blue = static_cast<uint8_t>(_targetBlue * brightness);
      break;
    }

    case AnimationType::BREATHE:
    {
      const float brightness = pow(sin(progress * PI), 2);
      _red = static_cast<uint8_t>(_targetRed * brightness);
      _green = static_cast<uint8_t>(_targetGreen * brightness);
      _blue = static_cast<uint8_t>(_targetBlue * brightness);
      break;
    }

    default:
      break;
  }
}

void RGBManager::updateLED()
{
  _leds[0] = CRGB(_red, _green, _blue);
  FastLED.show();
}