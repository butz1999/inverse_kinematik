// Joint-space data model and limits shared by application and later robotics
// code.

#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

#include "common/JointAxis.h"

namespace common
{

struct JointState
{
  float d_deg;
  float s_deg;
  float e_deg;
  float hp_deg;
  float hr_deg;
  float g_pct;
};

struct JointLimit
{
  float min_value;
  float max_value;
};

using JointLimits = std::array<JointLimit, kJointAxisCount>;

inline JointState initialJointState()
{
  return JointState{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
}

inline bool isFinite(const JointState &state)
{
  return std::isfinite(state.d_deg) && std::isfinite(state.s_deg) && std::isfinite(state.e_deg) &&
         std::isfinite(state.hp_deg) && std::isfinite(state.hr_deg) && std::isfinite(state.g_pct);
}

inline float &jointAxisValue(JointState &state, JointAxis axis)
{
  switch (axis)
  {
    case JointAxis::D:
      return state.d_deg;
    case JointAxis::S:
      return state.s_deg;
    case JointAxis::E:
      return state.e_deg;
    case JointAxis::Hp:
      return state.hp_deg;
    case JointAxis::Hr:
      return state.hr_deg;
    case JointAxis::G:
      return state.g_pct;
    case JointAxis::Count:
      return state.g_pct;
  }

  return state.g_pct;
}

inline float jointAxisValue(const JointState &state, JointAxis axis)
{
  switch (axis)
  {
    case JointAxis::D:
      return state.d_deg;
    case JointAxis::S:
      return state.s_deg;
    case JointAxis::E:
      return state.e_deg;
    case JointAxis::Hp:
      return state.hp_deg;
    case JointAxis::Hr:
      return state.hr_deg;
    case JointAxis::G:
    case JointAxis::Count:
      return state.g_pct;
  }

  return state.g_pct;
}

inline const JointLimit &jointLimitForAxis(const JointLimits &limits, JointAxis axis)
{
  return limits[jointAxisIndex(axis)];
}

inline std::optional<JointAxis> findFirstLimitViolation(const JointState &state, const JointLimits &limits)
{
  if (!isFinite(state))
  {
    return std::nullopt;
  }

  for (const auto axis : kJointAxes)
  {
    const auto value = jointAxisValue(state, axis);
    const auto &limit = jointLimitForAxis(limits, axis);
    if (value < limit.min_value || value > limit.max_value)
    {
      return axis;
    }
  }

  return std::nullopt;
}

inline bool isWithinJointLimits(const JointState &state, const JointLimits &limits)
{
  return isFinite(state) && !findFirstLimitViolation(state, limits).has_value();
}

}  // namespace common
