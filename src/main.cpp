/*
Project structure (current relevant excerpt):
src/
  main.cpp
*/

#include <Arduino.h>

namespace {

constexpr unsigned long kSerialBaudrate = 115200;
constexpr unsigned long kHeartbeatIntervalMs = 1000;

unsigned long lastHeartbeatMs = 0;
unsigned long heartbeatCount = 0;

}  // namespace

void setup() {
  Serial.begin(kSerialBaudrate);

  // Give the serial monitor a brief moment to attach after reset.
  delay(200);

  Serial.println();
  Serial.println("[BOOT] ESP32-S3 firmware start");
  Serial.println("[BOOT] Debug serial ready");
}

void loop() {
  const auto now = millis();
  if (now - lastHeartbeatMs >= kHeartbeatIntervalMs) {
    lastHeartbeatMs = now;
    ++heartbeatCount;

    Serial.print("[DEBUG] Heartbeat #");
    Serial.print(heartbeatCount);
    Serial.print(" at ");
    Serial.print(now);
    Serial.println(" ms");
  }
}
