// Native tests for the static robot settings factory.

#include <unity.h>

#include "config/RobotSettings.h"
#include "hardware/HardwareCalibration.h"

void test_robot_settings_expose_the_hp_mechanical_travel_range()
{
  const auto &limits = config::robotSettings().joint_limits;
  const auto &hp_limit = common::jointLimitForAxis(limits, common::JointAxis::Hp);

  TEST_ASSERT_EQUAL_UINT32(common::kJointAxisCount, limits.size());
  TEST_ASSERT_EQUAL_STRING("hp_deg", common::jointAxisFieldName(common::JointAxis::Hp));
  TEST_ASSERT_EQUAL_FLOAT(-135.0F, hp_limit.min_value);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, hp_limit.max_value);
}

void test_hardware_calibration_derives_input_ranges_from_robot_settings()
{
  const auto &settings = config::robotSettings();
  const auto &limits = settings.joint_limits;
  const auto calibration = hardware::defaultHardwareCalibration();
  const auto &hp_limit = common::jointLimitForAxis(limits, common::JointAxis::Hp);
  const auto &g_limit = common::jointLimitForAxis(limits, common::JointAxis::G);
  const auto &hp_pwm = config::servoPwmEndpointsFor(settings.servo_pwm_calibration, common::JointAxis::Hp);
  const auto &g_pwm = config::servoPwmEndpointsFor(settings.servo_pwm_calibration, common::JointAxis::G);

  TEST_ASSERT_EQUAL_FLOAT(hp_limit.min_value, calibration.hp.min_deg);
  TEST_ASSERT_EQUAL_FLOAT(hp_limit.max_value, calibration.hp.max_deg);
  TEST_ASSERT_EQUAL_FLOAT(g_limit.min_value, calibration.g.min_pct);
  TEST_ASSERT_EQUAL_FLOAT(g_limit.max_value, calibration.g.max_pct);
  TEST_ASSERT_EQUAL_UINT16(hp_pwm.min_pwm, calibration.hp.min_pwm);
  TEST_ASSERT_EQUAL_UINT16(hp_pwm.max_pwm, calibration.hp.max_pwm);
  TEST_ASSERT_EQUAL_UINT16(g_pwm.min_pwm, calibration.g.min_pwm);
  TEST_ASSERT_EQUAL_UINT16(g_pwm.max_pwm, calibration.g.max_pwm);
}

void test_robotics_and_driver_defaults_derive_from_robot_settings()
{
  const auto &settings = config::robotSettings();

  TEST_ASSERT_TRUE(robotics::isValidRobotModel(settings.robot_model));
  TEST_ASSERT_EQUAL_FLOAT(0.0F, settings.initial_joint_state.hp_deg);
  TEST_ASSERT_EQUAL_FLOAT(-105.0F, settings.robot_model_offset.o_d_offset_y_mm);
  TEST_ASSERT_EQUAL_UINT8(0x40U, settings.pca9685_driver.i2c_address);
  TEST_ASSERT_EQUAL_UINT16(50U, settings.pca9685_driver.pwm_frequency_hz);
  TEST_ASSERT_EQUAL_UINT8(3U, settings.pca9685_driver.channels.hp);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_robot_settings_expose_the_hp_mechanical_travel_range);
  RUN_TEST(test_hardware_calibration_derives_input_ranges_from_robot_settings);
  RUN_TEST(test_robotics_and_driver_defaults_derive_from_robot_settings);
  return UNITY_END();
}
