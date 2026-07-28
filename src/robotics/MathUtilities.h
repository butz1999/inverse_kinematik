// Lightweight math utilities for robotics geometry.

#pragma once

#include <cmath>

namespace robotics
{

constexpr float kPi = 3.14159265358979323846F;

struct Vector3
{
  float x_mm;
  float y_mm;
  float z_mm;
};

struct Orientation3
{
  // Orthonormal tool-frame axes expressed in world coordinates.
  Vector3 side_axis;
  Vector3 forward_axis;
  Vector3 up_axis;
};

inline float degreesToRadians(float degrees)
{
  return degrees * kPi / 180.0F;
}

inline float radiansToDegrees(float radians)
{
  return radians * 180.0F / kPi;
}

inline Vector3 add(const Vector3 &left, const Vector3 &right)
{
  return Vector3{left.x_mm + right.x_mm, left.y_mm + right.y_mm, left.z_mm + right.z_mm};
}

inline Vector3 subtract(const Vector3 &left, const Vector3 &right)
{
  return Vector3{left.x_mm - right.x_mm, left.y_mm - right.y_mm, left.z_mm - right.z_mm};
}

inline Vector3 scale(const Vector3 &vector, float factor)
{
  return Vector3{vector.x_mm * factor, vector.y_mm * factor, vector.z_mm * factor};
}

/**
 * Converts a vector from the vertical arm plane into world coordinates.
 *
 * The arm geometry is first calculated in the plane selected by the turntable:
 * radial_mm is the horizontal distance away from the turntable axis inside that plane,
 * z_mm is the vertical height. At d_deg = 0 the arm plane points along the world's
 * positive y axis. Positive d_deg rotates that plane toward the world's positive x axis.
 *
 * This is why x uses sin(d) and y uses cos(d): the angular zero direction is +y,
 * not +x as in the usual mathematical unit circle.
 *
 * @param radial_mm Distance from the turntable axis in the horizontal plane, in millimeters.
 * @param z_mm Vertical offset in millimeters.
 * @param d_deg Turntable angle in degrees.
 * @return Vector in world coordinates, in millimeters.
 */
inline Vector3 vectorFromRadialZ(float radial_mm, float z_mm, float d_deg)
{
  const auto d_rad = degreesToRadians(d_deg);
  return Vector3{radial_mm * std::sin(d_rad), radial_mm * std::cos(d_rad), z_mm};
}

/**
 * Converts a vector from turntable-local coordinates into world coordinates.
 *
 * At d_deg = 0 the local x axis matches world +x and the local y axis matches
 * world +y. Positive d_deg rotates the local y axis toward world +x, matching
 * the turntable convention used by vectorFromRadialZ().
 *
 * @param local_x_mm Local side offset in millimeters.
 * @param local_y_mm Local forward/radial offset in millimeters.
 * @param local_z_mm Local vertical offset in millimeters.
 * @param d_deg Turntable angle in degrees.
 * @return Rotated vector in world coordinates, in millimeters.
 */
inline Vector3 vectorFromTurntableLocal(float local_x_mm, float local_y_mm, float local_z_mm, float d_deg)
{
  const auto d_rad = degreesToRadians(d_deg);
  return Vector3{(local_x_mm * std::cos(d_rad)) + (local_y_mm * std::sin(d_rad)),
                 (-local_x_mm * std::sin(d_rad)) + (local_y_mm * std::cos(d_rad)), local_z_mm};
}

// Builds the world-space tool frame from turntable yaw, world elevation and axial tool roll.
inline Orientation3 toolOrientationFromWorldAngles(float d_deg, float p_deg, float r_deg)
{
  const auto d_rad = degreesToRadians(d_deg);
  const auto p_rad = degreesToRadians(p_deg);
  const auto r_rad = degreesToRadians(r_deg);

  const Vector3 base_side{std::cos(d_rad), -std::sin(d_rad), 0.0F};
  const Vector3 base_forward{std::sin(d_rad), std::cos(d_rad), 0.0F};
  const Vector3 base_up{0.0F, 0.0F, 1.0F};
  const auto pitched_forward = add(scale(base_forward, std::cos(p_rad)), scale(base_up, std::sin(p_rad)));
  const auto pitched_up = add(scale(base_up, std::cos(p_rad)), scale(base_forward, -std::sin(p_rad)));

  return Orientation3{add(scale(base_side, std::cos(r_rad)), scale(pitched_up, -std::sin(r_rad))),
                      pitched_forward,
                      add(scale(base_side, std::sin(r_rad)), scale(pitched_up, std::cos(r_rad)))};
}

}  // namespace robotics
