// Controls the onboard RGB status LED with a fixed brightness level.

#pragma once

#include <Arduino.h>

#include "hardware/StatusLedColor.h"

namespace hardware
{

class StatusLed
{
 public:
  using Color = StatusColor;

  StatusLed(uint8_t pin, uint8_t brightness);

  void show(Color color) const;
  void setEnabled(bool enabled, Color color) const;

 private:
  void writeColor(NeopixelWriteColor color) const;

  uint8_t pin_;
  uint8_t brightness_;
};

}  // namespace hardware
