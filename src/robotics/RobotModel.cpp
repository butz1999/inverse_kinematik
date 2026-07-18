#include "robotics/RobotModel.h"

#include <cmath>

namespace robotics
{

namespace
{

bool isPositiveFinite(float value)
{
  return std::isfinite(value) && value > 0.0F;
}

}  // namespace

RobotModel defaultRobotModel()
{
  const SegmentLengths segments{105.0F, 100.0F, 100.0F};
  const CartesianWorkspace workspace{0.0F, 260.0F, maxReachFromSegments(segments)};
  return RobotModel{segments, workspace};
}

RobotModelOffset defaultRobotModelOffset()
{
  // clang-format off
  return RobotModelOffset{
      .o_d_offset_x_mm =           0.0F,
      .o_d_offset_y_mm =        -105.0F,
      .o_d_offset_z_mm =          77.5F,
      .d_s_offset_x_mm =         -12.5F,
      .d_s_offset_y_mm =         -11.0F,
      .d_s_offset_z_mm =          12.5F,
      .hp_hr_offset_up_mm =       28.0F,
      .hp_hr_offset_side_mm =      0.0F,
      .hp_hr_offset_forward_mm =  45.0F,
  };
  // clang-format on
}

bool isValidRobotModel(const RobotModel &model)
{
  return isPositiveFinite(model.segments.s_e_length_mm) &&
         isPositiveFinite(model.segments.e_hp_length_mm) &&
         isPositiveFinite(model.segments.hr_g_length_mm) &&
         std::isfinite(model.workspace.min_z_mm) &&
         std::isfinite(model.workspace.max_z_mm) &&
         model.workspace.min_z_mm <= model.workspace.max_z_mm &&
         isPositiveFinite(model.workspace.max_reach_mm);
}

float maxReachFromSegments(const SegmentLengths &segments)
{
  return segments.s_e_length_mm + segments.e_hp_length_mm + segments.hr_g_length_mm;
}

OffsetTargetPose applyRobotModelOffset(const common::TargetPose &pose, const RobotModelOffset &offset)
{
  // Only static origin-to-turntable offsets can be applied before kinematics.
  // Joint-dependent offsets must be transformed inside the kinematic chain.
  auto x_mm = pose.x_mm - offset.o_d_offset_x_mm;
  auto y_mm = pose.y_mm - offset.o_d_offset_y_mm;
  auto z_mm = pose.z_mm - offset.o_d_offset_z_mm;

  return OffsetTargetPose{x_mm, y_mm, z_mm, pose.p_deg, pose.r_deg, pose.g_pct};
}

}  // namespace robotics
