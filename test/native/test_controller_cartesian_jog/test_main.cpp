// Native tests for accelerated Cartesian controller jogging.

#include <unity.h>

#include "application/ControllerCartesianJog.h"

void test_cartesian_jog_maps_sticks_to_positive_pose_axes()
{
  auto input = application::emptyControllerInput();
  input.valid = true;
  input.left_x = 2047;
  input.left_y = 2047;
  input.right_y = 2047;
  auto state = application::emptyControllerCartesianJogState();

  const auto result = application::applyControllerCartesianJog(input, common::initialTargetPose(), 1000U, state);

  TEST_ASSERT_TRUE(result.active);
  TEST_ASSERT_TRUE(result.changed);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 40.0F, result.target_pose.x_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 110.0F, result.target_pose.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 90.0F, result.target_pose.z_mm);
}

void test_cartesian_jog_ignores_stick_values_inside_deadzone()
{
  auto input = application::emptyControllerInput();
  input.valid = true;
  input.left_x = 250;
  auto state = application::emptyControllerCartesianJogState();
  const auto pose = common::initialTargetPose();

  const auto result = application::applyControllerCartesianJog(input, pose, 100U, state);

  TEST_ASSERT_FALSE(result.active);
  TEST_ASSERT_FALSE(result.changed);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, pose.x_mm, result.target_pose.x_mm);
}

void test_cartesian_jog_accelerates_and_brakes_after_stick_release()
{
  auto input = application::emptyControllerInput();
  input.valid = true;
  input.left_y = 2047;
  auto state = application::emptyControllerCartesianJogState();
  const auto pose = common::initialTargetPose();

  const auto accelerating = application::applyControllerCartesianJog(input, pose, 100U, state);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 18.0F, state.velocity_y_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.8F, accelerating.target_pose.y_mm - pose.y_mm);

  input.left_y = 0;
  const auto braking = application::applyControllerCartesianJog(input, accelerating.target_pose, 100U, state);
  TEST_ASSERT_FALSE(braking.active);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_y_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, braking.target_pose.y_mm - accelerating.target_pose.y_mm);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_cartesian_jog_maps_sticks_to_positive_pose_axes);
  RUN_TEST(test_cartesian_jog_ignores_stick_values_inside_deadzone);
  RUN_TEST(test_cartesian_jog_accelerates_and_brakes_after_stick_release);
  return UNITY_END();
}
