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
  bool ok{false};          // The actual result
  std::string field_name;  // Name of the field the error caused
  std::string message;     // The message text
  MotionStatus status{MotionStatus::KinematicsFailure};
  common::TargetPose target_pose{common::initialTargetPose()};
  common::JointState joint_state{common::initialJointState()};
  // clang-format off
  common::MotionPlan motion_plan{
      .profile = common::defaultMotionProfile(),
      .total_duration_ms = 0U,
      .sample_count = 0U,
      .samples = {}};
  robotics::OffsetTargetPose offset_target_pose{
      .x_mm =  0.0F,
      .y_mm =  0.0F,
      .z_mm =  0.0F,
      .p_deg = 0.0F,
      .r_deg = 0.0F,
      .g_pct = 0.0F
  };
  // clang-format on
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
