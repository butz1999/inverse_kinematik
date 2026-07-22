// Native tests for the application-level RunEngine.

#include <unity.h>

#include "application/RunEngine.h"

namespace
{

application::steps::SequenceStep reachableStep(float gripper_pct)
{
  return application::steps::poseSequenceStep(
      application::steps::PoseStep{common::TargetPose{-20.0F, 50.0F, 30.0F, -90.0F, 0.0F, gripper_pct},
                                   common::MotionProfile{common::MotionProfileType::ConstantVelocity, 30.0F, 20U}, ""});
}

application::RunEngine runEngine()
{
  static const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                              robotics::defaultRobotModelOffset());
  return application::RunEngine(orchestrator);
}

}  // namespace

void test_run_engine_starts_first_step_and_reports_motion_plan()
{
  auto engine = runEngine();
  auto sequence = application::emptySequenceDefinition();
  sequence.step_count = 1U;
  sequence.steps[0] = reachableStep(0.0F);

  const auto result = engine.start(sequence, common::initialJointState(), 100U);
  const auto state = engine.state();

  TEST_ASSERT_TRUE(result.has_motion_plan);
  TEST_ASSERT_NOT_NULL(result.motion_result);
  TEST_ASSERT_TRUE(result.motion_result->ok);
  TEST_ASSERT_EQUAL(application::SequenceRunStatus::MotionActive, state.status);
  TEST_ASSERT_EQUAL_STRING("motion_active", application::toString(state.status));
  TEST_ASSERT_EQUAL_UINT(0U, state.step_index);
  TEST_ASSERT_EQUAL_UINT(1U, state.step_count);
}

void test_run_engine_waits_between_steps()
{
  auto engine = runEngine();
  auto sequence = application::emptySequenceDefinition();
  sequence.step_count = 3U;
  sequence.steps[0] = reachableStep(0.0F);
  sequence.steps[1] = application::steps::waitSequenceStep(application::steps::WaitStep{50U});
  sequence.steps[2] = reachableStep(50.0F);

  const auto start_result = engine.start(sequence, common::initialJointState(), 100U);
  TEST_ASSERT_TRUE(start_result.has_motion_plan);

  auto service_result = engine.service(start_result.target_joint_state, false, 120U);
  TEST_ASSERT_FALSE(service_result.has_motion_plan);
  TEST_ASSERT_EQUAL(application::SequenceRunStatus::Waiting, engine.state().status);

  service_result = engine.service(start_result.target_joint_state, false, 180U);
  TEST_ASSERT_TRUE(service_result.has_motion_plan);
  TEST_ASSERT_NOT_NULL(service_result.motion_plan);
  TEST_ASSERT_EQUAL(application::SequenceRunStatus::MotionActive, engine.state().status);
  TEST_ASSERT_EQUAL_UINT(2U, engine.state().step_index);
}

void test_run_engine_completes_after_last_motion()
{
  auto engine = runEngine();
  auto sequence = application::emptySequenceDefinition();
  sequence.step_count = 1U;
  sequence.steps[0] = reachableStep(0.0F);

  const auto start_result = engine.start(sequence, common::initialJointState(), 100U);
  TEST_ASSERT_TRUE(start_result.has_motion_plan);

  const auto service_result = engine.service(start_result.target_joint_state, false, 120U);

  TEST_ASSERT_FALSE(service_result.has_motion_plan);
  TEST_ASSERT_EQUAL(application::SequenceRunStatus::Completed, engine.state().status);
}

void test_run_engine_reports_led_step()
{
  auto engine = runEngine();
  auto sequence = application::emptySequenceDefinition();
  sequence.step_count = 2U;
  auto led_step = application::steps::emptyLedStep();
  led_step.has_rgb_color = true;
  led_step.rgb_color = hardware::StatusLed::RgbColor{10U, 20U, 30U};
  led_step.has_mode = true;
  led_step.mode = hardware::StatusLed::Mode::Pulsing;
  led_step.has_interval_ms = true;
  led_step.interval_ms = 750U;
  led_step.name = "blue-ish";
  sequence.steps[0] = application::steps::ledSequenceStep(led_step);
  sequence.steps[1] = reachableStep(0.0F);

  const auto color_result = engine.start(sequence, common::initialJointState(), 100U);

  TEST_ASSERT_FALSE(color_result.has_motion_plan);
  TEST_ASSERT_TRUE(color_result.has_led_step);
  TEST_ASSERT_TRUE(color_result.led_step.has_rgb_color);
  TEST_ASSERT_EQUAL_UINT8(10U, color_result.led_step.rgb_color.r);
  TEST_ASSERT_EQUAL_UINT8(20U, color_result.led_step.rgb_color.g);
  TEST_ASSERT_EQUAL_UINT8(30U, color_result.led_step.rgb_color.b);
  TEST_ASSERT_TRUE(color_result.led_step.has_mode);
  TEST_ASSERT_EQUAL(hardware::StatusLed::Mode::Pulsing, color_result.led_step.mode);
  TEST_ASSERT_TRUE(color_result.led_step.has_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(750U, color_result.led_step.interval_ms);
  TEST_ASSERT_EQUAL(application::SequenceRunStatus::Planning, engine.state().status);

  const auto pose_result = engine.service(common::initialJointState(), false, 100U);
  TEST_ASSERT_TRUE(pose_result.has_motion_plan);
  TEST_ASSERT_FALSE(pose_result.has_led_step);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_run_engine_starts_first_step_and_reports_motion_plan);
  RUN_TEST(test_run_engine_waits_between_steps);
  RUN_TEST(test_run_engine_completes_after_last_motion);
  RUN_TEST(test_run_engine_reports_led_step);
  return UNITY_END();
}
