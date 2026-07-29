// Device-neutral continuous controller command.

#pragma once

#include "common/JointState.h"

namespace orchestration
{

struct JogCommand
{
  bool valid{false};
  float x_input{0.0F};
  float y_input{0.0F};
  float z_input{0.0F};
  common::JointState joint_velocity_per_second{common::initialJointState()};
  bool world_roll_toggle_pressed{false};
};

inline bool hasJointJog(const JogCommand &command)
{
  const auto &velocity = command.joint_velocity_per_second;
  // clang-format off
  return velocity.d_deg != 0.0F ||
         velocity.s_deg != 0.0F ||
         velocity.e_deg != 0.0F ||
         velocity.hp_deg != 0.0F ||
         velocity.hr_deg != 0.0F ||
         velocity.g_pct != 0.0F;
  // clang-format on
}

}  // namespace orchestration
