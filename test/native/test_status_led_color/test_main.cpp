// Native tests for the status LED color mapping used on the ESP32-S3 board.

#include <unity.h>

#include "hardware/StatusLedColor.h"

void test_status_led_color_maps_off_to_zero_arguments()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Off, 255);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_red_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Red, 64);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(64, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_orange_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Orange, 100);

  TEST_ASSERT_EQUAL_UINT8(50, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(100, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_yellow_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Yellow, 100);

  TEST_ASSERT_EQUAL_UINT8(100, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(100, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_green_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Green, 128);

  TEST_ASSERT_EQUAL_UINT8(128, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.blue_arg);
}

void test_status_led_color_maps_cyan_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Cyan, 150);

  TEST_ASSERT_EQUAL_UINT8(150, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(150, color.blue_arg);
}

void test_status_led_color_maps_blue_without_channel_swap()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Blue, 200);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(0, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(200, color.blue_arg);
}

void test_status_led_color_maps_violet_for_grb_led_order()
{
  const auto color = hardware::toNeopixelWriteColor(hardware::StatusColor::Violet, 220);

  TEST_ASSERT_EQUAL_UINT8(0, color.red_arg);
  TEST_ASSERT_EQUAL_UINT8(220, color.green_arg);
  TEST_ASSERT_EQUAL_UINT8(220, color.blue_arg);
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
  return UNITY_END();
}
