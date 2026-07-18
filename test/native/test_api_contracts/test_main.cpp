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
  TEST_ASSERT_EQUAL_STRING("/api/joint-state", application::kJointStatePath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-motion", application::kJointMotionPath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-pwm-state", application::kJointPwmStatePath);
  TEST_ASSERT_EQUAL_STRING("/api/servo-driver/init", application::kServoDriverInitPath);
  TEST_ASSERT_EQUAL_STRING("/api/joint-pwm-motion", application::kJointPwmMotionPath);
  TEST_ASSERT_EQUAL_STRING("/api/motion", application::kMotionPath);
}

void test_api_contracts_map_capability_status_to_json_values()
{
  TEST_ASSERT_EQUAL_STRING("available", application::toString(application::ApiCapabilityStatus::Available));
  TEST_ASSERT_EQUAL_STRING("not_available", application::toString(application::ApiCapabilityStatus::NotAvailable));
}

void test_api_contracts_map_result_codes_to_json_values()
{
  TEST_ASSERT_EQUAL_STRING("ok", application::toString(application::ApiResultCode::Ok));
  TEST_ASSERT_EQUAL_STRING("invalid_json", application::toString(application::ApiResultCode::InvalidJson));
  TEST_ASSERT_EQUAL_STRING("missing_field", application::toString(application::ApiResultCode::MissingField));
  TEST_ASSERT_EQUAL_STRING("invalid_target_pose",
                           application::toString(application::ApiResultCode::InvalidTargetPose));
  TEST_ASSERT_EQUAL_STRING("target_pose_out_of_workspace",
                           application::toString(application::ApiResultCode::TargetPoseOutOfWorkspace));
  TEST_ASSERT_EQUAL_STRING("joint_limit_violation",
                           application::toString(application::ApiResultCode::JointLimitViolation));
  TEST_ASSERT_EQUAL_STRING("joint_pwm_limit_violation",
                           application::toString(application::ApiResultCode::JointPwmLimitViolation));
  TEST_ASSERT_EQUAL_STRING("kinematics_failure",
                           application::toString(application::ApiResultCode::KinematicsFailure));
  TEST_ASSERT_EQUAL_STRING("hardware_driver_failure",
                           application::toString(application::ApiResultCode::HardwareDriverFailure));
  TEST_ASSERT_EQUAL_STRING("orchestrator_unavailable",
                           application::toString(application::ApiResultCode::OrchestratorUnavailable));
  TEST_ASSERT_EQUAL_STRING("unknown_route", application::toString(application::ApiResultCode::UnknownRoute));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_api_contracts_expose_stable_name_and_version);
  RUN_TEST(test_api_contracts_expose_stable_endpoint_paths);
  RUN_TEST(test_api_contracts_map_capability_status_to_json_values);
  RUN_TEST(test_api_contracts_map_result_codes_to_json_values);
  return UNITY_END();
}
