// Pure status LED color mapping used by the hardware driver and native tests.

#pragma once

#include <cstdint>

namespace hardware {

enum class StatusColor {
  Off,
  Red,
  Orange,
  Yellow,
  Green,
  Cyan,
  Blue,
  Violet,
};

struct NeopixelWriteColor {
  uint8_t red_arg;
  uint8_t green_arg;
  uint8_t blue_arg;
};

NeopixelWriteColor toNeopixelWriteColor(StatusColor color, uint8_t brightness);

}  // namespace hardware
