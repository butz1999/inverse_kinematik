// PCA9685 implementation using the Adafruit PWM Servo Driver library.

#include "hardware/Pca9685ServoDriver.h"

namespace hardware
{

namespace
{

constexpr HardwareDriverResult ok()
{
  return HardwareDriverResult{HardwareDriverStatus::Ok, "ok"};
}

constexpr HardwareDriverResult notInitialized()
{
  return HardwareDriverResult{HardwareDriverStatus::NotInitialized, "PCA9685 servo driver is not initialized."};
}

constexpr HardwareDriverResult invalidChannel()
{
  return HardwareDriverResult{HardwareDriverStatus::InvalidChannel, "PCA9685 channel is outside 0..15."};
}

constexpr HardwareDriverResult invalidPwmValue()
{
  return HardwareDriverResult{HardwareDriverStatus::InvalidPwmValue, "PWM value is outside the PCA9685 12-bit range."};
}

constexpr HardwareDriverResult driverBeginFailed()
{
  return HardwareDriverResult{HardwareDriverStatus::DriverBeginFailed, "PCA9685 driver begin failed."};
}

}  // namespace

Pca9685ServoDriver::Pca9685ServoDriver(const Pca9685ServoDriverConfig &config)
    : config_(config), pwm_driver_(config.i2c_address), initialized_(false)
{
}

HardwareDriverResult Pca9685ServoDriver::begin()
{
  if (!isValidChannelMap(config_.channels))
  {
    return invalidChannel();
  }

  if (!pwm_driver_.begin())
  {
    initialized_ = false;
    return driverBeginFailed();
  }

  pwm_driver_.setPWMFreq(config_.pwm_frequency_hz);
  initialized_ = true;
  return ok();
}

HardwareDriverResult Pca9685ServoDriver::write(const common::JointPwmState &state)
{
  if (!initialized_)
  {
    return notInitialized();
  }

  if (const auto *violation = common::findFirstLimitViolation(state))
  {
    (void)violation;
    return invalidPwmValue();
  }

  const auto &channels = config_.channels;
  if (!isValidChannelMap(channels))
  {
    return invalidChannel();
  }

  auto result = writeChannel(channels.d, state.d_pwm);
  if (result.status != HardwareDriverStatus::Ok)
  {
    return result;
  }

  result = writeChannel(channels.s, state.s_pwm);
  if (result.status != HardwareDriverStatus::Ok)
  {
    return result;
  }

  result = writeChannel(channels.e, state.e_pwm);
  if (result.status != HardwareDriverStatus::Ok)
  {
    return result;
  }

  result = writeChannel(channels.hp, state.hp_pwm);
  if (result.status != HardwareDriverStatus::Ok)
  {
    return result;
  }

  result = writeChannel(channels.hr, state.hr_pwm);
  if (result.status != HardwareDriverStatus::Ok)
  {
    return result;
  }

  return writeChannel(channels.g, state.g_pwm);
}

bool Pca9685ServoDriver::isInitialized() const
{
  return initialized_;
}

HardwareDriverResult Pca9685ServoDriver::writeChannel(uint8_t channel, uint16_t pwm_value)
{
  if (!isValidPca9685Channel(channel))
  {
    return invalidChannel();
  }

  if (pwm_value > common::kPca9685MaxPwm)
  {
    return invalidPwmValue();
  }

  pwm_driver_.setPWM(channel, 0U, pwm_value);
  return ok();
}

}  // namespace hardware
