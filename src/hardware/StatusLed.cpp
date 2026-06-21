/*
Project structure (current relevant excerpt):
src/
  main.cpp
  hardware/
    StatusLed.h
    StatusLed.cpp
*/

#include "hardware/StatusLed.h"

namespace hardware {

StatusLed::StatusLed(uint8_t pin, uint8_t brightness)
    : pin_(pin), brightness_(brightness) {}

void StatusLed::show(Color color) const {
  switch (color) {
    case Color::Off:
      writeRgb(0, 0, 0);
      break;
    case Color::Red:
      writeRgb(brightness_, 0, 0);
      break;
    case Color::Green:
      writeRgb(0, brightness_, 0);
      break;
    case Color::Blue:
      writeRgb(0, 0, brightness_);
      break;
  }
}

void StatusLed::setEnabled(bool enabled) const {
  show(enabled ? Color::Green : Color::Off);
}

void StatusLed::writeRgb(uint8_t red, uint8_t green, uint8_t blue) const {
  // The onboard LED uses GRB channel order.
  neopixelWrite(pin_, green, red, blue);
}

}  // namespace hardware
