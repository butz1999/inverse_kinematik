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

constexpr uint16_t kMinPwm = 0U;
constexpr uint16_t kMaxPwm = 4095U;
constexpr uint16_t kServoMinPulsePwm = 205U;
constexpr uint16_t kServoNeutralPulsePwm = 307U;
constexpr uint16_t kServoMaxPulsePwm = 410U;
constexpr std::size_t kJointPwmAxisCount = 6U;

inline JointPwmState initialJointPwmState()
{
  return JointPwmState{kServoNeutralPulsePwm, kServoNeutralPulsePwm, kServoNeutralPulsePwm,
                       kServoNeutralPulsePwm, kServoNeutralPulsePwm, kServoNeutralPulsePwm};
}

inline bool isWithinJointPwmLimits(const JointPwmState &state)
{
  return state.d_pwm <= kMaxPwm && state.s_pwm <= kMaxPwm && state.e_pwm <= kMaxPwm && state.hp_pwm <= kMaxPwm &&
         state.hr_pwm <= kMaxPwm && state.g_pwm <= kMaxPwm;
}

}  // namespace common
