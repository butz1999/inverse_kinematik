// Native tests for the Switch 2 Pro BLE controller input PoC parser.

#include <cstddef>
#include <cstdint>

#include <unity.h>

#include "application/ControllerInput.h"

namespace
{

void encodePackedStick(uint8_t *data, uint16_t x, uint16_t y)
{
  data[0] = static_cast<uint8_t>(x & 0xFFU);
  data[1] = static_cast<uint8_t>(((x >> 8U) & 0x0FU) | ((y & 0x0FU) << 4U));
  data[2] = static_cast<uint8_t>((y >> 4U) & 0xFFU);
}

void prepareNeutralReport(uint8_t *report, std::size_t report_size)
{
  for (std::size_t index = 0; index < report_size; ++index)
  {
    report[index] = 0U;
  }
  report[1] = 0x20U;
  encodePackedStick(&report[5], 2048U, 2048U);
  encodePackedStick(&report[8], 2048U, 2048U);
}

}  // namespace

void test_switch2_pro_ble_parser_rejects_short_reports()
{
  uint8_t report[10] = {};
  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 42U);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL_STRING("short_report", result.message);
  TEST_ASSERT_FALSE(result.input.valid);
}

void test_switch2_pro_ble_parser_rejects_unexpected_status_byte()
{
  uint8_t report[63] = {};
  prepareNeutralReport(report, sizeof(report));
  report[1] = 0x21U;

  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 42U);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL_STRING("unexpected_status_byte", result.message);
}

void test_switch2_pro_ble_parser_decodes_neutral_sticks()
{
  uint8_t report[63] = {};
  prepareNeutralReport(report, sizeof(report));

  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 1234U);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_TRUE(result.input.valid);
  TEST_ASSERT_EQUAL_INT16(0, result.input.left_x);
  TEST_ASSERT_EQUAL_INT16(0, result.input.left_y);
  TEST_ASSERT_EQUAL_INT16(0, result.input.right_x);
  TEST_ASSERT_EQUAL_INT16(0, result.input.right_y);
  TEST_ASSERT_EQUAL_UINT32(1234U, result.input.updated_at_ms);
}

void test_switch2_pro_ble_parser_accepts_real_ble_notification_size()
{
  uint8_t report[112] = {};
  prepareNeutralReport(report, sizeof(report));

  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 1234U);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_TRUE(result.input.valid);
}

void test_switch2_pro_ble_parser_decodes_stick_axes_with_positive_y_up()
{
  uint8_t report[63] = {};
  prepareNeutralReport(report, sizeof(report));
  encodePackedStick(&report[5], 4095U, 0U);
  encodePackedStick(&report[8], 0U, 4095U);

  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 1234U);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_INT16(2047, result.input.left_x);
  TEST_ASSERT_EQUAL_INT16(-2048, result.input.left_y);
  TEST_ASSERT_EQUAL_INT16(-2048, result.input.right_x);
  TEST_ASSERT_EQUAL_INT16(2047, result.input.right_y);
}

void test_switch2_pro_ble_parser_decodes_buttons_and_dpad()
{
  uint8_t report[63] = {};
  prepareNeutralReport(report, sizeof(report));
  report[2] = 0b01000010U;
  report[3] = 0b10101001U;
  report[4] = 0b00010101U;

  const auto result = application::parseSwitch2ProBleInputReport(report, sizeof(report), 1234U);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_BITS_HIGH(application::kControllerButtonA | application::kControllerButtonPlus |
                            application::kControllerButtonZL | application::kControllerButtonLeftStick |
                            application::kControllerButtonHome | application::kControllerButtonGripR |
                            application::kControllerButtonCamera,
                        result.input.buttons);
  TEST_ASSERT_BITS_HIGH(application::kControllerDpadDown | application::kControllerDpadUp, result.input.dpad);
  TEST_ASSERT_BITS_LOW(application::kControllerDpadRight | application::kControllerDpadLeft, result.input.dpad);
}

void test_controller_debug_driver_ingests_switch2_pro_report()
{
  uint8_t report[63] = {};
  prepareNeutralReport(report, sizeof(report));
  report[2] = 0b00000001U;

  application::ControllerDebugDriver driver;

  TEST_ASSERT_TRUE(driver.ingestSwitch2ProBleInputReport(report, sizeof(report), 500U));

  const auto state = driver.state();
  TEST_ASSERT_EQUAL(application::ControllerConnectionStatus::Connected, state.connection_status);
  TEST_ASSERT_EQUAL_STRING("switch2-pro-ble-poc", state.driver_name);
  TEST_ASSERT_EQUAL_STRING("Nintendo Switch 2 Pro Controller", state.controller_name);
  TEST_ASSERT_TRUE(state.input.valid);
  TEST_ASSERT_BITS_HIGH(application::kControllerButtonB, state.input.buttons);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_switch2_pro_ble_parser_rejects_short_reports);
  RUN_TEST(test_switch2_pro_ble_parser_rejects_unexpected_status_byte);
  RUN_TEST(test_switch2_pro_ble_parser_decodes_neutral_sticks);
  RUN_TEST(test_switch2_pro_ble_parser_accepts_real_ble_notification_size);
  RUN_TEST(test_switch2_pro_ble_parser_decodes_stick_axes_with_positive_y_up);
  RUN_TEST(test_switch2_pro_ble_parser_decodes_buttons_and_dpad);
  RUN_TEST(test_controller_debug_driver_ingests_switch2_pro_report);
  return UNITY_END();
}
