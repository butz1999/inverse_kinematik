// Firmware entry point.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

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

namespace {

constexpr uint8_t kRgbLedPin = 38;
constexpr uint8_t kRgbBrightness = 15;
constexpr unsigned long kBlinkIntervalMs = 500;
constexpr unsigned long kSerialBaudrate = 115200;
constexpr unsigned long kWifiConnectTimeoutMs = 15000;
constexpr uint16_t kHttpPort = 80;
constexpr const char *kNetworkHostname = "robot";

constexpr const char *kWifiSsid = IK_WIFI_SSID;
constexpr const char *kWifiPassword = IK_WIFI_PASSWORD;

unsigned long lastToggleMs = 0;
unsigned long heartbeatCount = 0;
bool isGreenOn = true;
constexpr const char *kDebugSerialName = "Serial";
hardware::SerialLogger logger(Serial, kSerialBaudrate);
hardware::StatusLed statusLed(kRgbLedPin, kRgbBrightness);
WebServer webServer(kHttpPort);
auto servoDriverConfig = hardware::defaultPca9685ServoDriverConfig();
hardware::Pca9685ServoDriver servoDriver(servoDriverConfig);
application::RestApiServer restApi(webServer, servoDriver, logger);

bool isConfigured(const char *value) {
  return value != nullptr && value[0] != '\0';
}

bool connectWifi() {
  if (!isConfigured(kWifiSsid)) {
    logger.println("[BOOT] WiFi STA not configured: IK_WIFI_SSID is empty");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(kNetworkHostname);
  logger.print("[BOOT] Connecting to WiFi SSID: ");
  logger.println(kWifiSsid);

  if (isConfigured(kWifiPassword)) {
    WiFi.begin(kWifiSsid, kWifiPassword);
  } else {
    WiFi.begin(kWifiSsid);
  }

  const auto startedAtMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAtMs < kWifiConnectTimeoutMs) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    logger.println("[BOOT] WiFi STA connection failed");
    return false;
  }

  logger.print("[BOOT] WiFi connected, IP address: ");
  logger.println(WiFi.localIP().toString().c_str());

  if (MDNS.begin(kNetworkHostname)) {
    logger.print("[BOOT] mDNS responder available at http://");
    logger.print(kNetworkHostname);
    logger.println(".local");
  } else {
    logger.println("[BOOT] mDNS responder failed to start");
  }

  return true;
}

}  // namespace

void setup() {
  statusLed.show(hardware::StatusLed::Color::Red);

  logger.begin();
  delay(250);

  statusLed.show(hardware::StatusLed::Color::Blue);
  logger.println();
  logger.println("[BOOT] Firmware setup reached");
  logger.print("[BOOT] Debug serial ready on ");
  logger.println(kDebugSerialName);
  logger.println("[BOOT] Status LED configured for GRB order on GPIO38");
  delay(250);

  const auto wifiConnected = connectWifi();
  if (wifiConnected) {
    logger.print("[BOOT] REST API listening on http://");
    logger.print(WiFi.localIP().toString().c_str());
    logger.println("/api/health");
    logger.println("[BOOT] or use it's hostname");
    logger.println("[BOOT] REST API listening on http://robot.fritz.box/api/status");
  }

  restApi.begin();
  logger.println("[BOOT] REST API endpoints registered");

  statusLed.show(hardware::StatusLed::Color::Green);
}

void loop() {
  const auto now = millis();
  restApi.handleClient();

  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    statusLed.setEnabled(isGreenOn);
    isGreenOn = !isGreenOn;
    ++heartbeatCount;
  }
}
