// The Motion Plan consists of a planned trajectory with a given sample time intevall

#pragma once

#include <array>

#include "MotionProfile.h"

namespace common
{

struct TimedJointState
{
  JointState joint_state;
  uint32_t time_from_start_ms;
};

// ToDo: Reduce if memory limit is reached
constexpr std::size_t kMaxMotionPlanSamples = 1024U;

struct MotionPlan
{
  MotionProfile profile;         // Motion profile for the movement
  uint32_t total_duration_ms;    // Duration of the movement
  std::size_t sample_count;      // Number of interpolation points
  uint32_t calculation_time_us;  // Calculation time for the motion plan
  // Sample list. Carefull! kMaxMotionPlanSamples is big so the samples array is HUGE!
  std::array<TimedJointState, kMaxMotionPlanSamples> samples;
};

}  // namespace common