// Domain validation for task-space targets and joint-space states.

#pragma once

#include "common/JointState.h"
#include "common/TargetPose.h"
#include "robotics/RobotModel.h"

namespace robotics
{

enum class ValidationStatus
{
  Ok,
  InvalidRobotModel,
  InvalidTargetPose,
  JointLimitViolation,
};

struct ValidationResult
{
  bool ok;
  ValidationStatus status;
  const char *field_name;
  const char *message;
};

using TargetPoseResult = ValidationResult;
using JointStateResult = ValidationResult;

TargetPoseResult validateTargetPose(const common::TargetPose &pose, const RobotModel &model);
JointStateResult validateJointState(const common::JointState &state);
const char *toString(ValidationStatus status);

}  // namespace robotics
