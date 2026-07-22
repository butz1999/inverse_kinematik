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
  bool has_mode;
  hardware::StatusLed::Mode mode;
  bool has_interval_ms;
  uint32_t interval_ms;
  std::string name;
};

inline LedStep emptyLedStep()
{
  return LedStep{false, hardware::StatusColor::Off, false, hardware::StatusLed::RgbColor{0U, 0U, 0U}, false,
                 hardware::StatusLed::Mode::On, false, 0U, ""};
}

}  // namespace application::steps
