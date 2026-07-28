// Native tests for the low-level joint-state model and limits.

#include <unity.h>

#include <limits>

#include "common/JointState.h"
#include "config/RobotSettings.h"

void test_initial_joint_state_uses_documented_zero_position()
{
  const auto state = common::initialJointState();

  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.d_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.s_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.e_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.hp_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.hr_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, state.g_pct);
}

void test_joint_state_accepts_values_inside_hardware_ranges()
{
  const common::JointState state{-90.0F, 90.0F, 50.0F, -45.0F, -90.0F, 100.0F};
  const auto &limits = config::robotSettings().joint_limits;

  TEST_ASSERT_TRUE(common::isWithinJointLimits(state, limits));
  TEST_ASSERT_FALSE(common::findFirstLimitViolation(state, limits).has_value());
}

void test_joint_state_rejects_first_axis_outside_limit()
{
  const common::JointState state{91.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const auto &limits = config::robotSettings().joint_limits;

  const auto violation = common::findFirstLimitViolation(state, limits);

  TEST_ASSERT_FALSE(common::isWithinJointLimits(state, limits));
  TEST_ASSERT_TRUE(violation.has_value());
  TEST_ASSERT_EQUAL(common::JointAxis::D, *violation);
  TEST_ASSERT_EQUAL_STRING("d_deg", common::jointAxisFieldName(*violation));
  const auto &limit = common::jointLimitForAxis(limits, *violation);
  TEST_ASSERT_EQUAL_FLOAT(-90.0F, limit.min_value);
  TEST_ASSERT_EQUAL_FLOAT(90.0F, limit.max_value);
}

void test_joint_state_rejects_non_finite_values()
{
  auto state = common::initialJointState();
  state.s_deg = std::numeric_limits<float>::infinity();

  TEST_ASSERT_FALSE(common::isWithinJointLimits(state, config::robotSettings().joint_limits));
  TEST_ASSERT_FALSE(common::isFinite(state));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_initial_joint_state_uses_documented_zero_position);
  RUN_TEST(test_joint_state_accepts_values_inside_hardware_ranges);
  RUN_TEST(test_joint_state_rejects_first_axis_outside_limit);
  RUN_TEST(test_joint_state_rejects_non_finite_values);
  return UNITY_END();
}
