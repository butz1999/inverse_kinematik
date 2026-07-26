// Generic status LED sequence step data.

#pragma once

#include <cstdint>
#include <string>

#include "hardware/StatusLed.h"

namespace application::steps
{

struct LedStep
{
  bool has_status_color;
  hardware::StatusColor status_color;
  bool has_rgb_color;
  hardware::StatusLed::RgbColor rgb_color;
  hardware::StatusLed::Mode mode;
  uint32_t interval_ms;
  std::string name;
};

inline LedStep emptyLedStep()
{
  return LedStep{false,
                 hardware::StatusColor::Off,
                 false,
                 hardware::StatusLed::RgbColor{0U, 0U, 0U},
                 hardware::StatusLed::Mode::On,
                 0U,
                 ""};
}

}  // namespace application::steps
