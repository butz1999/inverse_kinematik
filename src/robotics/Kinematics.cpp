#include "robotics/Kinematics.h"

#include <array>
#include <cmath>
#include <limits>

namespace robotics
{

namespace
{

constexpr Vector3 kOrigin{0.0F, 0.0F, 0.0F};
constexpr float kReachToleranceMm = 0.01F;
constexpr float kCosTolerance = 0.000001F;
constexpr float kTurntableCandidateToleranceDeg = 0.01F;
constexpr float kFkPositionToleranceMm = 0.5F;
constexpr float kFkAngleToleranceDeg = 0.05F;
constexpr float kPoseErrorTieTolerance = 0.0001F;

struct CandidateScore
{
  common::JointState joint_state;
  float pose_error;
  float reference_distance;
};

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

InverseKinematicsResult ikErrorResult(KinematicsStatus status, const char *message)
{
  return InverseKinematicsResult{false, status, common::initialJointState(), message};
}

InverseKinematicsResult ikOkResult(const common::JointState &joint_state)
{
  return InverseKinematicsResult{true, KinematicsStatus::Ok, joint_state, "ok"};
}

bool isWithinKinematicsJointLimits(const common::JointState &state)
{
  return common::isWithinJointLimits(state);
}

float square(float value)
{
  return value * value;
}

float poseErrorSquared(const OffsetTargetPose &pose, const ForwardKinematicsResult &fk)
{
  return square(fk.g_mm.x_mm - pose.x_mm) + square(fk.g_mm.y_mm - pose.y_mm) + square(fk.g_mm.z_mm - pose.z_mm) +
         square(fk.p_deg - pose.p_deg) + square(fk.r_deg - pose.r_deg) + square(fk.g_pct - pose.g_pct);
}

bool matchesTargetPose(const OffsetTargetPose &pose, const ForwardKinematicsResult &fk)
{
  return std::fabs(fk.g_mm.x_mm - pose.x_mm) <= kFkPositionToleranceMm &&
         std::fabs(fk.g_mm.y_mm - pose.y_mm) <= kFkPositionToleranceMm &&
         std::fabs(fk.g_mm.z_mm - pose.z_mm) <= kFkPositionToleranceMm &&
         std::fabs(fk.p_deg - pose.p_deg) <= kFkAngleToleranceDeg &&
         std::fabs(fk.r_deg - pose.r_deg) <= kFkAngleToleranceDeg &&
         std::fabs(fk.g_pct - pose.g_pct) <= kFkAngleToleranceDeg;
}

float jointDistanceSquared(const common::JointState &candidate, const common::JointState &reference)
{
  return square(candidate.d_deg - reference.d_deg) + square(candidate.s_deg - reference.s_deg) +
         square(candidate.e_deg - reference.e_deg) + square(candidate.hp_deg - reference.hp_deg) +
         square(candidate.hr_deg - reference.hr_deg);
}

bool isBetterCandidate(const CandidateScore &candidate, const CandidateScore &best,
                       const common::JointState *reference_joint_state)
{
  if (candidate.pose_error < best.pose_error - kPoseErrorTieTolerance)
  {
    return true;
  }

  return reference_joint_state != nullptr && std::fabs(candidate.pose_error - best.pose_error) <= kPoseErrorTieTolerance &&
         candidate.reference_distance < best.reference_distance;
}

bool solveArmPlane(float wrist_radial_mm, float wrist_z_mm, float p_deg, float e_delta_deg, float d_deg,
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

  const common::JointState candidate{d_deg, shoulder_angle_deg - 90.0F, e_delta_deg, hp_deg, pose.r_deg, pose.g_pct};
  if (!isWithinKinematicsJointLimits(candidate))
  {
    return false;
  }

  result = candidate;
  return true;
}

Vector3 hpToHrVector(float p_deg, float d_deg, const RobotModelOffset &offset)
{
  const auto p_rad = degreesToRadians(p_deg);
  const auto local_radial_mm =
      (offset.hp_hr_offset_forward_mm * std::cos(p_rad)) - (offset.hp_hr_offset_up_mm * std::sin(p_rad));
  const auto local_z_mm =
      (offset.hp_hr_offset_forward_mm * std::sin(p_rad)) + (offset.hp_hr_offset_up_mm * std::cos(p_rad));
  return vectorFromTurntableLocal(offset.hp_hr_offset_side_mm, local_radial_mm, local_z_mm, d_deg);
}

std::array<float, 2> turntableCandidatesDeg(const OffsetTargetPose &pose, float local_side_offset_mm)
{
  const auto target_radial_mm = std::sqrt((pose.x_mm * pose.x_mm) + (pose.y_mm * pose.y_mm));
  if (target_radial_mm <= kReachToleranceMm)
  {
    return {0.0F, 180.0F};
  }

  const auto target_angle_rad = std::atan2(pose.x_mm, pose.y_mm);
  const auto side_ratio = clampCos(local_side_offset_mm / target_radial_mm);
  const auto side_angle_rad = std::asin(side_ratio);
  return {radiansToDegrees(target_angle_rad - side_angle_rad),
          radiansToDegrees(target_angle_rad - (kPi - side_angle_rad))};
}

float localRadialForTurntableDeg(const OffsetTargetPose &pose, float d_deg)
{
  const auto d_rad = degreesToRadians(d_deg);
  return (pose.x_mm * std::sin(d_rad)) + (pose.y_mm * std::cos(d_rad));
}

}  // namespace

static InverseKinematicsResult inverseKinematicsImpl(const OffsetTargetPose &pose, const RobotModel &model,
                                                      const RobotModelOffset &offset,
                                                      const common::JointState *reference_joint_state);

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
  const auto s_point = add(d_point, vectorFromTurntableLocal(offset.d_s_offset_x_mm, offset.d_s_offset_y_mm,
                                                             offset.d_s_offset_z_mm, state.d_deg));
  const auto e_point = add(s_point, segmentVector(model.segments.s_e_length_mm, s_angle_deg, state.d_deg));
  const auto h_point = add(e_point, segmentVector(model.segments.e_hp_length_mm, e_angle_deg, state.d_deg));
  const auto hr_point = add(h_point, hpToHrVector(hp_angle_deg, state.d_deg, offset));
  const auto g_point = add(hr_point, segmentVector(model.segments.hr_g_length_mm, hp_angle_deg, state.d_deg));

