// Project-level controller input model and driver boundary.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(ARDUINO) && __has_include(<Bluepad32.h>)
#include <Bluepad32.h>
#include <bt/uni_bt.h>
#include <bt/uni_bt_le.h>
#define IK_HAS_BLUEPAD32 1
#else
#define IK_HAS_BLUEPAD32 0
#endif

#if defined(IK_REQUIRE_BLUEPAD32) && !IK_HAS_BLUEPAD32
#error "IK_REQUIRE_BLUEPAD32 is set, but Bluepad32.h is not available in this build environment."
#endif

namespace application
{

enum class ControllerConnectionStatus
{
  Disconnected,
  Pairing,
  Reconnecting,
  Connected,
  DriverUnavailable,
};

inline const char *toString(ControllerConnectionStatus status)
{
  switch (status)
  {
    case ControllerConnectionStatus::Disconnected:
      return "disconnected";
    case ControllerConnectionStatus::Pairing:
      return "pairing";
    case ControllerConnectionStatus::Reconnecting:
      return "reconnecting";
    case ControllerConnectionStatus::Connected:
      return "connected";
    case ControllerConnectionStatus::DriverUnavailable:
      return "driver_unavailable";
  }
  return "driver_unavailable";
}

struct ControllerInput
{
  int16_t left_x;
  int16_t left_y;
  int16_t right_x;
  int16_t right_y;
  uint32_t buttons;
  uint32_t dpad;
  bool valid;
  uint32_t updated_at_ms;
};

enum ControllerButton : uint32_t
{
  kControllerButtonB = 1UL << 0,
  kControllerButtonA = 1UL << 1,
  kControllerButtonY = 1UL << 2,
  kControllerButtonX = 1UL << 3,
  kControllerButtonR = 1UL << 4,
  kControllerButtonZR = 1UL << 5,
  kControllerButtonPlus = 1UL << 6,
  kControllerButtonRightStick = 1UL << 7,
  kControllerButtonL = 1UL << 8,
  kControllerButtonZL = 1UL << 9,
  kControllerButtonMinus = 1UL << 10,
  kControllerButtonLeftStick = 1UL << 11,
  kControllerButtonHome = 1UL << 12,
  kControllerButtonCapture = 1UL << 13,
  kControllerButtonGripR = 1UL << 14,
  kControllerButtonGripL = 1UL << 15,
  kControllerButtonCamera = 1UL << 16,
};

enum ControllerDpad : uint32_t
{
  kControllerDpadDown = 1UL << 0,
  kControllerDpadRight = 1UL << 1,
  kControllerDpadLeft = 1UL << 2,
  kControllerDpadUp = 1UL << 3,
};

inline ControllerInput emptyControllerInput()
{
  return ControllerInput{0, 0, 0, 0, 0U, 0U, false, 0U};
}

struct Switch2ProBleParseResult
{
  bool ok;
  const char *message;
  ControllerInput input;
  uint8_t battery_raw;
};

struct ControllerBatteryReading
{
  uint8_t raw;
  uint8_t percent;
  bool available;
};

inline ControllerBatteryReading unavailableControllerBattery()
{
  return ControllerBatteryReading{0U, 0U, false};
}

inline ControllerBatteryReading decodeBluepad32Battery(uint8_t raw)
{
  if (raw == 0U)
  {
    return unavailableControllerBattery();
  }

  const auto percent = static_cast<uint8_t>(((static_cast<uint16_t>(raw) - 1U) * 100U + 127U) / 254U);
  return ControllerBatteryReading{raw, percent, true};
}

inline ControllerBatteryReading decodeSwitch2ProBleBattery(uint8_t raw)
{
  constexpr uint8_t kBatteryLevelMask = 0x30U;
  constexpr uint8_t kBatteryLevelShift = 4U;
  constexpr uint8_t kBatteryLevelFull = 3U;

  const auto level = static_cast<uint8_t>((raw & kBatteryLevelMask) >> kBatteryLevelShift);
  const auto percent = static_cast<uint8_t>((static_cast<uint16_t>(level) * 100U) / kBatteryLevelFull);
  return ControllerBatteryReading{raw, percent, true};
}

inline int16_t normalizeSwitch2StickAxis(uint16_t value, bool invert)
{
  constexpr int16_t kCenter = 2048;
  auto normalized = static_cast<int16_t>(static_cast<int32_t>(value) - kCenter);
  return invert ? static_cast<int16_t>(-normalized) : normalized;
}

inline uint16_t decodeSwitch2Packed12BitAxis(const uint8_t *data)
{
  return static_cast<uint16_t>(data[0] | ((data[1] & 0x0FU) << 8U));
}

inline uint16_t decodeSwitch2Packed12BitSecondAxis(const uint8_t *data)
{
  return static_cast<uint16_t>((data[1] >> 4U) | (data[2] << 4U));
}

inline uint8_t decodeSwitch2ProBleBatteryRaw(const uint8_t *report, std::size_t report_size)
{
  constexpr std::size_t kSwitch2ProBleBatteryStatusOffset = 11U;
  if (report == nullptr || report_size <= kSwitch2ProBleBatteryStatusOffset)
  {
    return 0U;
  }
  return report[kSwitch2ProBleBatteryStatusOffset];
}

inline Switch2ProBleParseResult parseSwitch2ProBleInputReport(const uint8_t *report, std::size_t report_size,
                                                              uint32_t now_ms)
{
  constexpr std::size_t kSwitch2ProBleReportSize = 63U;
  if (report == nullptr)
  {
    return Switch2ProBleParseResult{false, "missing_report", emptyControllerInput(), 0U};
  }
  if (report_size < kSwitch2ProBleReportSize)
  {
    return Switch2ProBleParseResult{false, "short_report", emptyControllerInput(), 0U};
  }
  if (report[1] != 0x20U)
  {
    return Switch2ProBleParseResult{false, "unexpected_status_byte", emptyControllerInput(), 0U};
  }

  const auto buttons_r = report[2];
  const auto buttons_l = report[3];
  const auto buttons_3 = report[4];
  const auto left_x = decodeSwitch2Packed12BitAxis(&report[5]);
  const auto left_y = decodeSwitch2Packed12BitSecondAxis(&report[5]);
  const auto right_x = decodeSwitch2Packed12BitAxis(&report[8]);
  const auto right_y = decodeSwitch2Packed12BitSecondAxis(&report[8]);

  uint32_t buttons = 0U;
  buttons |= (buttons_r & (1U << 0U)) != 0U ? kControllerButtonB : 0U;
  buttons |= (buttons_r & (1U << 1U)) != 0U ? kControllerButtonA : 0U;
  buttons |= (buttons_r & (1U << 2U)) != 0U ? kControllerButtonY : 0U;
  buttons |= (buttons_r & (1U << 3U)) != 0U ? kControllerButtonX : 0U;
  buttons |= (buttons_r & (1U << 4U)) != 0U ? kControllerButtonR : 0U;
  buttons |= (buttons_r & (1U << 5U)) != 0U ? kControllerButtonZR : 0U;
  buttons |= (buttons_r & (1U << 6U)) != 0U ? kControllerButtonPlus : 0U;
  buttons |= (buttons_r & (1U << 7U)) != 0U ? kControllerButtonRightStick : 0U;
  buttons |= (buttons_l & (1U << 4U)) != 0U ? kControllerButtonL : 0U;
  buttons |= (buttons_l & (1U << 5U)) != 0U ? kControllerButtonZL : 0U;
  buttons |= (buttons_l & (1U << 6U)) != 0U ? kControllerButtonMinus : 0U;
  buttons |= (buttons_l & (1U << 7U)) != 0U ? kControllerButtonLeftStick : 0U;
  buttons |= (buttons_3 & (1U << 0U)) != 0U ? kControllerButtonHome : 0U;
  buttons |= (buttons_3 & (1U << 1U)) != 0U ? kControllerButtonCapture : 0U;
  buttons |= (buttons_3 & (1U << 2U)) != 0U ? kControllerButtonGripR : 0U;
  buttons |= (buttons_3 & (1U << 3U)) != 0U ? kControllerButtonGripL : 0U;
  buttons |= (buttons_3 & (1U << 4U)) != 0U ? kControllerButtonCamera : 0U;

  uint32_t dpad = 0U;
  dpad |= (buttons_l & (1U << 0U)) != 0U ? kControllerDpadDown : 0U;
  dpad |= (buttons_l & (1U << 1U)) != 0U ? kControllerDpadRight : 0U;
  dpad |= (buttons_l & (1U << 2U)) != 0U ? kControllerDpadLeft : 0U;
  dpad |= (buttons_l & (1U << 3U)) != 0U ? kControllerDpadUp : 0U;

  return Switch2ProBleParseResult{
      true, "ok",
      ControllerInput{normalizeSwitch2StickAxis(left_x, false), normalizeSwitch2StickAxis(left_y, false),
                      normalizeSwitch2StickAxis(right_x, false), normalizeSwitch2StickAxis(right_y, false), buttons,
                      dpad, true, now_ms},
      decodeSwitch2ProBleBatteryRaw(report, report_size)};
}

struct ControllerDebugState
{
  ControllerConnectionStatus connection_status;
  const char *driver_name;
  const char *controller_name;
  const char *message;
  bool pairing_requested;
  bool accepts_new_connections;
  bool bluepad_scanning;
  uint8_t battery_raw;
  uint8_t battery_percent;
  bool battery_available;
  const char *battery_encoding;
  uint32_t connected_at_ms;
  uint32_t last_update_ms;
  uint32_t reconnect_deadline_ms;
  ControllerInput input;
};

class ControllerDebugDriver
{
 public:
  static constexpr uint32_t kControllerReconnectWindowMs = 10UL * 60UL * 1000UL;
  static constexpr uint32_t kControllerInputGraceMs = 3000UL;

