// Converts logical colors to the argument order needed by the onboard LED.

#include "hardware/StatusLedColor.h"

namespace hardware
{

NeopixelWriteColor toNeopixelWriteColor(StatusColor color, uint8_t brightness)
{
  switch (color)
  {
    case StatusColor::Off:
      return {0, 0, 0};
    case StatusColor::Red:
      return {0, brightness, 0};
    case StatusColor::Orange:
      return {static_cast<uint8_t>(brightness / 2), brightness, 0};
    case StatusColor::Yellow:
      return {brightness, brightness, 0};
    case StatusColor::Green:
      return {brightness, 0, 0};
    case StatusColor::Cyan:
      return {brightness, 0, brightness};
    case StatusColor::Blue:
      return {0, 0, brightness};
    case StatusColor::Violet:
      return {0, brightness, brightness};
  }

  return {0, 0, 0};
}

}  // namespace hardware
