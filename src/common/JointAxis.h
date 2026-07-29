// Canonical joint-axis identities and their public field names.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace common
{

enum class JointAxis : uint8_t
{
  D,
  S,
  E,
  Hp,
  Hr,
  G,
  Count,
};

constexpr std::size_t kJointAxisCount = static_cast<std::size_t>(JointAxis::Count);

constexpr std::size_t jointAxisIndex(JointAxis axis)
{
  return static_cast<std::size_t>(axis);
}

constexpr const char *jointAxisFieldName(JointAxis axis)
{
  switch (axis)
  {
    case JointAxis::D:
      return "d_deg";
    case JointAxis::S:
      return "s_deg";
    case JointAxis::E:
      return "e_deg";
    case JointAxis::Hp:
      return "hp_deg";
    case JointAxis::Hr:
      return "hr_deg";
    case JointAxis::G:
      return "g_pct";
    case JointAxis::Count:
      return "";
  }

  return "";
}

constexpr const char *jointAxisPwmFieldName(JointAxis axis)
{
  switch (axis)
  {
    case JointAxis::D:
      return "d_pwm";
    case JointAxis::S:
      return "s_pwm";
    case JointAxis::E:
      return "e_pwm";
    case JointAxis::Hp:
      return "hp_pwm";
    case JointAxis::Hr:
      return "hr_pwm";
    case JointAxis::G:
      return "g_pwm";
    case JointAxis::Count:
      return "";
  }

  return "";
}

inline constexpr std::array<JointAxis, kJointAxisCount> kJointAxes{JointAxis::D,  JointAxis::S,  JointAxis::E,
                                                                   JointAxis::Hp, JointAxis::Hr, JointAxis::G};

}  // namespace common
