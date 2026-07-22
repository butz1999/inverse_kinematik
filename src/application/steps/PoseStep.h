// Task-space motion step data.

#pragma once

#include <string>

#include "common/MotionProfile.h"
#include "common/TargetPose.h"

namespace application::steps
{

struct PoseStep
{
  common::TargetPose target_pose;
  common::MotionProfile motion_profile;
  std::string name;
};

}  // namespace application::steps
