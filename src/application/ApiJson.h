// JSON parsing helpers for REST request payloads.

#pragma once

#include "application/ApiContracts.h"
#include "application/RunEngine.h"
#include "common/JointPwmState.h"
#include "common/JointState.h"
#include "common/MotionProfile.h"
#include "common/TargetPose.h"

namespace application
{

struct JointMotionParseResult
{
  bool ok;
  ApiResultCode code;
  const char *field_name;
  const char *message;
  common::JointState joint_state;
};

struct JointPwmMotionParseResult
{
  bool ok;
  ApiResultCode code;
  const char *field_name;
  const char *message;
  common::JointPwmState joint_pwm_state;
};

struct TargetPoseParseResult
{
  bool ok;
  ApiResultCode code;
  const char *field_name;
  const char *message;
  common::TargetPose target_pose;
  common::MotionProfile motion_profile;
};

struct SequenceDefinitionParseResult
{
  bool ok;
  ApiResultCode code;
  const char *field_name;
  const char *message;
};

JointMotionParseResult parseJointMotionRequestJson(const char *body);
JointPwmMotionParseResult parseJointPwmMotionRequestJson(const char *body);
TargetPoseParseResult parseTargetPoseRequestJson(const char *body);
SequenceDefinitionParseResult parseSequenceDefinitionRequestJson(const char *body, SequenceDefinition &sequence);

}  // namespace application
