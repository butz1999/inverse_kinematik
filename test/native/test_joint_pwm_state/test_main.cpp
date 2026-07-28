// Native tests for the low-level joint PWM model and PCA9685 limits.

#include <unity.h>

#include "common/JointPwmState.h"
#include "config/RobotSettings.h"

void test_joint_pwm_state_accepts_values_inside_pca9685_range()
{
  const common::JointPwmState state{0U, 1U, 205U, 307U, 410U, 4095U};

  TEST_ASSERT_TRUE(common::isWithinJointPwmLimits(state, config::robotSettings().pwm_limits));
}

void test_joint_pwm_limits_expose_shared_pca9685_range()
{
  const auto &limits = config::robotSettings().pwm_limits;

  TEST_ASSERT_EQUAL_UINT16(0U, limits.min_value);
  TEST_ASSERT_EQUAL_UINT16(4095U, limits.max_value);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_joint_pwm_state_accepts_values_inside_pca9685_range);
  RUN_TEST(test_joint_pwm_limits_expose_shared_pca9685_range);
  return UNITY_END();
}
