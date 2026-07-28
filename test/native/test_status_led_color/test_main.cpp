// Native tests for the status LED color mapping used on the ESP32-S3 board.

#include <unity.h>

#include "hardware/StatusLed.h"

void test_status_led_color_maps_off_to_zero_arguments()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Off), 255);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_red_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Red), 64);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(64, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_orange_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Orange), 100);

  TEST_ASSERT_EQUAL_UINT8(33, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(100, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_yellow_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Yellow), 100);

  TEST_ASSERT_EQUAL_UINT8(100, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(100, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_green_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Green), 128);

  TEST_ASSERT_EQUAL_UINT8(128, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_cyan_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Cyan), 150);

  TEST_ASSERT_EQUAL_UINT8(150, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(150, color.blue_arg);
}

void test_status_led_color_maps_blue_without_channel_swap()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Blue), 200);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(200, color.blue_arg);
}

void test_status_led_color_maps_violet_for_grb_led_order()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(
      hardware::StatusLed::rgbFromStatusColor(hardware::StatusColor::Violet), 220);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(220, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(220, color.blue_arg);
}

void test_status_led_color_maps_raw_rgb_to_grb_arguments_with_brightness()
{
  const auto color = hardware::StatusLed::toNeopixelWriteColor(hardware::StatusLed::RgbColor{255U, 128U, 64U}, 100U);

  TEST_ASSERT_EQUAL_UINT8(50, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(100, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(25, color.blue_arg);
}

void test_status_led_parses_modes()
{
  hardware::StatusLed::Mode mode = hardware::StatusLed::Mode::Off;

  TEST_ASSERT_TRUE(hardware::parseStatusLedMode("blinking", mode));
  TEST_ASSERT_EQUAL(hardware::StatusLed::Mode::Blinking, mode);
  TEST_ASSERT_EQUAL_STRING("blinking", hardware::toString(mode));

  TEST_ASSERT_TRUE(hardware::parseStatusLedMode("pulsing", mode));
  TEST_ASSERT_EQUAL(hardware::StatusLed::Mode::Pulsing, mode);
  TEST_ASSERT_EQUAL_STRING("pulsing", hardware::toString(mode));
}

void test_status_led_reports_the_last_named_color()
{
  hardware::StatusLed led(38U, 15U);

  led.set(hardware::StatusColor::Green);
  TEST_ASSERT_EQUAL(hardware::StatusColor::Green, led.color());

  led.set(hardware::StatusLed::RgbColor{1U, 2U, 3U});
  TEST_ASSERT_EQUAL(hardware::StatusColor::Off, led.color());
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_status_led_color_maps_off_to_zero_arguments);
  RUN_TEST(test_status_led_color_maps_red_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_orange_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_yellow_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_green_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_cyan_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_blue_without_channel_swap);
  RUN_TEST(test_status_led_color_maps_violet_for_grb_led_order);
  RUN_TEST(test_status_led_color_maps_raw_rgb_to_grb_arguments_with_brightness);
  RUN_TEST(test_status_led_parses_modes);
  RUN_TEST(test_status_led_reports_the_last_named_color);
  return UNITY_END();
}
