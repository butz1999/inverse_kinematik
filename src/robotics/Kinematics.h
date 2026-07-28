// Forward and inverse kinematics contracts for the robot model.

#pragma once

#include "common/JointState.h"
#include "robotics/MathUtilities.h"
#include "robotics/RobotModel.h"

namespace robotics
{

struct ForwardKinematicsResult
{
  // Vectors are modeling the 3D point in space where the current joint is placed.
  Vector3 d_mm;
  Vector3 s_mm;
  Vector3 e_mm;
  Vector3 h_mm;
  Vector3 g_mm;
  // Tool-frame orientation in world coordinates.
  Orientation3 tool_orientation;
  // World elevation of the tool forward axis, axial tool roll and gripper opening.
  float p_deg;
  float r_deg;
  float g_pct;
};

enum class KinematicsStatus
{
  Ok,
  InvalidRobotModel,
  UnreachableTarget,
  JointLimitViolation,
  UnsupportedOffset,
};

struct InverseKinematicsResult
{
  bool ok;
  KinematicsStatus status;
  common::JointState joint_state;
  const char *message;
};

ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model);
ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model,
                                          const RobotModelOffset &offset);
InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model);
InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model,
                                          const RobotModelOffset &offset);
InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model,
                                          const RobotModelOffset &offset,
                                          const common::JointState &reference_joint_state);
const char *toString(KinematicsStatus status);

}  // namespace robotics
