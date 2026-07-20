// Native tests for PCA9685 servo driver configuration helpers.

#include <unity.h>

#include "hardware/Pca9685ServoDriverConfig.h"

void test_default_pca9685_config_uses_documented_address_and_channels()
{
  const auto config = hardware::defaultPca9685ServoDriverConfig();

  TEST_ASSERT_EQUAL_UINT8(0x40U, config.i2c_address);
  TEST_ASSERT_EQUAL_UINT16(50U, config.pwm_frequency_hz);
  TEST_ASSERT_EQUAL_UINT8(0U, config.channels.d);
  TEST_ASSERT_EQUAL_UINT8(1U, config.channels.s);
  TEST_ASSERT_EQUAL_UINT8(2U, config.channels.e);
  TEST_ASSERT_EQUAL_UINT8(3U, config.channels.hp);
  TEST_ASSERT_EQUAL_UINT8(4U, config.channels.hr);
  TEST_ASSERT_EQUAL_UINT8(5U, config.channels.g);
}

void test_pca9685_channel_validation_accepts_0_to_15()
{
  TEST_ASSERT_TRUE(hardware::isValidPca9685Channel(0U));
  TEST_ASSERT_TRUE(hardware::isValidPca9685Channel(15U));
}

void test_pca9685_channel_validation_rejects_values_above_15()
{
  TEST_ASSERT_FALSE(hardware::isValidPca9685Channel(16U));
}

void test_pca9685_channel_map_rejects_invalid_axis_channel()
{
  auto config = hardware::defaultPca9685ServoDriverConfig();
  config.channels.hr = 16U;

  TEST_ASSERT_FALSE(hardware::isValidChannelMap(config.channels));
}

void test_hardware_driver_status_values_are_stable_api_strings()
{
  TEST_ASSERT_EQUAL_STRING("ok", hardware::toString(hardware::HardwareDriverStatus::Ok));
  TEST_ASSERT_EQUAL_STRING("driver_begin_failed",
                           hardware::toString(hardware::HardwareDriverStatus::DriverBeginFailed));
  TEST_ASSERT_EQUAL_STRING("driver_configuration_failed",
                           hardware::toString(hardware::HardwareDriverStatus::DriverConfigurationFailed));
  TEST_ASSERT_EQUAL_STRING("invalid_channel", hardware::toString(hardware::HardwareDriverStatus::InvalidChannel));
  TEST_ASSERT_EQUAL_STRING("invalid_pwm_value", hardware::toString(hardware::HardwareDriverStatus::InvalidPwmValue));
  TEST_ASSERT_EQUAL_STRING("not_initialized", hardware::toString(hardware::HardwareDriverStatus::NotInitialized));
  TEST_ASSERT_EQUAL_STRING("is_initialized", hardware::toString(hardware::HardwareDriverStatus::IsInitialized));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_default_pca9685_config_uses_documented_address_and_channels);
  RUN_TEST(test_pca9685_channel_validation_accepts_0_to_15);
  RUN_TEST(test_pca9685_channel_validation_rejects_values_above_15);
  RUN_TEST(test_pca9685_channel_map_rejects_invalid_axis_channel);
  RUN_TEST(test_hardware_driver_status_values_are_stable_api_strings);
  return UNITY_END();
}
