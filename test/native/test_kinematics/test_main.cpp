// Native tests for ideal kinematics.

#include <unity.h>

#include "robotics/Kinematics.h"

namespace
{

constexpr float kTolerance = 0.001F;

void assertVectorNear(float expected_x_mm, float expected_y_mm, float expected_z_mm, const robotics::Vector3 &actual)
{
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected_x_mm, actual.x_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected_y_mm, actual.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected_z_mm, actual.z_mm);
}

}  // namespace

void test_forward_kinematics_maps_home_pose_upwards()
{
  const auto model = robotics::defaultRobotModel();
  const auto state = common::initialJointState();

  const auto result = robotics::forwardKinematics(state, model);

  assertVectorNear(0.0F, 0.0F, 0.0F, result.d_mm);
  assertVectorNear(0.0F, 0.0F, 0.0F, result.s_mm);
  assertVectorNear(0.0F, 0.0F, 100.0F, result.e_mm);
  assertVectorNear(0.0F, 0.0F, 200.0F, result.h_mm);
  assertVectorNear(0.0F, 0.0F, 260.0F, result.g_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 90.0F, result.p_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.r_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.g_pct);
}

void test_forward_kinematics_maps_horizontal_arm_along_positive_y()
{
  const auto model = robotics::defaultRobotModel();
  const common::JointState state{0.0F, -90.0F, 0.0F, 0.0F, 0.0F, 50.0F};

  const auto result = robotics::forwardKinematics(state, model);

  assertVectorNear(0.0F, 0.0F, 0.0F, result.d_mm);
  assertVectorNear(0.0F, 0.0F, 0.0F, result.s_mm);
  assertVectorNear(0.0F, 100.0F, 0.0F, result.e_mm);
  assertVectorNear(0.0F, 200.0F, 0.0F, result.h_mm);
  assertVectorNear(0.0F, 260.0F, 0.0F, result.g_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.p_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 50.0F, result.g_pct);
}

void test_forward_kinematics_rotates_radial_plane_with_turntable()
{
  const auto model = robotics::defaultRobotModel();
  const common::JointState state{90.0F, -90.0F, 0.0F, 0.0F, 0.0F, 0.0F};

  const auto result = robotics::forwardKinematics(state, model);

  assertVectorNear(260.0F, 0.0F, 0.0F, result.g_mm);
}

void test_forward_kinematics_accumulates_elbow_and_pitch_angles()
{
  const auto model = robotics::defaultRobotModel();
  const common::JointState state{0.0F, -90.0F, 90.0F, -90.0F, 25.0F, 75.0F};

  const auto result = robotics::forwardKinematics(state, model);

  assertVectorNear(0.0F, 100.0F, 0.0F, result.e_mm);
  assertVectorNear(0.0F, 100.0F, 100.0F, result.h_mm);
  assertVectorNear(0.0F, 160.0F, 100.0F, result.g_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.p_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 25.0F, result.r_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 75.0F, result.g_pct);
}

void test_forward_kinematics_applies_turntable_to_shoulder_offset()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::RobotModelOffset offset{0.0F, 0.0F, 0.0F, 0.0F, 20.0F, 5.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState state{0.0F, -90.0F, 0.0F, 0.0F, 0.0F, 50.0F};

  const auto result = robotics::forwardKinematics(state, model, offset);

  assertVectorNear(0.0F, 0.0F, 0.0F, result.d_mm);
  assertVectorNear(0.0F, 20.0F, 5.0F, result.s_mm);
  assertVectorNear(0.0F, 120.0F, 5.0F, result.e_mm);
  assertVectorNear(0.0F, 220.0F, 5.0F, result.h_mm);
  assertVectorNear(0.0F, 280.0F, 5.0F, result.g_mm);
}

void test_forward_kinematics_rotates_turntable_to_shoulder_offset()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::RobotModelOffset offset{0.0F, 0.0F, 0.0F, 0.0F, 20.0F, 5.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState state{90.0F, -90.0F, 0.0F, 0.0F, 0.0F, 50.0F};

  const auto result = robotics::forwardKinematics(state, model, offset);

  assertVectorNear(20.0F, 0.0F, 5.0F, result.s_mm);
  assertVectorNear(280.0F, 0.0F, 5.0F, result.g_mm);
}

void test_inverse_kinematics_maps_reachable_forward_target()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::OffsetTargetPose pose{0.0F, 260.0F, 0.0F, 0.0F, 0.0F, 40.0F};

  const auto result = robotics::inverseKinematics(pose, model);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(robotics::KinematicsStatus::Ok, result.status);
  TEST_ASSERT_EQUAL_STRING("ok", robotics::toString(result.status));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.hr_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 40.0F, result.joint_state.g_pct);
}

