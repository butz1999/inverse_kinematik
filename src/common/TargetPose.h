// Task-space target pose shared by application, orchestration and robotics.

#pragma once

#include <cmath>

namespace common
{

struct TargetPose
{
  float x_mm;
  float y_mm;
  float z_mm;
  float p_deg;
  float r_deg;
  float g_pct;
};

constexpr float kMinTargetGripperPct = 0.0F;
constexpr float kMaxTargetGripperPct = 100.0F;

inline TargetPose initialTargetPose()
{
  return TargetPose{-20.0F, 50.0F, 30.0F, -90.0F, 0.0F, 0.0F};
}

inline bool isFinite(const TargetPose &pose)
{
  // clang-format off
  return 
    std::isfinite(pose.x_mm) &&
    std::isfinite(pose.y_mm) &&
    std::isfinite(pose.z_mm) &&
    std::isfinite(pose.p_deg) &&
    std::isfinite(pose.r_deg) &&
    std::isfinite(pose.g_pct);
  // clang-format on
}

inline bool isWithinTargetGripperLimits(const TargetPose &pose)
{
  return pose.g_pct >= kMinTargetGripperPct && pose.g_pct <= kMaxTargetGripperPct;
}

}  // namespace common
