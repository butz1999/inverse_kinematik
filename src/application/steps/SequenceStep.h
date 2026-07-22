// Tagged sequence step container without polymorphic base class.

#pragma once

#include "application/steps/LedStep.h"
#include "application/steps/PoseStep.h"
#include "application/steps/WaitStep.h"

namespace application::steps
{

enum class StepType
{
  Pose,
  Wait,
  Led,
};

struct SequenceStep
{
  StepType type;
  PoseStep pose;
  WaitStep wait;
  LedStep led;
};

inline SequenceStep poseSequenceStep(const PoseStep &step)
{
  return SequenceStep{StepType::Pose, step, WaitStep{0U}, emptyLedStep()};
}

inline SequenceStep waitSequenceStep(const WaitStep &step)
{
  return SequenceStep{StepType::Wait, PoseStep{}, step, emptyLedStep()};
}

inline SequenceStep ledSequenceStep(const LedStep &step)
{
  return SequenceStep{StepType::Led, PoseStep{}, WaitStep{0U}, step};
}

inline const char *toString(StepType type)
{
  switch (type)
  {
    case StepType::Pose:
      return "pose";
    case StepType::Wait:
      return "wait";
    case StepType::Led:
      return "led";
  }

  return "pose";
}

}  // namespace application::steps
