// Maps analog controller sticks to accelerated Cartesian pose increments.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "application/ControllerInput.h"
#include "common/TargetPose.h"

namespace application
{

struct ControllerCartesianJogConfig
{
  int16_t deadzone;
  int16_t maximum_input;
  float maximum_velocity_mm_s;
  float maximum_acceleration_mm_s2;
};

struct ControllerCartesianJogState
{
  float velocity_x_mm_s;
  float velocity_y_mm_s;
  float velocity_z_mm_s;
};

struct ControllerCartesianJogResult
{
  bool active;
  bool changed;
  common::TargetPose target_pose;
};

inline constexpr ControllerCartesianJogConfig kDefaultControllerCartesianJogConfig{
    250,     // Dead-zone Analog Stick
    2047,    // Maximum input Analog Stick
    120.0F,   // Move-Speed
    180.0F,  // Move-Velocity
};

inline ControllerCartesianJogState emptyControllerCartesianJogState()
{
  return ControllerCartesianJogState{0.0F, 0.0F, 0.0F};
}

inline void resetControllerCartesianJog(ControllerCartesianJogState &state)
{
  state = emptyControllerCartesianJogState();
}

inline float normalizedControllerStickAxis(int16_t value, const ControllerCartesianJogConfig &config)
{
  const auto magnitude = std::abs(static_cast<int32_t>(value));
  if (magnitude <= config.deadzone)
  {
    return 0.0F;
  }

  const auto range = static_cast<float>(config.maximum_input - config.deadzone);
  const auto normalized = std::min(1.0F, static_cast<float>(magnitude - config.deadzone) / range);
  return value < 0 ? -normalized : normalized;
}

inline float approachControllerVelocity(float current_velocity, float target_velocity, float maximum_delta)
{
  if (current_velocity < target_velocity)
  {
    return std::min(target_velocity, current_velocity + maximum_delta);
  }
  return std::max(target_velocity, current_velocity - maximum_delta);
}

inline ControllerCartesianJogResult applyControllerCartesianJog(const ControllerInput &input,
                                                                 const common::TargetPose &current_pose,
                                                                 uint32_t elapsed_ms,
                                                                 ControllerCartesianJogState &state,
                                                                 const ControllerCartesianJogConfig &config =
                                                                     kDefaultControllerCartesianJogConfig)
{
  if (!input.valid || elapsed_ms == 0U)
  {
    return ControllerCartesianJogResult{false, false, current_pose};
  }

  const auto target_velocity_x = normalizedControllerStickAxis(input.left_x, config) * config.maximum_velocity_mm_s;
  const auto target_velocity_y = normalizedControllerStickAxis(input.left_y, config) * config.maximum_velocity_mm_s;
  const auto target_velocity_z = normalizedControllerStickAxis(input.right_y, config) * config.maximum_velocity_mm_s;
  const auto elapsed_seconds = static_cast<float>(elapsed_ms) / 1000.0F;
  const auto maximum_velocity_delta = config.maximum_acceleration_mm_s2 * elapsed_seconds;

  state.velocity_x_mm_s = approachControllerVelocity(state.velocity_x_mm_s, target_velocity_x, maximum_velocity_delta);
  state.velocity_y_mm_s = approachControllerVelocity(state.velocity_y_mm_s, target_velocity_y, maximum_velocity_delta);
  state.velocity_z_mm_s = approachControllerVelocity(state.velocity_z_mm_s, target_velocity_z, maximum_velocity_delta);

  auto target_pose = current_pose;
  target_pose.x_mm += state.velocity_x_mm_s * elapsed_seconds;
  target_pose.y_mm += state.velocity_y_mm_s * elapsed_seconds;
  target_pose.z_mm += state.velocity_z_mm_s * elapsed_seconds;

  const auto active = target_velocity_x != 0.0F || target_velocity_y != 0.0F || target_velocity_z != 0.0F ||
                      state.velocity_x_mm_s != 0.0F || state.velocity_y_mm_s != 0.0F || state.velocity_z_mm_s != 0.0F;
  const auto changed = target_pose.x_mm != current_pose.x_mm || target_pose.y_mm != current_pose.y_mm ||
                       target_pose.z_mm != current_pose.z_mm;
  return ControllerCartesianJogResult{active, changed, target_pose};
}

}  // namespace application
