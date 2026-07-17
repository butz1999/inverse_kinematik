// Static phase-1 calibration for mapping joint-space values to PCA9685 PWM ticks.

#pragma once

#include <cstdint>

#include "common/JointPwmState.h"
#include "common/JointState.h"

namespace hardware
{

struct ServoAxisCalibration
{
  float min_deg;
  float max_deg;
  uint16_t min_pwm;
  uint16_t max_pwm;
};

struct GripperCalibration
{
  float min_pct;
  float max_pct;
  uint16_t min_pwm;
  uint16_t max_pwm;
};

struct HardwareCalibration
{
  ServoAxisCalibration d;
  ServoAxisCalibration s;
  ServoAxisCalibration e;
  ServoAxisCalibration hp;
  ServoAxisCalibration hr;
  GripperCalibration g;
  common::JointPwmState initial_pwm_state;
};

enum class HardwareCalibrationStatus
{
  Ok,
  InvalidCalibration,
};

struct HardwareCalibrationResult
{
  bool ok;
  HardwareCalibrationStatus status;
  const char *field_name;
  const char *message;
  common::JointPwmState joint_pwm_state;
};

HardwareCalibration defaultHardwareCalibration();
HardwareCalibrationResult mapJointStateToPwm(const common::JointState &state, const HardwareCalibration &calibration);
const char *toString(HardwareCalibrationStatus status);

}  // namespace hardware
