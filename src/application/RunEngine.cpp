#include "application/RunEngine.h"

namespace application
{

namespace
{

SequenceState initialSequenceState()
{
  return SequenceState{SequenceRunStatus::Idle, 0U, 0U, 0U, "idle", orchestration::MotionStatus::Accepted};
}

}  // namespace

RunEngine::RunEngine(const orchestration::MotionOrchestrator &orchestrator)
    : orchestrator_(orchestrator),
      definition_(emptySequenceDefinition()),
      state_(initialSequenceState()),
      motion_result_()
{
}

RunEngineServiceResult RunEngine::start(const SequenceDefinition &definition,
                                        const common::JointState &current_joint_state, uint32_t now_ms)
{
  definition_ = definition;
  state_ = SequenceState{SequenceRunStatus::Planning,          0U, definition.step_count, now_ms, "planning",
                         orchestration::MotionStatus::Accepted};

  if (definition_.step_count == 0U)
  {
    state_.status = SequenceRunStatus::Completed;
    state_.message = "completed";
    return emptyServiceResult();
  }

  return processCurrentStep(current_joint_state, now_ms);
}

RunEngineServiceResult RunEngine::service(const common::JointState &current_joint_state, bool motion_plan_active,
                                          uint32_t now_ms)
{
  if (state_.status == SequenceRunStatus::MotionActive)
  {
    if (motion_plan_active)
    {
      return emptyServiceResult();
    }

    advanceToNextStep();
  }

  if (state_.status == SequenceRunStatus::Waiting)
  {
    if (static_cast<int32_t>(now_ms - state_.wait_until_ms) < 0)
    {
      return emptyServiceResult();
    }

    advanceToNextStep();
  }

  if (state_.status == SequenceRunStatus::Planning)
  {
    return processCurrentStep(current_joint_state, now_ms);
  }

  return emptyServiceResult();
}

void RunEngine::stop()
{
  state_.status = SequenceRunStatus::Stopped;
  state_.message = "stopped";
}

const SequenceState &RunEngine::state() const
{
  return state_;
}

bool RunEngine::isActive() const
{
  return state_.status == SequenceRunStatus::Planning || state_.status == SequenceRunStatus::MotionActive ||
         state_.status == SequenceRunStatus::Waiting;
}

RunEngineServiceResult RunEngine::emptyServiceResult() const
{
  return RunEngineServiceResult{false, false, nullptr, common::initialJointState(), steps::emptyLedStep(), nullptr};
}

RunEngineServiceResult RunEngine::planCurrentStep(const common::JointState &current_joint_state)
{
  state_.status = SequenceRunStatus::Planning;
  state_.message = "planning";

  const auto &step = definition_.steps[state_.step_index];
  const orchestration::MotionRequest request{step.pose.target_pose, step.pose.motion_profile};
  orchestrator_.processMotionRequestInto(request, current_joint_state, motion_result_);
  state_.last_motion_status = motion_result_.status;

  if (!motion_result_.ok)
  {
    state_.status = SequenceRunStatus::Failed;
    state_.message = motion_result_.message != nullptr ? motion_result_.message : "motion planning failed";
    return RunEngineServiceResult{false,          false, nullptr, common::initialJointState(), steps::emptyLedStep(),
                                  &motion_result_};
  }

  state_.status = SequenceRunStatus::MotionActive;
  state_.message = "motion_active";
  return RunEngineServiceResult{
      true, false, &motion_result_.motion_plan, motion_result_.joint_state, steps::emptyLedStep(), &motion_result_};
}

RunEngineServiceResult RunEngine::processCurrentStep(const common::JointState &current_joint_state, uint32_t now_ms)
{
  while (state_.status == SequenceRunStatus::Planning)
  {
    const auto &step = definition_.steps[state_.step_index];
    switch (step.type)
    {
      case steps::StepType::Pose:
        return planCurrentStep(current_joint_state);
      case steps::StepType::Wait:
        if (step.wait.duration_ms > 0U)
        {
          state_.status = SequenceRunStatus::Waiting;
          state_.wait_until_ms = now_ms + step.wait.duration_ms;
          state_.message = "waiting";
          return emptyServiceResult();
        }
        advanceToNextStep();
        break;
      case steps::StepType::Led:
      {
        const auto led_step = step.led;
        advanceToNextStep();
        return RunEngineServiceResult{false, true, nullptr, common::initialJointState(), led_step, nullptr};
      }
    }
  }

  return emptyServiceResult();
}

void RunEngine::advanceToNextStep()
{
  ++state_.step_index;
  if (state_.step_index >= definition_.step_count)
  {
    state_.status = SequenceRunStatus::Completed;
    state_.message = "completed";
    return;
  }

  state_.status = SequenceRunStatus::Planning;
  state_.message = "planning";
}

const char *toString(SequenceRunStatus status)
{
  switch (status)
  {
    case SequenceRunStatus::Idle:
      return "idle";
    case SequenceRunStatus::Planning:
      return "planning";
    case SequenceRunStatus::MotionActive:
      return "motion_active";
    case SequenceRunStatus::Waiting:
      return "waiting";
    case SequenceRunStatus::Completed:
      return "completed";
    case SequenceRunStatus::Failed:
      return "failed";
    case SequenceRunStatus::Stopped:
      return "stopped";
  }

  return "failed";
}

SequenceDefinition emptySequenceDefinition()
{
  return SequenceDefinition{0U, {}};
}

}  // namespace application