  ControllerDebugDriver()
      : connection_status_(ControllerConnectionStatus::Disconnected),
        driver_name_(IK_HAS_BLUEPAD32 ? "bluepad32" : "bluepad32-unavailable"),
        pairing_requested_(false),
        accepts_new_connections_(false),
        battery_raw_(0U),
        battery_percent_(0U),
        battery_available_(false),
        battery_encoding_("unavailable"),
        connected_at_ms_(0U),
        reconnect_deadline_ms_(0U),
        switch2_pro_ble_active_(false),
        last_update_ms_(0U),
        controller_name_(""),
        message_(IK_HAS_BLUEPAD32 ? "Bluepad32 driver is ready; request pairing to test the controller."
                                  : "Bluepad32.h is not available in this build environment."),
        input_(emptyControllerInput())
  {
    controller_name_storage_[0] = '\0';
  }

  void service(uint32_t now_ms)
  {
#if IK_HAS_BLUEPAD32
    setupBluepad32IfNeeded();
    BP32.update();
    refreshBluepad32Controller(now_ms);
#endif
    last_update_ms_ = now_ms;
  }

  void requestPairing(uint32_t now_ms)
  {
#if IK_HAS_BLUEPAD32
    setupBluepad32IfNeeded();
    BP32.enableNewBluetoothConnections(true);
    pairing_requested_ = true;
    accepts_new_connections_ = true;
    reconnect_deadline_ms_ = now_ms + kControllerReconnectWindowMs;
    connection_status_ = ControllerConnectionStatus::Pairing;
    message_ =
        "Bluepad32 reconnect/pairing enabled for 10 minutes. Wake the Pro Controller or put it into pairing mode.";
#else
    pairing_requested_ = false;
    accepts_new_connections_ = false;
    reconnect_deadline_ms_ = 0U;
    connection_status_ = ControllerConnectionStatus::DriverUnavailable;
    message_ = "Bluepad32.h is missing; install/use the Bluepad32 ESP32 build before testing.";
#endif
    last_update_ms_ = now_ms;
  }

