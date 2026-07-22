// Native tests for ArduinoJson-backed REST request parsing.

#include <unity.h>

#include "application/ApiJson.h"
#include "hardware/StatusLed.h"

namespace
{

struct SequenceParseFixture
{
  application::SequenceDefinition sequence;
  application::SequenceDefinitionParseResult result;
};

SequenceParseFixture parseSequence(const char *body)
{
  auto sequence = application::emptySequenceDefinition();
  const auto result = application::parseSequenceDefinitionRequestJson(body, sequence);
  return SequenceParseFixture{sequence, result};
}

}  // namespace

void test_parse_joint_motion_accepts_valid_json()
{
  const auto result = application::parseJointMotionRequestJson(
      "{\"d_deg\":1,\"s_deg\":2,\"e_deg\":3,\"hp_deg\":-4,\"hr_deg\":5,"
      "\"g_pct\":6}");

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, result.code);
  TEST_ASSERT_EQUAL_FLOAT(1.0F, result.joint_state.d_deg);
  TEST_ASSERT_EQUAL_FLOAT(-4.0F, result.joint_state.hp_deg);
  TEST_ASSERT_EQUAL_FLOAT(6.0F, result.joint_state.g_pct);
}

void test_parse_joint_motion_rejects_malformed_json()
{
  const auto result = application::parseJointMotionRequestJson("{");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::InvalidJson, result.code);
}

void test_parse_joint_motion_rejects_missing_field()
{
  const auto result =
      application::parseJointMotionRequestJson("{\"d_deg\":1,\"s_deg\":2,\"e_deg\":3,\"hp_deg\":4,\"hr_deg\":5}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::MissingField, result.code);
  TEST_ASSERT_EQUAL_STRING("g_pct", result.field_name);
}

void test_parse_joint_motion_rejects_limit_violation()
{
  const auto result = application::parseJointMotionRequestJson(
      "{\"d_deg\":91,\"s_deg\":0,\"e_deg\":0,\"hp_deg\":0,\"hr_deg\":0,"
      "\"g_pct\":0}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::JointLimitViolation, result.code);
  TEST_ASSERT_EQUAL_STRING("d_deg", result.field_name);
}

void test_parse_joint_pwm_motion_accepts_valid_json()
{
  const auto result = application::parseJointPwmMotionRequestJson(
      "{\"d_pwm\":0,\"s_pwm\":1,\"e_pwm\":307,\"hp_pwm\":307,"
      "\"hr_pwm\":307,\"g_pwm\":410}");

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, result.code);
  TEST_ASSERT_EQUAL_UINT16(307U, result.joint_pwm_state.e_pwm);
  TEST_ASSERT_EQUAL_UINT16(410U, result.joint_pwm_state.g_pwm);
}

void test_parse_joint_pwm_motion_rejects_fractional_pwm()
{
  const auto result = application::parseJointPwmMotionRequestJson(
      "{\"d_pwm\":1.5,\"s_pwm\":1,\"e_pwm\":1,\"hp_pwm\":1,"
      "\"hr_pwm\":1,\"g_pwm\":1}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::MissingField, result.code);
  TEST_ASSERT_EQUAL_STRING("d_pwm", result.field_name);
}

void test_parse_joint_pwm_motion_rejects_pwm_limit_violation()
{
  const auto result = application::parseJointPwmMotionRequestJson(
      "{\"d_pwm\":4096,\"s_pwm\":0,\"e_pwm\":0,\"hp_pwm\":0,"
      "\"hr_pwm\":0,\"g_pwm\":0}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::JointPwmLimitViolation, result.code);
  TEST_ASSERT_EQUAL_STRING("d_pwm", result.field_name);
}

void test_parse_target_pose_accepts_valid_json()
{
  const auto result = application::parseTargetPoseRequestJson(
      "{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5,"
      "\"g_pct\":6}");

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, result.code);
  TEST_ASSERT_EQUAL_FLOAT(1.0F, result.target_pose.x_mm);
  TEST_ASSERT_EQUAL_FLOAT(4.0F, result.target_pose.p_deg);
  TEST_ASSERT_EQUAL_FLOAT(6.0F, result.target_pose.g_pct);
  TEST_ASSERT_EQUAL(common::MotionProfileType::SmoothStartStop, result.motion_profile.type);
  TEST_ASSERT_EQUAL_FLOAT(common::defaultMotionProfile().target_velocity_deg_s,
                          result.motion_profile.target_velocity_deg_s);
  TEST_ASSERT_EQUAL_UINT32(common::defaultMotionProfile().sample_time_ms, result.motion_profile.sample_time_ms);
}

void test_parse_target_pose_accepts_motion_profile_options()
{
  const auto result = application::parseTargetPoseRequestJson(
      "{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5,\"g_pct\":6,"
      "\"motionProfile\":{\"type\":\"smooth_start_stop\",\"target_velocity_deg_s\":15,"
      "\"sample_time_ms\":40}}");

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, result.code);
  TEST_ASSERT_EQUAL(common::MotionProfileType::SmoothStartStop, result.motion_profile.type);
  TEST_ASSERT_EQUAL_FLOAT(15.0F, result.motion_profile.target_velocity_deg_s);
  TEST_ASSERT_EQUAL_UINT32(40U, result.motion_profile.sample_time_ms);
}

void test_parse_target_pose_accepts_fast_start_stop_motion_profile()
{
  const auto result = application::parseTargetPoseRequestJson(
      "{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5,\"g_pct\":6,"
      "\"motionProfile\":{\"type\":\"fast_start_stop\"}}");

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, result.code);
  TEST_ASSERT_EQUAL(common::MotionProfileType::FastStartStop, result.motion_profile.type);
}

