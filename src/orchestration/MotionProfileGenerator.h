// Motion profile generation for planned joint-space transitions.

#pragma once

#include <string>

#include "common/MotionPlan.h"
#include "common/MotionProfile.h"

namespace orchestration
{

enum class MotionProfileGeneratorStatus
{
  Ok,
  InvalidProfile,
  UnsupportedProfile,
  TooManySamples,
};

struct MotionProfileGeneratorResult
{
  bool ok;
  MotionProfileGeneratorStatus status;
  common::MotionPlan plan;
  std::string message;
};

struct MotionProfileGenerationStatus
{
  bool ok;
  MotionProfileGeneratorStatus status;
  std::string message;
};

MotionProfileGenerationStatus generateMotionPlanInto(const common::JointState &start_state,
                                                     const common::JointState &target_state,
                                                     const common::MotionProfile &profile, common::MotionPlan &plan);

MotionProfileGeneratorResult generateMotionPlan(const common::JointState &start_state,
                                                const common::JointState &target_state,
                                                const common::MotionProfile &profile);

const char *toString(MotionProfileGeneratorStatus status);

}  // namespace orchestration