  void disconnect(uint32_t now_ms)
  {
#if IK_HAS_BLUEPAD32
    for (auto *controller : bluepad_controllers_)
    {
      if (controller != nullptr && controller->isConnected())
      {
        controller->disconnect();
      }
    }
    BP32.enableNewBluetoothConnections(false);
    uni_bt_le_switch2_pro_disconnect();
#endif
    pairing_requested_ = false;
    accepts_new_connections_ = false;
    connected_at_ms_ = 0U;
    reconnect_deadline_ms_ = 0U;
    battery_raw_ = 0U;
    battery_percent_ = 0U;
    battery_available_ = false;
    battery_encoding_ = "unavailable";
    switch2_pro_ble_active_ = false;
    input_ = emptyControllerInput();
    connection_status_ = ControllerConnectionStatus::Disconnected;
    controller_name_storage_[0] = '\0';
    controller_name_ = "";
    message_ = IK_HAS_BLUEPAD32 ? "Controller disconnected; Bluepad32 discovery disabled."
                                : "Bluepad32.h is not available in this build environment.";
    last_update_ms_ = now_ms;
  }

  bool ingestSwitch2ProBleInputReport(const uint8_t *report, std::size_t report_size, uint32_t now_ms)
  {
    const auto parse_result = parseSwitch2ProBleInputReport(report, report_size, now_ms);
    if (!parse_result.ok)
    {
      last_update_ms_ = now_ms;
      message_ = parse_result.message;
      return false;
    }

    input_ = parse_result.input;
    pairing_requested_ = false;
    accepts_new_connections_ = false;
    switch2_pro_ble_active_ = true;
    connection_status_ = ControllerConnectionStatus::Connected;
    connected_at_ms_ = connected_at_ms_ == 0U ? now_ms : connected_at_ms_;
    reconnect_deadline_ms_ = now_ms + kControllerReconnectWindowMs;
    const auto battery = decodeSwitch2ProBleBattery(parse_result.battery_raw);
    battery_raw_ = battery.raw;
    battery_percent_ = battery.percent;
    battery_available_ = battery.available;
    battery_encoding_ = "switch2_status_bits_5_to_4";
    last_update_ms_ = now_ms;
    controller_name_ = "Nintendo Switch 2 Pro Controller";
    driver_name_ = "Switch 2 Pro BLE";
    message_ = "Switch 2 Pro BLE input report received.";
    return true;
  }

