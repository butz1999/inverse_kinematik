// Forward and inverse kinematics contracts for the ideal robot model.

#pragma once

#include "common/JointState.h"
#include "robotics/Math.h"
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
  // Hand pitch, rotation and gripper open percentage as value.
  float p_deg;
  float r_deg;
  float g_pct;
};

ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model);

}  // namespace robotics
