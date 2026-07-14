// Configuration and result types for the PCA9685 servo driver.

#pragma once

#include <cstdint>

namespace hardware {

constexpr uint8_t kPca9685DefaultI2cAddress = 0x40U;
constexpr uint8_t kPca9685MaxChannel = 15U;
constexpr uint16_t kDefaultServoPwmFrequencyHz = 50U;

struct ServoChannelMap {
  uint8_t d;
  uint8_t s;
  uint8_t e;
  uint8_t hp;
  uint8_t hr;
  uint8_t g;
};

struct Pca9685ServoDriverConfig {
  uint8_t i2c_address;
  uint16_t pwm_frequency_hz;
  ServoChannelMap channels;
};

enum class HardwareDriverStatus {
  Ok,
  DriverBeginFailed,
  InvalidChannel,
  InvalidPwmValue,
  NotInitialized,
};

struct HardwareDriverResult {
  HardwareDriverStatus status;
  const char *message;
};

inline Pca9685ServoDriverConfig defaultPca9685ServoDriverConfig() {
  return Pca9685ServoDriverConfig{
      kPca9685DefaultI2cAddress,
      kDefaultServoPwmFrequencyHz,
      ServoChannelMap{0U, 1U, 2U, 3U, 4U, 5U},
  };
}

inline bool isValidPca9685Channel(uint8_t channel) {
  return channel <= kPca9685MaxChannel;
}

inline bool isValidChannelMap(const ServoChannelMap &channels) {
  return isValidPca9685Channel(channels.d) &&
         isValidPca9685Channel(channels.s) &&
         isValidPca9685Channel(channels.e) &&
         isValidPca9685Channel(channels.hp) &&
         isValidPca9685Channel(channels.hr) &&
         isValidPca9685Channel(channels.g);
}

const char *toString(HardwareDriverStatus status);

}  // namespace hardware
