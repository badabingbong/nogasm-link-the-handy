#include <Arduino.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "NogasmBLEManager.h"
#include "NogasmHttp.h"
#include "NogasmConfig.h"
#include "ArousalManager.h"
#include "EncoderManager.h"
#include "RGBManager.h"
#include "Util.h"
#include "HandyOutput.h"
#include "SerialDeviceOutput.h"
#include "CompositeDeviceOutput.h"
#include "SerialLink.h"

#define HX710_SCK_PIN 32
#define HX710_OUT_PIN 26
#define NEOPIXEL_PIN 27
#define FORMAT_LITTLEFS_IF_FAILED true
// Handy control and the dashboard now go through laptop-app/ over USB serial
// (see SerialLink) instead of this device hosting WiFi itself - no WiFi/HTTP
// stack needed on the ESP32 at all. Flip this back on if you ever want to
// return to the ESP32-hosted WiFi dashboard (NogasmHttp) instead.
#define ENABLE_WIFI_WEB_SERVER false
// The ESP32 has a single 2.4GHz radio shared between WiFi and BLE. Bringing up
// NimBLE can starve or hang WiFi bring-up/operation on some boards/firmware combos.
// If you don't need a Lovense BLE toy (e.g. Handy-only setups), turn this off to
// skip BLE entirely and eliminate that contention. Safe to re-enable now that
// WiFi is off above - the radio contention was specifically BLE-vs-WiFi.
#define ENABLE_BLE false
#define FILESYSTEM LittleFS
#define CONFIG_FILE "/config.json"
#define HOST_NAME "NogasmLink"

// we add samples every (1000/frequency)~16.6ms
// so if we want a window of 2 seconds, that is (2*1000)/16.66~120samples
#define PRESSURE_WINDOW_SECONDS 2

WiFiManager wifiManager;
NogasmConfig nogasmConfig(FILESYSTEM, CONFIG_FILE);  // NOLINT(*-interfaces-global-init)
NogasmBLEManager nogasmBLEManager(nogasmConfig);
SerialDeviceOutput serialDeviceOutput;
CompositeDeviceOutput deviceOutput(nogasmBLEManager, serialDeviceOutput);
// Only used if ENABLE_WIFI_WEB_SERVER is flipped back on - the active output
// path is serialDeviceOutput above. Kept wired so NogasmHttp's WiFi-hosted
// dashboard (including its own Handy-over-WiFi support) still works if you
// ever want to go back to it.
HandyOutput handyOutput("", "");
const int pressureSamples = (PRESSURE_WINDOW_SECONDS * 1000) / (1000 / nogasmConfig.getArousalConfig().frequency);
PressureSensor pressureSensor(HX710_SCK_PIN, HX710_OUT_PIN, pressureSamples);
ArousalManager arousalManager(pressureSensor, deviceOutput);
EncoderManager encoderManager(arousalManager);
RGBManager rgbManager(NEOPIXEL_PIN);
SerialLink serialLink(arousalManager, nogasmConfig, serialDeviceOutput);
NogasmHttp nogasmHttp(FILESYSTEM, nogasmBLEManager, wifiManager, nogasmConfig, arousalManager, encoderManager, handyOutput);  // NOLINT(*-interfaces-global-init)

// Function prototypes
void setupBLE();
void setupWifiAndWebServer();
void setupNogasmLink();
void onBLEStatusChange(BLEConnectionState state);
void onArousalStateChange(const ArousalStateEvent& event);

void setup()
{
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  if (!FILESYSTEM.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Util::logInfo("LittleFS Mount Failed");
    return;
  }

  Util::logInfo("Starting up %s...", HOST_NAME);
  rgbManager.begin();
  rgbManager.setLEDState(LEDState::OFF);

  if (ENABLE_BLE)
  {
    setupBLE();
  }

  setupWifiAndWebServer();
  setupNogasmLink();

  if (ENABLE_BLE)
  {
    // when the scan completes, all auto(re)connect behavior initializes
    nogasmBLEManager.startScan(nogasmConfig.getScanDuration());
  }
}

void setupNogasmLink()
{
  nogasmConfig.load();

  arousalManager.setConfig(nogasmConfig.getArousalConfig());
  arousalManager.setArousalLimit(nogasmConfig.getArousalLimit());
  arousalManager.onStateChange(onArousalStateChange);

  encoderManager.begin();
  pressureSensor.begin();
  pressureSensor.calibrateZero();

  if (ENABLE_WIFI_WEB_SERVER)
  {
    // Handy control normally happens over serial via laptop-app/ (see SerialDeviceOutput) -
    // this only matters if you've flipped ENABLE_WIFI_WEB_SERVER back on.
    handyOutput.setKeys(nogasmConfig.getHandyConnectionKey(), nogasmConfig.getHandyAppKey());
    if (nogasmConfig.hasHandyCredentials())
    {
      Util::logInfo("Connecting to The Handy...");
      if (!handyOutput.begin())
      {
        Util::logInfo("Could not reach The Handy yet - will keep retrying in the background");
      }
    }
  }
}

