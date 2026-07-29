// Limits Cartesian controller output in joint space to avoid abrupt servo commands.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "common/JointState.h"

namespace application
{

struct ControllerJointSlewLimiterConfig
{
  float maximum_velocity_deg_s;
  float maximum_acceleration_deg_s2;
};

struct ControllerJointSlewLimiterState
{
  common::JointState velocity_deg_s;
};

struct ControllerJointSlewLimiterResult
{
  bool active;
  bool changed;
  common::JointState joint_state;
};

inline constexpr ControllerJointSlewLimiterConfig kDefaultControllerJointSlewLimiterConfig{
    .maximum_velocity_deg_s = 180.0F,
    .maximum_acceleration_deg_s2 = 360.0F,
};

inline ControllerJointSlewLimiterState emptyControllerJointSlewLimiterState()
{
  return ControllerJointSlewLimiterState{common::initialJointState()};
}

inline void resetControllerJointSlewLimiter(ControllerJointSlewLimiterState &state)
{
  state = emptyControllerJointSlewLimiterState();
}

inline float limitControllerJointAxis(float current, float target, float &velocity_deg_s, float elapsed_seconds,
                                      const ControllerJointSlewLimiterConfig &config)
{
  const auto remaining = target - current;
  const auto maximum_braking_velocity = std::sqrt(2.0F * config.maximum_acceleration_deg_s2 * std::fabs(remaining));
  const auto target_velocity =
      remaining == 0.0F ? 0.0F
                        : std::copysign(std::min(config.maximum_velocity_deg_s, maximum_braking_velocity), remaining);
  const auto maximum_velocity_delta = config.maximum_acceleration_deg_s2 * elapsed_seconds;
  if (velocity_deg_s < target_velocity)
  {
    velocity_deg_s = std::min(target_velocity, velocity_deg_s + maximum_velocity_delta);
  }
  else
  {
    velocity_deg_s = std::max(target_velocity, velocity_deg_s - maximum_velocity_delta);
  }

  const auto delta = velocity_deg_s * elapsed_seconds;
  if (std::fabs(delta) >= std::fabs(remaining))
  {
    velocity_deg_s = 0.0F;
    return target;
  }

  return current + delta;
}

inline ControllerJointSlewLimiterResult applyControllerJointSlewLimiter(
    const common::JointState &current_joint_state, const common::JointState &target_joint_state, uint32_t elapsed_ms,
    ControllerJointSlewLimiterState &state,
    const ControllerJointSlewLimiterConfig &config = kDefaultControllerJointSlewLimiterConfig)
{
  if (elapsed_ms == 0U)
  {
    return ControllerJointSlewLimiterResult{false, false, current_joint_state};
  }

  const auto elapsed_seconds = static_cast<float>(elapsed_ms) / 1000.0F;
  auto limited = current_joint_state;
  limited.d_deg = limitControllerJointAxis(current_joint_state.d_deg, target_joint_state.d_deg,
                                           state.velocity_deg_s.d_deg, elapsed_seconds, config);
  limited.s_deg = limitControllerJointAxis(current_joint_state.s_deg, target_joint_state.s_deg,
                                           state.velocity_deg_s.s_deg, elapsed_seconds, config);
  limited.e_deg = limitControllerJointAxis(current_joint_state.e_deg, target_joint_state.e_deg,
                                           state.velocity_deg_s.e_deg, elapsed_seconds, config);
  limited.hp_deg = limitControllerJointAxis(current_joint_state.hp_deg, target_joint_state.hp_deg,
                                            state.velocity_deg_s.hp_deg, elapsed_seconds, config);
  limited.hr_deg = limitControllerJointAxis(current_joint_state.hr_deg, target_joint_state.hr_deg,
                                            state.velocity_deg_s.hr_deg, elapsed_seconds, config);
  limited.g_pct = target_joint_state.g_pct;

  const auto changed = limited.d_deg != current_joint_state.d_deg || limited.s_deg != current_joint_state.s_deg ||
                       limited.e_deg != current_joint_state.e_deg || limited.hp_deg != current_joint_state.hp_deg ||
                       limited.hr_deg != current_joint_state.hr_deg || limited.g_pct != current_joint_state.g_pct;
  const auto active = changed || state.velocity_deg_s.d_deg != 0.0F || state.velocity_deg_s.s_deg != 0.0F ||
                      state.velocity_deg_s.e_deg != 0.0F || state.velocity_deg_s.hp_deg != 0.0F ||
                      state.velocity_deg_s.hr_deg != 0.0F;
  return ControllerJointSlewLimiterResult{active, changed, limited};
}

}  // namespace application
