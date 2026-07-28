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

// The configured joint limits need at most 202 samples with the default
// 90 deg/s profile and 10 ms sample interval. Keep a bounded reserve while
// avoiding several large plan buffers in internal SRAM.
constexpr std::size_t kMaxMotionPlanSamples = 1024U;

struct MotionPlan
{
  MotionProfile profile;         // Motion profile for the movement
  uint32_t total_duration_ms;    // Duration of the movement
  std::size_t sample_count;      // Number of interpolation points
  uint32_t calculation_time_us;  // Calculation time for the motion plan
  // Sample list with a bounded internal-SRAM footprint.
  std::array<TimedJointState, kMaxMotionPlanSamples> samples;
};

}  // namespace common
