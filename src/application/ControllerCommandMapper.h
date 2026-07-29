// Translates the physical controller layout into device-neutral jog commands.

#pragma once

#include <algorithm>
#include <cmath>

#include "application/ControllerInput.h"
#include "orchestration/JogCommand.h"

namespace application
{

inline float normalizeControllerAxis(int16_t value)
{
  constexpr int16_t kDeadzone = 250;
  constexpr int16_t kMaximumInput = 2047;
  const auto magnitude = std::abs(static_cast<int32_t>(value));
  if (magnitude <= kDeadzone)
  {
    return 0.0F;
  }
  const auto normalized =
      std::min(1.0F, static_cast<float>(magnitude - kDeadzone) / static_cast<float>(kMaximumInput - kDeadzone));
  return value < 0 ? -normalized : normalized;
}

inline orchestration::JogCommand mapControllerInputToJogCommand(const ControllerInput &input)
{
  orchestration::JogCommand command;
  command.valid = input.valid;
  if (!input.valid)
  {
    return command;
  }

  command.x_input = normalizeControllerAxis(input.left_x);
  command.y_input = normalizeControllerAxis(input.left_y);
  command.z_input = normalizeControllerAxis(input.right_y);
  command.world_roll_toggle_pressed = (input.buttons & kControllerButtonRightStick) != 0U;

  constexpr float kJointJogVelocityPerSecond = 31.25F;
  command.joint_velocity_per_second.d_deg =
      ((input.buttons & kControllerButtonGripR) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.buttons & kControllerButtonGripL) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  command.joint_velocity_per_second.s_deg =
      ((input.dpad & kControllerDpadUp) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.dpad & kControllerDpadDown) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  command.joint_velocity_per_second.e_deg =
      ((input.dpad & kControllerDpadRight) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.dpad & kControllerDpadLeft) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  command.joint_velocity_per_second.hp_deg =
      ((input.buttons & kControllerButtonX) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.buttons & kControllerButtonB) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  command.joint_velocity_per_second.hr_deg =
      ((input.buttons & kControllerButtonA) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.buttons & kControllerButtonY) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  command.joint_velocity_per_second.g_pct =
      ((input.buttons & kControllerButtonR) != 0U ? kJointJogVelocityPerSecond : 0.0F) -
      ((input.buttons & kControllerButtonL) != 0U ? kJointJogVelocityPerSecond : 0.0F);
  return command;
}

}  // namespace application
