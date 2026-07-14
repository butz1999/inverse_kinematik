// Shared REST API constants and status helpers.

#pragma once

namespace application {

enum class ApiCapabilityStatus {
  Available,
  NotAvailable,
};

enum class ApiResultCode {
  Ok,
  InvalidJson,
  MissingField,
  JointLimitViolation,
  JointPwmLimitViolation,
  OrchestratorUnavailable,
  UnknownRoute,
};

constexpr const char *kApiName = "inverse_kinematic";
constexpr const char *kApiVersion = "v1";

const char *toString(ApiCapabilityStatus status);
const char *toString(ApiResultCode code);

}  // namespace application
