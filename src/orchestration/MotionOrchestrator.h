// Coordinates task-space motion requests through validation, IK and planning.

#pragma once

#include <string>

#include "common/MotionProfile.h"
#include "common/TargetPose.h"
#include "orchestration/MotionProfileGenerator.h"
#include "robotics/Kinematics.h"
#include "robotics/Validation.h"

namespace orchestration
{

enum class MotionStatus
{
  Accepted,
  InvalidTargetPose,
  KinematicsFailure,
  JointLimitViolation,
  MotionPlanFailure,
};

struct MotionRequest
{
  common::TargetPose target_pose;
  common::MotionProfile profile;
};

struct MotionResult
{
  bool ok{false};
  MotionStatus status{MotionStatus::KinematicsFailure};
  std::string field_name;
  std::string message;
  common::TargetPose target_pose{common::initialTargetPose()};
  robotics::OffsetTargetPose offset_target_pose{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  common::JointState joint_state{common::initialJointState()};
  common::MotionPlan motion_plan{common::defaultMotionProfile(), 0U, {}, 0U};
  robotics::ValidationStatus target_validation_status{robotics::ValidationStatus::Ok};
  robotics::KinematicsStatus kinematics_status{robotics::KinematicsStatus::Ok};
  robotics::ValidationStatus joint_validation_status{robotics::ValidationStatus::Ok};
  MotionProfileGeneratorStatus motion_profile_status{MotionProfileGeneratorStatus::Ok};
};

class MotionOrchestrator
{
 public:
  MotionOrchestrator(const robotics::RobotModel &robot_model, const robotics::RobotModelOffset &robot_offset);

  void processMotionRequestInto(const MotionRequest &request, const common::JointState &current_joint_state,
                                MotionResult &result) const;
  MotionResult processMotionRequest(const MotionRequest &request, const common::JointState &current_joint_state) const;

 private:
  robotics::RobotModel robot_model_;
  robotics::RobotModelOffset robot_offset_;
};

const char *toString(MotionStatus status);

}  // namespace orchestration
