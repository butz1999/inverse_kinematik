// Minimal ESP32 REST interface for bring-up and later Orchestrator integration.

#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "application/ApiContracts.h"
#include "common/JointPwmState.h"
#include "common/JointState.h"
#include "hardware/HardwareCalibration.h"
#include "hardware/Pca9685ServoDriver.h"
#include "hardware/SerialLogger.h"

namespace application
{

class RestApiServer
{
 public:
  explicit RestApiServer(WebServer &server);
  RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver);
  RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver, const hardware::SerialLogger &logger);

  void begin();
  void handleClient();

 private:
  void handleHealth();
  void handleStatus();
  void handleJointState();
  void handleJointMotionRequest();
  void handleJointPwmState();
  void handleServoDriverInitRequest();
  void handleJointPwmMotionRequest();
  void handleMotionRequest();
  void handleCorsPreflight();
  void handleFavicon();
  void handleNotFound();
  void logRequest(const char *method, const char *path) const;
  void logResult(const char *message) const;
  void sendCorsHeaders();
  void sendJson(int status_code, const String &body);

  WebServer &server_;
  hardware::Pca9685ServoDriver *servo_driver_;
  const hardware::SerialLogger *logger_;
  hardware::HardwareCalibration hardware_calibration_;
  common::JointState current_joint_state_;
  common::JointPwmState current_joint_pwm_state_;
};

}  // namespace application
