#include "orchestration/ControllerHandler.h"

#include <algorithm>
#include <cmath>

#include "config/RobotSettings.h"
#include "robotics/Kinematics.h"
#include "robotics/Validation.h"

namespace orchestration
{

namespace
{

constexpr float kCartesianMaximumVelocityMmS = 120.0F;
constexpr float kCartesianMaximumAccelerationMmS2 = 240.0F;
constexpr float kJointMaximumVelocityDegS = 180.0F;
constexpr float kJointMaximumAccelerationDegS2 = 360.0F;
constexpr float kWorldRollLockPitchDeg = -90.0F;
constexpr float kWorldRollLockPitchToleranceDeg = 20.0F;

bool jointStatesDiffer(const common::JointState &left, const common::JointState &right)
{
  return left.d_deg != right.d_deg || left.s_deg != right.s_deg || left.e_deg != right.e_deg ||
         left.hp_deg != right.hp_deg || left.hr_deg != right.hr_deg || left.g_pct != right.g_pct;
}

float approach(float current, float target, float maximum_delta)
{
  return current < target ? std::min(target, current + maximum_delta) : std::max(target, current - maximum_delta);
}

float singularitySpeedScale(float elbow_deg)
{
  constexpr float kSlowdownElbowAngleDeg = 20.0F;
  constexpr float kMinimumSpeedScale = 0.15F;
  constexpr float kPi = 3.14159265358979323846F;
  const auto normalized =
      std::fabs(std::sin((elbow_deg * kPi) / 180.0F)) / std::sin((kSlowdownElbowAngleDeg * kPi) / 180.0F);
  return kMinimumSpeedScale + ((1.0F - kMinimumSpeedScale) * std::min(1.0F, normalized));
}

common::TargetPose targetPoseFromJointState(const common::JointState &joint_state)
{
  const auto &settings = config::robotSettings();
  const auto fk = robotics::forwardKinematics(joint_state, settings.robot_model, settings.robot_model_offset);
  return common::TargetPose{fk.g_mm.x_mm + settings.robot_model_offset.o_d_offset_x_mm,
                            fk.g_mm.y_mm + settings.robot_model_offset.o_d_offset_y_mm,
                            fk.g_mm.z_mm + settings.robot_model_offset.o_d_offset_z_mm,
                            fk.p_deg,
                            fk.r_deg,
                            fk.g_pct};
}

bool isWorldRollLockEligible(float pitch_deg)
{
  return std::fabs(pitch_deg - kWorldRollLockPitchDeg) <= kWorldRollLockPitchToleranceDeg;
}

float limitJointAxis(float current, float target, float &velocity_deg_s, float elapsed_seconds)
{
  const auto remaining = target - current;
  const auto maximum_braking_velocity = std::sqrt(2.0F * kJointMaximumAccelerationDegS2 * std::fabs(remaining));
  const auto target_velocity =
      remaining == 0.0F ? 0.0F
                        : std::copysign(std::min(kJointMaximumVelocityDegS, maximum_braking_velocity), remaining);
  velocity_deg_s = approach(velocity_deg_s, target_velocity, kJointMaximumAccelerationDegS2 * elapsed_seconds);
  const auto delta = velocity_deg_s * elapsed_seconds;
  if (std::fabs(delta) >= std::fabs(remaining))
  {
    velocity_deg_s = 0.0F;
    return target;
  }
  return current + delta;
}

common::JointState limitJointState(const common::JointState &current, const common::JointState &target,
                                   common::JointState &velocity, float elapsed_seconds)
{
  auto limited = current;
  limited.d_deg = limitJointAxis(current.d_deg, target.d_deg, velocity.d_deg, elapsed_seconds);
  limited.s_deg = limitJointAxis(current.s_deg, target.s_deg, velocity.s_deg, elapsed_seconds);
  limited.e_deg = limitJointAxis(current.e_deg, target.e_deg, velocity.e_deg, elapsed_seconds);
  limited.hp_deg = limitJointAxis(current.hp_deg, target.hp_deg, velocity.hp_deg, elapsed_seconds);
  limited.hr_deg = limitJointAxis(current.hr_deg, target.hr_deg, velocity.hr_deg, elapsed_seconds);
  limited.g_pct = target.g_pct;
  return limited;
}

ControllerHandlerStatus statusFromMotionStatus(MotionStatus status)
{
  switch (status)
  {
    case MotionStatus::InvalidTargetPose:
      return ControllerHandlerStatus::InvalidTargetPose;
    case MotionStatus::JointLimitViolation:
      return ControllerHandlerStatus::JointLimitViolation;
    case MotionStatus::Accepted:
    case MotionStatus::KinematicsFailure:
    case MotionStatus::MotionPlanFailure:
      return ControllerHandlerStatus::KinematicsFailure;
  }
  return ControllerHandlerStatus::KinematicsFailure;
}

}  // namespace

ControllerHandler::ControllerHandler(const MotionOrchestrator &motion_orchestrator)
    : motion_orchestrator_(motion_orchestrator)
{
}

ControllerHandlerState ControllerHandler::state() const
{
  return ControllerHandlerState{world_roll_lock_enabled_, locked_world_roll_deg_};
}

void ControllerHandler::synchronizeJointState(const common::JointState &joint_state)
{
  cartesian_velocity_x_mm_s_ = 0.0F;
  cartesian_velocity_y_mm_s_ = 0.0F;
  cartesian_velocity_z_mm_s_ = 0.0F;
  target_pose_ = targetPoseFromJointState(joint_state);
  target_pose_initialized_ = true;
  slew_velocity_deg_s_ = common::initialJointState();
  world_roll_lock_enabled_ = false;
  previous_toggle_pressed_ = false;
  locked_world_roll_deg_ = 0.0F;
}

void ControllerHandler::reset()
{
  cartesian_velocity_x_mm_s_ = 0.0F;
  cartesian_velocity_y_mm_s_ = 0.0F;
  cartesian_velocity_z_mm_s_ = 0.0F;
  target_pose_initialized_ = false;
  target_pose_ = common::initialTargetPose();
  slew_velocity_deg_s_ = common::initialJointState();
  world_roll_lock_enabled_ = false;
  previous_toggle_pressed_ = false;
  locked_world_roll_deg_ = 0.0F;
}

ControllerHandlerResult ControllerHandler::update(const JogCommand &command,
                                                  const common::JointState &current_joint_state, uint32_t elapsed_ms)
{
  if (!command.valid || elapsed_ms == 0U)
  {
    return ControllerHandlerResult{false, false, ControllerHandlerStatus::Inactive, current_joint_state};
  }

  const auto current_pose = targetPoseFromJointState(current_joint_state);
  const auto toggle_edge = command.world_roll_toggle_pressed && !previous_toggle_pressed_;
  previous_toggle_pressed_ = command.world_roll_toggle_pressed;
  if (world_roll_lock_enabled_ && (!isWorldRollLockEligible(current_pose.p_deg) || toggle_edge))
  {
    world_roll_lock_enabled_ = false;
    locked_world_roll_deg_ = 0.0F;
    target_pose_ = current_pose;
    target_pose_initialized_ = true;
    slew_velocity_deg_s_ = common::initialJointState();
  }
  else if (toggle_edge && isWorldRollLockEligible(current_pose.p_deg))
  {
    world_roll_lock_enabled_ = !world_roll_lock_enabled_;
    if (world_roll_lock_enabled_)
    {
      locked_world_roll_deg_ = current_joint_state.d_deg + current_joint_state.hr_deg;
    }
  }

  if (hasJointJog(command))
  {
    if (command.joint_velocity_per_second.hr_deg != 0.0F)
    {
      world_roll_lock_enabled_ = false;
      locked_world_roll_deg_ = 0.0F;
    }
    cartesian_velocity_x_mm_s_ = 0.0F;
    cartesian_velocity_y_mm_s_ = 0.0F;
    cartesian_velocity_z_mm_s_ = 0.0F;
    target_pose_initialized_ = false;
    slew_velocity_deg_s_ = common::initialJointState();

    const auto elapsed_seconds = static_cast<float>(elapsed_ms) / 1000.0F;
    auto next = current_joint_state;
    for (const auto axis : common::kJointAxes)
    {
      auto &value = common::jointAxisValue(next, axis);
      const auto velocity = common::jointAxisValue(command.joint_velocity_per_second, axis);
      const auto &limit = common::jointLimitForAxis(config::robotSettings().joint_limits, axis);
      value = std::clamp(value + velocity * elapsed_seconds, limit.min_value, limit.max_value);
    }
    return ControllerHandlerResult{true, jointStatesDiffer(next, current_joint_state), ControllerHandlerStatus::Updated,
                                   next};
  }

  if (!target_pose_initialized_)
  {
    target_pose_ = current_pose;
    target_pose_initialized_ = true;
  }

  const auto elapsed_seconds = static_cast<float>(elapsed_ms) / 1000.0F;
  const auto scale = singularitySpeedScale(current_joint_state.e_deg);
  cartesian_velocity_x_mm_s_ =
      approach(cartesian_velocity_x_mm_s_, command.x_input * kCartesianMaximumVelocityMmS * scale,
               kCartesianMaximumAccelerationMmS2 * elapsed_seconds);
  cartesian_velocity_y_mm_s_ =
      approach(cartesian_velocity_y_mm_s_, command.y_input * kCartesianMaximumVelocityMmS * scale,
               kCartesianMaximumAccelerationMmS2 * elapsed_seconds);
  cartesian_velocity_z_mm_s_ =
      approach(cartesian_velocity_z_mm_s_, command.z_input * kCartesianMaximumVelocityMmS * scale,
               kCartesianMaximumAccelerationMmS2 * elapsed_seconds);

  auto candidate_pose = target_pose_;
  candidate_pose.x_mm += cartesian_velocity_x_mm_s_ * elapsed_seconds;
  candidate_pose.y_mm += cartesian_velocity_y_mm_s_ * elapsed_seconds;
  candidate_pose.z_mm += cartesian_velocity_z_mm_s_ * elapsed_seconds;
  const auto target_result = motion_orchestrator_.resolveTargetPose(candidate_pose, current_joint_state);
  if (!target_result.ok)
  {
    cartesian_velocity_x_mm_s_ = 0.0F;
    cartesian_velocity_y_mm_s_ = 0.0F;
    cartesian_velocity_z_mm_s_ = 0.0F;
    return ControllerHandlerResult{false, false, statusFromMotionStatus(target_result.status), current_joint_state};
  }
  target_pose_ = candidate_pose;

  auto target_joint_state = target_result.joint_state;
  if (world_roll_lock_enabled_)
  {
    target_joint_state.hr_deg = locked_world_roll_deg_ - target_joint_state.d_deg;
    if (!robotics::validateJointState(target_joint_state).ok)
    {
      return ControllerHandlerResult{false, false, ControllerHandlerStatus::JointLimitViolation, current_joint_state};
    }
  }

  const auto limited = limitJointState(current_joint_state, target_joint_state, slew_velocity_deg_s_, elapsed_seconds);
  const auto active = command.x_input != 0.0F || command.y_input != 0.0F || command.z_input != 0.0F ||
                      cartesian_velocity_x_mm_s_ != 0.0F || cartesian_velocity_y_mm_s_ != 0.0F ||
                      cartesian_velocity_z_mm_s_ != 0.0F || jointStatesDiffer(limited, target_joint_state);
  return ControllerHandlerResult{active, jointStatesDiffer(limited, current_joint_state),
                                 ControllerHandlerStatus::Updated, limited};
}

const char *toString(ControllerHandlerStatus status)
{
  switch (status)
  {
    case ControllerHandlerStatus::Inactive:
      return "inactive";
    case ControllerHandlerStatus::Updated:
      return "updated";
    case ControllerHandlerStatus::InvalidTargetPose:
      return "invalid_target_pose";
    case ControllerHandlerStatus::KinematicsFailure:
      return "kinematics_failure";
    case ControllerHandlerStatus::JointLimitViolation:
      return "joint_limit_violation";
  }
  return "inactive";
}

}  // namespace orchestration
