/*
Project structure (current relevant excerpt):
src/
  main.cpp
  hardware/
    StatusLed.h
    StatusLed.cpp
    SerialLogger.h
    SerialLogger.cpp
*/

#pragma once

#include <Arduino.h>

namespace hardware {

class SerialLogger {
 public:
  SerialLogger(HardwareSerial &serial, unsigned long baudrate);

  void begin() const;
  void print(const char *message) const;
  void println() const;
  void println(const char *message) const;

 private:
  HardwareSerial &serial_;
  unsigned long baudrate_;
};

}  // namespace hardware
