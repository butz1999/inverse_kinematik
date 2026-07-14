// Minimal ESP32 REST interface for bring-up and later Orchestrator integration.

#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "application/ApiContracts.h"
#include "common/JointPwmState.h"
#include "common/JointState.h"

namespace application {

class RestApiServer {
 public:
  explicit RestApiServer(WebServer &server);

  void begin();
  void handleClient();

 private:
  void handleHealth();
  void handleStatus();
  void handleJointState();
  void handleJointMotionRequest();
  void handleJointPwmState();
  void handleJointPwmMotionRequest();
  void handleMotionRequest();
  void handleNotFound();
  void sendJson(int status_code, const String &body);

  WebServer &server_;
  common::JointState current_joint_state_;
  common::JointPwmState current_joint_pwm_state_;
};

}  // namespace application
