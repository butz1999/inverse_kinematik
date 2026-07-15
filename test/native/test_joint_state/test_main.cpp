// Native tests for the low-level joint-state model and limits.

#include <unity.h>

#include <limits>

#include "common/JointState.h"

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
  const common::JointState state{-180.0F, 90.0F, 50.0F, 135.0F, -180.0F, 100.0F};

  TEST_ASSERT_TRUE(common::isWithinJointLimits(state));
  TEST_ASSERT_NULL(common::findFirstLimitViolation(state));
}

void test_joint_state_rejects_first_axis_outside_limit()
{
  const common::JointState state{91.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};

  const auto *violation = common::findFirstLimitViolation(state);

  TEST_ASSERT_FALSE(common::isWithinJointLimits(state));
  TEST_ASSERT_NOT_NULL(violation);
  TEST_ASSERT_EQUAL_STRING("d_deg", violation->field_name);
  TEST_ASSERT_EQUAL_FLOAT(-180.0F, violation->min_value);
  TEST_ASSERT_EQUAL_FLOAT(90.0F, violation->max_value);
}

void test_joint_state_rejects_non_finite_values()
{
  auto state = common::initialJointState();
  state.s_deg = std::numeric_limits<float>::infinity();

  TEST_ASSERT_FALSE(common::isWithinJointLimits(state));
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
