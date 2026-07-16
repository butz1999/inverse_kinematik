// Low-level PCA9685 driver for writing prepared joint PWM states.

#pragma once

#include <Adafruit_PWMServoDriver.h>

#include "common/JointPwmState.h"
#include "hardware/Pca9685ServoDriverConfig.h"

namespace hardware
{

class Pca9685ServoDriver
{
 public:
  explicit Pca9685ServoDriver(uint8_t output_enable_pin);
  explicit Pca9685ServoDriver(const Pca9685ServoDriverConfig &config);

  HardwareDriverResult begin();
  HardwareDriverResult init();
  HardwareDriverResult write(const common::JointPwmState &state);
  bool isInitialized() const;

 private:
  void disableOutputs() const;
  void enableOutputs() const;
  HardwareDriverResult softwareReset();
  HardwareDriverResult configureMode2ForServoOutputs();
  bool writeRegister(uint8_t register_address, uint8_t value) const;
  HardwareDriverResult writeChannels(const common::JointPwmState &state);
  HardwareDriverResult writeChannel(uint8_t channel, uint16_t pwm_value);

  Pca9685ServoDriverConfig config_;
  Adafruit_PWMServoDriver pwm_driver_;
  bool initialized_;
};

}  // namespace hardware
