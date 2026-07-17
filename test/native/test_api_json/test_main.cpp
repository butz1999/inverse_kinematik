// Native tests for ArduinoJson-backed REST request parsing.

#include <unity.h>

#include "application/ApiJson.h"

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
  return UNITY_END();
}
