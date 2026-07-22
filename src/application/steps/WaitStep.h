// Wait-only sequence step data.

#pragma once

#include <cstdint>

namespace application::steps
{

struct WaitStep
{
  uint32_t duration_ms;
};

}  // namespace application::steps
