#include "orchestration/MotionOrchestrator.h"

namespace orchestration
{

namespace
{

MotionResult baseResult(const MotionRequest &request)
{
  MotionResult result;
  result.target_pose = request.target_pose;
  result.motion_plan.profile = request.profile;
  return result;
}

MotionResult rejected(const MotionRequest &request, MotionStatus status, const std::string &field_name,
                      const std::string &message)
{
  auto result = baseResult(request);
  result.status = status;
  result.field_name = field_name;
  result.message = message;
  return result;
}

void resetResult(const MotionRequest &request, MotionResult &result)
{
  result.ok = false;
  result.status = MotionStatus::KinematicsFailure;
  result.field_name.clear();
  result.message.clear();
  result.target_pose = request.target_pose;
  result.offset_target_pose = robotics::OffsetTargetPose{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  result.joint_state = common::initialJointState();
  result.motion_plan.profile = request.profile;
  result.motion_plan.total_duration_ms = 0U;
  result.motion_plan.sample_count = 0U;
  result.target_validation_status = robotics::ValidationStatus::Ok;
  result.kinematics_status = robotics::KinematicsStatus::Ok;
  result.joint_validation_status = robotics::ValidationStatus::Ok;
  result.motion_profile_status = MotionProfileGeneratorStatus::Ok;
}

void rejectInto(const MotionRequest &request, MotionResult &result, MotionStatus status,
                const std::string &field_name, const std::string &message)
{
  resetResult(request, result);
  result.status = status;
  result.field_name = field_name;
  result.message = message;
}

}  // namespace

MotionOrchestrator::MotionOrchestrator(const robotics::RobotModel &robot_model,
                                       const robotics::RobotModelOffset &robot_offset)
    : robot_model_(robot_model), robot_offset_(robot_offset)
{
}

MotionResult MotionOrchestrator::processMotionRequest(const MotionRequest &request,
                                                      const common::JointState &current_joint_state) const
{
  auto result = baseResult(request);
  processMotionRequestInto(request, current_joint_state, result);
  return result;
}

void MotionOrchestrator::processMotionRequestInto(const MotionRequest &request,
                                                  const common::JointState &current_joint_state,
                                                  MotionResult &result) const
{
  resetResult(request, result);

  const auto target_validation = robotics::validateTargetPose(request.target_pose, robot_model_);
  if (!target_validation.ok)
  {
    rejectInto(request, result, MotionStatus::InvalidTargetPose, target_validation.field_name,
               target_validation.message);
    result.target_validation_status = target_validation.status;
    return;
  }

  const auto offset_pose = robotics::applyRobotModelOffset(request.target_pose, robot_offset_);
  const auto ik_result = robotics::inverseKinematics(offset_pose, robot_model_, robot_offset_);
  if (!ik_result.ok)
  {
    rejectInto(request, result, MotionStatus::KinematicsFailure, "", ik_result.message);
    result.offset_target_pose = offset_pose;
    result.kinematics_status = ik_result.status;
    return;
  }

  const auto joint_validation = robotics::validateJointState(ik_result.joint_state);
  if (!joint_validation.ok)
  {
    rejectInto(request, result, MotionStatus::JointLimitViolation, joint_validation.field_name,
               joint_validation.message);
    result.offset_target_pose = offset_pose;
    result.joint_state = ik_result.joint_state;
    result.joint_validation_status = joint_validation.status;
    return;
  }

  const auto motion_plan_status =
      generateMotionPlanInto(current_joint_state, ik_result.joint_state, request.profile, result.motion_plan);
  if (!motion_plan_status.ok)
  {
    rejectInto(request, result, MotionStatus::MotionPlanFailure, "", motion_plan_status.message);
    result.offset_target_pose = offset_pose;
    result.joint_state = ik_result.joint_state;
    result.motion_profile_status = motion_plan_status.status;
    return;
  }

  result.ok = true;
  result.status = MotionStatus::Accepted;
  result.message = "ok";
  result.offset_target_pose = offset_pose;
  result.joint_state = ik_result.joint_state;
}

const char *toString(MotionStatus status)
{
  switch (status)
  {
    case MotionStatus::Accepted:
      return "accepted";
    case MotionStatus::InvalidTargetPose:
      return "invalid_target_pose";
    case MotionStatus::KinematicsFailure:
      return "kinematics_failure";
    case MotionStatus::JointLimitViolation:
      return "joint_limit_violation";
    case MotionStatus::MotionPlanFailure:
      return "motion_plan_failure";
  }

  return "kinematics_failure";
}

}  // namespace orchestration
