// PWM-space data model used at the low-level hardware boundary.

#pragma once

#include <cstddef>
#include <cstdint>

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

struct JointPwmLimit
{
  const char *field_name;
  uint16_t min_value;
  uint16_t max_value;
};

constexpr uint16_t kPca9685MinPwm = 0U;
constexpr uint16_t kPca9685MaxPwm = 4095U;

constexpr JointPwmLimit kJointPwmLimits[] = {
    {"d_pwm", kPca9685MinPwm, kPca9685MaxPwm},  {"s_pwm", kPca9685MinPwm, kPca9685MaxPwm},
    {"e_pwm", kPca9685MinPwm, kPca9685MaxPwm},  {"hp_pwm", kPca9685MinPwm, kPca9685MaxPwm},
    {"hr_pwm", kPca9685MinPwm, kPca9685MaxPwm}, {"g_pwm", kPca9685MinPwm, kPca9685MaxPwm},
};

constexpr std::size_t kJointPwmAxisCount = sizeof(kJointPwmLimits) / sizeof(kJointPwmLimits[0]);

inline JointPwmState initialJointPwmState()
{
  return JointPwmState{0U, 0U, 0U, 0U, 0U, 0U};
}

inline uint16_t valueForLimit(const JointPwmState &state, std::size_t index)
{
  switch (index)
  {
    case 0:
      return state.d_pwm;
    case 1:
      return state.s_pwm;
    case 2:
      return state.e_pwm;
    case 3:
      return state.hp_pwm;
    case 4:
      return state.hr_pwm;
    default:
      return state.g_pwm;
  }
}

inline const JointPwmLimit *findFirstLimitViolation(const JointPwmState &state)
{
  for (std::size_t i = 0; i < kJointPwmAxisCount; ++i)
  {
    const auto value = valueForLimit(state, i);
    const auto &limit = kJointPwmLimits[i];
    if (value < limit.min_value || value > limit.max_value)
    {
      return &limit;
    }
  }

  return nullptr;
}

inline bool isWithinJointPwmLimits(const JointPwmState &state)
{
  return findFirstLimitViolation(state) == nullptr;
}

}  // namespace common
