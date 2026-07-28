// Native tests for controller joint-space slew limiting.

#include <unity.h>

#include "application/ControllerJointSlewLimiter.h"

void test_joint_slew_limiter_limits_first_controller_step()
{
  const common::JointState current{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target{0.0F, 0.0F, 90.0F, -90.0F, 0.0F, 0.0F};
  auto state = application::emptyControllerJointSlewLimiterState();

  const auto result = application::applyControllerJointSlewLimiter(current, target, 100U, state);

  TEST_ASSERT_TRUE(result.active);
  TEST_ASSERT_TRUE(result.changed);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.6F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, -3.6F, result.joint_state.hp_deg);
}

void test_joint_slew_limiter_reaches_near_target_without_overshoot()
{
  const common::JointState current{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target{0.0F, 0.0F, 1.0F, -1.0F, 0.0F, 0.0F};
  auto state = application::emptyControllerJointSlewLimiterState();

  const auto result = application::applyControllerJointSlewLimiter(current, target, 100U, state);

  TEST_ASSERT_TRUE(result.changed);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, -1.0F, result.joint_state.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_deg_s.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_deg_s.hp_deg);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_joint_slew_limiter_limits_first_controller_step);
  RUN_TEST(test_joint_slew_limiter_reaches_near_target_without_overshoot);
  return UNITY_END();
}
