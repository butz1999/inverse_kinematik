// String conversions for hardware driver result values.

#include "hardware/Pca9685ServoDriverConfig.h"

namespace hardware
{

const char *toString(HardwareDriverStatus status)
{
  switch (status)
  {
    case HardwareDriverStatus::Ok:
      return "ok";
    case HardwareDriverStatus::DriverBeginFailed:
      return "driver_begin_failed";
    case HardwareDriverStatus::DriverConfigurationFailed:
      return "driver_configuration_failed";
    case HardwareDriverStatus::InvalidChannel:
      return "invalid_channel";
    case HardwareDriverStatus::InvalidPwmValue:
      return "invalid_pwm_value";
    case HardwareDriverStatus::NotInitialized:
      return "not_initialized";
    case HardwareDriverStatus::IsInitialized:
      return "is_initialized";
  }

  return "not_initialized";
}

}  // namespace hardware
