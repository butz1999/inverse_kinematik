// Minimal ESP32 REST interface for bring-up and later Orchestrator integration.

#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "application/ApiContracts.h"
#include "application/ControllerCartesianJog.h"
#include "application/ControllerInput.h"
#include "application/ControllerJointSlewLimiter.h"
#include "application/ControllerJog.h"
#include "application/ControllerWorldRollLock.h"
#include "application/RunEngine.h"
#include "common/JointPwmState.h"
#include "common/JointState.h"
#include "common/MotionProfile.h"
#include "hardware/HardwareCalibration.h"
#include "hardware/Logger.h"
#include "hardware/Pca9685ServoDriver.h"
#include "hardware/StatusLed.h"
#include "orchestration/MotionOrchestrator.h"

namespace application
{

class RestApiServer
{
 public:
  RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver, hardware::StatusLed &status_led,
                const hardware::Logger &logger);

  void init();
  void handleClient();
  bool ingestSwitch2ProBleInputReport(const uint8_t *report, std::size_t report_size, uint32_t now_ms);

 private:
  void handleHealth();
  void handleStatus();
  void handleJointState();
  void handleJointMotionRequest();
  void handleJointPwmState();
  void handleServoDriverInitRequest();
  void handleJointPwmMotionRequest();
  void handleMotionRequest();
  void handleForwardKinematicsRequest();
  void handleSequenceStartRequest();
  void handleSequenceStopRequest();
  void handleSequenceStatus();
  void handleControllerConnectRequest();
  void handleControllerDisconnectRequest();
  void handleControllerStatus();
  void handleControllerDebug();
  void handleCorsPreflight();
  void handleFavicon();
  void handleNotFound();
  bool hasActiveMotionPlan() const;
  void startMotionPlan(const common::MotionPlan &plan, const common::JointState &target_joint_state);
  void serviceActiveMotionPlan();
  void serviceSequenceRun();
  void serviceControllerInput();
  void serviceControllerJog(uint32_t now_ms);
  void queueLedStep(const steps::LedStep &step);
  void servicePendingLedStep();
  void logRequest(const char *method, const char *path) const;
  void logResult(const char *message) const;
  void sendCorsHeaders();
  void sendJson(int status_code, const String &body);

  WebServer &server_;
  hardware::Pca9685ServoDriver &servo_driver_;
  hardware::StatusLed &status_led_;
  const hardware::Logger &logger_;
  hardware::HardwareCalibration hardware_calibration_;
  orchestration::MotionOrchestrator orchestrator_;
  RunEngine run_engine_;
  ControllerDebugDriver controller_driver_;
  common::JointState current_joint_state_;
  common::JointPwmState current_joint_pwm_state_;
  common::MotionPlan active_motion_plan_;
  orchestration::MotionResult motion_result_scratch_;
  common::JointState active_motion_target_joint_state_;
  std::size_t active_motion_sample_index_;
  unsigned long active_motion_started_ms_;
  bool motion_plan_active_;
  steps::LedStep pending_led_step_;
  bool has_pending_led_step_;
  uint32_t last_controller_jog_ms_;
  bool controller_jog_active_;
  ControllerCartesianJogState controller_cartesian_jog_state_;
  ControllerJointSlewLimiterState controller_joint_slew_limiter_state_;
  ControllerWorldRollLockState controller_world_roll_lock_state_;
};

}  // namespace application
