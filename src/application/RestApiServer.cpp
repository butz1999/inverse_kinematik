// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

#include <ArduinoJson.h>

#include "application/ApiJson.h"

namespace application
{

namespace
{

constexpr std::size_t kRestJsonCapacity = 512;
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
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState())
{
}

RestApiServer::RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(nullptr),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState())
{
}

RestApiServer::RestApiServer(WebServer &server, hardware::Pca9685ServoDriver &servo_driver,
                             const hardware::SerialLogger &logger)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(&logger),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState())
{
}

void RestApiServer::begin()
{
  server_.on("/api/health", HTTP_GET,
             [this]()
             {
               handleHealth();
             });
  server_.on("/api/status", HTTP_GET,
             [this]()
             {
               handleStatus();
             });
  server_.on("/api/joint-state", HTTP_GET,
             [this]()
             {
               handleJointState();
             });
  server_.on("/api/joint-motion", HTTP_POST,
             [this]()
             {
               handleJointMotionRequest();
             });
  server_.on("/api/joint-pwm-state", HTTP_GET,
             [this]()
             {
               handleJointPwmState();
             });
  server_.on("/api/joint-pwm-motion", HTTP_POST,
             [this]()
             {
               handleJointPwmMotionRequest();
             });
  server_.on("/api/motion", HTTP_POST,
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
  logRequest("GET", "/api/health");

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
  logRequest("GET", "/api/status");

  RestJsonDocument doc;
  doc["restApi"] = toString(ApiCapabilityStatus::Available);
  doc["orchestrator"] = toString(ApiCapabilityStatus::NotAvailable);
  doc["jointStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmStateEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmMotionEndpoint"] = toString(ApiCapabilityStatus::Available);
  doc["jointPwmHardwareOutput"] =
      toString(servo_driver_ != nullptr ? ApiCapabilityStatus::Available : ApiCapabilityStatus::NotAvailable);
  doc["motionEndpoint"] = "reserved";
  doc["uptimeMs"] = millis();

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointState()
{
  logRequest("GET", "/api/joint-state");

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["source"] = "assumed_low_level_state";
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointMotionRequest()
{
  logRequest("POST", "/api/joint-motion");

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

  current_joint_state_ = parsed.joint_state;
  logResult("[REST] Joint motion request accepted");

  RestJsonDocument doc;
  doc["status"] = "accepted";
  doc["code"] = toString(ApiResultCode::Ok);
  doc["mode"] = "joint_space_direct";
  doc["hardware"] = toString(ApiCapabilityStatus::NotAvailable);
  setJointStateJson(doc.createNestedObject("jointState"), current_joint_state_);
  doc["message"] = "Joint state accepted as assumed low-level target; hardware output is not connected yet.";

  sendJson(202, jsonBody(doc));
}

void RestApiServer::handleJointPwmState()
{
  logRequest("GET", "/api/joint-pwm-state");

  RestJsonDocument doc;
  doc["status"] = toString(ApiResultCode::Ok);
  doc["code"] = toString(ApiResultCode::Ok);
  doc["source"] = "assumed_low_level_pwm_state";
  setJointPwmStateJson(doc.createNestedObject("jointPwmState"), current_joint_pwm_state_);

  sendJson(200, jsonBody(doc));
}

void RestApiServer::handleJointPwmMotionRequest()
{
  logRequest("POST", "/api/joint-pwm-motion");

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
      const auto begin_result = servo_driver_->begin();
      if (begin_result.status != hardware::HardwareDriverStatus::Ok)
      {
        logResult("[REST] PCA9685 begin failed");
        RestJsonDocument doc;
        doc["status"] = "hardware_failed";
        doc["code"] = toString(ApiResultCode::HardwareDriverFailure);
        doc["mode"] = "joint_pwm_direct";
        doc["hardware"] = toString(ApiCapabilityStatus::Available);
        setHardwareDriverResultJson(doc.createNestedObject("driver"), begin_result);
        sendJson(503, jsonBody(doc));
        return;
      }
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
  logRequest("POST", "/api/motion");

  RestJsonDocument doc;
  doc["status"] = "not_implemented";
  doc["code"] = toString(ApiResultCode::OrchestratorUnavailable);
  doc["message"] = "Motion endpoint is reserved for Orchestrator integration.";

  sendJson(501, jsonBody(doc));
}

void RestApiServer::handleFavicon()
{
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
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status_code, "application/json", body);
}

}  // namespace application
