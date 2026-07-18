// Native tests for ideal forward kinematics.

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

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_forward_kinematics_maps_home_pose_upwards);
  RUN_TEST(test_forward_kinematics_maps_horizontal_arm_along_positive_y);
  RUN_TEST(test_forward_kinematics_rotates_radial_plane_with_turntable);
  RUN_TEST(test_forward_kinematics_accumulates_elbow_and_pitch_angles);
  return UNITY_END();
}
