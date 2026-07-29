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
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 100.0F, result.target_pose.x_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 170.0F, result.target_pose.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 150.0F, result.target_pose.z_mm);
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
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 24.0F, state.velocity_y_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.4F, accelerating.target_pose.y_mm - pose.y_mm);

  input.left_y = 0;
  const auto braking = application::applyControllerCartesianJog(input, accelerating.target_pose, 100U, state);
  TEST_ASSERT_FALSE(braking.active);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_y_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, braking.target_pose.y_mm - accelerating.target_pose.y_mm);
}

void test_cartesian_jog_keeps_a_persistent_target_pose_while_joint_output_catches_up()
{
  auto input = application::emptyControllerInput();
  input.valid = true;
  input.left_y = 2047;
  auto state = application::emptyControllerCartesianJogState();
  const auto initial_pose = common::initialTargetPose();

  const auto first = application::applyControllerCartesianJog(input, initial_pose, 100U, state);
  const common::TargetPose delayed_joint_output_pose{initial_pose.x_mm,  initial_pose.y_mm + 0.5F, initial_pose.z_mm,
                                                     initial_pose.p_deg, initial_pose.r_deg,       initial_pose.g_pct};
  const auto second = application::applyControllerCartesianJog(input, delayed_joint_output_pose, 100U, state);

  TEST_ASSERT_TRUE(application::hasControllerCartesianJogTarget(state));
  TEST_ASSERT_TRUE(second.target_pose.y_mm > first.target_pose.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, state.target_pose.y_mm, second.target_pose.y_mm);
}

void test_cartesian_jog_reduces_but_does_not_stop_at_elbow_singularity()
{
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.15F, application::controllerCartesianSingularitySpeedScale(0.0F));
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, application::controllerCartesianSingularitySpeedScale(30.0F));
}

void test_cartesian_jog_stops_velocity_without_discarding_last_valid_target_pose()
{
  auto state = application::emptyControllerCartesianJogState();
  state.velocity_x_mm_s = 12.0F;
  state.velocity_y_mm_s = -8.0F;
  state.velocity_z_mm_s = 4.0F;
  state.target_pose_initialized = true;
  state.target_pose = common::TargetPose{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};

  application::stopControllerCartesianJog(state);

  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_x_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_y_mm_s);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.velocity_z_mm_s);
  TEST_ASSERT_TRUE(state.target_pose_initialized);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.0F, state.target_pose.y_mm);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_cartesian_jog_maps_sticks_to_positive_pose_axes);
  RUN_TEST(test_cartesian_jog_ignores_stick_values_inside_deadzone);
  RUN_TEST(test_cartesian_jog_accelerates_and_brakes_after_stick_release);
  RUN_TEST(test_cartesian_jog_keeps_a_persistent_target_pose_while_joint_output_catches_up);
  RUN_TEST(test_cartesian_jog_reduces_but_does_not_stop_at_elbow_singularity);
  RUN_TEST(test_cartesian_jog_stops_velocity_without_discarding_last_valid_target_pose);
  return UNITY_END();
}
