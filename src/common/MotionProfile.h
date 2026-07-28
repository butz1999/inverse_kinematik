// Motion profile data models shared between orchestration and hardware output.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "common/JointState.h"

namespace common
{

enum class MotionProfileType
{
  ConstantVelocity,
  SmoothStartStop,
  FastStartStop,
  ConstantAcceleration,
};

struct MotionProfile
{
  MotionProfileType type;
  float target_velocity_deg_s;
  uint32_t sample_time_ms;
};

inline MotionProfile defaultMotionProfile()
{
  return MotionProfile{MotionProfileType::SmoothStartStop, 40.0F, 10U};
}

inline const char *toString(MotionProfileType type)
{
  switch (type)
  {
    case MotionProfileType::ConstantVelocity:
      return "constant_velocity";
    case MotionProfileType::ConstantAcceleration:
      return "constant_acceleration";
    case MotionProfileType::SmoothStartStop:
      return "smooth_start_stop";
    case MotionProfileType::FastStartStop:
      return "fast_start_stop";
  }

  return "constant_velocity";
}

}  // namespace common