  return ForwardKinematicsResult{d_point,
                                 s_point,
                                 e_point,
                                 h_point,
                                 g_point,
                                 toolOrientationFromWorldAngles(state.d_deg, hp_angle_deg, state.hr_deg),
                                 hp_angle_deg,
                                 state.hr_deg,
                                 state.g_pct};
}

InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model)
{
  return inverseKinematics(pose, model, defaultRobotModelOffset());
}

InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model,
                                          const RobotModelOffset &offset)
{
  return inverseKinematicsImpl(pose, model, offset, nullptr);
}

InverseKinematicsResult inverseKinematics(const OffsetTargetPose &pose, const RobotModel &model,
                                          const RobotModelOffset &offset,
                                          const common::JointState &reference_joint_state)
{
  return inverseKinematicsImpl(pose, model, offset, &reference_joint_state);
}

static InverseKinematicsResult inverseKinematicsImpl(const OffsetTargetPose &pose, const RobotModel &model,
                                                      const RobotModelOffset &offset,
                                                      const common::JointState *reference_joint_state)
{
  if (!isValidRobotModel(model))
  {
    return ikErrorResult(KinematicsStatus::InvalidRobotModel, "Robot model is invalid.");
  }

  const auto local_side_offset_mm = offset.d_s_offset_x_mm + offset.hp_hr_offset_side_mm;
  const auto target_radial_mm = std::sqrt((pose.x_mm * pose.x_mm) + (pose.y_mm * pose.y_mm));
  if (target_radial_mm + kReachToleranceMm < std::fabs(local_side_offset_mm))
  {
    return ikErrorResult(KinematicsStatus::UnreachableTarget, "Target cannot align with the sideways shoulder offset.");
  }

  const auto p_rad = degreesToRadians(pose.p_deg);
  const auto l1 = model.segments.s_e_length_mm;
  const auto l2 = model.segments.e_hp_length_mm;
  const auto min_reach_mm = std::fabs(l1 - l2);
  const auto max_reach_mm = l1 + l2;

  bool had_geometric_solution = false;
  bool had_verified_solution = false;
  CandidateScore best_candidate{common::initialJointState(), std::numeric_limits<float>::max(),
                                std::numeric_limits<float>::max()};
  const auto candidates = turntableCandidatesDeg(pose, local_side_offset_mm);
  for (const auto d_deg : candidates)
  {
    if (d_deg < common::kJointLimits[0].min_value - kTurntableCandidateToleranceDeg ||
        d_deg > common::kJointLimits[0].max_value + kTurntableCandidateToleranceDeg)
    {
      continue;
    }

    const auto target_local_radial_mm = localRadialForTurntableDeg(pose, d_deg);
    const auto hp_hr_vector = hpToHrVector(pose.p_deg, d_deg, offset);
    const auto hp_hr_local_radial_mm = localRadialForTurntableDeg(
        OffsetTargetPose{hp_hr_vector.x_mm, hp_hr_vector.y_mm, hp_hr_vector.z_mm, pose.p_deg, pose.r_deg, pose.g_pct},
        d_deg);
    const auto wrist_radial_mm = target_local_radial_mm - offset.d_s_offset_y_mm - hp_hr_local_radial_mm -
                                 (model.segments.hr_g_length_mm * std::cos(p_rad));
    const auto wrist_z_mm =
        pose.z_mm - offset.d_s_offset_z_mm - hp_hr_vector.z_mm - (model.segments.hr_g_length_mm * std::sin(p_rad));
    const auto wrist_distance_mm = std::sqrt((wrist_radial_mm * wrist_radial_mm) + (wrist_z_mm * wrist_z_mm));

    if (wrist_distance_mm > max_reach_mm + kReachToleranceMm || wrist_distance_mm < min_reach_mm - kReachToleranceMm)
    {
      continue;
    }

    const auto cos_e = ((wrist_distance_mm * wrist_distance_mm) - (l1 * l1) - (l2 * l2)) / (2.0F * l1 * l2);
    if (cos_e > 1.0F + kCosTolerance || cos_e < -1.0F - kCosTolerance)
    {
      continue;
    }

    had_geometric_solution = true;
    const auto e_abs_deg = radiansToDegrees(std::acos(clampCos(cos_e)));
    const std::array<float, 2> elbow_candidates{e_abs_deg, -e_abs_deg};
    for (const auto e_delta_deg : elbow_candidates)
    {
      common::JointState joint_state{};
      if (!solveArmPlane(wrist_radial_mm, wrist_z_mm, pose.p_deg, e_delta_deg, d_deg, pose, model, joint_state))
      {
        continue;
      }

      const auto fk = forwardKinematics(joint_state, model, offset);
      if (!matchesTargetPose(pose, fk))
      {
        continue;
      }

      const CandidateScore candidate{joint_state, poseErrorSquared(pose, fk),
                                     reference_joint_state != nullptr
                                         ? jointDistanceSquared(joint_state, *reference_joint_state)
                                         : std::numeric_limits<float>::max()};
      if (!had_verified_solution || isBetterCandidate(candidate, best_candidate, reference_joint_state))
      {
        best_candidate = candidate;
        had_verified_solution = true;
      }
    }
  }

  if (had_verified_solution)
  {
    return ikOkResult(best_candidate.joint_state);
  }

  if (had_geometric_solution)
  {
    return ikErrorResult(KinematicsStatus::JointLimitViolation, "Target can only be reached outside joint limits.");
  }

  return ikErrorResult(KinematicsStatus::UnreachableTarget, "Target wrist position is outside the arm reach.");
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