void setupWifiAndWebServer()
{
  if (!ENABLE_WIFI_WEB_SERVER)
  {
    return;
  }

  wifiManager.setDebugOutput(NOGASM_LOG_LEVEL == 2);
  wifiManager.setHostname(HOST_NAME);
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  rgbManager.setLEDState(LEDState::WIFI_SETUP_REQUIRED);

  const bool connected = wifiManager.autoConnect(HOST_NAME);
  if (!connected)
  {
    Util::logInfo("Failed to connect to WiFi.");
  }
  else
  {
    Util::logInfo("WiFi Connected! IP Address: %s", WiFi.localIP().toString().c_str());
    rgbManager.setLEDState(LEDState::WIFI_CONNECTED);

    if (MDNS.begin(HOST_NAME))
    {
      Util::logInfo("mDNS responder started, Device can be reached at: %s.local", HOST_NAME);
      MDNS.addService("http", "tcp", NOGASM_HTTP_PORT);
    }
    else
    {
      Util::logInfo("Error setting up mDNS responder!");
    }
  }

  nogasmHttp.begin();
}

void loop()
{
  encoderManager.update();
  if (ENABLE_BLE)
  {
    nogasmBLEManager.update();
  }
  if (ENABLE_WIFI_WEB_SERVER)
  {
    handyOutput.update();
  }
  serialLink.update();
  arousalManager.update();
  rgbManager.update();

  if (ENABLE_WIFI_WEB_SERVER)
  {
    nogasmHttp.update();
  }
}

void setupBLE()
{
  Util::logInfo("Initializing BLE...");
  nogasmBLEManager.begin(HOST_NAME);
  nogasmBLEManager.onStatusChange(onBLEStatusChange);
}

void onBLEStatusChange(BLEConnectionState state)
{
  switch (state)
  {
    case BLE_SCANNING:
      rgbManager.setLEDState(LEDState::SCANNING);
      break;

    case BLE_CONNECTING:
    case BLE_RECONNECTING:
    case BLE_FINDING_SERVICE:
      rgbManager.setLEDState(LEDState::CONNECTING);
      break;

    case BLE_CONNECTED:
    {
      const CompatibleDevice* device = nogasmBLEManager.getCurrentDevice();
      if (device != nullptr)
      {
        rgbManager.setLEDState(LEDState::CONNECTED);
        nogasmConfig.saveDeviceInfo(device->address.c_str(), device->name.c_str());
      }
      break;
    }
    case BLE_FAILED:
      rgbManager.setLEDState(LEDState::BLE_ERROR);
      arousalManager.end();
      break;

    case BLE_IDLE:
      if (WiFi.status() == WL_CONNECTED)
      {
        rgbManager.setLEDState(LEDState::WIFI_CONNECTED);
      }
      else
      {
        rgbManager.setLEDState(LEDState::OFF);
      }

      arousalManager.end();
      break;
  }
}

void onArousalStateChange(const ArousalStateEvent& event)
{
  switch (event.state)
  {
    case ArousalState::ERROR:
      rgbManager.setLEDState(LEDState::PRESSURE_ERROR);
      break;

    case ArousalState::IDLE:
      if (rgbManager.getLEDState() == LEDState::AROUSAL_ACTIVE)
      {
        if (nogasmBLEManager.isConnectedState())
        {
          rgbManager.setLEDState(LEDState::CONNECTED);
        }
        else if (WiFi.status() == WL_CONNECTED)
        {
          rgbManager.setLEDState(LEDState::WIFI_CONNECTED);
        }
        else
        {
          rgbManager.setLEDState(LEDState::OFF);
        }
      }
      break;

    case ArousalState::AROUSAL_INCREASE:
    case ArousalState::VIBRATION_CHANGE:
    case ArousalState::LIMIT_EXCEEDED:
    case ArousalState::COOL_OFF_ACTIVE:
    case ArousalState::COOL_OFF_ENDED:
    case ArousalState::CLENCH_DETECTED:
    case ArousalState::THRESHOLD_ADJUSTED:
      rgbManager.setLEDState(LEDState::AROUSAL_ACTIVE);
      break;

    case ArousalState::LIMIT_EXCEEDED_BUT_IGNORED:
      rgbManager.setLEDState(LEDState::AROUSAL_ALLOW_ORGASM);
      break;

    case ArousalState::AROUSAL_LIMIT_CHANGE:
      nogasmConfig.setArousalLimit(event.arousalLimit);
      // ReSharper disable once CppExpressionWithoutSideEffects
      nogasmConfig.save();
      break;
    default:
      break;
  }
}