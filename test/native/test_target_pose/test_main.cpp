// Native tests for the task-space target pose model.

#include <unity.h>

#include <limits>

#include "common/TargetPose.h"

void test_initial_target_pose_uses_documented_zero_position()
{
  const auto pose = common::initialTargetPose();

  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.x_mm);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.y_mm);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.z_mm);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.p_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.r_deg);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, pose.g_pct);
}

void test_target_pose_rejects_non_finite_values()
{
  auto pose = common::initialTargetPose();
  pose.x_mm = std::numeric_limits<float>::infinity();

  TEST_ASSERT_FALSE(common::isFinite(pose));
}

void test_target_pose_accepts_gripper_percent_range()
{
  auto pose = common::initialTargetPose();
  pose.g_pct = 100.0F;

  TEST_ASSERT_TRUE(common::isWithinTargetGripperLimits(pose));

  pose.g_pct = -1.0F;
  TEST_ASSERT_FALSE(common::isWithinTargetGripperLimits(pose));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_initial_target_pose_uses_documented_zero_position);
  RUN_TEST(test_target_pose_rejects_non_finite_values);
  RUN_TEST(test_target_pose_accepts_gripper_percent_range);
  return UNITY_END();
}
