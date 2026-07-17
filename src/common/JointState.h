// Joint-space data model and limits shared by application and later robotics
// code.

#pragma once

#include <cmath>
#include <cstddef>

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
  const char *field_name;
  float min_value;
  float max_value;
};

// clang-format off
constexpr JointLimit kJointLimits[] = {
    {"d_deg",   -90.0F,  90.0F},
    {"s_deg",   -90.0F,  90.0F},
    {"e_deg",   -90.0F,  90.0F},
    {"hp_deg",  -90.0F,   0.0F},  
    {"hr_deg",  -90.0F,  90.0F}, 
    {"g_pct",     0.0F, 100.0F},
};
// clang-format on

constexpr std::size_t kJointAxisCount = sizeof(kJointLimits) / sizeof(kJointLimits[0]);

inline JointState initialJointState()
{
  return JointState{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
}

inline bool isFinite(const JointState &state)
{
  return std::isfinite(state.d_deg) && std::isfinite(state.s_deg) && std::isfinite(state.e_deg) &&
         std::isfinite(state.hp_deg) && std::isfinite(state.hr_deg) && std::isfinite(state.g_pct);
}

inline float valueForLimit(const JointState &state, std::size_t index)
{
  switch (index)
  {
    case 0:
      return state.d_deg;
    case 1:
      return state.s_deg;
    case 2:
      return state.e_deg;
    case 3:
      return state.hp_deg;
    case 4:
      return state.hr_deg;
    default:
      return state.g_pct;
  }
}

inline const JointLimit *findFirstLimitViolation(const JointState &state)
{
  if (!isFinite(state))
  {
    return nullptr;
  }

  for (std::size_t i = 0; i < kJointAxisCount; ++i)
  {
    const auto value = valueForLimit(state, i);
    const auto &limit = kJointLimits[i];
    if (value < limit.min_value || value > limit.max_value)
    {
      return &limit;
    }
  }

  return nullptr;
}

inline bool isWithinJointLimits(const JointState &state)
{
  return isFinite(state) && findFirstLimitViolation(state) == nullptr;
}

}  // namespace common
