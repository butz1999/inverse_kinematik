#include "robotics/Kinematics.h"

#include <cmath>

namespace robotics
{

namespace
{

constexpr Vector3 kOrigin{0.0F, 0.0F, 0.0F};
constexpr float kReachToleranceMm = 0.001F;
constexpr float kCosTolerance = 0.000001F;
constexpr float kOffsetToleranceMm = 0.001F;

Vector3 segmentVector(float length_mm, float angle_from_radial_deg, float d_deg)
{
  const auto angle_rad = degreesToRadians(angle_from_radial_deg);
  const auto radial_mm = length_mm * std::cos(angle_rad);
  const auto z_mm = length_mm * std::sin(angle_rad);
  return vectorFromRadialZ(radial_mm, z_mm, d_deg);
}

float shoulderAngleFromRadialDeg(float s_deg)
{
  return 90.0F + s_deg;
}

float clampCos(float value)
{
  if (value > 1.0F)
  {
    return 1.0F;
  }
  if (value < -1.0F)
  {
    return -1.0F;
  }
  return value;
}

InverseKinematicsResult error(KinematicsStatus status, const char *message)
{
  return InverseKinematicsResult{false, status, common::initialJointState(), message};
}

InverseKinematicsResult ok(const common::JointState &joint_state)
{
  return InverseKinematicsResult{true, KinematicsStatus::Ok, joint_state, "ok"};
}

bool isWithinKinematicsJointLimits(const common::JointState &state)
{
  return common::isWithinJointLimits(state);
}

bool solveArmPlane(float wrist_radial_mm, float wrist_z_mm, float p_deg, float e_delta_deg,
                   const OffsetTargetPose &pose, const RobotModel &model, common::JointState &result)
{
  const auto l1 = model.segments.s_e_length_mm;
  const auto l2 = model.segments.e_hp_length_mm;
  const auto target_angle_rad = std::atan2(wrist_z_mm, wrist_radial_mm);
  const auto e_delta_rad = degreesToRadians(e_delta_deg);
  const auto shoulder_angle_rad =
      target_angle_rad - std::atan2(l2 * std::sin(e_delta_rad), l1 + (l2 * std::cos(e_delta_rad)));
  const auto shoulder_angle_deg = radiansToDegrees(shoulder_angle_rad);
  const auto e_angle_deg = shoulder_angle_deg + e_delta_deg;
  const auto hp_deg = p_deg - e_angle_deg;
  const auto d_deg = radiansToDegrees(std::atan2(pose.x_mm, pose.y_mm));

  const common::JointState candidate{d_deg, shoulder_angle_deg - 90.0F, e_delta_deg, hp_deg, pose.r_deg, pose.g_pct};
  if (!isWithinKinematicsJointLimits(candidate))
  {
    return false;
  }

  result = candidate;
  return true;
}

}  // namespace

ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model)
{
  return forwardKinematics(state, model, defaultRobotModelOffset());
}

ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model,
                                          const RobotModelOffset &offset)
{
  const auto s_angle_deg = shoulderAngleFromRadialDeg(state.s_deg);
  const auto e_angle_deg = s_angle_deg + state.e_deg;
  const auto hp_angle_deg = e_angle_deg + state.hp_deg;

  const auto d_point = kOrigin;
  const auto s_point =
      add(d_point, vectorFromTurntableLocal(offset.d_s_offset_x_mm, offset.d_s_offset_y_mm, offset.d_s_offset_z_mm,
                                            state.d_deg));
  const auto e_point = add(s_point, segmentVector(model.segments.s_e_length_mm, s_angle_deg, state.d_deg));
  const auto h_point = add(e_point, segmentVector(model.segments.e_hp_length_mm, e_angle_deg, state.d_deg));
  const auto g_point = add(h_point, segmentVector(model.segments.hr_g_length_mm, hp_angle_deg, state.d_deg));

  return ForwardKinematicsResult{d_point, s_point, e_point, h_point, g_point, hp_angle_deg, state.hr_deg, state.g_pct};
}

InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model)
{
  return inverseKinematics(pose, model, defaultRobotModelOffset());
}

InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model,
                                          const RobotModelOffset &offset)
{
  if (!isValidRobotModel(model))
  {
    return error(KinematicsStatus::InvalidRobotModel, "Robot model is invalid.");
  }

  if (std::fabs(offset.d_s_offset_x_mm) > kOffsetToleranceMm)
  {
    return error(KinematicsStatus::UnsupportedOffset,
                 "Sideways turntable-to-shoulder offsets are not supported by the analytic IK yet.");
  }

  const auto target_radial_mm = std::sqrt((pose.x_mm * pose.x_mm) + (pose.y_mm * pose.y_mm));
  const auto p_rad = degreesToRadians(pose.p_deg);
  const auto wrist_radial_mm =
      target_radial_mm - offset.d_s_offset_y_mm - (model.segments.hr_g_length_mm * std::cos(p_rad));
  const auto wrist_z_mm =
      pose.z_mm - offset.d_s_offset_z_mm - (model.segments.hr_g_length_mm * std::sin(p_rad));
  const auto wrist_distance_mm =
      std::sqrt((wrist_radial_mm * wrist_radial_mm) + (wrist_z_mm * wrist_z_mm));

  const auto l1 = model.segments.s_e_length_mm;
  const auto l2 = model.segments.e_hp_length_mm;
  const auto min_reach_mm = std::fabs(l1 - l2);
  const auto max_reach_mm = l1 + l2;
  if (wrist_distance_mm > max_reach_mm + kReachToleranceMm || wrist_distance_mm < min_reach_mm - kReachToleranceMm)
  {
    return error(KinematicsStatus::UnreachableTarget, "Target wrist position is outside the arm reach.");
  }

  const auto cos_e = ((wrist_distance_mm * wrist_distance_mm) - (l1 * l1) - (l2 * l2)) / (2.0F * l1 * l2);
  if (cos_e > 1.0F + kCosTolerance || cos_e < -1.0F - kCosTolerance)
  {
    return error(KinematicsStatus::UnreachableTarget, "Target wrist position has no geometric solution.");
  }

  const auto e_abs_deg = radiansToDegrees(std::acos(clampCos(cos_e)));
  common::JointState joint_state{};
  if (solveArmPlane(wrist_radial_mm, wrist_z_mm, pose.p_deg, e_abs_deg, pose, model, joint_state) ||
      solveArmPlane(wrist_radial_mm, wrist_z_mm, pose.p_deg, -e_abs_deg, pose, model, joint_state))
  {
    return ok(joint_state);
  }

  return error(KinematicsStatus::JointLimitViolation, "Target can only be reached outside joint limits.");
}

const char *toString(KinematicsStatus status)
{
  switch (status)
  {
    case KinematicsStatus::Ok:
      return "ok";
    case KinematicsStatus::InvalidRobotModel:
      return "invalid_robot_model";
    case KinematicsStatus::UnreachableTarget:
      return "unreachable_target";
    case KinematicsStatus::JointLimitViolation:
      return "joint_limit_violation";
    case KinematicsStatus::UnsupportedOffset:
      return "unsupported_offset";
  }

  return "unreachable_target";
}

}  // namespace robotics
