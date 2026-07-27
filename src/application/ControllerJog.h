// Maps digital controller input to continuous joint-space jogging.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "application/ControllerInput.h"
#include "common/JointState.h"

namespace application
{

enum class ControllerJogSource
{
  // Dpad
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  // ABXY
  ButtonA,
  ButtonB,
  ButtonX,
  ButtonY,
  // Links
  ButtonL,
  ButtonZL,
  ButtonGripL,
  // Rechts
  ButtonR,
  ButtonZR,
  ButtonGripR,
  // + und -
  ButtonPlus,
  ButtonMinus,
  // Analog Sticks
  ButtonLeftStick,
  ButtonRightStick,
  // Andere
  ButtonHome,
  ButtonCapture,
  ButtonCamera,
};

enum class ControllerJogAxis : std::size_t
{
  D = 0U,
  S = 1U,
  E = 2U,
  Hp = 3U,
  Hr = 4U,
  G = 5U,
};

struct ControllerJogMapping
{
  ControllerJogSource source;
  ControllerJogAxis axis;
  float direction;
  float velocity_per_second;
};

struct ControllerJogResult
{
  bool active;
  bool changed;
  common::JointState joint_state;
};

// clang-format off
inline constexpr ControllerJogMapping kDefaultControllerJogMappings[] = {
    {ControllerJogSource::ButtonGripL, ControllerJogAxis::D,  -2.0F, 20.0F},
    {ControllerJogSource::ButtonGripR, ControllerJogAxis::D,   2.0F, 20.0F},
    {ControllerJogSource::DpadUp,      ControllerJogAxis::S,   2.0F, 20.0F},
    {ControllerJogSource::DpadDown,    ControllerJogAxis::S,  -2.0F, 20.0F},
    {ControllerJogSource::DpadLeft,    ControllerJogAxis::E,  -2.0F, 20.0F},
    {ControllerJogSource::DpadRight,   ControllerJogAxis::E,   2.0F, 20.0F},
    {ControllerJogSource::ButtonB,     ControllerJogAxis::Hp, -2.0F, 20.0F},
    {ControllerJogSource::ButtonX,     ControllerJogAxis::Hp,  2.0F, 20.0F},
    {ControllerJogSource::ButtonY,     ControllerJogAxis::Hr, -2.0F, 20.0F},
    {ControllerJogSource::ButtonA,     ControllerJogAxis::Hr,  2.0F, 20.0F},
    {ControllerJogSource::ButtonL,     ControllerJogAxis::G,  -2.0F, 20.0F},
    {ControllerJogSource::ButtonR,     ControllerJogAxis::G,   2.0F, 20.0F},
};
// clang-format on

inline bool isControllerJogSourceActive(const ControllerInput &input, ControllerJogSource source)
{
  if (!input.valid)
  {
    return false;
  }

  switch (source)
  {
    case ControllerJogSource::ButtonB:
      return (input.buttons & kControllerButtonB) != 0U;
    case ControllerJogSource::ButtonA:
      return (input.buttons & kControllerButtonA) != 0U;
    case ControllerJogSource::ButtonY:
      return (input.buttons & kControllerButtonY) != 0U;
    case ControllerJogSource::ButtonX:
      return (input.buttons & kControllerButtonX) != 0U;
    case ControllerJogSource::ButtonR:
      return (input.buttons & kControllerButtonR) != 0U;
    case ControllerJogSource::ButtonZR:
      return (input.buttons & kControllerButtonZR) != 0U;
    case ControllerJogSource::ButtonPlus:
      return (input.buttons & kControllerButtonPlus) != 0U;
    case ControllerJogSource::ButtonRightStick:
      return (input.buttons & kControllerButtonRightStick) != 0U;
    case ControllerJogSource::ButtonL:
      return (input.buttons & kControllerButtonL) != 0U;
    case ControllerJogSource::ButtonZL:
      return (input.buttons & kControllerButtonZL) != 0U;
    case ControllerJogSource::ButtonMinus:
      return (input.buttons & kControllerButtonMinus) != 0U;
    case ControllerJogSource::ButtonLeftStick:
      return (input.buttons & kControllerButtonLeftStick) != 0U;
    case ControllerJogSource::ButtonHome:
      return (input.buttons & kControllerButtonHome) != 0U;
    case ControllerJogSource::ButtonCapture:
      return (input.buttons & kControllerButtonCapture) != 0U;
    case ControllerJogSource::ButtonGripR:
      return (input.buttons & kControllerButtonGripR) != 0U;
    case ControllerJogSource::ButtonGripL:
      return (input.buttons & kControllerButtonGripL) != 0U;
    case ControllerJogSource::ButtonCamera:
      return (input.buttons & kControllerButtonCamera) != 0U;
    case ControllerJogSource::DpadUp:
      return (input.dpad & kControllerDpadUp) != 0U;
    case ControllerJogSource::DpadDown:
      return (input.dpad & kControllerDpadDown) != 0U;
    case ControllerJogSource::DpadLeft:
      return (input.dpad & kControllerDpadLeft) != 0U;
    case ControllerJogSource::DpadRight:
      return (input.dpad & kControllerDpadRight) != 0U;
  }

  return false;
}

inline float &jointAxisValue(common::JointState &state, ControllerJogAxis axis)
{
  switch (axis)
  {
    case ControllerJogAxis::D:
      return state.d_deg;
    case ControllerJogAxis::S:
      return state.s_deg;
    case ControllerJogAxis::E:
      return state.e_deg;
    case ControllerJogAxis::Hp:
      return state.hp_deg;
    case ControllerJogAxis::Hr:
      return state.hr_deg;
    case ControllerJogAxis::G:
      return state.g_pct;
  }

  return state.g_pct;
}

inline const common::JointLimit &jointLimitForAxis(ControllerJogAxis axis)
{
  return common::kJointLimits[static_cast<std::size_t>(axis)];
}

inline ControllerJogResult applyControllerJog(const ControllerInput &input, const common::JointState &current_state,
                                              uint32_t elapsed_ms, const ControllerJogMapping *mappings,
                                              std::size_t mapping_count)
{
  auto next_state = current_state;
  bool active = false;

  for (std::size_t index = 0; index < mapping_count; ++index)
  {
    const auto &mapping = mappings[index];
    if (!isControllerJogSourceActive(input, mapping.source))
    {
      continue;
    }

    active = true;
    auto &value = jointAxisValue(next_state, mapping.axis);
    const auto &limit = jointLimitForAxis(mapping.axis);
    const auto delta = mapping.direction * mapping.velocity_per_second * (static_cast<float>(elapsed_ms) / 1000.0F);
    value = std::max(limit.min_value, std::min(limit.max_value, value + delta));
  }

  return ControllerJogResult{active,
                             active && (next_state.d_deg != current_state.d_deg ||
                                        next_state.s_deg != current_state.s_deg ||
                                        next_state.e_deg != current_state.e_deg ||
                                        next_state.hp_deg != current_state.hp_deg ||
                                        next_state.hr_deg != current_state.hr_deg ||
                                        next_state.g_pct != current_state.g_pct),
                             next_state};
}

inline ControllerJogResult applyDefaultControllerJog(const ControllerInput &input,
                                                     const common::JointState &current_state, uint32_t elapsed_ms)
{
  return applyControllerJog(input, current_state, elapsed_ms, kDefaultControllerJogMappings,
                            sizeof(kDefaultControllerJogMappings) / sizeof(kDefaultControllerJogMappings[0]));
}

}  // namespace application
