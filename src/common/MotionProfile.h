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

struct TimedJointState
{
  JointState joint_state;
  uint32_t time_from_start_ms;
};

// ToDo: Reduce if memory limit is reached
constexpr std::size_t kMaxMotionPlanSamples = 1024U;

struct MotionPlan
{
  MotionProfile profile;
  uint32_t total_duration_ms;
  std::size_t sample_count;
  std::array<TimedJointState, kMaxMotionPlanSamples> samples;
};

inline MotionProfile defaultMotionProfile()
{
  return MotionProfile{MotionProfileType::SmoothStartStop, 90.0F, 10U};
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
