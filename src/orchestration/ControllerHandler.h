// Handles continuous controller commands independently of transport and hardware.

#pragma once

#include <cstdint>

#include "common/JointState.h"
#include "orchestration/JogCommand.h"
#include "orchestration/MotionOrchestrator.h"

namespace orchestration
{

enum class ControllerHandlerStatus
{
  Inactive,
  Updated,
  InvalidTargetPose,
  KinematicsFailure,
  JointLimitViolation,
};

struct ControllerHandlerResult
{
  bool active{false};
  bool changed{false};
  ControllerHandlerStatus status{ControllerHandlerStatus::Inactive};
  common::JointState joint_state{common::initialJointState()};
};

struct ControllerHandlerState
{
  bool world_roll_lock_enabled{false};
  float locked_world_roll_deg{0.0F};
};

class ControllerHandler
{
 public:
  explicit ControllerHandler(const MotionOrchestrator &motion_orchestrator);

  ControllerHandlerResult update(const JogCommand &command, const common::JointState &current_joint_state,
                                 uint32_t elapsed_ms);
  ControllerHandlerState state() const;
  void synchronizeJointState(const common::JointState &joint_state);
  void reset();

 private:
  MotionOrchestrator motion_orchestrator_;
  float cartesian_velocity_x_mm_s_{0.0F};
  float cartesian_velocity_y_mm_s_{0.0F};
  float cartesian_velocity_z_mm_s_{0.0F};
  bool target_pose_initialized_{false};
  common::TargetPose target_pose_{common::initialTargetPose()};
  common::JointState slew_velocity_deg_s_{common::initialJointState()};
  bool world_roll_lock_enabled_{false};
  bool previous_toggle_pressed_{false};
  float locked_world_roll_deg_{0.0F};
};

const char *toString(ControllerHandlerStatus status);

}  // namespace orchestration
