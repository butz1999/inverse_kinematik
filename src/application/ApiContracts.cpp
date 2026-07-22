// String conversions for externally visible API status values.

#include "application/ApiContracts.h"

namespace application
{

const char *toString(ApiCapabilityStatus status)
{
  switch (status)
  {
    case ApiCapabilityStatus::Available:
      return "available";
    case ApiCapabilityStatus::NotAvailable:
      return "not_available";
  }

  return "not_available";
}

const char *toString(ApiResultCode code)
{
  switch (code)
  {
    case ApiResultCode::Ok:
      return "ok";
    case ApiResultCode::InvalidJson:
      return "invalid_json";
    case ApiResultCode::MissingField:
      return "missing_field";
    case ApiResultCode::InvalidTargetPose:
      return "invalid_target_pose";
    case ApiResultCode::TargetPoseOutOfWorkspace:
      return "target_pose_out_of_workspace";
    case ApiResultCode::JointLimitViolation:
      return "joint_limit_violation";
    case ApiResultCode::JointPwmLimitViolation:
      return "joint_pwm_limit_violation";
    case ApiResultCode::KinematicsFailure:
      return "kinematics_failure";
    case ApiResultCode::HardwareDriverFailure:
      return "hardware_driver_failure";
    case ApiResultCode::SequenceBusy:
      return "sequence_busy";
    case ApiResultCode::OrchestratorUnavailable:
      return "orchestrator_unavailable";
    case ApiResultCode::UnknownRoute:
      return "unknown_route";
  }

  return "unknown_route";
}

}  // namespace application
