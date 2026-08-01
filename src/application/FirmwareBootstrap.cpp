// Non-blocking startup sequence for network services and the REST API.

#include "application/FirmwareBootstrap.h"

#include <ESPmDNS.h>
#include <WiFi.h>

namespace application
{

namespace
{

bool isConfigured(const char *value)
{
  return value != nullptr && value[0] != '\0';
}

}  // namespace

FirmwareBootstrap::FirmwareBootstrap(const FirmwareBootstrapConfig &config, fs::SPIFFSFS &file_system,
                                     RestApiServer &rest_api, hardware::StatusLed &status_led,
                                     const hardware::Logger &logger)
    : config_(config), file_system_(file_system), rest_api_(rest_api), status_led_(status_led), logger_(logger)
{
}

void FirmwareBootstrap::begin(uint32_t now_ms, bool servo_driver_initialized)
{
  servo_driver_initialized_ = servo_driver_initialized;
  logger_.println("[BOOT] Starting network bootstrap");
  startWifi(now_ms);
}

void FirmwareBootstrap::update(uint32_t now_ms)
{
  switch (phase_)
  {
    case Phase::Idle:
    case Phase::Finished:
      return;
    case Phase::WaitingForWifi:
      if (WiFi.status() == WL_CONNECTED)
      {
        wifi_connected_ = true;
        logger_.print("[BOOT] WiFi connected, IP address: ");
        logger_.println(WiFi.localIP().toString().c_str());
        phase_ = Phase::StartingMdns;
        return;
      }
      if (static_cast<uint32_t>(now_ms - wifi_started_at_ms_) >= config_.wifi_connect_timeout_ms)
      {
        logger_.println("[BOOT] WiFi STA connection timed out");
        status_led_.set(hardware::StatusLed::Color::Red);
        phase_ = Phase::MountingFileSystem;
      }
      return;
    case Phase::StartingMdns:
      startMdns();
      phase_ = Phase::MountingFileSystem;
      return;
    case Phase::MountingFileSystem:
      mountFileSystem();
      phase_ = Phase::RegisteringRestApi;
      return;
    case Phase::RegisteringRestApi:
      registerRestApi();
      finish();
      return;
  }
}

bool FirmwareBootstrap::isFinished() const
{
  return phase_ == Phase::Finished;
}

void FirmwareBootstrap::startWifi(uint32_t now_ms)
{
  if (!isConfigured(config_.wifi_ssid))
  {
    logger_.println("[BOOT] WiFi STA not configured: IK_WIFI_SSID is empty");
    status_led_.set(hardware::StatusLed::Color::Red);
    phase_ = Phase::MountingFileSystem;
    return;
  }

  status_led_.set(hardware::StatusLed::Color::Blue);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(config_.network_hostname);
  logger_.print("[BOOT] Connecting to WiFi SSID: ");
  logger_.println(config_.wifi_ssid);
  if (isConfigured(config_.wifi_password))
  {
    WiFi.begin(config_.wifi_ssid, config_.wifi_password);
  }
  else
  {
    WiFi.begin(config_.wifi_ssid);
  }

  wifi_started_at_ms_ = now_ms;
  phase_ = Phase::WaitingForWifi;
}

void FirmwareBootstrap::startMdns()
{
  if (!MDNS.begin(config_.network_hostname))
  {
    logger_.println("[BOOT] mDNS responder failed to start");
    return;
  }
  if (!MDNS.addService("http", "tcp", config_.http_port))
  {
    logger_.println("[BOOT] mDNS service registration failed");
    return;
  }

  mdns_started_ = true;
  logger_.print("[BOOT] mDNS responder available at http://");
  logger_.print(config_.network_hostname);
  logger_.println(".local");
}

void FirmwareBootstrap::mountFileSystem()
{
  file_system_mounted_ = file_system_.begin(false);
  if (file_system_mounted_)
  {
    logger_.println("[BOOT] SPIFFS static web assets mounted");
    return;
  }

  logger_.println("[BOOT] SPIFFS mount failed; static web UI is unavailable");
}

void FirmwareBootstrap::registerRestApi()
{
  rest_api_.init();
  logger_.println("[BOOT] REST API endpoints registered");
}

void FirmwareBootstrap::finish()
{
  status_led_.set(finalStatusColor(), hardware::StatusLed::Mode::Pulsing, config_.status_blink_interval_ms);
  phase_ = Phase::Finished;
  logger_.println("[BOOT] Network bootstrap finished");
}

hardware::StatusLed::Color FirmwareBootstrap::finalStatusColor() const
{
  if (!file_system_mounted_)
  {
    return hardware::StatusLed::Color::Cyan;
  }
  if (!wifi_connected_)
  {
    return hardware::StatusLed::Color::Red;
  }
  if (!servo_driver_initialized_)
  {
    return hardware::StatusLed::Color::Orange;
  }
  if (!mdns_started_)
  {
    return hardware::StatusLed::Color::Yellow;
  }
  return hardware::StatusLed::Color::Green;
}

}  // namespace application
