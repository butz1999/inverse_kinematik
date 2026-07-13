// Native tests for REST API contract constants and string mappings.

#include <unity.h>

#include "application/ApiContracts.h"

void test_api_contracts_expose_stable_name_and_version() {
  TEST_ASSERT_EQUAL_STRING("inverse_kinematic", application::kApiName);
  TEST_ASSERT_EQUAL_STRING("v1", application::kApiVersion);
}

void test_api_contracts_map_capability_status_to_json_values() {
  TEST_ASSERT_EQUAL_STRING(
      "available",
      application::toString(application::ApiCapabilityStatus::Available));
  TEST_ASSERT_EQUAL_STRING(
      "not_available",
      application::toString(application::ApiCapabilityStatus::NotAvailable));
}

void test_api_contracts_map_result_codes_to_json_values() {
  TEST_ASSERT_EQUAL_STRING("ok",
                           application::toString(application::ApiResultCode::Ok));
  TEST_ASSERT_EQUAL_STRING(
      "orchestrator_unavailable",
      application::toString(application::ApiResultCode::OrchestratorUnavailable));
  TEST_ASSERT_EQUAL_STRING(
      "unknown_route",
      application::toString(application::ApiResultCode::UnknownRoute));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_api_contracts_expose_stable_name_and_version);
  RUN_TEST(test_api_contracts_map_capability_status_to_json_values);
  RUN_TEST(test_api_contracts_map_result_codes_to_json_values);
  return UNITY_END();
}
