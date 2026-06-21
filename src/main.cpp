/*
Project structure (current relevant excerpt):
src/
  main.cpp
*/

#include <Arduino.h>

namespace {

constexpr uint8_t kRgbLedPin = 38;
constexpr uint8_t kRgbBrightness = 255;
constexpr unsigned long kBlinkIntervalMs = 500;
constexpr unsigned long kSerialBaudrate = 115200;

unsigned long lastToggleMs = 0;
unsigned long heartbeatCount = 0;
bool isGreenOn = false;
auto &debugSerial = Serial;
constexpr const char *kDebugSerialName = "Serial";

void writeStatusLed(uint8_t red, uint8_t green, uint8_t blue) {
  // The onboard LED uses GRB channel order.
  neopixelWrite(kRgbLedPin, green, red, blue);
}

void setStatusLed(bool enabled) {
  writeStatusLed(0, enabled ? kRgbBrightness : 0, 0);
}

void setStatusLedRed() {
  writeStatusLed(kRgbBrightness, 0, 0);
}

void setStatusLedBlue() {
  writeStatusLed(0, 0, kRgbBrightness);
}

void logLine(const char *message) {
  debugSerial.println(message);
  debugSerial.flush();
}

}  // namespace

void setup() {
  setStatusLedRed();

  debugSerial.begin(kSerialBaudrate);
  delay(200);

  setStatusLedBlue();
  logLine("");
  logLine("[BOOT] Firmware setup reached");
  debugSerial.print("[BOOT] Debug serial ready on ");
  debugSerial.println(kDebugSerialName);
  debugSerial.flush();
  logLine("[BOOT] Status LED configured for GRB order on GPIO38");
  delay(500);
  setStatusLed(false);
}

void loop() {
  const auto now = millis();

  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    isGreenOn = !isGreenOn;
    setStatusLed(isGreenOn);
    ++heartbeatCount;

    debugSerial.print("[DEBUG] Heartbeat #");
    debugSerial.print(heartbeatCount);
    debugSerial.print(" LED=");
    debugSerial.println(isGreenOn ? "GREEN" : "OFF");
    debugSerial.flush();
  }
}
