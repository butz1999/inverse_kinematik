// Configuration and result types for the PCA9685 servo driver.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace hardware
{

constexpr uint8_t kPca9685DefaultI2cAddress = 0x40U;
constexpr uint8_t kPca9685MaxChannel = 15U;
constexpr uint16_t kDefaultServoPwmFrequencyHz = 50U;

struct ServoChannelMap
{
  uint8_t d;
  uint8_t s;
  uint8_t e;
  uint8_t hp;
  uint8_t hr;
  uint8_t g;
};

struct Pca9685ServoDriverConfig
{
  uint8_t i2c_address;
  uint16_t pwm_frequency_hz;
  ServoChannelMap channels;
};

enum class HardwareDriverStatus
{
  Ok,
  DriverBeginFailed,
  DriverConfigurationFailed,
  InvalidChannel,
  InvalidPwmValue,
  NotInitialized,
};

struct HardwareDriverResult
{
  HardwareDriverStatus status;
  const char *message;
};

inline Pca9685ServoDriverConfig defaultPca9685ServoDriverConfig()
{
  return Pca9685ServoDriverConfig{
      kPca9685DefaultI2cAddress,
      kDefaultServoPwmFrequencyHz,
      ServoChannelMap{0U, 1U, 2U, 3U, 4U, 5U},
  };
}

inline bool isValidPca9685Channel(uint8_t channel)
{
  return channel <= kPca9685MaxChannel;
}

inline bool isValidChannelMap(const ServoChannelMap &channels)
{
  const std::array<uint8_t, 6U> mapped_channels = {channels.d, channels.s, channels.e,
                                                     channels.hp, channels.hr, channels.g};
  for (std::size_t index = 0U; index < mapped_channels.size(); ++index)
  {
    if (!isValidPca9685Channel(mapped_channels[index]))
    {
      return false;
    }

    for (std::size_t other_index = index + 1U; other_index < mapped_channels.size(); ++other_index)
    {
      if (mapped_channels[index] == mapped_channels[other_index])
      {
        return false;
      }
    }
  }

  return true;
}

const char *toString(HardwareDriverStatus status);

}  // namespace hardware
