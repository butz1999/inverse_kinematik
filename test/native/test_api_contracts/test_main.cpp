// Native tests for REST API contract constants and string mappings.

#include <unity.h>

#include "application/ApiContracts.h"

void test_api_contracts_expose_stable_name_and_version()
{
  TEST_ASSERT_EQUAL_STRING("inverse_kinematic", application::kApiName);
  TEST_ASSERT_EQUAL_STRING("v1", application::kApiVersion);
}

void test_api_contracts_expose_stable_endpoint_paths()
{
  TEST_ASSERT_EQUAL_STRING("/api/health", application::kHealthPath);
  TEST_ASSERT_EQUAL_STRING("/api/status", application::kStatusPath);
  TEST_ASSERT_EQUAL_STRING("/api/settings/motion-limits", application::kMotionLimitsPath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-state", application::kJointStatePath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-motion", application::kJointMotionPath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-pwm-state", application::kJointPwmStatePath);
  TEST_ASSERT_EQUAL_STRING("/api/servo-driver/init", application::kServoDriverInitPath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-pwm-motion", application::kJointPwmMotionPath);
  TEST_ASSERT_EQUAL_STRING("/api/motion", application::kMotionPath);
  TEST_ASSERT_EQUAL_STRING("/api/forward-kinematics", application::kForwardKinematicsPath);
  TEST_ASSERT_EQUAL_STRING("/api/sequence/start", application::kSequenceStartPath);
  TEST_ASSERT_EQUAL_STRING("/api/sequence/stop", application::kSequenceStopPath);
  TEST_ASSERT_EQUAL_STRING("/api/sequence/status", application::kSequenceStatusPath);
}

void test_api_contracts_expose_available_capability_value()
{
  TEST_ASSERT_EQUAL_STRING("available", application::kApiCapabilityAvailable);
}

void test_api_contracts_map_result_codes_to_json_values()
{
  TEST_ASSERT_EQUAL_STRING("ok", application::toString(application::ApiResultCode::Ok));
  TEST_ASSERT_EQUAL_STRING("invalid_json", application::toString(application::ApiResultCode::InvalidJson));
  TEST_ASSERT_EQUAL_STRING("missing_field", application::toString(application::ApiResultCode::MissingField));
  TEST_ASSERT_EQUAL_STRING("invalid_target_pose", application::toString(application::ApiResultCode::InvalidTargetPose));
  TEST_ASSERT_EQUAL_STRING("joint_limit_violation",
                           application::toString(application::ApiResultCode::JointLimitViolation));
  TEST_ASSERT_EQUAL_STRING("joint_pwm_limit_violation",
                           application::toString(application::ApiResultCode::JointPwmLimitViolation));
  TEST_ASSERT_EQUAL_STRING("kinematics_failure", application::toString(application::ApiResultCode::KinematicsFailure));
  TEST_ASSERT_EQUAL_STRING("hardware_driver_failure",
                           application::toString(application::ApiResultCode::HardwareDriverFailure));
  TEST_ASSERT_EQUAL_STRING("sequence_busy", application::toString(application::ApiResultCode::SequenceBusy));
  TEST_ASSERT_EQUAL_STRING("unknown_route", application::toString(application::ApiResultCode::UnknownRoute));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_api_contracts_expose_stable_name_and_version);
  RUN_TEST(test_api_contracts_expose_stable_endpoint_paths);
  RUN_TEST(test_api_contracts_expose_available_capability_value);
  RUN_TEST(test_api_contracts_map_result_codes_to_json_values);
  return UNITY_END();
}