  ControllerDebugState state() const
  {
    return ControllerDebugState{connection_status_,
                                driver_name_,
                                controller_name_,
                                message_,
                                pairing_requested_,
                                accepts_new_connections_,
                                isBluepad32Scanning(),
                                battery_raw_,
                                battery_percent_,
                                battery_available_,
                                battery_encoding_,
                                connected_at_ms_,
                                last_update_ms_,
                                reconnect_deadline_ms_,
                                input_};
  }

 private:
#if IK_HAS_BLUEPAD32
  static void onBluepad32ControllerConnected(ControllerPtr controller)
  {
    for (auto &slot : bluepad_controllers_)
    {
      if (slot == nullptr)
      {
        slot = controller;
        return;
      }
    }
  }

  static void onBluepad32ControllerDisconnected(ControllerPtr controller)
  {
    for (auto &slot : bluepad_controllers_)
    {
      if (slot == controller)
      {
        slot = nullptr;
        return;
      }
    }
  }

  void setupBluepad32IfNeeded()
  {
    if (bluepad_setup_done_)
    {
      return;
    }

    BP32.setup(&ControllerDebugDriver::onBluepad32ControllerConnected,
               &ControllerDebugDriver::onBluepad32ControllerDisconnected, false);
    BP32.enableVirtualDevice(false);
    BP32.enableBLEService(false);
    BP32.enableNewBluetoothConnections(false);
    bluepad_setup_done_ = true;
  }

  void refreshBluepad32Controller(uint32_t now_ms)
  {
    ControllerPtr active_controller = nullptr;
    for (auto *controller : bluepad_controllers_)
    {
      if (controller != nullptr && controller->isConnected())
      {
        active_controller = controller;
        break;
      }
    }

    if (active_controller == nullptr)
    {
      if (switch2_pro_ble_active_)
      {
        if (now_ms - input_.updated_at_ms <= kControllerInputGraceMs)
        {
          connection_status_ = ControllerConnectionStatus::Connected;
          message_ = "Switch 2 Pro BLE input report received.";
          return;
        }
        if (uni_bt_le_switch2_pro_is_connected())
        {
          connection_status_ = ControllerConnectionStatus::Connected;
          message_ = "Switch 2 Pro BLE connected; waiting for input.";
          return;
        }
        if (!hasReconnectWindowExpired(now_ms))
        {
          startReconnectWait("Switch 2 Pro BLE input paused; keeping the host ready for reconnect.");
          return;
        }

        finishControllerDisconnect("Switch 2 Pro BLE reconnect window expired; connection closed.");
        return;
      }
      if (pairing_requested_)
      {
        if (hasReconnectWindowExpired(now_ms))
        {
          finishControllerDisconnect("Bluepad32 reconnect/pairing window expired; discovery disabled.");
          return;
        }
        connection_status_ = ControllerConnectionStatus::Pairing;
        message_ = "Bluepad32 reconnect/pairing is active; no controller connected yet.";
      }
      else if (connection_status_ == ControllerConnectionStatus::Connected ||
               connection_status_ == ControllerConnectionStatus::Reconnecting)
      {
        if (!hasReconnectWindowExpired(now_ms))
        {
          startReconnectWait("Bluepad32 controller disconnected; waiting for reconnect.");
          return;
        }

        finishControllerDisconnect("Bluepad32 reconnect window expired; controller disconnected.");
      }
      return;
    }

    const auto model_name = active_controller->getModelName();
    copyControllerName(model_name.c_str());

    input_ = ControllerInput{static_cast<int16_t>(active_controller->axisX()),
                             static_cast<int16_t>(active_controller->axisY()),
                             static_cast<int16_t>(active_controller->axisRX()),
                             static_cast<int16_t>(active_controller->axisRY()),
                             static_cast<uint32_t>(active_controller->buttons()) |
                                 (static_cast<uint32_t>(active_controller->miscButtons()) << 16U),
                             static_cast<uint32_t>(active_controller->dpad()),
                             active_controller->hasData(),
                             now_ms};
    switch2_pro_ble_active_ = false;
    pairing_requested_ = false;
    accepts_new_connections_ = false;
    connection_status_ = ControllerConnectionStatus::Connected;
    connected_at_ms_ = connected_at_ms_ == 0U ? now_ms : connected_at_ms_;
    reconnect_deadline_ms_ = now_ms + kControllerReconnectWindowMs;
    const auto battery = decodeBluepad32Battery(active_controller->battery());
    battery_raw_ = battery.raw;
    battery_percent_ = battery.percent;
    battery_available_ = battery.available;
    battery_encoding_ = "bluepad32_normalized_1_to_255";
    message_ = active_controller->hasData() ? "Bluepad32 controller data received."
                                            : "Bluepad32 controller connected; waiting for input data.";
  }
#endif

