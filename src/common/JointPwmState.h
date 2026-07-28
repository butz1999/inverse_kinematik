// PWM-space data model used at the low-level hardware boundary.

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/JointAxis.h"

namespace common
{

struct JointPwmState
{
  uint16_t d_pwm;
  uint16_t s_pwm;
  uint16_t e_pwm;
  uint16_t hp_pwm;
  uint16_t hr_pwm;
  uint16_t g_pwm;
};

struct PwmLimits
{
  uint16_t min_value;
  uint16_t max_value;
};

inline uint16_t jointAxisPwmValue(const JointPwmState &state, JointAxis axis)
{
  switch (axis)
  {
    case JointAxis::D:
      return state.d_pwm;
    case JointAxis::S:
      return state.s_pwm;
    case JointAxis::E:
      return state.e_pwm;
    case JointAxis::Hp:
      return state.hp_pwm;
    case JointAxis::Hr:
      return state.hr_pwm;
    case JointAxis::G:
    case JointAxis::Count:
      return state.g_pwm;
  }

  return state.g_pwm;
}

inline bool isWithinJointPwmLimits(const JointPwmState &state, const PwmLimits &limits)
{
  for (const auto axis : kJointAxes)
  {
    const auto value = jointAxisPwmValue(state, axis);
    if (value < limits.min_value || value > limits.max_value)
    {
      return false;
    }
  }

  return true;
}

}  // namespace common
