// Lightweight math helpers for robotics geometry.

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

}  // namespace robotics
