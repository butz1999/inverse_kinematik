// Shared REST API constants and status helpers.

#pragma once

namespace application
{

enum class ApiCapabilityStatus
{
  Available,
  NotAvailable,
};

enum class ApiResultCode
{
  Ok,
  InvalidJson,
  MissingField,
  InvalidTargetPose,
  TargetPoseOutOfWorkspace,
  JointLimitViolation,
  JointPwmLimitViolation,
  KinematicsFailure,
  HardwareDriverFailure,
  SequenceBusy,
  OrchestratorUnavailable,
  UnknownRoute,
};

// clang-format off
constexpr const char *kApiName =               "inverse_kinematic";
constexpr const char *kApiVersion =            "v1";
constexpr const char *kHealthPath =            "/api/health";
constexpr const char *kStatusPath =            "/api/status";
constexpr const char *kJointStatePath =        "/api/joint-state";
constexpr const char *kJointMotionPath =       "/api/joint-motion";
constexpr const char *kJointPwmStatePath =     "/api/joint-pwm-state";
constexpr const char *kJointPwmMotionPath =    "/api/joint-pwm-motion";
constexpr const char *kServoDriverInitPath =   "/api/servo-driver/init";
constexpr const char *kMotionPath =            "/api/motion";
constexpr const char *kForwardKinematicsPath = "/api/forward-kinematics";
constexpr const char *kSequenceStartPath =     "/api/sequence/start";
constexpr const char *kSequenceStopPath =      "/api/sequence/stop";
constexpr const char *kSequenceStatusPath =    "/api/sequence/status";
// clang-format on

const char *toString(ApiCapabilityStatus status);
const char *toString(ApiResultCode code);

}  // namespace application
