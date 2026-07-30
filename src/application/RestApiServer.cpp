// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

#include <ArduinoJson.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>

#include "application/ApiJson.h"
#include "config/RobotSettings.h"
#include "orchestration/MotionOrchestrator.h"
#include "robotics/Kinematics.h"
#include "robotics/Validation.h"

// ToDo: Das ist grosser Spaghetti-Code. Alternativen?

namespace application
{

namespace
{

constexpr std::size_t kRestJsonCapacity = 4096;
constexpr std::size_t kStaticAssetBufferSize = 1024;
constexpr uint32_t kStaticAssetWriteTimeoutMs = 5000;
using RestJsonDocument = StaticJsonDocument<kRestJsonCapacity>;

void allowNetworkTaskToTransmit()
{
  vTaskDelay(pdMS_TO_TICKS(1));
}

RestJsonDocument &sequenceResponseJsonDocument()
{
  static RestJsonDocument document;
  document.clear();
  return document;
}

template <typename JsonDocumentType>
String jsonBody(const JsonDocumentType &doc)
{
  String body;
  body.reserve(measureJson(doc));
  serializeJson(doc, body);
  return body;
}

float roundReportedValue(float value)
{
  const auto rounded = std::round(value * 10.0F) / 10.0F;
  return rounded == 0.0F ? 0.0F : rounded;
}

void setJointStateJson(JsonObject object, const common::JointState &state)
{
  for (const auto axis : common::kJointAxes)
  {
    object[common::jointAxisFieldName(axis)] = roundReportedValue(common::jointAxisValue(state, axis));
  }
}

void setMotionLimitsJson(JsonObject object)
{
  const auto &settings = config::robotSettings();
  object["schemaVersion"] = 1U;

  auto joint_limits_json = object.createNestedObject("jointLimits");
  auto servo_pwm_limits_json = object.createNestedObject("servoPwmLimits");
  for (const auto axis : common::kJointAxes)
  {
    const auto &joint_limit = common::jointLimitForAxis(settings.joint_limits, axis);
    auto joint_limit_json = joint_limits_json.createNestedObject(common::jointAxisFieldName(axis));
    joint_limit_json["min"] = joint_limit.min_value;
    joint_limit_json["max"] = joint_limit.max_value;
    joint_limit_json["unit"] = axis == common::JointAxis::G ? "pct" : "deg";

    const auto &pwm_endpoints = config::servoPwmEndpointsFor(settings.servo_pwm_calibration, axis);
    auto servo_pwm_limit_json = servo_pwm_limits_json.createNestedObject(common::jointAxisPwmFieldName(axis));
    servo_pwm_limit_json["min"] = std::min(pwm_endpoints.min_pwm, pwm_endpoints.max_pwm);
    servo_pwm_limit_json["max"] = std::max(pwm_endpoints.min_pwm, pwm_endpoints.max_pwm);
    servo_pwm_limit_json["unit"] = "pwm";
  }

  auto pwm_limits_json = object.createNestedObject("pwmLimits");
  pwm_limits_json["min"] = settings.pwm_limits.min_value;
  pwm_limits_json["max"] = settings.pwm_limits.max_value;
  pwm_limits_json["unit"] = "pwm";
}

void setTargetPoseJson(JsonObject object, const common::TargetPose &pose)
{
  object["x_mm"] = roundReportedValue(pose.x_mm);
  object["y_mm"] = roundReportedValue(pose.y_mm);
  object["z_mm"] = roundReportedValue(pose.z_mm);
  object["p_deg"] = roundReportedValue(pose.p_deg);
  object["r_deg"] = roundReportedValue(pose.r_deg);
  object["g_pct"] = roundReportedValue(pose.g_pct);
}

void setOffsetTargetPoseJson(JsonObject object, const robotics::OffsetTargetPose &pose)
{
  object["x_mm"] = roundReportedValue(pose.x_mm);
  object["y_mm"] = roundReportedValue(pose.y_mm);
  object["z_mm"] = roundReportedValue(pose.z_mm);
  object["p_deg"] = roundReportedValue(pose.p_deg);
  object["r_deg"] = roundReportedValue(pose.r_deg);
  object["g_pct"] = roundReportedValue(pose.g_pct);
}

common::TargetPose targetPoseFromForwardKinematics(const robotics::ForwardKinematicsResult &fk,
                                                   const robotics::RobotModelOffset &offset)
{
  return common::TargetPose{fk.g_mm.x_mm + offset.o_d_offset_x_mm,
                            fk.g_mm.y_mm + offset.o_d_offset_y_mm,
                            fk.g_mm.z_mm + offset.o_d_offset_z_mm,
                            fk.p_deg,
                            fk.r_deg,
                            fk.g_pct};
}

common::TargetPose targetPoseFromJointState(const common::JointState &joint_state)
{
  const auto robot_model = robotics::defaultRobotModel();
  const auto robot_offset = robotics::defaultRobotModelOffset();
  const auto fk = robotics::forwardKinematics(joint_state, robot_model, robot_offset);
  return targetPoseFromForwardKinematics(fk, robot_offset);
}

bool setReportedTargetPoseJson(JsonObject object, const common::JointState &joint_state)
{
  if (!robotics::validateJointState(joint_state).ok)
  {
    return false;
  }

  const auto robot_model = robotics::defaultRobotModel();
  const auto pose = targetPoseFromJointState(joint_state);
  if (!robotics::validateTargetPose(pose, robot_model).ok)
  {
    return false;
  }

  setTargetPoseJson(object, pose);
  return true;
}

bool isSameJointPwmState(const common::JointPwmState &left, const common::JointPwmState &right)
{
  return left.d_pwm == right.d_pwm && left.s_pwm == right.s_pwm && left.e_pwm == right.e_pwm &&
         left.hp_pwm == right.hp_pwm && left.hr_pwm == right.hr_pwm && left.g_pwm == right.g_pwm;
}

void setJointPwmStateJson(JsonObject object, const common::JointPwmState &state)
{
  object["d_pwm"] = state.d_pwm;
  object["s_pwm"] = state.s_pwm;
  object["e_pwm"] = state.e_pwm;
  object["hp_pwm"] = state.hp_pwm;
  object["hr_pwm"] = state.hr_pwm;
  object["g_pwm"] = state.g_pwm;
}

void setMotionPlanSummaryJson(JsonObject object, const common::MotionPlan &plan)
{
  object["profile"] = common::toString(plan.profile.type);
  object["totalDurationMs"] = plan.total_duration_ms;
  object["sampleCount"] = plan.sample_count;
  object["sampleTimeMs"] = plan.profile.sample_time_ms;
  object["targetVelocityDegS"] = plan.profile.target_velocity_deg_s;
  object["calculationTimeUs"] = plan.calculation_time_us;
}

void setSequenceStateJson(JsonObject object, const SequenceState &state, uint32_t now_ms)
{
  object["status"] = toString(state.status);
  object["stepIndex"] = state.step_index;
  object["stepCount"] = state.step_count;
  object["message"] = state.message != nullptr ? state.message : "";
  object["lastMotionStatus"] = orchestration::toString(state.last_motion_status);
  object["waitRemainingMs"] =
      state.status == SequenceRunStatus::Waiting && static_cast<int32_t>(state.wait_until_ms - now_ms) > 0
          ? state.wait_until_ms - now_ms
          : 0U;
}

void setHardwareDriverResultJson(JsonObject object, const hardware::HardwareDriverResult &result)
{
  object["status"] = hardware::toString(result.status);
  object["message"] = result.message;
}

void setControllerInputJson(JsonObject object, const ControllerInput &input)
{
  object["valid"] = input.valid;
  object["leftX"] = input.left_x;
  object["leftY"] = input.left_y;
  object["rightX"] = input.right_x;
  object["rightY"] = input.right_y;
  object["buttons"] = input.buttons;
  object["dpad"] = input.dpad;
  object["updatedAtMs"] = input.updated_at_ms;
}

void setControllerStateJson(JsonObject object, const ControllerDebugState &state,
                            const orchestration::ControllerHandlerState &handler_state, uint32_t now_ms)
{
  object["status"] = toString(state.connection_status);
  object["driver"] = state.driver_name;
  object["controllerName"] = state.controller_name;
  object["message"] = state.message;
  object["pairingRequested"] = state.pairing_requested;
  object["acceptsNewConnections"] = state.accepts_new_connections;
  object["bluepadScanning"] = state.bluepad_scanning;
  object["batteryRaw"] = state.battery_raw;
  if (state.battery_available)
  {
    object["batteryLevel"] = state.battery_percent;
  }
  else
  {
    object["batteryLevel"] = nullptr;
  }
  object["batteryAvailable"] = state.battery_available;
  object["batteryEncoding"] = state.battery_encoding;
  object["connectedAtMs"] = state.connected_at_ms;
  object["lastUpdateMs"] = state.last_update_ms;
  object["reconnectDeadlineMs"] = state.reconnect_deadline_ms;
  object["reconnectRemainingMs"] =
      state.reconnect_deadline_ms > 0U && static_cast<int32_t>(state.reconnect_deadline_ms - now_ms) > 0
          ? state.reconnect_deadline_ms - now_ms
          : 0U;
  object["ageMs"] = state.last_update_ms > 0U ? now_ms - state.last_update_ms : 0U;
  auto world_roll_lock = object.createNestedObject("worldRollLock");
  world_roll_lock["enabled"] = handler_state.world_roll_lock_enabled;
  world_roll_lock["lockedWorldRollDeg"] = roundReportedValue(handler_state.locked_world_roll_deg);
  setControllerInputJson(object.createNestedObject("input"), state.input);
}

}  // namespace

RestApiServer::RestApiServer(WebServer &server, fs::FS &asset_file_system,
                             hardware::Pca9685ServoDriver &servo_driver, hardware::StatusLed &status_led,
                             const hardware::Logger &logger)
    : server_(server),
      asset_file_system_(asset_file_system),
      servo_driver_(servo_driver),
      status_led_(status_led),
      logger_(logger),
      hardware_calibration_(hardware::defaultHardwareCalibration()),
      orchestrator_(robotics::defaultRobotModel(), robotics::defaultRobotModelOffset()),
      controller_handler_(orchestrator_),
      run_engine_(orchestrator_),
      controller_driver_(),
      current_joint_state_(hardware_calibration_.initial_joint_state),
      current_joint_pwm_state_(
          hardware::mapJointStateToPwm(hardware_calibration_.initial_joint_state, hardware_calibration_)
              .joint_pwm_state),
      active_motion_plan_{common::defaultMotionProfile(), 0U, 0U, 0U, {}},
      active_motion_target_joint_state_(common::initialJointState()),
      active_motion_sample_index_(0U),
      active_motion_started_ms_(0UL),
      motion_plan_active_(false),
      pending_led_step_(steps::emptyLedStep()),
      has_pending_led_step_(false),
      last_controller_jog_ms_(0U),
      controller_jog_active_(false)
{
}

void RestApiServer::init()
{
  server_.on("/", HTTP_GET,
             [this]()
             {
               handleRoot();
             });
  server_.on(kHealthPath, HTTP_GET,
             [this]()
             {
               handleHealth();
             });
  server_.on(kStatusPath, HTTP_GET,
             [this]()
             {
               handleStatus();
             });
  server_.on(kMotionLimitsPath, HTTP_GET,
             [this]()
             {
               handleMotionLimits();
             });
  server_.on(kJointStatePath, HTTP_GET,
             [this]()
             {
               handleJointState();
             });
  server_.on(kJointMotionPath, HTTP_POST,
             [this]()
             {
               handleJointMotionRequest();
             });
  server_.on(kJointMotionPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kJointPwmStatePath, HTTP_GET,
             [this]()
             {
               handleJointPwmState();
             });
  server_.on(kServoDriverInitPath, HTTP_POST,
             [this]()
             {
               handleServoDriverInitRequest();
             });
  server_.on(kServoDriverInitPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kJointPwmMotionPath, HTTP_POST,
             [this]()
             {
               handleJointPwmMotionRequest();
             });
  server_.on(kJointPwmMotionPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kMotionPath, HTTP_POST,
             [this]()
             {
               handleMotionRequest();
             });
  server_.on(kMotionPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kForwardKinematicsPath, HTTP_POST,
             [this]()
             {
               handleForwardKinematicsRequest();
             });
  server_.on(kForwardKinematicsPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kSequenceStartPath, HTTP_POST,
             [this]()
             {
               handleSequenceStartRequest();
             });
  server_.on(kSequenceStartPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kSequenceStopPath, HTTP_POST,
             [this]()
             {
               handleSequenceStopRequest();
             });
  server_.on(kSequenceStopPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kSequenceStatusPath, HTTP_GET,
             [this]()
             {
               handleSequenceStatus();
             });
  server_.on(kControllerConnectPath, HTTP_POST,
             [this]()
             {
               handleControllerConnectRequest();
             });
  server_.on(kControllerConnectPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kControllerDisconnectPath, HTTP_POST,
             [this]()
             {
               handleControllerDisconnectRequest();
             });
  server_.on(kControllerDisconnectPath, HTTP_OPTIONS,
             [this]()
             {
               handleCorsPreflight();
             });
  server_.on(kControllerStatusPath, HTTP_GET,
             [this]()
             {
               handleControllerStatus();
             });
  server_.on(kControllerDebugPath, HTTP_GET,
             [this]()
             {
               handleControllerDebug();
             });
  server_.onNotFound(
      [this]()
      {
        handleNotFound();
      });
  server_.begin();
}

bool RestApiServer::ingestSwitch2ProBleInputReport(const uint8_t *report, std::size_t report_size, uint32_t now_ms)
{
  return controller_driver_.ingestSwitch2ProBleInputReport(report, report_size, now_ms);
}

void RestApiServer::handleClient()
{
  server_.handleClient();
  serviceControllerInput();
  servicePendingLedStep();
  serviceActiveMotionPlan();
  serviceSequenceRun();
  servicePendingLedStep();
}

bool RestApiServer::hasActiveMotionPlan() const
{
  return motion_plan_active_;
}

void RestApiServer::startMotionPlan(const common::MotionPlan &plan, const common::JointState &target_joint_state)
{
  active_motion_plan_ = plan;
  active_motion_target_joint_state_ = target_joint_state;
  active_motion_sample_index_ = 0U;
  active_motion_started_ms_ = millis();
  motion_plan_active_ = plan.sample_count > 0U;
  serviceActiveMotionPlan();
}

void RestApiServer::serviceActiveMotionPlan()
{
  if (!motion_plan_active_ || !servo_driver_.isInitialized())
  {
    return;
  }

  if (active_motion_sample_index_ >= active_motion_plan_.sample_count)
  {
    motion_plan_active_ = false;
    current_joint_state_ = active_motion_target_joint_state_;
    return;
  }

  const auto &sample = active_motion_plan_.samples[active_motion_sample_index_];
  const auto elapsed_ms = static_cast<uint32_t>(millis() - active_motion_started_ms_);
  if (elapsed_ms < sample.time_from_start_ms)
  {
    return;
  }

  const auto calibration_result = hardware::mapJointStateToPwm(sample.joint_state, hardware_calibration_);
  if (!calibration_result.ok)
  {
    logResult("[REST] Active motion plan stopped by calibration failure");
    motion_plan_active_ = false;
    return;
  }

  const auto write_result = servo_driver_.write(calibration_result.joint_pwm_state);
  if (write_result.status != hardware::HardwareDriverStatus::Ok)
  {
    logResult("[REST] Active motion plan stopped by hardware failure");
    motion_plan_active_ = false;
    return;
  }

  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  ++active_motion_sample_index_;
  if (active_motion_sample_index_ >= active_motion_plan_.sample_count)
  {
    current_joint_state_ = active_motion_target_joint_state_;
    motion_plan_active_ = false;
    logResult("[REST] Active motion plan completed");
  }
}

void RestApiServer::serviceSequenceRun()
{
  if (!run_engine_.isActive())
  {
    return;
  }

  const auto service_result = run_engine_.service(current_joint_state_, hasActiveMotionPlan(), millis());
  if (service_result.has_led_step)
  {
    queueLedStep(service_result.led_step);
  }

  if (!service_result.has_motion_plan)
  {
    return;
  }

  startMotionPlan(*service_result.motion_plan, service_result.target_joint_state);
}

void RestApiServer::serviceControllerInput()
{
  const auto now_ms = millis();
  controller_driver_.service(now_ms);
  serviceControllerJog(now_ms);
}

void RestApiServer::serviceControllerJog(uint32_t now_ms)
{
  const auto state = controller_driver_.state();
  if (state.connection_status != ControllerConnectionStatus::Connected || !state.input.valid ||
      !servo_driver_.isInitialized() || hasActiveMotionPlan() || run_engine_.isActive())
  {
    controller_jog_active_ = false;
    controller_handler_.reset();
    last_controller_jog_ms_ = now_ms;
    return;
  }

  if (!controller_jog_active_)
  {
    controller_jog_active_ = true;
    last_controller_jog_ms_ = now_ms;
    return;
  }

  constexpr uint32_t kControllerJogPeriodMs = 5U;
  if (static_cast<uint32_t>(now_ms - last_controller_jog_ms_) < kControllerJogPeriodMs)
  {
    return;
  }

  last_controller_jog_ms_ = now_ms;
  constexpr uint32_t elapsed_ms = kControllerJogPeriodMs;
  const auto handler_result =
      controller_handler_.update(mapControllerInputToJogCommand(state.input), current_joint_state_, elapsed_ms);
  if (!handler_result.changed)
  {
    return;
  }

  const auto calibration_result = hardware::mapJointStateToPwm(handler_result.joint_state, hardware_calibration_);
  if (!calibration_result.ok)
  {
    logResult("[REST] Controller handler stopped by calibration failure");
    controller_jog_active_ = false;
    controller_handler_.reset();
    return;
  }

  if (!isSameJointPwmState(calibration_result.joint_pwm_state, current_joint_pwm_state_))
  {
    const auto write_result = servo_driver_.write(calibration_result.joint_pwm_state);
    if (write_result.status != hardware::HardwareDriverStatus::Ok)
    {
      logResult("[REST] Controller handler stopped by hardware failure");
      controller_jog_active_ = false;
      controller_handler_.reset();
      return;
    }
  }

  current_joint_state_ = handler_result.joint_state;
  current_joint_pwm_state_ = servo_driver_.jointPwmState();
}

void RestApiServer::queueLedStep(const steps::LedStep &step)
{
  pending_led_step_ = step;
  has_pending_led_step_ = true;
  logResult("[REST] LED step queued");
}

void RestApiServer::servicePendingLedStep()
{
  if (!has_pending_led_step_)
  {
    return;
  }

  const auto step = pending_led_step_;
  has_pending_led_step_ = false;

  logResult("[REST] Applying LED step");
  if (step.has_status_color)
  {
    status_led_.set(step.status_color, step.mode, step.interval_ms);
  }
  else if (step.has_rgb_color)
  {
    status_led_.set(step.rgb_color, step.mode, step.interval_ms);
  }
  logResult("[REST] LED step applied");
}

void RestApiServer::handleHealth()
{
  logRequest("GET", kHealthPath);

  RestJsonDocument doc;
  doc["service"] = kApiName;
  doc["apiVersion"] = kApiVersion;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["orchestrator"] = toString(ApiCapabilityStatus::Available);
  doc["uptimeMs"] = millis();

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleStatus()
{
  logRequest("GET", kStatusPath);

  RestJsonDocument doc;
  doc["restApi"] = toString(ApiCapabilityStatus::Available);
  doc["orchestrator"] = toString(ApiCapabilityStatus::Available);
  doc["jointStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["servoDriverInitEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["forwardKinematicsEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmHardwareOutput"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmHardwareInitialized"] = servo_driver_.isInitialized();
  doc["motionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["sequenceEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["controllerEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["motionLimitsEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["motionPlanActive"] = hasActiveMotionPlan();
  doc["motionPlanSampleIndex"] = active_motion_sample_index_;
  doc["motionPlanSampleCount"] = active_motion_plan_.sample_count;
  setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
  setControllerStateJson(doc.createNestedObject("controller"), controller_driver_.state(), controller_handler_.state(),
                         millis());
  doc["uptimeMs"] = millis();

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleMotionLimits()
{
  logRequest("GET", kMotionLimitsPath);

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  setMotionLimitsJson(doc.as<JsonObject>());

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointState()
{
  logRequest("GET", kJointStatePath);

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["source"] = "assumed_low_level_state";
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointMotionRequest()
{
  logRequest("POST", kJointMotionPath);

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseJointMotionRequestJson(body_arg.c_str());

  if (!parsed.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(parsed.code);
    if (parsed.field_name[0] != '\0')
    {
      doc["field"] = parsed.field_name;
    }
    doc["message"] = parsed.message;
    logResult("[REST] Joint motion request rejected");
    sendJson(400, jsonBody(doc));
    return;
  }

  const auto calibration_result = hardware::mapJointStateToPwm(parsed.joint_state, hardware_calibration_);
  if (!calibration_result.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "calibration_failed";
    doc["code"] = hardware::toString(calibration_result.status);
    if (calibration_result.field_name[0] != '\0')
    {
      doc["field"] = calibration_result.field_name;
    }
    doc["message"] = calibration_result.message;
    logResult("[REST] Joint motion request rejected by calibration");
    sendJson(500, jsonBody(doc));
    return;
  }

  if (!servo_driver_.isInitialized())
  {
    logResult("[REST] Joint motion request rejected because PCA9685 is not initialized");
    RestJsonDocument doc;
    doc["status"] = "hardware_not_initialized";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "joint_space_calibrated";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] =
        "PCA9685 servo driver is not initialized; check the boot log or call POST /api/servo-driver/init for "
        "diagnostics.";
    sendJson(503, jsonBody(doc));
    return;
  }

  const auto manual_hand_roll_changed = parsed.joint_state.hr_deg != current_joint_state_.hr_deg;
  const auto write_result = servo_driver_.write(calibration_result.joint_pwm_state);
  if (write_result.status != hardware::HardwareDriverStatus::Ok)
  {
    logResult("[REST] PCA9685 calibrated joint write failed");
    RestJsonDocument doc;
    doc["status"] = "hardware_failed";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "joint_space_calibrated";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    setHardwareDriverResultJson(doc.createNestedObject("driver"), write_result);
    sendJson(503, jsonBody(doc));
    return;
  }

  current_joint_state_ = parsed.joint_state;
  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  if (manual_hand_roll_changed)
  {
    controller_handler_.synchronizeJointState(current_joint_state_);
  }
  logResult("[REST] Joint motion request mapped and written to hardware");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "joint_space_calibrated";
  doc["hardware"] = toString(ApiCapabilityStatus::Available);
  setHardwareDriverResultJson(doc.createNestedObject("driver"), write_result);
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleForwardKinematicsRequest()
{
  logRequest("POST", kForwardKinematicsPath);

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseJointMotionRequestJson(body_arg.c_str());

  if (!parsed.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(parsed.code);
    if (parsed.field_name[0] != '\0')
    {
      doc["field"] = parsed.field_name;
    }
    doc["message"] = parsed.message;
    logResult("[REST] Forward kinematics request rejected");
    sendJson(400, jsonBody(doc));
    return;
  }

  const auto robot_model = robotics::defaultRobotModel();
  const auto robot_offset = robotics::defaultRobotModelOffset();
  const auto fk = robotics::forwardKinematics(parsed.joint_state, robot_model, robot_offset);
  const robotics::OffsetTargetPose offset_target_pose{fk.g_mm.x_mm, fk.g_mm.y_mm, fk.g_mm.z_mm,
                                                      fk.p_deg,     fk.r_deg,     fk.g_pct};
  const auto target_pose = targetPoseFromForwardKinematics(fk, robot_offset);

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "forward_kinematics";
  setJointStateJson(doc.createNestedObject("jointState"), parsed.joint_state);
  setOffsetTargetPoseJson(doc.createNestedObject("offsetTargetPose"), offset_target_pose);
  setTargetPoseJson(doc.createNestedObject("targetPose"), target_pose);
  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointPwmState()
{
  logRequest("GET", kJointPwmStatePath);

  if (servo_driver_.isInitialized())
  {
    current_joint_pwm_state_ = servo_driver_.jointPwmState();
  }

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["source"] = servo_driver_.isInitialized() ? "hardware_driver_pwm_state" : "assumed_low_level_pwm_state";
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleServoDriverInitRequest()
{
  logRequest("POST", kServoDriverInitPath);

  const auto init_result = servo_driver_.init();
  if (init_result.status != hardware::HardwareDriverStatus::Ok)
  {
    logResult("[REST] PCA9685 init failed");
    RestJsonDocument doc;
    doc["status"] = "hardware_failed";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    setHardwareDriverResultJson(doc.createNestedObject("driver"), init_result);
    sendJson(503, jsonBody(doc));
    return;
  }

  current_joint_state_ = hardware_calibration_.initial_joint_state;
  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  logResult("[REST] PCA9685 initialized");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["hardware"] = toString(ApiCapabilityStatus::Available);
  setHardwareDriverResultJson(doc.createNestedObject("driver"), init_result);
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleJointPwmMotionRequest()
{
  logRequest("POST", kJointPwmMotionPath);

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseJointPwmMotionRequestJson(body_arg.c_str());

  if (!parsed.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(parsed.code);
    if (parsed.field_name[0] != '\0')
    {
      doc["field"] = parsed.field_name;
    }
    doc["message"] = parsed.message;
    logResult("[REST] Joint PWM request rejected");
    sendJson(400, jsonBody(doc));
    return;
  }

  if (!servo_driver_.isInitialized())
  {
    logResult("[REST] Joint PWM request rejected because PCA9685 is not initialized");
    RestJsonDocument doc;
    doc["status"] = "hardware_not_initialized";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "joint_pwm_direct";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] =
        "PCA9685 servo driver is not initialized; check the boot log or call POST /api/servo-driver/init for "
        "diagnostics.";
    sendJson(503, jsonBody(doc));
    return;
  }

  const auto write_result = servo_driver_.write(parsed.joint_pwm_state);
  if (write_result.status != hardware::HardwareDriverStatus::Ok)
  {
    logResult("[REST] PCA9685 write failed");
    RestJsonDocument doc;
    doc["status"] = "hardware_failed";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "joint_pwm_direct";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    setHardwareDriverResultJson(doc.createNestedObject("driver"), write_result);
    sendJson(503, jsonBody(doc));
    return;
  }

  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  logResult("[REST] Joint PWM request written to hardware");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "joint_pwm_direct";
  doc["hardware"] = toString(ApiCapabilityStatus::Available);
  setHardwareDriverResultJson(doc.createNestedObject("driver"), write_result);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleMotionRequest()
{
  logRequest("POST", kMotionPath);

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseTargetPoseRequestJson(body_arg.c_str());

  if (!parsed.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(parsed.code);
    if (parsed.field_name[0] != '\0')
    {
      doc["field"] = parsed.field_name;
    }
    doc["message"] = parsed.message;
    logResult("[REST] Motion request rejected by JSON parser");
    sendJson(400, jsonBody(doc));
    return;
  }

  if (hasActiveMotionPlan())
  {
    logResult("[REST] Motion request rejected because a motion plan is already active");
    RestJsonDocument doc;
    doc["status"] = "busy";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "task_space_ik";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] = "A motion plan is already active.";
    setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
    setMotionPlanSummaryJson(doc.createNestedObject("activeMotionPlan"), active_motion_plan_);
    sendJson(409, jsonBody(doc));
    return;
  }

  if (run_engine_.isActive())
  {
    logResult("[REST] Motion request rejected because a sequence is active");
    RestJsonDocument doc;
    doc["status"] = "busy";
    doc["code"] = toString(ApiResultCode::SequenceBusy);
    doc["mode"] = "task_space_ik";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] = "A sequence is already active.";
    setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
    sendJson(409, jsonBody(doc));
    return;
  }

  const orchestration::MotionRequest motion_request{parsed.target_pose, parsed.motion_profile};
  orchestrator_.processMotionRequestInto(motion_request, current_joint_state_, motion_result_scratch_);
  const auto &motion_result = motion_result_scratch_;
  if (!motion_result.ok && motion_result.status == orchestration::MotionStatus::InvalidTargetPose)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = motion_result.target_validation_status == robotics::ValidationStatus::TargetPoseOutOfWorkspace
                      ? toString(ApiResultCode::TargetPoseOutOfWorkspace)
                      : toString(ApiResultCode::InvalidTargetPose);
    if (motion_result.field_name[0] != '\0')
    {
      doc["field"] = motion_result.field_name;
    }
    doc["message"] = motion_result.message;
    setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
    logResult("[REST] Motion request rejected by target validation");
    sendJson(400, jsonBody(doc));
    return;
  }

  if (!motion_result.ok && motion_result.status == orchestration::MotionStatus::KinematicsFailure)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(ApiResultCode::KinematicsFailure);
    doc["kinematicsStatus"] = robotics::toString(motion_result.kinematics_status);
    doc["message"] = motion_result.message;
    setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
    setOffsetTargetPoseJson(doc.createNestedObject("offsetTargetPose"), motion_result.offset_target_pose);
    logResult("[REST] Motion request rejected by inverse kinematics");
    sendJson(422, jsonBody(doc));
    return;
  }

  if (!motion_result.ok && motion_result.status == orchestration::MotionStatus::JointLimitViolation)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(ApiResultCode::JointLimitViolation);
    if (motion_result.field_name[0] != '\0')
    {
      doc["field"] = motion_result.field_name;
    }
    doc["message"] = motion_result.message;
    setJointStateJson(doc.createNestedObject("jointState"), motion_result.joint_state);
    logResult("[REST] Motion request rejected by joint validation");
    sendJson(400, jsonBody(doc));
    return;
  }

  if (!motion_result.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = toString(ApiResultCode::KinematicsFailure);
    doc["motionStatus"] = orchestration::toString(motion_result.status);
    doc["motionProfileStatus"] = orchestration::toString(motion_result.motion_profile_status);
    doc["message"] = motion_result.message;
    setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
    setOffsetTargetPoseJson(doc.createNestedObject("offsetTargetPose"), motion_result.offset_target_pose);
    setJointStateJson(doc.createNestedObject("jointState"), motion_result.joint_state);
    logResult("[REST] Motion request rejected by motion planning");
    sendJson(422, jsonBody(doc));
    return;
  }

  const auto calibration_result = hardware::mapJointStateToPwm(motion_result.joint_state, hardware_calibration_);
  if (!calibration_result.ok)
  {
    RestJsonDocument doc;
    doc["status"] = "calibration_failed";
    doc["code"] = hardware::toString(calibration_result.status);
    if (calibration_result.field_name[0] != '\0')
    {
      doc["field"] = calibration_result.field_name;
    }
    doc["message"] = calibration_result.message;
    setJointStateJson(doc.createNestedObject("jointState"), motion_result.joint_state);
    logResult("[REST] Motion request rejected by calibration");
    sendJson(500, jsonBody(doc));
    return;
  }

  if (!servo_driver_.isInitialized())
  {
    logResult("[REST] Motion request rejected because PCA9685 is not initialized");
    RestJsonDocument doc;
    doc["status"] = "hardware_not_initialized";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["mode"] = "task_space_ik";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] =
        "PCA9685 servo driver is not initialized; check the boot log or call POST /api/servo-driver/init for "
        "diagnostics.";
    setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
    setOffsetTargetPoseJson(doc.createNestedObject("offsetTargetPose"), motion_result.offset_target_pose);
    setJointStateJson(doc.createNestedObject("jointState"), motion_result.joint_state);
    setMotionPlanSummaryJson(doc.createNestedObject("motionPlan"), motion_result.motion_plan);
    setJointPwmStateJson(doc.createNestedObject("jointPwmState"), calibration_result.joint_pwm_state);
    sendJson(503, jsonBody(doc));
    return;
  }

  startMotionPlan(motion_result.motion_plan, motion_result.joint_state);
  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  logResult("[REST] Motion request solved, planned and started on hardware");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "task_space_ik";
  doc["hardware"] = toString(ApiCapabilityStatus::Available);
  doc["execution"] = hasActiveMotionPlan() ? "motion_plan_active" : "motion_plan_completed";
  setTargetPoseJson(doc.createNestedObject("targetPose"), parsed.target_pose);
  setOffsetTargetPoseJson(doc.createNestedObject("offsetTargetPose"), motion_result.offset_target_pose);
  setJointStateJson(doc.createNestedObject("jointState"), motion_result.joint_state);
  setMotionPlanSummaryJson(doc.createNestedObject("motionPlan"), motion_result.motion_plan);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleSequenceStartRequest()
{
  logRequest("POST", kSequenceStartPath);

  const auto body_arg = server_.arg("plain");
  static SequenceDefinition sequence;
  const auto parsed = parseSequenceDefinitionRequestJson(body_arg.c_str(), sequence);

  if (!parsed.ok)
  {
    auto &doc = sequenceResponseJsonDocument();
    doc["status"] = "rejected";
    doc["code"] = toString(parsed.code);
    if (parsed.field_name[0] != '\0')
    {
      doc["field"] = parsed.field_name;
    }
    doc["message"] = parsed.message;
    logResult("[REST] Sequence request rejected by JSON parser");
    sendJson(400, jsonBody(doc));
    return;
  }

  if (hasActiveMotionPlan() || run_engine_.isActive())
  {
    logResult("[REST] Sequence request rejected because motion is already active");
    auto &doc = sequenceResponseJsonDocument();
    doc["status"] = "busy";
    doc["code"] = toString(ApiResultCode::SequenceBusy);
    doc["message"] = "A motion plan or sequence is already active.";
    setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
    sendJson(409, jsonBody(doc));
    return;
  }

  if (!servo_driver_.isInitialized())
  {
    logResult("[REST] Sequence request rejected because PCA9685 is not initialized");
    auto &doc = sequenceResponseJsonDocument();
    doc["status"] = "hardware_not_initialized";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    doc["message"] =
        "PCA9685 servo driver is not initialized; check the boot log or call POST /api/servo-driver/init for "
        "diagnostics.";
    sendJson(503, jsonBody(doc));
    return;
  }

  const auto run_result = run_engine_.start(sequence, current_joint_state_, millis());
  if (run_result.has_led_step)
  {
    queueLedStep(run_result.led_step);
  }

  if (!run_result.has_motion_plan && !run_result.has_led_step && !run_engine_.isActive())
  {
    const auto &sequence_state = run_engine_.state();
    auto &doc = sequenceResponseJsonDocument();
    doc["status"] = sequence_state.status == SequenceRunStatus::Completed ? "completed" : "rejected";
    doc["code"] = sequence_state.status == SequenceRunStatus::Completed ? toString(ApiResultCode::Ok)
                                                                        : toString(ApiResultCode::KinematicsFailure);
    doc["motionStatus"] =
        run_result.motion_result != nullptr ? orchestration::toString(run_result.motion_result->status) : "accepted";
    doc["message"] = sequence_state.message;
    setSequenceStateJson(doc.createNestedObject("sequence"), sequence_state, millis());
    logResult("[REST] Sequence request rejected by motion planning");
    sendJson(sequence_state.status == SequenceRunStatus::Completed ? 202 : 422, jsonBody(doc));
    return;
  }

  if (run_result.has_motion_plan)
  {
    startMotionPlan(*run_result.motion_plan, run_result.target_joint_state);
  }
  current_joint_pwm_state_ = servo_driver_.jointPwmState();
  logResult("[REST] Sequence started");

  auto &doc = sequenceResponseJsonDocument();
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["execution"] = hasActiveMotionPlan() ? "motion_plan_active" : "motion_plan_completed";
  setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
  if (run_result.has_motion_plan)
  {
    setTargetPoseJson(doc.createNestedObject("targetPose"), run_result.motion_result->target_pose);
    setJointStateJson(doc.createNestedObject("jointState"), run_result.target_joint_state);
    setMotionPlanSummaryJson(doc.createNestedObject("motionPlan"), *run_result.motion_plan);
  }
  if (run_result.has_led_step)
  {
    auto led = doc.createNestedObject("led");
    led["hasColor"] = run_result.led_step.has_status_color || run_result.led_step.has_rgb_color;
    led["mode"] = hardware::toString(run_result.led_step.mode);
    led["intervalMs"] = run_result.led_step.interval_ms;
  }
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleSequenceStopRequest()
{
  logRequest("POST", kSequenceStopPath);

  run_engine_.stop();
  motion_plan_active_ = false;
  logResult("[REST] Sequence stopped");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleSequenceStatus()
{
  logRequest("GET", kSequenceStatusPath);

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["motionPlanActive"] = hasActiveMotionPlan();
  doc["motionPlanSampleIndex"] = active_motion_sample_index_;
  doc["motionPlanSampleCount"] = active_motion_plan_.sample_count;
  setSequenceStateJson(doc.createNestedObject("sequence"), run_engine_.state(), millis());
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleControllerConnectRequest()
{
  logRequest("POST", kControllerConnectPath);

  controller_driver_.requestPairing(millis());
  logResult("[REST] Controller pairing requested");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "controller_jog";
  doc["readOnly"] = false;
  setControllerStateJson(doc.createNestedObject("controller"), controller_driver_.state(), controller_handler_.state(),
                         millis());
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleControllerDisconnectRequest()
{
  logRequest("POST", kControllerDisconnectPath);

  controller_driver_.disconnect(millis());
  logResult("[REST] Controller disconnected");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "controller_jog";
  doc["readOnly"] = false;
  setControllerStateJson(doc.createNestedObject("controller"), controller_driver_.state(), controller_handler_.state(),
                         millis());
  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleControllerStatus()
{
  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "controller_jog";
  doc["readOnly"] = false;
  setControllerStateJson(doc.createNestedObject("controller"), controller_driver_.state(), controller_handler_.state(),
                         millis());
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  if (!setReportedTargetPoseJson(doc.createNestedObject("targetPose"), current_joint_state_))
  {
    doc.remove("targetPose");
  }
  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleControllerDebug()
{
  logRequest("GET", kControllerDebugPath);

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "controller_jog";
  doc["readOnly"] = false;
  doc["motionOutput"] = "enabled_when_connected_and_idle";
  setControllerStateJson(doc.createNestedObject("controller"), controller_driver_.state(), controller_handler_.state(),
                         millis());
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  if (!setReportedTargetPoseJson(doc.createNestedObject("targetPose"), current_joint_state_))
  {
    doc.remove("targetPose");
  }
  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleCorsPreflight()
{
  logRequest("OPTIONS", server_.uri().c_str());
  sendCorsHeaders();
  server_.send(204);
}

void RestApiServer::handleRoot()
{
  logRequest("GET", "/");
  if (!serveStaticAsset("/dilbert.html"))
  {
    handleNotFound();
  }
}

void RestApiServer::handleNotFound()
{
  if (server_.method() == HTTP_GET && serveStaticAsset(server_.uri()))
  {
    return;
  }

  logRequest(server_.method() == HTTP_POST ? "POST" : "HTTP", server_.uri().c_str());

  RestJsonDocument doc;
  doc["status"] = "not_found";
  doc["code"] = toString(ApiResultCode::UnknownRoute);
  doc["path"] = server_.uri();

  sendJson(404, jsonBody(doc));
}

bool RestApiServer::serveStaticAsset(const String &requested_path)
{
  const auto query_start = requested_path.indexOf('?');
  const auto path = query_start >= 0 ? requested_path.substring(0, query_start) : requested_path;
  if (!path.startsWith("/") || path.indexOf("..") >= 0 || path.indexOf('\\') >= 0)
  {
    return false;
  }

  auto file = asset_file_system_.open(path, FILE_READ);
  if (!file || file.isDirectory())
  {
    return false;
  }

  const char *content_type = "application/octet-stream";
  if (path.endsWith(".html"))
  {
    content_type = "text/html";
  }
  else if (path.endsWith(".css"))
  {
    content_type = "text/css";
  }
  else if (path.endsWith(".js"))
  {
    content_type = "application/javascript";
  }
  else if (path.endsWith(".json"))
  {
    content_type = "application/json";
  }
  else if (path.endsWith(".svg"))
  {
    content_type = "image/svg+xml";
  }
  else if (path.endsWith(".png"))
  {
    content_type = "image/png";
  }
  else if (path.endsWith(".ico"))
  {
    content_type = "image/x-icon";
  }

  const auto expected_size = file.size();
  server_.sendHeader("Cache-Control", path.endsWith(".html") ? "no-store" : "public, max-age=31536000, immutable");
  server_.setContentLength(expected_size);
  server_.send(200, content_type, "");

  std::array<uint8_t, kStaticAssetBufferSize> buffer{};
  std::size_t transferred_size = 0U;
  bool transfer_failed = false;
  while (file.available() > 0)
  {
    const auto read_size = file.read(buffer.data(), buffer.size());
    if (read_size == 0U || !writeStaticAssetBytes(buffer.data(), read_size))
    {
      transfer_failed = true;
      break;
    }
    transferred_size += read_size;
  }
  file.close();

  if (transfer_failed || transferred_size != expected_size)
  {
    char message[128]{};
    std::snprintf(message, sizeof(message), "[REST] Static asset transfer incomplete: %s sent=%u expected=%u",
                  path.c_str(), static_cast<unsigned>(transferred_size), static_cast<unsigned>(expected_size));
    logger_.println(message);
  }
  return true;
}

bool RestApiServer::writeStaticAssetBytes(const uint8_t *data, std::size_t size)
{
  const auto socket_fd = server_.client().fd();
  if (socket_fd < 0)
  {
    return false;
  }

  std::size_t offset = 0U;
  const auto started_at_ms = millis();
  while (offset < size)
  {
    const auto written_size = send(socket_fd, data + offset, size - offset, MSG_DONTWAIT);
    if (written_size > 0)
    {
      offset += static_cast<std::size_t>(written_size);
      status_led_.updateOutput(millis());
      allowNetworkTaskToTransmit();
      continue;
    }

    if (written_size == 0 || (errno != EAGAIN && errno != EWOULDBLOCK) ||
        millis() - started_at_ms >= kStaticAssetWriteTimeoutMs)
    {
      return false;
    }
    allowNetworkTaskToTransmit();
  }
  return true;
}

void RestApiServer::logRequest(const char *method, const char *path) const
{
  logger_.print("[REST] ");
  logger_.print(method);
  logger_.print(" ");
  logger_.println(path);
}

void RestApiServer::logResult(const char *message) const
{
  logger_.println(message);
}

void RestApiServer::sendJson(int status_code, const String &body)
{
  sendCorsHeaders();
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status_code, "application/json", body);
}

void RestApiServer::sendCorsHeaders()
{
  server_.sendHeader("Access-Control-Allow-Origin", "*");
  server_.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server_.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

}  // namespace application