  bool hasReconnectWindowExpired(uint32_t now_ms) const
  {
    return reconnect_deadline_ms_ == 0U || static_cast<int32_t>(now_ms - reconnect_deadline_ms_) >= 0;
  }

  void startReconnectWait(const char *message)
  {
#if IK_HAS_BLUEPAD32
    BP32.enableNewBluetoothConnections(true);
#endif
    pairing_requested_ = false;
    accepts_new_connections_ = true;
    connection_status_ = ControllerConnectionStatus::Reconnecting;
    message_ = message;
  }

  void finishControllerDisconnect(const char *message)
  {
#if IK_HAS_BLUEPAD32
    BP32.enableNewBluetoothConnections(false);
    uni_bt_le_switch2_pro_disconnect();
#endif
    pairing_requested_ = false;
    accepts_new_connections_ = false;
    connected_at_ms_ = 0U;
    reconnect_deadline_ms_ = 0U;
    battery_raw_ = 0U;
    battery_percent_ = 0U;
    battery_available_ = false;
    battery_encoding_ = "unavailable";
    switch2_pro_ble_active_ = false;
    input_ = emptyControllerInput();
    connection_status_ = ControllerConnectionStatus::Disconnected;
    controller_name_storage_[0] = '\0';
    controller_name_ = "";
    driver_name_ = IK_HAS_BLUEPAD32 ? "bluepad32" : "bluepad32-unavailable";
    message_ = message;
  }

  void copyControllerName(const char *controller_name)
  {
    if (controller_name == nullptr || controller_name[0] == '\0')
    {
      controller_name_storage_[0] = '\0';
      controller_name_ = "";
      return;
    }

    std::snprintf(controller_name_storage_, sizeof(controller_name_storage_), "%s", controller_name);
    controller_name_ = controller_name_storage_;
  }

  bool isBluepad32Scanning() const
  {
#if IK_HAS_BLUEPAD32
    return uni_bt_is_scanning();
#else
    return false;
#endif
  }

#if IK_HAS_BLUEPAD32
  inline static ControllerPtr bluepad_controllers_[BP32_MAX_GAMEPADS] = {};
  inline static bool bluepad_setup_done_ = false;
#endif

  ControllerConnectionStatus connection_status_;
  const char *driver_name_;
  bool pairing_requested_;
  bool accepts_new_connections_;
  uint8_t battery_raw_;
  uint8_t battery_percent_;
  bool battery_available_;
  const char *battery_encoding_;
  uint32_t connected_at_ms_;
  uint32_t reconnect_deadline_ms_;
  bool switch2_pro_ble_active_;
  uint32_t last_update_ms_;
  const char *controller_name_;
  const char *message_;
  char controller_name_storage_[64];
  ControllerInput input_;
};

}  // namespace application
