// Firmware entry point.

#include <Arduino.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Wire.h>

#include "application/FirmwareBootstrap.h"
#include "application/RestApiServer.h"
#include "hardware/Pca9685ServoDriver.h"
#include "hardware/Pin.h"
#include "hardware/SerialLogger.h"
#include "hardware/StatusLed.h"

// The REST API handlers allocate JSON documents on the loop task stack.
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
constexpr uint32_t kWifiConnectTimeoutMs = 15000U;
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

// Create Components
hardware::Pin<kPca9685OePin> pinPcaOe;
hardware::SerialLogger logger(Serial, kSerialBaudrate);
hardware::StatusLed statusLed(kRgbLedPin, kRgbBrightness);
WebServer webServer(kHttpPort);
hardware::Pca9685ServoDriver servoDriver;
application::RestApiServer restApi(webServer, SPIFFS, servoDriver, statusLed, logger);
const application::FirmwareBootstrapConfig bootstrapConfig{kWifiSsid, kWifiPassword,         kNetworkHostname,
                                                           kHttpPort, kWifiConnectTimeoutMs, kStatusBlinkIntervalMs};
application::FirmwareBootstrap firmwareBootstrap(bootstrapConfig, SPIFFS, restApi, statusLed, logger);

extern "C" void ik_switch2_pro_ble_input_report(const uint8_t *report, uint16_t report_size)
{
  restApi.ingestSwitch2ProBleInputReport(report, report_size, millis());
}

}  // namespace

void setup()
{
  // Start logger
  logger.init();
  logger.println();
  logger.println("[BOOT] Firmware setup entered");
  logger.println("[BOOT] Setting initial status LED");
  statusLed.set(hardware::StatusLed::Color::Yellow);
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
  const auto servoDriverInitialized = servo_driver_init_result.status == hardware::HardwareDriverStatus::Ok;
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
    statusLed.set(hardware::StatusLed::Color::Orange);
  }
  firmwareBootstrap.begin(millis(), servoDriverInitialized);
}

void loop()
{
  const auto now = millis();
  if (!firmwareBootstrap.isFinished())
  {
    firmwareBootstrap.update(now);
    return;
  }

  restApi.handleClient();
  statusLed.updateOutput(now);
}
