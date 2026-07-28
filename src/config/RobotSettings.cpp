// Default settings for the installed robot hardware.

#include "config/RobotSettings.h"

namespace config
{

namespace
{

robotics::RobotModel installedRobotModel()
{
  const robotics::SegmentLengths segments{
      .s_e_length_mm = 105.0F,
      .e_hp_length_mm = 100.0F,
      .hr_g_length_mm = 100.0F,
  };
  return robotics::RobotModel{
      .segments = segments,
      .workspace = robotics::CartesianWorkspace{
          .min_z_mm = 0.0F,
          .max_z_mm = 260.0F,
          .max_reach_mm = robotics::maxReachFromSegments(segments),
      },
  };
}

}  // namespace

const RobotSettings &robotSettings()
{
  using common::JointLimit;
  using common::JointLimits;
  using common::PwmLimits;
  using common::initialJointState;

  // clang-format off
  static const RobotSettings settings{
      .joint_limits = JointLimits{{
          JointLimit{.min_value =  -90.0F, .max_value =  90.0F},  // d
          JointLimit{.min_value =  -90.0F, .max_value =  90.0F},  // s
          JointLimit{.min_value =  -90.0F, .max_value =  90.0F},  // e
          JointLimit{.min_value = -135.0F, .max_value =   0.0F},  // hp
          JointLimit{.min_value =  -90.0F, .max_value =  90.0F},  // hr
          JointLimit{.min_value =    0.0F, .max_value = 100.0F},  // g
      }},
      .pwm_limits = PwmLimits{.min_value = 0U, .max_value = 4095U},
      .servo_pwm_calibration = ServoPwmCalibration{{
          ServoPwmEndpoints{.min_pwm = 530U, .max_pwm = 100U},  // d
          ServoPwmEndpoints{.min_pwm = 490U, .max_pwm =  80U},  // s
          ServoPwmEndpoints{.min_pwm =  90U, .max_pwm = 500U},  // e
          ServoPwmEndpoints{.min_pwm = 200U, .max_pwm = 520U},  // hp
          ServoPwmEndpoints{.min_pwm = 100U, .max_pwm = 500U},  // hr
          ServoPwmEndpoints{.min_pwm = 130U, .max_pwm = 375U},  // g
      }},
      .initial_joint_state = initialJointState(),
      .robot_model = installedRobotModel(),
      .robot_model_offset = robotics::RobotModelOffset{
          .o_d_offset_x_mm =           0.0F,
          .o_d_offset_y_mm =        -105.0F,
          .o_d_offset_z_mm =          77.5F,
          .d_s_offset_x_mm =         -12.5F,
          .d_s_offset_y_mm =         -11.0F,
          .d_s_offset_z_mm =          12.5F,
          .hp_hr_offset_up_mm =       28.0F,
          .hp_hr_offset_side_mm =      0.0F,
          .hp_hr_offset_forward_mm =  45.0F,
      },
      .pca9685_driver = hardware::Pca9685ServoDriverConfig{
          .i2c_address = 0x40U,
          .pwm_frequency_hz = 50U,
          .channels = hardware::ServoChannelMap{
              .d = 0U,
              .s = 1U,
              .e = 2U,
              .hp = 3U,
              .hr = 4U,
              .g = 5U,
          },
      },
  };
  // clang-format on
  return settings;
}

const ServoPwmEndpoints &servoPwmEndpointsFor(const ServoPwmCalibration &calibration, common::JointAxis axis)
{
  return calibration[common::jointAxisIndex(axis)];
}

}  // namespace config
