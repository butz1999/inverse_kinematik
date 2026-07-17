// Linear hardware calibration from joint-space values to PCA9685 PWM ticks.

#include "hardware/HardwareCalibration.h"

#include <cmath>

namespace hardware
{

namespace
{

constexpr const char *kEmptyField = "";

HardwareCalibrationResult ok(common::JointPwmState state)
{
  return HardwareCalibrationResult{true, HardwareCalibrationStatus::Ok, kEmptyField, "ok", state};
}

HardwareCalibrationResult invalidCalibration(const char *field_name, const char *message)
{
  return HardwareCalibrationResult{false, HardwareCalibrationStatus::InvalidCalibration, field_name, message,
                                   common::initialJointPwmState()};
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

uint16_t roundedPwm(float value)
{
  if (value <= static_cast<float>(common::kMinPwm))
  {
    return common::kMinPwm;
  }

  if (value >= static_cast<float>(common::kMaxPwm))
  {
    return common::kMaxPwm;
  }

  return static_cast<uint16_t>(std::lround(value));
}

bool isValidAxisPwmRange(const ServoAxisCalibration &axis)
{
  return axis.min_pwm <= common::kMaxPwm && axis.max_pwm <= common::kMaxPwm;
}

bool isValidAxisCalibration(const ServoAxisCalibration &axis)
{
  return std::isfinite(axis.min_deg) && std::isfinite(axis.max_deg) && axis.min_deg < axis.max_deg &&
         isValidAxisPwmRange(axis);
}

bool isValidGripperCalibration(const GripperCalibration &gripper)
{
  return std::isfinite(gripper.min_pct) && std::isfinite(gripper.max_pct) && gripper.min_pct < gripper.max_pct &&
         gripper.min_pwm <= common::kMaxPwm && gripper.max_pwm <= common::kMaxPwm;
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

uint16_t mapAxisValueToPwm(float value, const ServoAxisCalibration &axis)
{
  const auto clamped = clampFloat(value, axis.min_deg, axis.max_deg);
  return roundedPwm(interpolate(clamped, axis.min_deg, axis.max_deg, axis.min_pwm, axis.max_pwm));
}

uint16_t mapGripperValueToPwm(float value, const GripperCalibration &gripper)
{
  const auto clamped = clampFloat(value, gripper.min_pct, gripper.max_pct);
  return roundedPwm(interpolate(clamped, gripper.min_pct, gripper.max_pct, gripper.min_pwm, gripper.max_pwm));
}

}  // namespace

HardwareCalibration defaultHardwareCalibration()
{
  // clang-format off
  return HardwareCalibration{
      ServoAxisCalibration{-180.0F,  90.0F, common::kServoMinPulsePwm, common::kServoMaxPulsePwm},
      ServoAxisCalibration{ -90.0F,  90.0F, common::kServoMinPulsePwm, common::kServoMaxPulsePwm},
      ServoAxisCalibration{-100.0F, 100.0F, common::kServoMinPulsePwm, common::kServoMaxPulsePwm},
      ServoAxisCalibration{   0.0F, 135.0F, common::kServoMinPulsePwm, common::kServoMaxPulsePwm},
      ServoAxisCalibration{-180.0F, 180.0F, common::kServoMinPulsePwm, common::kServoMaxPulsePwm},
      GripperCalibration{     0.0F, 100.0F, common::kServoMaxPulsePwm, common::kServoMinPulsePwm},
  };
  // clang-format on
}

HardwareCalibrationResult mapJointStateToPwm(const common::JointState &state, const HardwareCalibration &calibration)
{
  if (!common::isFinite(state))
  {
    return invalidCalibration(kEmptyField, "Joint values must be finite numbers.");
  }

  if (!isValidAxisCalibration(calibration.d))
  {
    return invalidCalibration("d", "D axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.s))
  {
    return invalidCalibration("s", "S axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.e))
  {
    return invalidCalibration("e", "E axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.hp))
  {
    return invalidCalibration("hp", "HP axis calibration is invalid.");
  }
  if (!isValidAxisCalibration(calibration.hr))
  {
    return invalidCalibration("hr", "HR axis calibration is invalid.");
  }
  if (!isValidGripperCalibration(calibration.g))
  {
    return invalidCalibration("g", "Gripper calibration is invalid.");
  }

  return ok(common::JointPwmState{
      mapAxisValueToPwm(state.d_deg, calibration.d),
      mapAxisValueToPwm(state.s_deg, calibration.s),
      mapAxisValueToPwm(state.e_deg, calibration.e),
      mapAxisValueToPwm(state.hp_deg, calibration.hp),
      mapAxisValueToPwm(state.hr_deg, calibration.hr),
      mapGripperValueToPwm(state.g_pct, calibration.g),
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
