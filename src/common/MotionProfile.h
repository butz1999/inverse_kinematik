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
  ConstantAcceleration,
  SmoothStartStop,
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

constexpr std::size_t kMaxMotionPlanSamples = 256U;

struct MotionPlan
{
  MotionProfile profile;
  uint32_t total_duration_ms;
  std::array<TimedJointState, kMaxMotionPlanSamples> samples;
  std::size_t sample_count;
};

inline MotionProfile defaultMotionProfile()
{
  return MotionProfile{MotionProfileType::SmoothStartStop, 60.0F, 10U};
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
  }

  return "constant_velocity";
}

}  // namespace common
