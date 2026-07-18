// Native tests for robot model defaults and offset handling.

#include <unity.h>

#include "robotics/RobotModel.h"

void test_default_robot_model_is_valid()
{
  const auto model = robotics::defaultRobotModel();

  TEST_ASSERT_TRUE(robotics::isValidRobotModel(model));
  TEST_ASSERT_EQUAL_FLOAT(robotics::maxReachFromSegments(model.segments), model.workspace.max_reach_mm);
}

void test_robot_model_rejects_invalid_segment_lengths()
{
  auto model = robotics::defaultRobotModel();
  model.segments.e_hp_length_mm = 0.0F;

  TEST_ASSERT_FALSE(robotics::isValidRobotModel(model));
}

void test_robot_model_offset_moves_target_into_turntable_space()
{
  const common::TargetPose pose{100.0F, 120.0F, 50.0F, 0.0F, 20.0F, 30.0F};
  const robotics::RobotModelOffset offset{10.0F, 20.0F, 5.0F, 5.0F, 10.0F, 15.0F, 1.0F, 2.0F, 3.0F};

  const auto adjusted = robotics::applyRobotModelOffset(pose, offset);

  TEST_ASSERT_FLOAT_WITHIN(0.001F, 90.0F, adjusted.x_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 100.0F, adjusted.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 45.0F, adjusted.z_mm);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, adjusted.p_deg);
  TEST_ASSERT_EQUAL_FLOAT(20.0F, adjusted.r_deg);
  TEST_ASSERT_EQUAL_FLOAT(30.0F, adjusted.g_pct);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_default_robot_model_is_valid);
  RUN_TEST(test_robot_model_rejects_invalid_segment_lengths);
  RUN_TEST(test_robot_model_offset_moves_target_into_turntable_space);
  return UNITY_END();
}
