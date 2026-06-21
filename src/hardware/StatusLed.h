/*
Project structure (current relevant excerpt):
src/
  main.cpp
  hardware/
    StatusLed.h
    StatusLed.cpp
*/

#pragma once

#include <Arduino.h>

namespace hardware {

class StatusLed {
 public:
  enum class Color {
    Off,
    Red,
    Green,
    Blue,
  };

  StatusLed(uint8_t pin, uint8_t brightness);

  void show(Color color) const;
  void setEnabled(bool enabled) const;

 private:
  void writeRgb(uint8_t red, uint8_t green, uint8_t blue) const;

  uint8_t pin_;
  uint8_t brightness_;
};

}  // namespace hardware
