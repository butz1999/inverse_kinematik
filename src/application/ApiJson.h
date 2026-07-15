// JSON parsing helpers for REST request payloads.

#pragma once

#include "application/ApiContracts.h"
#include "common/JointPwmState.h"
#include "common/JointState.h"

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

JointMotionParseResult parseJointMotionRequestJson(const char *body);
JointPwmMotionParseResult parseJointPwmMotionRequestJson(const char *body);

}  // namespace application
