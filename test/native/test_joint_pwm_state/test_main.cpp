// Native tests for the low-level joint PWM model and PCA9685 limits.

#include <unity.h>

#include "common/JointPwmState.h"

void test_initial_joint_pwm_state_uses_neutral_servo_pulses()
{
  const auto state = common::initialJointPwmState();

  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.d_pwm);
  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.s_pwm);
  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.hp_pwm);
  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.hr_pwm);
  TEST_ASSERT_EQUAL_UINT16(common::kServoNeutralPulsePwm, state.g_pwm);
}

void test_joint_pwm_state_accepts_values_inside_pca9685_range()
{
  const common::JointPwmState state{0U, 1U, common::kServoMinPulsePwm, common::kServoNeutralPulsePwm,
                                    common::kServoMaxPulsePwm, 4095U};

  TEST_ASSERT_TRUE(common::isWithinJointPwmLimits(state));
}

void test_joint_pwm_limits_expose_shared_pca9685_range()
{
  TEST_ASSERT_EQUAL_UINT16(0U, common::kMinPwm);
  TEST_ASSERT_EQUAL_UINT16(4095U, common::kMaxPwm);
  TEST_ASSERT_EQUAL_UINT16(205U, common::kServoMinPulsePwm);
  TEST_ASSERT_EQUAL_UINT16(307U, common::kServoNeutralPulsePwm);
  TEST_ASSERT_EQUAL_UINT16(410U, common::kServoMaxPulsePwm);
  TEST_ASSERT_EQUAL_UINT(6U, common::kJointPwmAxisCount);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_initial_joint_pwm_state_uses_neutral_servo_pulses);
  RUN_TEST(test_joint_pwm_state_accepts_values_inside_pca9685_range);
  RUN_TEST(test_joint_pwm_limits_expose_shared_pca9685_range);
  return UNITY_END();
}
