// PCA9685 implementation using the Adafruit PWM Servo Driver library.

#include "hardware/Pca9685ServoDriver.h"

#include <Arduino.h>
#include <Wire.h>

#include "hardware/HardwareCalibration.h"
#include "config/RobotSettings.h"

namespace hardware
{

namespace
{

constexpr HardwareDriverResult hwDriverOk()
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

constexpr HardwareDriverResult driverConfigurationFailed()
{
  return HardwareDriverResult{HardwareDriverStatus::DriverConfigurationFailed, "PCA9685 driver configuration failed."};
}

constexpr uint8_t kPca9685GeneralCallAddress = 0x00U;
constexpr uint8_t kPca9685SoftwareResetCommand = 0x06U;
constexpr uint8_t kPca9685ServoMode2 = MODE2_OUTDRV | MODE2_OUTNE_1;

}  // namespace

Pca9685ServoDriver::Pca9685ServoDriver() : Pca9685ServoDriver(defaultPca9685ServoDriverConfig())
{
}

Pca9685ServoDriver::Pca9685ServoDriver(const Pca9685ServoDriverConfig &config)
    : config_(config), pwm_driver_(config.i2c_address), current_pwm_state_{}, initialized_(false)
{
}

HardwareDriverResult Pca9685ServoDriver::init()
{
  initialized_ = false;

  if (!isValidChannelMap(config_.channels))
  {
    return invalidChannel();
  }

  const auto reset_result = softwareReset();
  if (reset_result.status != HardwareDriverStatus::Ok)
  {
    return reset_result;
  }

  if (!pwm_driver_.begin())
  {
    initialized_ = false;
    return driverBeginFailed();
  }

  const auto mode2_result = configureMode2ForServoOutputs();
  if (mode2_result.status != HardwareDriverStatus::Ok)
  {
    return mode2_result;
  }

  pwm_driver_.setPWMFreq(config_.pwm_frequency_hz);

  const auto calibration = defaultHardwareCalibration();
  const auto initial_pwm_result = mapJointStateToPwm(calibration.initial_joint_state, calibration);
  if (!initial_pwm_result.ok)
  {
    return driverConfigurationFailed();
  }

  const auto initial_write_result = writeChannels(initial_pwm_result.joint_pwm_state);
  if (initial_write_result.status != HardwareDriverStatus::Ok)
  {
    return initial_write_result;
  }

  initialized_ = true;
  return hwDriverOk();
}

HardwareDriverResult Pca9685ServoDriver::write(const common::JointPwmState &state)
{
  if (!initialized_)
  {
    return notInitialized();
  }

  if (!common::isWithinJointPwmLimits(state, config::robotSettings().pwm_limits))
  {
    return invalidPwmValue();
  }

  if (!isValidChannelMap(config_.channels))
  {
    return invalidChannel();
  }

  return writeChannels(state);
}

common::JointPwmState Pca9685ServoDriver::jointPwmState() const
{
  return current_pwm_state_;
}

bool Pca9685ServoDriver::isInitialized() const
{
  return initialized_;
}

HardwareDriverResult Pca9685ServoDriver::softwareReset()
{
  Wire.beginTransmission(kPca9685GeneralCallAddress);
  Wire.write(kPca9685SoftwareResetCommand);
  if (Wire.endTransmission() != 0)
  {
    return driverConfigurationFailed();
  }

  delay(1);
  return hwDriverOk();
}

HardwareDriverResult Pca9685ServoDriver::configureMode2ForServoOutputs()
{
  if (!writeRegister(PCA9685_MODE2, kPca9685ServoMode2))
  {
    return driverConfigurationFailed();
  }

  return hwDriverOk();
}

bool Pca9685ServoDriver::writeRegister(uint8_t register_address, uint8_t value) const
{
  Wire.beginTransmission(config_.i2c_address);
  Wire.write(register_address);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

HardwareDriverResult Pca9685ServoDriver::writeChannels(const common::JointPwmState &state)
{
  const auto &channels = config_.channels;

  auto write_if_changed = [this](uint8_t channel, uint16_t current_value, uint16_t target_value)
  {
    return current_value == target_value ? hwDriverOk() : writeChannel(channel, target_value);
  };

  const auto d_result = write_if_changed(channels.d, current_pwm_state_.d_pwm, state.d_pwm);
  if (d_result.status != HardwareDriverStatus::Ok) return d_result;
  const auto s_result = write_if_changed(channels.s, current_pwm_state_.s_pwm, state.s_pwm);
  if (s_result.status != HardwareDriverStatus::Ok) return s_result;
  const auto e_result = write_if_changed(channels.e, current_pwm_state_.e_pwm, state.e_pwm);
  if (e_result.status != HardwareDriverStatus::Ok) return e_result;
  const auto hp_result = write_if_changed(channels.hp, current_pwm_state_.hp_pwm, state.hp_pwm);
  if (hp_result.status != HardwareDriverStatus::Ok) return hp_result;
  const auto hr_result = write_if_changed(channels.hr, current_pwm_state_.hr_pwm, state.hr_pwm);
  if (hr_result.status != HardwareDriverStatus::Ok) return hr_result;
  const auto g_result = write_if_changed(channels.g, current_pwm_state_.g_pwm, state.g_pwm);
  if (g_result.status != HardwareDriverStatus::Ok) return g_result;

  current_pwm_state_ = state;
  return hwDriverOk();
}

HardwareDriverResult Pca9685ServoDriver::writeChannel(uint8_t channel, uint16_t pwm_value)
{
  if (!isValidPca9685Channel(channel))
  {
    return invalidChannel();
  }

  const auto &pwm_limits = config::robotSettings().pwm_limits;
  if (pwm_value < pwm_limits.min_value || pwm_value > pwm_limits.max_value)
  {
    return invalidPwmValue();
  }

  pwm_driver_.setPWM(channel, 0U, pwm_value);
  return hwDriverOk();
}

}  // namespace hardware
