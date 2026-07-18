// Robot model primitives for task-space validation and later kinematics.

#pragma once

#include "common/TargetPose.h"

namespace robotics
{

struct SegmentLengths
{
  float s_e_length_mm;
  float e_hp_length_mm;
  float hr_g_length_mm;
};

struct CartesianWorkspace
{
  float min_z_mm;
  float max_z_mm;
  float max_reach_mm;
};

struct RobotModel
{
  SegmentLengths segments;
  CartesianWorkspace workspace;
};

struct RobotModelOffset
{
  float o_d_offset_x_mm;
  float o_d_offset_y_mm;
  float o_d_offset_z_mm;
  float d_s_offset_x_mm;
  float d_s_offset_y_mm;
  float d_s_offset_z_mm;
  float hp_hr_offset_up_mm;
  float hp_hr_offset_side_mm;
  float hp_hr_offset_forward_mm;
};

struct OffsetTargetPose
{
  float x_mm;
  float y_mm;
  float z_mm;
  float p_deg;
  float r_deg;
  float g_pct;
};

RobotModel defaultRobotModel();
RobotModelOffset defaultRobotModelOffset();
bool isValidRobotModel(const RobotModel &model);
float maxReachFromSegments(const SegmentLengths &segments);
OffsetTargetPose applyRobotModelOffset(const common::TargetPose &pose, const RobotModelOffset &offset);

}  // namespace robotics
