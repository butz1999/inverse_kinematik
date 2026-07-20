// Common logging interface for components that require diagnostic output.

#pragma once

namespace hardware
{

class Logger
{
 public:
  virtual ~Logger() = default;

  virtual void print(const char *message) const = 0;
  virtual void println(const char *message) const = 0;
};

}  // namespace hardware
