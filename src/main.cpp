// Firmware entry point.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include "application/RestApiServer.h"
#include "hardware/Pca9685ServoDriver.h"
#include "hardware/SerialLogger.h"
#include "hardware/StatusLed.h"

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
constexpr unsigned long kBlinkIntervalMs = 500;
// Debug console
constexpr unsigned long kSerialBaudrate = 115200;
constexpr unsigned long kWifiConnectTimeoutMs = 15000;
constexpr const char *kDebugSerialName = "Serial";
// I2C bus
constexpr uint8_t kI2cSdaPin = 4;
constexpr uint8_t kI2cSclPin = 5;
constexpr uint8_t kI2cOePin = 6;
// Webserver
constexpr uint16_t kHttpPort = 80;
// WiFi
constexpr const char *kNetworkHostname = "robot";
constexpr const char *kWifiSsid = IK_WIFI_SSID;
constexpr const char *kWifiPassword = IK_WIFI_PASSWORD;

// RGB LED blinker
unsigned long lastToggleMs = 0;
unsigned long heartbeatCount = 0;
bool isLedOn = true;
hardware::StatusColor color = hardware::StatusColor::Off;

// Create Components
hardware::SerialLogger logger(Serial, kSerialBaudrate);
hardware::StatusLed statusLed(kRgbLedPin, kRgbBrightness);
WebServer webServer(kHttpPort);
auto servoDriverConfig = hardware::defaultPca9685ServoDriverConfig();
hardware::Pca9685ServoDriver servoDriver(servoDriverConfig);
application::RestApiServer restApi(webServer, servoDriver, logger);

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
  color = hardware::StatusLed::Color::Yellow;
  statusLed.show(color);
  logger.begin();
  // Report success
  logger.println();
  logger.println("[BOOT] Firmware setup reached");
  logger.print("[BOOT] Debug serial ready on ");
  logger.println(kDebugSerialName);
  logger.println("[BOOT] Status LED configured for GRB order on GPIO38");
  // Enable PCA9685 PWM outputs
  pinMode(kI2cOePin, OUTPUT);
  digitalWrite(kI2cOePin, LOW);
  logger.print("[BOOT] PCA9685 OE configured active-low on GPIO");
  logger.println(kI2cOePin);
  // Start I2C bus
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  logger.print("[BOOT] I2C bus configured: SDA GPIO");
  logger.print(kI2cSdaPin);
  logger.print(" SCL GPIO");
  logger.println(kI2cSclPin);
  delay(250);
  // Connect Wifi
  color = hardware::StatusLed::Color::Blue;
  statusLed.show(color);
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
  // Register REST API
  restApi.begin();
  logger.println("[BOOT] REST API endpoints registered");
  statusLed.show(color);
}

void loop()
{
  const auto now = millis();
  restApi.handleClient();

  if (now - lastToggleMs >= kBlinkIntervalMs)
  {
    lastToggleMs = now;
    statusLed.setEnabled(isLedOn, color);
    isLedOn = !isLedOn;
    ++heartbeatCount;
  }
}
