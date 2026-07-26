// Collection of helpers and utitlity funtions.

#include <chrono>

#pragma once

#include <cstdint>

namespace common
{

namespace utils
{

namespace ticks
{

inline uint32_t currentMillis()
{
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline uint32_t currentMicros()
{
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

}  // namespace ticks

}  // namespace utils

}  // namespace common
