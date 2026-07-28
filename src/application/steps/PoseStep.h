// Task-space motion step data.

#pragma once

#include "common/FixedString.h"
#include "common/MotionProfile.h"
#include "common/TargetPose.h"

namespace application::steps
{

struct PoseStep
{
  common::TargetPose target_pose;
  common::MotionProfile motion_profile;
  common::FixedString<64U> name;
};

}  // namespace application::steps
