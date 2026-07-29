// Maintains a world-referenced tool roll while the tool points nearly downward.

#pragma once

#include <cmath>

namespace application
{

struct ControllerWorldRollLockState
{
  bool enabled;
  bool previous_toggle_pressed;
  float locked_world_roll_deg;
};

enum class ControllerWorldRollLockUpdate
{
  Unchanged,
  Enabled,
  Disabled,
  Rejected,
};

inline constexpr float kControllerWorldRollLockPitchDeg = -90.0F;
inline constexpr float kControllerWorldRollLockPitchToleranceDeg = 20.0F;

inline ControllerWorldRollLockState emptyControllerWorldRollLockState()
{
  return ControllerWorldRollLockState{false, false, 0.0F};
}

inline void resetControllerWorldRollLock(ControllerWorldRollLockState &state)
{
  state = emptyControllerWorldRollLockState();
}

inline bool isControllerWorldRollLockEligible(float pitch_deg)
{
  return std::fabs(pitch_deg - kControllerWorldRollLockPitchDeg) <= kControllerWorldRollLockPitchToleranceDeg;
}

inline ControllerWorldRollLockUpdate updateControllerWorldRollLock(bool toggle_pressed, float pitch_deg,
                                                                   float turntable_deg, float hand_roll_deg,
                                                                   ControllerWorldRollLockState &state)
{
  const auto toggle_edge = toggle_pressed && !state.previous_toggle_pressed;
  state.previous_toggle_pressed = toggle_pressed;

  if (state.enabled && !isControllerWorldRollLockEligible(pitch_deg))
  {
    state.enabled = false;
    return ControllerWorldRollLockUpdate::Disabled;
  }
  if (!toggle_edge)
  {
    return ControllerWorldRollLockUpdate::Unchanged;
  }
  if (state.enabled)
  {
    state.enabled = false;
    return ControllerWorldRollLockUpdate::Disabled;
  }
  if (!isControllerWorldRollLockEligible(pitch_deg))
  {
    return ControllerWorldRollLockUpdate::Rejected;
  }

  state.enabled = true;
  state.locked_world_roll_deg = turntable_deg + hand_roll_deg;
  return ControllerWorldRollLockUpdate::Enabled;
}

inline float handRollForControllerWorldRollLock(const ControllerWorldRollLockState &state, float turntable_deg)
{
  return state.locked_world_roll_deg - turntable_deg;
}

}  // namespace application
