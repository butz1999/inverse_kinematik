// Firmware entry point.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include "application/RestApiServer.h"
#include "hardware/Pca9685ServoDriver.h"
#include "hardware/Pin.h"
#include "hardware/SerialLogger.h"
#include "hardware/StatusLed.h"

// ToDo: Klären, ob das genügt, oder aber architektonische Änderungen nötig sind.
SET_LOOP_TASK_STACK_SIZE(16 * 1024)

#if __has_include("config/WifiCredentials.h")
#include "config/WifiCredentials.h"
#else
#warning "src/config/WifiCredentials.h not found; using empty default WiFi credentials"
#define IK_WIFI_SSID ""
#define IK_WIFI_PASSWORD ""
#endif

namespace
{
// RGB LED
constexpr uint8_t kRgbLedPin = 38;
constexpr uint8_t kRgbBrightness = 15;
constexpr uint32_t kStatusBlinkIntervalMs = 500;
// Debug console
constexpr unsigned long kSerialBaudrate = 115200;
constexpr unsigned long kWifiConnectTimeoutMs = 15000;
constexpr const char *kDebugSerialName = "Serial";
// I2C bus
constexpr uint8_t kI2cSdaPin = 4;
constexpr uint8_t kI2cSclPin = 5;
// PCA9685 Output Enable
constexpr uint8_t kPca9685OePin = 6;
// Webserver
constexpr uint16_t kHttpPort = 80;
// WiFi
constexpr const char *kNetworkHostname = "robot";
constexpr const char *kWifiSsid = IK_WIFI_SSID;
constexpr const char *kWifiPassword = IK_WIFI_PASSWORD;

hardware::StatusColor color = hardware::StatusColor::Off;

// Create Components
hardware::Pin<kPca9685OePin> pinPcaOe;
hardware::SerialLogger logger(Serial, kSerialBaudrate);
hardware::StatusLed statusLed(kRgbLedPin, kRgbBrightness);
WebServer webServer(kHttpPort);
hardware::Pca9685ServoDriver servoDriver;
application::RestApiServer restApi(webServer, servoDriver, statusLed, logger);

bool isConfigured(const char *value)
{
  return value != nullptr && value[0] != '\0';
}

bool connectWifi()
{
  // Check valid SSID
  if (!isConfigured(kWifiSsid))
  {
    logger.println("[BOOT] WiFi STA not configured: IK_WIFI_SSID is empty");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(kNetworkHostname);
  logger.print("[BOOT] Connecting to WiFi SSID: ");
  logger.println(kWifiSsid);

  // Check valid password, start with or without password
  if (isConfigured(kWifiPassword))
  {
    WiFi.begin(kWifiSsid, kWifiPassword);
  }
  else
  {
    WiFi.begin(kWifiSsid);
  }
  // Start WiFi with connection timeout
  const auto startedAtMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAtMs < kWifiConnectTimeoutMs)
  {
    delay(250);
  }
  // Abort if connection failed
  if (WiFi.status() != WL_CONNECTED)
  {
    logger.println("[BOOT] WiFi STA connection failed");
    return false;
  }
  // Otherwise log success
  logger.print("[BOOT] WiFi connected, IP address: ");
  logger.println(WiFi.localIP().toString().c_str());

  return true;
}

bool startMdns()
{
  // Register mDNS
  if (!MDNS.begin(kNetworkHostname))
  {
    logger.println("[BOOT] mDNS responder failed to start");
    return false;
  }
  // Register mDNS services
  if (!MDNS.addService("http", "tcp", kHttpPort))
  {
    logger.println("[BOOT] mDNS service registration failed");
    return false;
  }
  // Report success
  logger.print("[BOOT] mDNS responder available at http://");
  logger.print(kNetworkHostname);
  logger.println(".local");

  return true;
}

}  // namespace

void setup()
{
  // Start logger
  logger.init();
  delay(300);
  logger.println();
  logger.println("[BOOT] Firmware setup entered");
  color = hardware::StatusLed::Color::Yellow;
  logger.println("[BOOT] Setting initial status LED");
  statusLed.set(color);
  // Report success
  logger.println("[BOOT] Firmware setup reached");
  logger.print("[BOOT] Debug serial ready on ");
  logger.println(kDebugSerialName);
  logger.println("[BOOT] Status LED configured for GRB order on GPIO38");
  // Disable PCA OE pin
  pinPcaOe.mode(OUTPUT);
  pinPcaOe.disableLow();
  logger.print("[BOOT] PCA9685 OE disabled active-low on GPIO");
  logger.println(kPca9685OePin);
  // Start I2C bus
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  logger.print("[BOOT] I2C bus configured: SDA GPIO");
  logger.print(kI2cSdaPin);
  logger.print(" SCL GPIO");
  logger.println(kI2cSclPin);
  // Start servo driver
  const auto servo_driver_init_result = servoDriver.init();
  const bool servoDriverInitialized = servo_driver_init_result.status == hardware::HardwareDriverStatus::Ok;
  if (servoDriverInitialized)
  {
    logger.println("[BOOT] PCA9685 servo driver initialized");
    // Enable PCA OE pin
    pinPcaOe.enableLow();
    logger.print("[BOOT] PCA9685 OE enabled active-low on GPIO");
    logger.println(kPca9685OePin);
  }
  else
  {
    logger.print("[BOOT] PCA9685 servo driver initialization failed: ");
    logger.println(servo_driver_init_result.message);
    color = hardware::StatusLed::Color::Orange;
    statusLed.set(color);
  }
  delay(250);
  // Connect Wifi
  color = hardware::StatusLed::Color::Blue;
  statusLed.set(color);
  const auto wifiConnected = connectWifi();
  if (wifiConnected)
  {
    // Report success
    logger.print("[BOOT] REST API listening on http://");
    logger.print(WiFi.localIP().toString().c_str());
    logger.println("/api/health");
    // Start mDNS
    const auto mdnsStarted = startMdns();
    if (mdnsStarted)
    {
      // Report success
      logger.println("[BOOT] or use its hostname");
      logger.print("[BOOT] REST API listening on http://");
      logger.print(kNetworkHostname);
      logger.println(".local/api/status");
      color = hardware::StatusLed::Color::Green;
    }
    else
    {
      logger.println("[BOOT] mDNS not started");
      color = hardware::StatusLed::Color::Orange;
    }
  }
  else
  {
    logger.println("[BOOT] Wifi not started");
    color = hardware::StatusLed::Color::Red;
  }
  if (!servoDriverInitialized && wifiConnected)
  {
    color = hardware::StatusLed::Color::Orange;
  }
  statusLed.setColor(color);
  delay(250);
  // Register REST API
  restApi.init();
  logger.println("[BOOT] REST API endpoints registered");
  statusLed.set(color, hardware::StatusLed::Mode::Pulsing, kStatusBlinkIntervalMs);
  delay(250);
  logger.println("[BOOT] setup() finished, entering loop()");
}

void loop()
{
  const auto now = millis();
  restApi.handleClient();
  statusLed.service(now);
}
