// Linear hardware calibration from joint-space values to PCA9685 PWM ticks.

#include "hardware/HardwareCalibration.h"

#include <cmath>

#include "config/RobotSettings.h"

namespace hardware
{

namespace
{

constexpr const char *kEmptyField = "";

HardwareCalibrationResult calibrationOk(common::JointPwmState state)
{
  return HardwareCalibrationResult{true, HardwareCalibrationStatus::Ok, kEmptyField, "ok", state};
}

HardwareCalibrationResult invalidCalibration(const char *field_name, const char *message)
{
  return HardwareCalibrationResult{false, HardwareCalibrationStatus::InvalidCalibration, field_name, message,
                                   common::JointPwmState{}};
}

float clampFloat(float value, float min_value, float max_value)
{
  if (value < min_value)
  {
    return min_value;
  }

  if (value > max_value)
  {
    return max_value;
  }

  return value;
}

uint16_t roundedPwm(float value, const common::PwmLimits &pwm_limits)
{
  if (value <= static_cast<float>(pwm_limits.min_value))
  {
    return pwm_limits.min_value;
  }

  if (value >= static_cast<float>(pwm_limits.max_value))
  {
    return pwm_limits.max_value;
  }

  return static_cast<uint16_t>(std::lround(value));
}

bool isValidAxisPwmRange(const ServoAxisCalibration &axis, const common::PwmLimits &pwm_limits)
{
  return axis.min_pwm >= pwm_limits.min_value && axis.min_pwm <= pwm_limits.max_value &&
         axis.max_pwm >= pwm_limits.min_value && axis.max_pwm <= pwm_limits.max_value;
}

bool isValidAxisCalibration(const ServoAxisCalibration &axis, const common::PwmLimits &pwm_limits)
{
  return std::isfinite(axis.min_deg) && std::isfinite(axis.max_deg) && axis.min_deg < axis.max_deg &&
         isValidAxisPwmRange(axis, pwm_limits);
}

bool isValidGripperCalibration(const GripperCalibration &gripper, const common::PwmLimits &pwm_limits)
{
  return std::isfinite(gripper.min_pct) && std::isfinite(gripper.max_pct) && gripper.min_pct < gripper.max_pct &&
         gripper.min_pwm >= pwm_limits.min_value && gripper.min_pwm <= pwm_limits.max_value &&
         gripper.max_pwm >= pwm_limits.min_value && gripper.max_pwm <= pwm_limits.max_value;
}

float interpolate(float value, float in_min, float in_max, float out_min, float out_max)
{
  if (in_min == in_max)
  {
    return out_max;
  }

  const auto normalized = (value - in_min) / (in_max - in_min);
  return out_min + normalized * (out_max - out_min);
}

uint16_t mapAxisValueToPwm(float value, const ServoAxisCalibration &axis, const common::PwmLimits &pwm_limits)
{
  const auto clamped = clampFloat(value, axis.min_deg, axis.max_deg);
  return roundedPwm(interpolate(clamped, axis.min_deg, axis.max_deg, axis.min_pwm, axis.max_pwm), pwm_limits);
}

uint16_t mapGripperValueToPwm(float value, const GripperCalibration &gripper, const common::PwmLimits &pwm_limits)
{
  const auto clamped = clampFloat(value, gripper.min_pct, gripper.max_pct);
  return roundedPwm(interpolate(clamped, gripper.min_pct, gripper.max_pct, gripper.min_pwm, gripper.max_pwm),
                    pwm_limits);
}

}  // namespace

HardwareCalibration defaultHardwareCalibration()
{
  using common::JointAxis;
  using common::jointLimitForAxis;

  const auto &settings = config::robotSettings();
  const auto &limits = settings.joint_limits;
  const auto &servo_pwm_calibration = settings.servo_pwm_calibration;

  // clang-format off
  const auto &d_limit =  jointLimitForAxis(limits, JointAxis::D);
  const auto &s_limit =  jointLimitForAxis(limits, JointAxis::S);
  const auto &e_limit =  jointLimitForAxis(limits, JointAxis::E);
  const auto &hp_limit = jointLimitForAxis(limits, JointAxis::Hp);
  const auto &hr_limit = jointLimitForAxis(limits, JointAxis::Hr);
  const auto &g_limit =  jointLimitForAxis(limits, JointAxis::G);
  const auto &d_pwm =  config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::D);
  const auto &s_pwm =  config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::S);
  const auto &e_pwm =  config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::E);
  const auto &hp_pwm = config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::Hp);
  const auto &hr_pwm = config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::Hr);
  const auto &g_pwm =  config::servoPwmEndpointsFor(servo_pwm_calibration, JointAxis::G);

  return HardwareCalibration{
      // Axis calibration:
      ServoAxisCalibration{d_limit.min_value,  d_limit.max_value,  d_pwm.min_pwm,  d_pwm.max_pwm},
      ServoAxisCalibration{s_limit.min_value,  s_limit.max_value,  s_pwm.min_pwm,  s_pwm.max_pwm},
      ServoAxisCalibration{e_limit.min_value,  e_limit.max_value,  e_pwm.min_pwm,  e_pwm.max_pwm},
      ServoAxisCalibration{hp_limit.min_value, hp_limit.max_value, hp_pwm.min_pwm, hp_pwm.max_pwm},
      ServoAxisCalibration{hr_limit.min_value, hr_limit.max_value, hr_pwm.min_pwm, hr_pwm.max_pwm},
      // Gripper calibration:
      GripperCalibration{  g_limit.min_value,  g_limit.max_value,  g_pwm.min_pwm,  g_pwm.max_pwm},
      // The manually established reference pose used by both the PCA9685
      // initialization and the logical robot state.
      settings.initial_joint_state,
  };
  // clang-format on
}

HardwareCalibrationResult mapJointStateToPwm(const common::JointState &state, const HardwareCalibration &calibration)
{
  const auto &pwm_limits = config::robotSettings().pwm_limits;
  if (!common::isFinite(state))
  {
    return invalidCalibration(kEmptyField, "Joint values must be finite numbers.");
  }

  if (!isValidAxisCalibration(calibration.d, pwm_limits))
  {
    return invalidCalibration("d", "D axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.s, pwm_limits))
  {
    return invalidCalibration("s", "S axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.e, pwm_limits))
  {
    return invalidCalibration("e", "E axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.hp, pwm_limits))
  {
    return invalidCalibration("hp", "HP axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.hr, pwm_limits))
  {
    return invalidCalibration("hr", "HR axis calibration is invalid.");
  }
  if (!isValidGripperCalibration(calibration.g, pwm_limits))
  {
    return invalidCalibration("g", "Gripper calibration is invalid.");
  }

  return calibrationOk(common::JointPwmState{
      mapAxisValueToPwm(state.d_deg, calibration.d, pwm_limits),
      mapAxisValueToPwm(state.s_deg, calibration.s, pwm_limits),
      mapAxisValueToPwm(state.e_deg, calibration.e, pwm_limits),
      mapAxisValueToPwm(state.hp_deg, calibration.hp, pwm_limits),
      mapAxisValueToPwm(state.hr_deg, calibration.hr, pwm_limits),
      mapGripperValueToPwm(state.g_pct, calibration.g, pwm_limits),
  });
}

const char *toString(HardwareCalibrationStatus status)
{
  switch (status)
  {
    case HardwareCalibrationStatus::Ok:
      return "ok";
    case HardwareCalibrationStatus::InvalidCalibration:
      return "invalid_calibration";
  }

  return "invalid_calibration";
}

}  // namespace hardware
