// Fixed-capacity string storage for deterministic embedded data models.

#pragma once

#include <array>
#include <cstddef>
#include <cstring>

namespace common
{

template <std::size_t Capacity>
class FixedString
{
 public:
  static_assert(Capacity > 0U, "FixedString capacity must include the terminator.");

  FixedString() = default;
  FixedString(const char *value)
  {
    assign(value);
  }

  FixedString &operator=(const char *value)
  {
    assign(value);
    return *this;
  }

  bool assign(const char *value)
  {
    const auto *source = value == nullptr ? "" : value;
    const auto length = std::strlen(source);
    if (length >= Capacity)
    {
      storage_[0] = '\0';
      return false;
    }
    std::memcpy(storage_.data(), source, length + 1U);
    return true;
  }

  const char *c_str() const
  {
    return storage_.data();
  }

 private:
  std::array<char, Capacity> storage_{};
};

}  // namespace common
