// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

#include <ArduinoJson.h>

#include "application/ApiJson.h"
#include "orchestration/MotionOrchestrator.h"
#include "robotics/Kinematics.h"
#include "robotics/Validation.h"

namespace application
{

namespace
{

constexpr std::size_t kRestJsonCapacity = 1536;
using RestJsonDocument = StaticJsonDocument<kRestJsonCapacity>;

template <typename JsonDocumentType>
String jsonBody(const JsonDocumentType &doc)
{
  String body;
  body.reserve(measureJson(doc));
  serializeJson(doc, body);
  return body;
}

void setJointStateJson(JsonObject object, const common::JointState &state)
{
  object["d_deg"] = serialized(String(state.d_deg, 3));
  object["s_deg"] = serialized(String(state.s_deg, 3));
  object["e_deg"] = serialized(String(state.e_deg, 3));
  object["hp_deg"] = serialized(String(state.hp_deg, 3));
  object["hr_deg"] = serialized(String(state.hr_deg, 3));
  object["g_pct"] = serialized(String(state.g_pct, 3));
}

void setTargetPoseJson(JsonObject object, const common::TargetPose &pose)
{
  object["x_mm"] = serialized(String(pose.x_mm, 3));
  object["y_mm"] = serialized(String(pose.y_mm, 3));
  object["z_mm"] = serialized(String(pose.z_mm, 3));
  object["p_deg"] = serialized(String(pose.p_deg, 3));
  object["r_deg"] = serialized(String(pose.r_deg, 3));
  object["g_pct"] = serialized(String(pose.g_pct, 3));
}

void setOffsetTargetPoseJson(JsonObject object, const robotics::OffsetTargetPose &pose)
{
  object["x_mm"] = serialized(String(pose.x_mm, 3));
  object["y_mm"] = serialized(String(pose.y_mm, 3));
  object["z_mm"] = serialized(String(pose.z_mm, 3));
  object["p_deg"] = serialized(String(pose.p_deg, 3));
  object["r_deg"] = serialized(String(pose.r_deg, 3));
  object["g_pct"] = serialized(String(pose.g_pct, 3));
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
  object["targetVelocityDegS"] = serialized(String(plan.profile.target_velocity_deg_s, 3));
}

void setHardwareDriverResultJson(JsonObject object, const hardware::HardwareDriverResult &result)
{
  object["status"] = hardware::toString(result.status);
  object["message"] = result.message;
}

}  // namespace

RestApiServer::RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver,
                             const hardware::Logger &logger)
    : server_(server),
      servo_driver_(servo_driver),
      logger_(logger),
      hardware_calibration_(hardware::defaultHardwareCalibration()),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(hardware_calibration_.initial_pwm_state),
      active_motion_plan_{common::defaultMotionProfile(), 0U, {}, 0U},
      active_motion_target_joint_state_(common::initialJointState()),
      active_motion_sample_index_(0U),
      active_motion_started_ms_(0UL),
      motion_plan_active_(false)
{
}

void RestApiServer::begin()
{
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
  server_.on("/favicon.ico", HTTP_GET,
             [this]()
             {
               handleFavicon();
             });
  server_.onNotFound(
      [this]()
      {
        handleNotFound();
      });
  server_.begin();
}

void RestApiServer::handleClient()
{
  server_.handleClient();
  serviceActiveMotionPlan();
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
  doc["motionPlanActive"] = hasActiveMotionPlan();
  doc["motionPlanSampleIndex"] = active_motion_sample_index_;
  doc["motionPlanSampleCount"] = active_motion_plan_.sample_count;
  doc["uptimeMs"] = millis();

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

  current_joint_state_ = common::initialJointState();
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

  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const orchestration::MotionRequest motion_request{parsed.target_pose, parsed.motion_profile};
  orchestrator.processMotionRequestInto(motion_request, current_joint_state_, motion_result_scratch_);
  const auto &motion_result = motion_result_scratch_;
  if (!motion_result.ok && motion_result.status == orchestration::MotionStatus::InvalidTargetPose)
  {
    RestJsonDocument doc;
    doc["status"] = "rejected";
    doc["code"] = motion_result.target_validation_status == robotics::ValidationStatus::TargetPoseOutOfWorkspace
                      ? toString(ApiResultCode::TargetPoseOutOfWorkspace)
                      : toString(ApiResultCode::InvalidTargetPose);
    if (!motion_result.field_name.empty())
    {
      doc["field"] = motion_result.field_name.c_str();
    }
    doc["message"] = motion_result.message.c_str();
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
    doc["message"] = motion_result.message.c_str();
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
    if (!motion_result.field_name.empty())
    {
      doc["field"] = motion_result.field_name.c_str();
    }
    doc["message"] = motion_result.message.c_str();
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
    doc["message"] = motion_result.message.c_str();
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

void RestApiServer::handleCorsPreflight()
{
  logRequest("OPTIONS", server_.uri().c_str());
  sendCorsHeaders();
  server_.send(204);
}

void RestApiServer::handleFavicon()
{
  sendCorsHeaders();
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(204);
}

void RestApiServer::handleNotFound()
{
  logRequest(server_.method() == HTTP_POST ? "POST" : "HTTP", server_.uri().c_str());

  RestJsonDocument doc;
  doc["status"] = "not_found";
  doc["code"] = toString(ApiResultCode::UnknownRoute);
  doc["path"] = server_.uri();

  sendJson(404, jsonBody(doc));
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
