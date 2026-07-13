// Firmware entry point.

#include <Arduino.h>

#include "hardware/SerialLogger.h"
#include "hardware/StatusLed.h"

namespace {

constexpr uint8_t kRgbLedPin = 38;
constexpr uint8_t kRgbBrightness = 255;
constexpr unsigned long kBlinkIntervalMs = 500;
constexpr unsigned long kSerialBaudrate = 115200;

unsigned long lastToggleMs = 0;
unsigned long heartbeatCount = 0;
bool isGreenOn = true;
constexpr const char *kDebugSerialName = "Serial";
hardware::SerialLogger logger(Serial, kSerialBaudrate);
hardware::StatusLed statusLed(kRgbLedPin, kRgbBrightness);

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
  statusLed.show(hardware::StatusLed::Color::Green);
}

void loop() {
  const auto now = millis();

  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    statusLed.setEnabled(isGreenOn);
    isGreenOn = !isGreenOn;
    ++heartbeatCount;
  }
}
