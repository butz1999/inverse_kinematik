// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

#include <ArduinoJson.h>

#include "application/ApiJson.h"

namespace application
{

namespace
{

constexpr std::size_t kRestJsonCapacity = 768;
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

void setJointPwmStateJson(JsonObject object, const common::JointPwmState &state)
{
  object["d_pwm"] = state.d_pwm;
  object["s_pwm"] = state.s_pwm;
  object["e_pwm"] = state.e_pwm;
  object["hp_pwm"] = state.hp_pwm;
  object["hr_pwm"] = state.hr_pwm;
  object["g_pwm"] = state.g_pwm;
}

void setHardwareDriverResultJson(JsonObject object, const hardware::HardwareDriverResult &result)
{
  object["status"] = hardware::toString(result.status);
  object["message"] = result.message;
}

}  // namespace

RestApiServer::RestApiServer(WebServer &server)
    : server_(server),
      servo_driver_(nullptr),
      logger_(nullptr),
      hardware_calibration_(hardware::defaultHardwareCalibration()),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(hardware_calibration_.initial_pwm_state)
{
}

RestApiServer::RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(nullptr),
      hardware_calibration_(hardware::defaultHardwareCalibration()),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(hardware_calibration_.initial_pwm_state)
{
}

RestApiServer::RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver,
                             const hardware::SerialLogger &logger)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(&logger),
      hardware_calibration_(hardware::defaultHardwareCalibration()),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(hardware_calibration_.initial_pwm_state)
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
}

void RestApiServer::handleHealth()
{
  logRequest("GET", kHealthPath);

  RestJsonDocument doc;
  doc["service"] = kApiName;
  doc["apiVersion"] = kApiVersion;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["orchestrator"] = toString(ApiCapabilityStatus::NotAvailable);
  doc["uptimeMs"] = millis();

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleStatus()
{
  logRequest("GET", kStatusPath);

  RestJsonDocument doc;
  doc["restApi"] = toString(ApiCapabilityStatus::Available);
  doc["orchestrator"] = toString(ApiCapabilityStatus::NotAvailable);
  doc["jointStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["servoDriverInitEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmHardwareOutput"] =
      toString(servo_driver_ != nullptr ? ApiCapabilityStatus::Available : ApiCapabilityStatus::NotAvailable);
  doc["jointPwmHardwareInitialized"] = servo_driver_ != nullptr && servo_driver_->isInitialized();
  doc["motionEndpoint"] = "reserved";
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

  if (servo_driver_ != nullptr)
  {
    if (!servo_driver_->isInitialized())
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

    const auto write_result = servo_driver_->write(calibration_result.joint_pwm_state);
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
    current_joint_pwm_state_ = calibration_result.joint_pwm_state;
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
    return;
  }

  current_joint_state_ = parsed.joint_state;
  current_joint_pwm_state_ = calibration_result.joint_pwm_state;
  logResult("[REST] Joint motion request mapped without hardware output");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "joint_space_calibrated";
  doc["hardware"] = toString(ApiCapabilityStatus::NotAvailable);
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  doc["message"] =
      "Joint state accepted and mapped through default hardware calibration; hardware output is not connected yet.";

  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleJointPwmState()
{
  logRequest("GET", kJointPwmStatePath);

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["source"] = "assumed_low_level_pwm_state";
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleServoDriverInitRequest()
{
  logRequest("POST", kServoDriverInitPath);

  if (servo_driver_ == nullptr)
  {
    RestJsonDocument doc;
    doc["status"] = "hardware_not_available";
    doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
    doc["hardware"] = toString(ApiCapabilityStatus::NotAvailable);
    doc["message"] = "PCA9685 servo driver is not connected.";
    sendJson(503, jsonBody(doc));
    return;
  }

  const auto init_result = servo_driver_->init();
  if (init_result.status != hardware::HardwareDriverStatus::Ok &&
      init_result.status != hardware::HardwareDriverStatus::IsInitialized)
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

  current_joint_pwm_state_ = hardware_calibration_.initial_pwm_state;
  logResult("[REST] PCA9685 initialized");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["hardware"] = toString(ApiCapabilityStatus::Available);
  setHardwareDriverResultJson(doc.createNestedObject("driver"), init_result);
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

  if (servo_driver_ != nullptr)
  {
    if (!servo_driver_->isInitialized())
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

    const auto write_result = servo_driver_->write(parsed.joint_pwm_state);
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

    current_joint_pwm_state_ = parsed.joint_pwm_state;
    logResult("[REST] Joint PWM request written to hardware");

    RestJsonDocument doc;
    doc["status"] = "accepted";
    doc["code"] = toString(ApiResultCode::Ok);
    doc["mode"] = "joint_pwm_direct";
    doc["hardware"] = toString(ApiCapabilityStatus::Available);
    setHardwareDriverResultJson(doc.createNestedObject("driver"), write_result);
    setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
    sendJson(202, jsonBody(doc));
    return;
  }

  current_joint_pwm_state_ = parsed.joint_pwm_state;
  logResult("[REST] Joint PWM request accepted without hardware output");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "joint_pwm_direct";
  doc["hardware"] = toString(ApiCapabilityStatus::NotAvailable);
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);
  doc["message"] = "Joint PWM state accepted as assumed low-level target; hardware output is not connected yet.";

  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleMotionRequest()
{
  logRequest("POST", kMotionPath);

  RestJsonDocument doc;
  doc["status"] = "not_implemented";
  doc["code"] = toString(ApiResultCode::OrchestratorUnavailable);
  doc["message"] = "Motion endpoint is reserved for Orchestrator integration.";

  sendJson(501, jsonBody(doc));
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
  if (logger_ == nullptr)
  {
    return;
  }

  logger_->print("[REST] ");
  logger_->print(method);
  logger_->print(" ");
  logger_->println(path);
}

void RestApiServer::logResult(const char *message) const
{
  if (logger_ == nullptr)
  {
    return;
  }

  logger_->println(message);
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
