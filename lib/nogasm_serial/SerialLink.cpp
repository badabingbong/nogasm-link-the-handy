#include "SerialLink.h"
#include <cstring>
#include "Util.h"

void SerialLink::update()
{
  readIncoming();

  const bool active = _arousalManager.isActive();
  const uint32_t interval = active ? SERIAL_LINK_STATUS_ACTIVE_MS : SERIAL_LINK_STATUS_IDLE_MS;
  if (Util::hasTimeExpired(interval, _lastStatusUpdate))
  {
    sendArousalStatus();
    _lastStatusUpdate = millis();
  }
}

void SerialLink::readIncoming()
{
  while (Serial.available())
  {
    const char c = static_cast<char>(Serial.read());

    if (c == '\n')
    {
      handleLine(_buffer, _bufferLen);
      _bufferLen = 0;
      continue;
    }

    if (c == '\r')
    {
      continue;
    }

    if (_bufferLen < SERIAL_LINK_BUFFER_SIZE - 1)
    {
      _buffer[_bufferLen++] = c;
    }
    else
    {
      // line too long (or garbage) - drop it rather than overflow the buffer
      _bufferLen = 0;
    }
  }
}

void SerialLink::handleLine(const char* line, const size_t len)
{
  if (len == 0)
  {
    return;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, line, len);
  if (error)
  {
    // not JSON - just a plain log line sharing the same UART, ignore it
    return;
  }

  handleMessage(doc);
}

void SerialLink::handleMessage(JsonDocument& doc)
{
  const char* type = doc["type"] | "";

  if (strcmp(type, "get_config") == 0)
  {
    sendArousalConfig();
    return;
  }

  if (strcmp(type, "arousal_state") == 0)
  {
    if (!doc["reset"].isNull() && doc["reset"].as<bool>())
    {
      _arousalManager.end();
      _arousalManager.reset();
      _arousalManager.recalibratePressure();
      _arousalManager.begin();
      return;
    }

    if (!doc["active"].isNull())
    {
      const bool active = doc["active"].as<bool>();
      if (active && !_arousalManager.isActive())
      {
        _arousalManager.begin();
      }
      else if (!active && _arousalManager.isActive())
      {
        _arousalManager.end();
      }
    }
    return;
  }

  if (strcmp(type, "arousal_sensitivity") == 0)
  {
    if (!doc["sensitivity"].isNull())
    {
      const int sensitivity = constrain(doc["sensitivity"].as<int>(), 0, 255);
      _arousalManager.setSensitivity(sensitivity);
    }
    return;
  }

  if (strcmp(type, "arousal_config") == 0)
  {
    ArousalConfig config = _arousalManager.getConfig();

    if (!doc["arousalDecayRate"].isNull())
    {
      config.arousalDecayRate = doc["arousalDecayRate"].as<float>();
    }
    if (!doc["sensitivityThreshold"].isNull())
    {
      config.sensitivityThreshold = doc["sensitivityThreshold"].as<int>();
    }
    if (!doc["sensitivityAfterEdgeDecayRate"].isNull())
    {
      config.sensitivityAfterEdgeDecayRate = doc["sensitivityAfterEdgeDecayRate"].as<float>();
    }
    if (!doc["minSensitivityWhileDecaying"].isNull())
    {
      config.minSensitivityWhileDecaying = doc["minSensitivityWhileDecaying"].as<int>();
    }
    if (!doc["maxArousalLimit"].isNull())
    {
      config.maxArousalLimit = doc["maxArousalLimit"].as<int>();
    }
    if (!doc["minVibrationLevel"].isNull())
    {
      const int level = constrain(doc["minVibrationLevel"].as<int>(), 0, SPEED_LEVEL_MAX);
      config.minSpeed = ArousalManager::levelToSpeed(level);
    }
    if (!doc["maxVibrationLevel"].isNull())
    {
      const int level = constrain(doc["maxVibrationLevel"].as<int>(), 0, SPEED_LEVEL_MAX);
      config.maxSpeed = ArousalManager::levelToSpeed(level);
    }
    if (!doc["frequency"].isNull())
    {
      config.frequency = doc["frequency"].as<int>();
    }
    if (!doc["rampTimeSeconds"].isNull())
    {
      config.rampTimeSeconds = doc["rampTimeSeconds"].as<float>();
    }
    if (!doc["coolTimeSeconds"].isNull())
    {
      config.coolTimeSeconds = doc["coolTimeSeconds"].as<float>();
    }
    if (!doc["clenchPressureSensitivity"].isNull())
    {
      config.clenchPressureSensitivity = doc["clenchPressureSensitivity"].as<int>();
    }
    if (!doc["clenchTimeMinThresholdMs"].isNull())
    {
      config.clenchTimeMinThresholdMs = doc["clenchTimeMinThresholdMs"].as<int>();
    }
    if (!doc["clenchTimeMaxThresholdMs"].isNull())
    {
      config.clenchTimeMaxThresholdMs = doc["clenchTimeMaxThresholdMs"].as<int>();
    }
    if (!doc["targetEdgeCount"].isNull())
    {
      config.targetEdgeCount = doc["targetEdgeCount"].as<int>();
    }

    _arousalManager.setConfig(config);
    _config.setArousalConfig(config);
    // ReSharper disable once CppExpressionWithoutSideEffects
    _config.save();
    return;
  }

  if (strcmp(type, "handy_status") == 0)
  {
    if (!doc["connected"].isNull())
    {
      _deviceOutput.setHostConnected(doc["connected"].as<bool>());
    }
    return;
  }
}

