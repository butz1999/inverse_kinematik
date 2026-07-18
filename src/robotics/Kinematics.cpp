#include "robotics/Kinematics.h"

#include <cmath>

namespace robotics
{

namespace
{

constexpr Vector3 kOrigin{0.0F, 0.0F, 0.0F};

Vector3 segmentVector(float length_mm, float angle_from_radial_deg, float d_deg)
{
  const auto angle_rad = degreesToRadians(angle_from_radial_deg);
  const auto radial_mm = length_mm * std::cos(angle_rad);
  const auto z_mm = length_mm * std::sin(angle_rad);
  return vectorFromRadialZ(radial_mm, z_mm, d_deg);
}

float shoulderAngleFromRadialDeg(float s_deg)
{
  return 90.0F + s_deg;
}

}  // namespace

ForwardKinematicsResult forwardKinematics(const common::JointState &state, const RobotModel &model)
{
  const auto s_angle_deg = shoulderAngleFromRadialDeg(state.s_deg);
  const auto e_angle_deg = s_angle_deg + state.e_deg;
  const auto hp_angle_deg = e_angle_deg + state.hp_deg;

  const auto d_point = kOrigin;
  const auto s_point = d_point;
  const auto e_point = add(s_point, segmentVector(model.segments.s_e_length_mm, s_angle_deg, state.d_deg));
  const auto h_point = add(e_point, segmentVector(model.segments.e_hp_length_mm, e_angle_deg, state.d_deg));
  const auto g_point = add(h_point, segmentVector(model.segments.hr_g_length_mm, hp_angle_deg, state.d_deg));

  return ForwardKinematicsResult{d_point, s_point, e_point, h_point, g_point, hp_angle_deg, state.hr_deg, state.g_pct};
}

}  // namespace robotics
