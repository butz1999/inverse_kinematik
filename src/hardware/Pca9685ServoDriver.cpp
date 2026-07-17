// PCA9685 implementation using the Adafruit PWM Servo Driver library.

#include "hardware/Pca9685ServoDriver.h"

#include <Arduino.h>
#include <Wire.h>

#include "hardware/HardwareCalibration.h"

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

constexpr HardwareDriverResult alreadyInitialized()
{
  return HardwareDriverResult{HardwareDriverStatus::IsInitialized, "PCA9685 servo driver is initialized."};
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
constexpr uint8_t kPca9685ServoMode2 = MODE2_OUTDRV;

}  // namespace

Pca9685ServoDriver::Pca9685ServoDriver(uint8_t output_enable_pin)
    : Pca9685ServoDriver(defaultPca9685ServoDriverConfig(output_enable_pin))
{
}

Pca9685ServoDriver::Pca9685ServoDriver(const Pca9685ServoDriverConfig &config)
    : config_(config), pwm_driver_(config.i2c_address), initialized_(false)
{
}

HardwareDriverResult Pca9685ServoDriver::begin()
{
  initialized_ = false;
  disableOutputs();

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
  const auto initial_write_result = writeChannels(calibration.initial_pwm_state);
  if (initial_write_result.status != HardwareDriverStatus::Ok)
  {
    return initial_write_result;
  }

  initialized_ = true;
  enableOutputs();
  return ok();
}

HardwareDriverResult Pca9685ServoDriver::init()
{
  if (initialized_)
  {
    return alreadyInitialized();
  }

  return begin();
}

HardwareDriverResult Pca9685ServoDriver::write(const common::JointPwmState &state)
{
  if (!initialized_)
  {
    return notInitialized();
  }

  if (!common::isWithinJointPwmLimits(state))
  {
    return invalidPwmValue();
  }

  if (!isValidChannelMap(config_.channels))
  {
    return invalidChannel();
  }

  return writeChannels(state);
}

bool Pca9685ServoDriver::isInitialized() const
{
  return initialized_;
}

void Pca9685ServoDriver::disableOutputs() const
{
  digitalWrite(config_.output_enable_pin, HIGH);
  pinMode(config_.output_enable_pin, OUTPUT);
}

void Pca9685ServoDriver::enableOutputs() const
{
  digitalWrite(config_.output_enable_pin, LOW);
  pinMode(config_.output_enable_pin, OUTPUT);
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
  return ok();
}

HardwareDriverResult Pca9685ServoDriver::configureMode2ForServoOutputs()
{
  if (!writeRegister(PCA9685_MODE2, kPca9685ServoMode2))
  {
    return driverConfigurationFailed();
  }

  return ok();
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

HardwareDriverResult Pca9685ServoDriver::writeChannel(uint8_t channel, uint16_t pwm_value)
{
  if (!isValidPca9685Channel(channel))
  {
    return invalidChannel();
  }

  if (pwm_value > common::kMaxPwm)
  {
    return invalidPwmValue();
  }

  pwm_driver_.setPWM(channel, 0U, pwm_value);
  return ok();
}

}  // namespace hardware