void SerialLink::sendArousalStatus()
{
  JsonDocument doc;
  doc["type"] = "arousal_status";
  doc["active"] = _arousalManager.isActive();
  doc["arousalPercent"] = _arousalManager.getArousalPercent();
  doc["pressure"] = _arousalManager.getCurrentPressure();
  doc["limit"] = _arousalManager.getArousalLimit();
  doc["limitExceededCounter"] = _arousalManager.getLimitExceededCounter();
  doc["sensitivity"] = _arousalManager.getSensitivity();
  doc["currentSessionDuration"] = _arousalManager.getCurrentSessionDuration();
  doc["clenchThreshold"] = _arousalManager.getConfig().clenchPressureThreshold;
  doc["lastClenchDuration"] = _arousalManager.getLastClenchDuration();
  doc["state"] = _arousalManager.getCurrentStateString();

  serializeJson(doc, Serial);
  Serial.println();
}

void SerialLink::sendArousalConfig()
{
  const ArousalConfig& config = _arousalManager.getConfig();

  JsonDocument doc;
  doc["type"] = "arousal_config";
  doc["arousalDecayRate"] = config.arousalDecayRate;
  doc["sensitivityAfterEdgeDecayRate"] = config.sensitivityAfterEdgeDecayRate;
  doc["minSensitivityWhileDecaying"] = config.minSensitivityWhileDecaying;
  doc["sensitivityThreshold"] = config.sensitivityThreshold;
  doc["maxPressureLimit"] = _arousalManager.getPressureLimit();
  doc["maxArousalLimit"] = config.maxArousalLimit;
  doc["minVibrationLevel"] = ArousalManager::speedToLevel(config.minSpeed);
  doc["maxVibrationLevel"] = ArousalManager::speedToLevel(config.maxSpeed);
  doc["frequency"] = config.frequency;
  doc["rampTimeSeconds"] = config.rampTimeSeconds;
  doc["coolTimeSeconds"] = config.coolTimeSeconds;
  doc["targetEdgeCount"] = config.targetEdgeCount;
  doc["clenchPressureThreshold"] = config.clenchPressureThreshold;
  doc["clenchPressureSensitivity"] = config.clenchPressureSensitivity;
  doc["clenchTimeMinThresholdMs"] = config.clenchTimeMinThresholdMs;
  doc["clenchTimeMaxThresholdMs"] = config.clenchTimeMaxThresholdMs;

  serializeJson(doc, Serial);
  Serial.println();
}
