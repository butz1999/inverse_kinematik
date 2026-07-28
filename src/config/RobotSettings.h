// Static, typed configuration for the installed robot hardware.

#pragma once

#include "common/JointState.h"
#include "common/JointPwmState.h"
#include "hardware/Pca9685ServoDriverConfig.h"
#include "robotics/RobotModel.h"

namespace config
{

struct ServoPwmEndpoints
{
  uint16_t min_pwm;
  uint16_t max_pwm;
};

using ServoPwmCalibration = std::array<ServoPwmEndpoints, common::kJointAxisCount>;

struct RobotSettings
{
  common::JointLimits joint_limits;
  common::PwmLimits pwm_limits;
  ServoPwmCalibration servo_pwm_calibration;
  common::JointState initial_joint_state;
  robotics::RobotModel robot_model;
  robotics::RobotModelOffset robot_model_offset;
  hardware::Pca9685ServoDriverConfig pca9685_driver;
};

const RobotSettings &robotSettings();
const ServoPwmEndpoints &servoPwmEndpointsFor(const ServoPwmCalibration &calibration, common::JointAxis axis);

}  // namespace config
