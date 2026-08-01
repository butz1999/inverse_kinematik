#include "robotics/Validation.h"

#include <cmath>

#include "config/RobotSettings.h"

namespace robotics
{

namespace
{

constexpr const char *kEmptyField = "";

ValidationResult ok()
{
  return ValidationResult{true, ValidationStatus::Ok, kEmptyField, "ok"};
}

ValidationResult error(ValidationStatus status, const char *field_name, const char *message)
{
  return ValidationResult{false, status, field_name, message};
}

const char *firstNonFiniteTargetField(const common::TargetPose &pose)
{
  if (!std::isfinite(pose.x_mm))
  {
    return "x_mm";
  }
  if (!std::isfinite(pose.y_mm))
  {
    return "y_mm";
  }
  if (!std::isfinite(pose.z_mm))
  {
    return "z_mm";
  }
  if (!std::isfinite(pose.p_deg))
  {
    return "p_deg";
  }
  if (!std::isfinite(pose.r_deg))
  {
    return "r_deg";
  }
  if (!std::isfinite(pose.g_pct))
  {
    return "g_pct";
  }
  return kEmptyField;
}

}  // namespace

TargetPoseResult validateTargetPose(const common::TargetPose &pose, const RobotModel &model)
{
  if (!isValidRobotModel(model))
  {
    return error(ValidationStatus::InvalidRobotModel, "robot_model", "Robot model is invalid.");
  }

  if (!common::isFinite(pose))
  {
    return error(ValidationStatus::InvalidTargetPose, firstNonFiniteTargetField(pose),
                 "Target pose values must be finite numbers.");
  }

  if (!common::isWithinTargetGripperLimits(pose))
  {
    return error(ValidationStatus::InvalidTargetPose, "g_pct", "Target gripper value is outside 0..100 percent.");
  }

  return ok();
}

JointStateResult validateJointState(const common::JointState &state)
{
  if (!common::isFinite(state))
  {
    return error(ValidationStatus::JointLimitViolation, kEmptyField, "Joint values must be finite numbers.");
  }

  const auto violation = common::findFirstLimitViolation(state, config::robotSettings().joint_limits);
  if (violation.has_value())
  {
    return error(ValidationStatus::JointLimitViolation, common::jointAxisFieldName(*violation),
                 "Joint value is outside the configured joint limits.");
  }

  return ok();
}

const char *toString(ValidationStatus status)
{
  switch (status)
  {
    case ValidationStatus::Ok:
      return "ok";
    case ValidationStatus::InvalidRobotModel:
      return "invalid_robot_model";
    case ValidationStatus::InvalidTargetPose:
      return "invalid_target_pose";
    case ValidationStatus::JointLimitViolation:
      return "joint_limit_violation";
  }

  return "invalid_target_pose";
}

}  // namespace robotics
