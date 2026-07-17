// Native tests for phase-1 hardware calibration.

#include <unity.h>

#include "hardware/HardwareCalibration.h"

void test_default_calibration_exposes_initial_pwm_state()
{
  const auto state = hardware::defaultHardwareCalibration().initial_pwm_state;

  TEST_ASSERT_EQUAL_UINT16(310U, state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(290U, state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(290U, state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(320U, state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(310U, state.hr_pwm);
  TEST_ASSERT_EQUAL_UINT16(130U, state.g_pwm);
}

void test_default_calibration_maps_values_linearly_between_axis_limits()
{
  const auto result = hardware::mapJointStateToPwm(common::initialJointState(), hardware::defaultHardwareCalibration());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(hardware::HardwareCalibrationStatus::Ok, result.status);
  TEST_ASSERT_EQUAL_UINT16(310U, result.joint_pwm_state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(285U, result.joint_pwm_state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(295U, result.joint_pwm_state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(320U, result.joint_pwm_state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(300U, result.joint_pwm_state.hr_pwm);
}

void test_default_calibration_maps_axis_limits_to_pwm_limits()
{
  const common::JointState state{-180.0F, -90.0F, 100.0F, 135.0F, 180.0F, 100.0F};
  const auto result = hardware::mapJointStateToPwm(state, hardware::defaultHardwareCalibration());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT16(530U, result.joint_pwm_state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(490U, result.joint_pwm_state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(500U, result.joint_pwm_state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(320U, result.joint_pwm_state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(500U, result.joint_pwm_state.hr_pwm);
  TEST_ASSERT_EQUAL_UINT16(375U, result.joint_pwm_state.g_pwm);
}

void test_default_calibration_clamps_values_before_mapping()
{
  const common::JointState state{-250.0F, 120.0F, 0.0F, 200.0F, 0.0F, -10.0F};
  const auto result = hardware::mapJointStateToPwm(state, hardware::defaultHardwareCalibration());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT16(530U, result.joint_pwm_state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(80U, result.joint_pwm_state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(295U, result.joint_pwm_state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(320U, result.joint_pwm_state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(300U, result.joint_pwm_state.hr_pwm);
  TEST_ASSERT_EQUAL_UINT16(130U, result.joint_pwm_state.g_pwm);
}

void test_calibration_can_map_in_descending_pwm_order()
{
  auto calibration = hardware::defaultHardwareCalibration();
  calibration.s.min_pwm = 410U;
  calibration.s.max_pwm = 205U;

  const common::JointState state{0.0F, 90.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const auto result = hardware::mapJointStateToPwm(state, calibration);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT16(205U, result.joint_pwm_state.s_pwm);
}

void test_invalid_calibration_is_reported()
{
  auto calibration = hardware::defaultHardwareCalibration();
  calibration.e.min_deg = 100.0F;

  const auto result = hardware::mapJointStateToPwm(common::initialJointState(), calibration);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(hardware::HardwareCalibrationStatus::InvalidCalibration, result.status);
  TEST_ASSERT_EQUAL_STRING("e", result.field_name);
  TEST_ASSERT_EQUAL_STRING("invalid_calibration", hardware::toString(result.status));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_default_calibration_exposes_initial_pwm_state);
  RUN_TEST(test_default_calibration_maps_values_linearly_between_axis_limits);
  RUN_TEST(test_default_calibration_maps_axis_limits_to_pwm_limits);
  RUN_TEST(test_default_calibration_clamps_values_before_mapping);
  RUN_TEST(test_calibration_can_map_in_descending_pwm_order);
  RUN_TEST(test_invalid_calibration_is_reported);
  return UNITY_END();
}
