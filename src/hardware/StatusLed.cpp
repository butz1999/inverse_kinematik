// Maps logical status colors to the ESP32-S3 onboard RGB LED.

#include "hardware/StatusLed.h"

namespace hardware
{

StatusLed::StatusLed(uint8_t pin, uint8_t brightness) : pin_(pin), brightness_(brightness)
{
}

void StatusLed::show(Color color) const
{
  writeColor(toNeopixelWriteColor(color, brightness_));
}

void StatusLed::setEnabled(bool enabled, Color color) const
{
  show(enabled ? color : Color::Off);
}

void StatusLed::writeColor(NeopixelWriteColor color) const
{
  neopixelWrite(pin_, color.red_arg, color.green_arg, color.blue_arg);
}

}  // namespace hardware