void test_parse_target_pose_rejects_unknown_motion_profile_type()
{
  const auto result = application::parseTargetPoseRequestJson(
      "{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5,\"g_pct\":6,"
      "\"motionProfile\":{\"type\":\"teleport\"}}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::InvalidTargetPose, result.code);
  TEST_ASSERT_EQUAL_STRING("motionProfile.type", result.field_name);
}

void test_parse_target_pose_rejects_missing_field()
{
  const auto result =
      application::parseTargetPoseRequestJson("{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::MissingField, result.code);
  TEST_ASSERT_EQUAL_STRING("g_pct", result.field_name);
}

void test_parse_target_pose_rejects_gripper_limit_violation()
{
  const auto result = application::parseTargetPoseRequestJson(
      "{\"x_mm\":1,\"y_mm\":2,\"z_mm\":3,\"p_deg\":4,\"r_deg\":5,"
      "\"g_pct\":101}");

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::InvalidTargetPose, result.code);
  TEST_ASSERT_EQUAL_STRING("g_pct", result.field_name);
}

void test_parse_sequence_definition_accepts_flat_steps()
{
  const auto parsed = parseSequence(
      "{\"steps\":[{\"name\":\"pick\",\"x_mm\":-20,\"y_mm\":50,\"z_mm\":30,\"p_deg\":-90,"
      "\"r_deg\":0,\"g_pct\":0,\"wait_ms\":250,\"motionProfile\":{\"type\":\"constant_velocity\","
      "\"target_velocity_deg_s\":30,\"sample_time_ms\":20}}]}");

  TEST_ASSERT_TRUE(parsed.result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::Ok, parsed.result.code);
  TEST_ASSERT_EQUAL_UINT(1U, parsed.sequence.step_count);
  TEST_ASSERT_EQUAL(application::steps::StepType::Pose, parsed.sequence.steps[0].type);
  TEST_ASSERT_EQUAL_STRING("pick", parsed.sequence.steps[0].pose.name.c_str());
  TEST_ASSERT_EQUAL_FLOAT(-20.0F, parsed.sequence.steps[0].pose.target_pose.x_mm);
  TEST_ASSERT_EQUAL(common::MotionProfileType::ConstantVelocity, parsed.sequence.steps[0].pose.motion_profile.type);
  TEST_ASSERT_EQUAL_FLOAT(30.0F, parsed.sequence.steps[0].pose.motion_profile.target_velocity_deg_s);
  TEST_ASSERT_EQUAL_UINT32(20U, parsed.sequence.steps[0].pose.motion_profile.sample_time_ms);
}

void test_parse_sequence_definition_accepts_nested_target_pose()
{
  const auto parsed = parseSequence(
      "{\"steps\":[{\"targetPose\":{\"x_mm\":-20,\"y_mm\":50,\"z_mm\":30,\"p_deg\":-90,"
      "\"r_deg\":0,\"g_pct\":50}}]}");

  TEST_ASSERT_TRUE(parsed.result.ok);
  TEST_ASSERT_EQUAL_UINT(1U, parsed.sequence.step_count);
  TEST_ASSERT_EQUAL(application::steps::StepType::Pose, parsed.sequence.steps[0].type);
  TEST_ASSERT_EQUAL_FLOAT(50.0F, parsed.sequence.steps[0].pose.target_pose.g_pct);
  TEST_ASSERT_EQUAL(common::MotionProfileType::SmoothStartStop, parsed.sequence.steps[0].pose.motion_profile.type);
}

void test_parse_sequence_definition_accepts_wait_step()
{
  const auto parsed = parseSequence("{\"steps\":[{\"type\":\"wait\",\"duration_ms\":250}]}");

  TEST_ASSERT_TRUE(parsed.result.ok);
  TEST_ASSERT_EQUAL_UINT(1U, parsed.sequence.step_count);
  TEST_ASSERT_EQUAL(application::steps::StepType::Wait, parsed.sequence.steps[0].type);
  TEST_ASSERT_EQUAL_UINT32(250U, parsed.sequence.steps[0].wait.duration_ms);
}

void test_parse_sequence_definition_accepts_led_step_with_rgb_mode_and_interval()
{
  const auto parsed = parseSequence(
      "{\"steps\":[{\"type\":\"led\",\"name\":\"signal\",\"rgb\":{\"r\":10,\"g\":20,\"b\":30},"
      "\"mode\":\"pulsing\",\"interval_ms\":750}]}");

  TEST_ASSERT_TRUE(parsed.result.ok);
  TEST_ASSERT_EQUAL_UINT(1U, parsed.sequence.step_count);
  TEST_ASSERT_EQUAL(application::steps::StepType::Led, parsed.sequence.steps[0].type);
  TEST_ASSERT_EQUAL_STRING("signal", parsed.sequence.steps[0].led.name.c_str());
  TEST_ASSERT_TRUE(parsed.sequence.steps[0].led.has_rgb_color);
  TEST_ASSERT_EQUAL_UINT8(10U, parsed.sequence.steps[0].led.rgb_color.r);
  TEST_ASSERT_EQUAL_UINT8(20U, parsed.sequence.steps[0].led.rgb_color.g);
  TEST_ASSERT_EQUAL_UINT8(30U, parsed.sequence.steps[0].led.rgb_color.b);
  TEST_ASSERT_TRUE(parsed.sequence.steps[0].led.has_mode);
  TEST_ASSERT_EQUAL(hardware::StatusLed::Mode::Pulsing, parsed.sequence.steps[0].led.mode);
  TEST_ASSERT_TRUE(parsed.sequence.steps[0].led.has_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(750U, parsed.sequence.steps[0].led.interval_ms);
}

void test_parse_sequence_definition_accepts_led_step_without_color()
{
  const auto parsed = parseSequence("{\"steps\":[{\"type\":\"led\",\"mode\":\"blinking\",\"interval_ms\":500}]}");

  TEST_ASSERT_TRUE(parsed.result.ok);
  TEST_ASSERT_EQUAL(application::steps::StepType::Led, parsed.sequence.steps[0].type);
  TEST_ASSERT_FALSE(parsed.sequence.steps[0].led.has_status_color);
  TEST_ASSERT_FALSE(parsed.sequence.steps[0].led.has_rgb_color);
  TEST_ASSERT_TRUE(parsed.sequence.steps[0].led.has_mode);
  TEST_ASSERT_EQUAL(hardware::StatusLed::Mode::Blinking, parsed.sequence.steps[0].led.mode);
}

void test_parse_sequence_definition_rejects_led_step_with_color_and_rgb()
{
  const auto parsed =
      parseSequence("{\"steps\":[{\"type\":\"led\",\"color\":\"green\",\"rgb\":{\"r\":10,\"g\":20,\"b\":30}}]}");

  TEST_ASSERT_FALSE(parsed.result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::InvalidTargetPose, parsed.result.code);
  TEST_ASSERT_EQUAL_STRING("color/rgb", parsed.result.field_name);
}

void test_parse_sequence_definition_rejects_empty_steps()
{
  const auto parsed = parseSequence("{\"steps\":[]}");

  TEST_ASSERT_FALSE(parsed.result.ok);
  TEST_ASSERT_EQUAL(application::ApiResultCode::InvalidTargetPose, parsed.result.code);
  TEST_ASSERT_EQUAL_STRING("steps", parsed.result.field_name);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_parse_joint_motion_accepts_valid_json);
  RUN_TEST(test_parse_joint_motion_rejects_malformed_json);
  RUN_TEST(test_parse_joint_motion_rejects_missing_field);
  RUN_TEST(test_parse_joint_motion_rejects_limit_violation);
  RUN_TEST(test_parse_joint_pwm_motion_accepts_valid_json);
  RUN_TEST(test_parse_joint_pwm_motion_rejects_fractional_pwm);
  RUN_TEST(test_parse_joint_pwm_motion_rejects_pwm_limit_violation);
  RUN_TEST(test_parse_target_pose_accepts_valid_json);
  RUN_TEST(test_parse_target_pose_accepts_motion_profile_options);
  RUN_TEST(test_parse_target_pose_accepts_fast_start_stop_motion_profile);
  RUN_TEST(test_parse_target_pose_rejects_unknown_motion_profile_type);
  RUN_TEST(test_parse_target_pose_rejects_missing_field);
  RUN_TEST(test_parse_target_pose_rejects_gripper_limit_violation);
  RUN_TEST(test_parse_sequence_definition_accepts_flat_steps);
  RUN_TEST(test_parse_sequence_definition_accepts_nested_target_pose);
  RUN_TEST(test_parse_sequence_definition_accepts_wait_step);
  RUN_TEST(test_parse_sequence_definition_accepts_led_step_with_rgb_mode_and_interval);
  RUN_TEST(test_parse_sequence_definition_accepts_led_step_without_color);
  RUN_TEST(test_parse_sequence_definition_rejects_led_step_with_color_and_rgb);
  RUN_TEST(test_parse_sequence_definition_rejects_empty_steps);
  return UNITY_END();
}
