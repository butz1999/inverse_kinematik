// Native tests for the low-level joint PWM model and PCA9685 limits.

#include <unity.h>

#include "common/JointPwmState.h"

void test_initial_joint_pwm_state_uses_zero_outputs() {
  const auto state = common::initialJointPwmState();

  TEST_ASSERT_EQUAL_UINT16(0U, state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(0U, state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(0U, state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(0U, state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(0U, state.hr_pwm);
  TEST_ASSERT_EQUAL_UINT16(0U, state.g_pwm);
}

void test_joint_pwm_state_accepts_values_inside_pca9685_range() {
  const common::JointPwmState state{0U, 1U, 1500U, 2048U, 3000U, 4095U};

  TEST_ASSERT_TRUE(common::isWithinJointPwmLimits(state));
  TEST_ASSERT_NULL(common::findFirstLimitViolation(state));
}

void test_joint_pwm_limits_expose_pca9685_range_per_axis() {
  TEST_ASSERT_EQUAL_UINT16(0U, common::kPca9685MinPwm);
  TEST_ASSERT_EQUAL_UINT16(4095U, common::kPca9685MaxPwm);
  TEST_ASSERT_EQUAL_UINT(6U, common::kJointPwmAxisCount);
  TEST_ASSERT_EQUAL_STRING("d_pwm", common::kJointPwmLimits[0].field_name);
  TEST_ASSERT_EQUAL_STRING("g_pwm", common::kJointPwmLimits[5].field_name);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_joint_pwm_state_uses_zero_outputs);
  RUN_TEST(test_joint_pwm_state_accepts_values_inside_pca9685_range);
  RUN_TEST(test_joint_pwm_limits_expose_pca9685_range_per_axis);
  return UNITY_END();
}
