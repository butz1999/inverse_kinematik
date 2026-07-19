// Native tests for robotics validation rules.

#include <unity.h>

#include <limits>

#include "robotics/Validation.h"

void test_target_pose_validation_accepts_reachable_pose()
{
  const common::TargetPose pose{50.0F, 100.0F, 40.0F, 0.0F, 0.0F, 50.0F};
  const auto result = robotics::validateTargetPose(pose, robotics::defaultRobotModel());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::Ok, result.status);
  TEST_ASSERT_EQUAL_STRING("ok", robotics::toString(result.status));
}

void test_target_pose_validation_rejects_non_finite_pose()
{
  auto pose = common::initialTargetPose();
  pose.y_mm = std::numeric_limits<float>::quiet_NaN();

  const auto result = robotics::validateTargetPose(pose, robotics::defaultRobotModel());

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::InvalidTargetPose, result.status);
  TEST_ASSERT_EQUAL_STRING("y_mm", result.field_name.c_str());
}

void test_target_pose_validation_rejects_gripper_outside_range()
{
  auto pose = common::initialTargetPose();
  pose.g_pct = 101.0F;

  const auto result = robotics::validateTargetPose(pose, robotics::defaultRobotModel());

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::InvalidTargetPose, result.status);
  TEST_ASSERT_EQUAL_STRING("g_pct", result.field_name.c_str());
}

void test_target_pose_validation_defers_workspace_reach_to_kinematics()
{
  const common::TargetPose pose{1000.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const auto result = robotics::validateTargetPose(pose, robotics::defaultRobotModel());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::Ok, result.status);
}

void test_target_pose_validation_rejects_invalid_model()
{
  auto model = robotics::defaultRobotModel();
  model.workspace.max_reach_mm = -1.0F;

  const auto result = robotics::validateTargetPose(common::initialTargetPose(), model);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::InvalidRobotModel, result.status);
  TEST_ASSERT_EQUAL_STRING("invalid_robot_model", robotics::toString(result.status));
}

void test_joint_state_validation_uses_common_joint_limits()
{
  common::JointState state = common::initialJointState();
  state.hp_deg = 1.0F;

  const auto result = robotics::validateJointState(state);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::JointLimitViolation, result.status);
  TEST_ASSERT_EQUAL_STRING("hp_deg", result.field_name.c_str());
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_target_pose_validation_accepts_reachable_pose);
  RUN_TEST(test_target_pose_validation_rejects_non_finite_pose);
  RUN_TEST(test_target_pose_validation_rejects_gripper_outside_range);
  RUN_TEST(test_target_pose_validation_defers_workspace_reach_to_kinematics);
  RUN_TEST(test_target_pose_validation_rejects_invalid_model);
  RUN_TEST(test_joint_state_validation_uses_common_joint_limits);
  return UNITY_END();
}
