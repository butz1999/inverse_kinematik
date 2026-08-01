// Non-blocking startup sequence for network services and the REST API.

#pragma once

#include <SPIFFS.h>

#include <cstdint>

#include "application/RestApiServer.h"
#include "hardware/Logger.h"
#include "hardware/StatusLed.h"

namespace application
{

struct FirmwareBootstrapConfig
{
  const char *wifi_ssid;
  const char *wifi_password;
  const char *network_hostname;
  uint16_t http_port;
  uint32_t wifi_connect_timeout_ms;
  uint32_t status_blink_interval_ms;
};

class FirmwareBootstrap
{
 public:
  FirmwareBootstrap(const FirmwareBootstrapConfig &config, fs::SPIFFSFS &file_system, RestApiServer &rest_api,
                    hardware::StatusLed &status_led, const hardware::Logger &logger);

  void begin(uint32_t now_ms, bool servo_driver_initialized);
  void update(uint32_t now_ms);
  bool isFinished() const;

 private:
  enum class Phase
  {
    Idle,
    WaitingForWifi,
    StartingMdns,
    MountingFileSystem,
    RegisteringRestApi,
    Finished,
  };

  void startWifi(uint32_t now_ms);
  void startMdns();
  void mountFileSystem();
  void registerRestApi();
  void finish();
  hardware::StatusLed::Color finalStatusColor() const;

  FirmwareBootstrapConfig config_;
  fs::SPIFFSFS &file_system_;
  RestApiServer &rest_api_;
  hardware::StatusLed &status_led_;
  const hardware::Logger &logger_;
  Phase phase_{Phase::Idle};
  uint32_t wifi_started_at_ms_{0U};
  bool servo_driver_initialized_{false};
  bool wifi_connected_{false};
  bool mdns_started_{false};
  bool file_system_mounted_{false};
};

}  // namespace application