void test_inverse_kinematics_maps_turntable_angle()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::OffsetTargetPose pose{260.0F, 0.0F, 0.0F, 0.0F, 15.0F, 25.0F};

  const auto result = robotics::inverseKinematics(pose, model);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 90.0F, result.joint_state.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 15.0F, result.joint_state.hr_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 25.0F, result.joint_state.g_pct);
}

void test_inverse_kinematics_maps_elbow_and_pitch_solution()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::OffsetTargetPose pose{0.0F, 160.0F, 100.0F, 0.0F, 25.0F, 75.0F};

  const auto result = robotics::inverseKinematics(pose, model);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 90.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 25.0F, result.joint_state.hr_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 75.0F, result.joint_state.g_pct);
}

void test_inverse_kinematics_applies_turntable_to_shoulder_offset()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::RobotModelOffset offset{0.0F, 0.0F, 0.0F, 0.0F, 20.0F, 5.0F, 0.0F, 0.0F, 0.0F};
  const robotics::OffsetTargetPose pose{0.0F, 280.0F, 5.0F, 0.0F, 10.0F, 40.0F};

  const auto result = robotics::inverseKinematics(pose, model, offset);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 10.0F, result.joint_state.hr_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 40.0F, result.joint_state.g_pct);
}

void test_inverse_kinematics_rotates_turntable_to_shoulder_offset()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::RobotModelOffset offset{0.0F, 0.0F, 0.0F, 0.0F, 20.0F, 5.0F, 0.0F, 0.0F, 0.0F};
  const robotics::OffsetTargetPose pose{280.0F, 0.0F, 5.0F, 0.0F, 10.0F, 40.0F};

  const auto result = robotics::inverseKinematics(pose, model, offset);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 90.0F, result.joint_state.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -90.0F, result.joint_state.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 0.0F, result.joint_state.hp_deg);
}

void test_inverse_kinematics_rejects_unreachable_target()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::OffsetTargetPose pose{0.0F, 1000.0F, 0.0F, 0.0F, 0.0F, 0.0F};

  const auto result = robotics::inverseKinematics(pose, model);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::KinematicsStatus::UnreachableTarget, result.status);
  TEST_ASSERT_EQUAL_STRING("unreachable_target", robotics::toString(result.status));
}

void test_inverse_kinematics_rejects_solution_outside_joint_limits()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::OffsetTargetPose pose{0.0F, 200.0F, 60.0F, 90.0F, 0.0F, 0.0F};

  const auto result = robotics::inverseKinematics(pose, model);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::KinematicsStatus::JointLimitViolation, result.status);
  TEST_ASSERT_EQUAL_STRING("joint_limit_violation", robotics::toString(result.status));
}

void test_inverse_kinematics_rejects_sideways_turntable_to_shoulder_offset()
{
  const auto model = robotics::defaultRobotModel();
  const robotics::RobotModelOffset offset{0.0F, 0.0F, 0.0F, 5.0F, 20.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const robotics::OffsetTargetPose pose{0.0F, 280.0F, 0.0F, 0.0F, 0.0F, 0.0F};

  const auto result = robotics::inverseKinematics(pose, model, offset);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(robotics::KinematicsStatus::UnsupportedOffset, result.status);
  TEST_ASSERT_EQUAL_STRING("unsupported_offset", robotics::toString(result.status));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_forward_kinematics_maps_home_pose_upwards);
  RUN_TEST(test_forward_kinematics_maps_horizontal_arm_along_positive_y);
  RUN_TEST(test_forward_kinematics_rotates_radial_plane_with_turntable);
  RUN_TEST(test_forward_kinematics_accumulates_elbow_and_pitch_angles);
  RUN_TEST(test_forward_kinematics_applies_turntable_to_shoulder_offset);
  RUN_TEST(test_forward_kinematics_rotates_turntable_to_shoulder_offset);
  RUN_TEST(test_inverse_kinematics_maps_reachable_forward_target);
  RUN_TEST(test_inverse_kinematics_maps_turntable_angle);
  RUN_TEST(test_inverse_kinematics_maps_elbow_and_pitch_solution);
  RUN_TEST(test_inverse_kinematics_applies_turntable_to_shoulder_offset);
  RUN_TEST(test_inverse_kinematics_rotates_turntable_to_shoulder_offset);
  RUN_TEST(test_inverse_kinematics_rejects_unreachable_target);
  RUN_TEST(test_inverse_kinematics_rejects_solution_outside_joint_limits);
  RUN_TEST(test_inverse_kinematics_rejects_sideways_turntable_to_shoulder_offset);
  return UNITY_END();
}
