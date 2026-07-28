// Sequenced application-level execution of task-space motion steps.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "application/steps/SequenceStep.h"
#include "common/MotionPlan.h"
#include "common/MotionProfile.h"
#include "common/TargetPose.h"
#include "orchestration/MotionOrchestrator.h"

namespace application
{

constexpr std::size_t kMaxSequenceSteps = 16U;

struct SequenceDefinition
{
  std::size_t step_count;
  std::array<steps::SequenceStep, kMaxSequenceSteps> steps;
};

enum class SequenceRunStatus
{
  Idle,
  Planning,
  MotionActive,
  Waiting,
  Completed,
  Failed,
  Stopped,
};

struct SequenceState
{
  SequenceRunStatus status;
  std::size_t step_index;
  std::size_t step_count;
  uint32_t wait_until_ms;
  const char *message{""};
  orchestration::MotionStatus last_motion_status;
};

struct RunEngineServiceResult
{
  bool has_motion_plan;
  bool has_led_step;
  const common::MotionPlan *motion_plan;
  common::JointState target_joint_state;
  steps::LedStep led_step;
  const orchestration::MotionResult *motion_result;
};

class RunEngine
{
 public:
  explicit RunEngine(const orchestration::MotionOrchestrator &orchestrator);

  RunEngineServiceResult start(const SequenceDefinition &definition, const common::JointState &current_joint_state,
                               uint32_t now_ms);
  RunEngineServiceResult service(const common::JointState &current_joint_state, bool motion_plan_active,
                                 uint32_t now_ms);
  void stop();

  const SequenceState &state() const;
  bool isActive() const;

 private:
  RunEngineServiceResult emptyServiceResult() const;
  RunEngineServiceResult planCurrentStep(const common::JointState &current_joint_state);
  RunEngineServiceResult processCurrentStep(const common::JointState &current_joint_state, uint32_t now_ms);
  void advanceToNextStep();

  const orchestration::MotionOrchestrator &orchestrator_;
  SequenceDefinition definition_;
  SequenceState state_;
  orchestration::MotionResult motion_result_;
};

const char *toString(SequenceRunStatus status);
SequenceDefinition emptySequenceDefinition();

}  // namespace application
