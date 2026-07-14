// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

#include "application/ApiJson.h"

namespace application {

namespace {

void appendJsonEscaped(String &body, const String &value) {
  for (std::size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if (c == '"' || c == '\\') {
      body += '\\';
    }
    body += c;
  }
}

void appendJointStateJson(String &body, const common::JointState &state) {
  body += "{\"d_deg\":";
  body += String(state.d_deg, 3);
  body += ",\"s_deg\":";
  body += String(state.s_deg, 3);
  body += ",\"e_deg\":";
  body += String(state.e_deg, 3);
  body += ",\"hp_deg\":";
  body += String(state.hp_deg, 3);
  body += ",\"hr_deg\":";
  body += String(state.hr_deg, 3);
  body += ",\"g_pct\":";
  body += String(state.g_pct, 3);
  body += "}";
}

void appendJointPwmStateJson(String &body, const common::JointPwmState &state) {
  body += "{\"d_pwm\":";
  body += state.d_pwm;
  body += ",\"s_pwm\":";
  body += state.s_pwm;
  body += ",\"e_pwm\":";
  body += state.e_pwm;
  body += ",\"hp_pwm\":";
  body += state.hp_pwm;
  body += ",\"hr_pwm\":";
  body += state.hr_pwm;
  body += ",\"g_pwm\":";
  body += state.g_pwm;
  body += "}";
}

void appendHardwareDriverResultJson(String &body,
                                    const hardware::HardwareDriverResult &result) {
  body += "{\"status\":\"";
  body += hardware::toString(result.status);
  body += "\",\"message\":\"";
  appendJsonEscaped(body, result.message);
  body += "\"}";
}

}  // namespace

RestApiServer::RestApiServer(WebServer &server)
    : server_(server),
      servo_driver_(nullptr),
      logger_(nullptr),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState()) {}

RestApiServer::RestApiServer(WebServer &server,
                             hardware::Pca9685ServoDriver &servo_driver)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(nullptr),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState()) {}

RestApiServer::RestApiServer(WebServer &server,
                             hardware::Pca9685ServoDriver &servo_driver,
                             const hardware::SerialLogger &logger)
    : server_(server),
      servo_driver_(&servo_driver),
      logger_(&logger),
      current_joint_state_(common::initialJointState()),
      current_joint_pwm_state_(common::initialJointPwmState()) {}

void RestApiServer::begin() {
  server_.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/joint-state", HTTP_GET, [this]() { handleJointState(); });
  server_.on("/api/joint-motion", HTTP_POST,
             [this]() { handleJointMotionRequest(); });
  server_.on("/api/joint-pwm-state", HTTP_GET,
             [this]() { handleJointPwmState(); });
  server_.on("/api/joint-pwm-motion", HTTP_POST,
             [this]() { handleJointPwmMotionRequest(); });
  server_.on("/api/motion", HTTP_POST, [this]() { handleMotionRequest(); });
  server_.on("/favicon.ico", HTTP_GET, [this]() { handleFavicon(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
}

void RestApiServer::handleClient() {
  server_.handleClient();
}

void RestApiServer::handleHealth() {
  logRequest("GET", "/api/health");

  String body;
  body.reserve(160);
  body += "{\"service\":\"";
  body += kApiName;
  body += "\",\"apiVersion\":\"";
  body += kApiVersion;
  body += "\",\"status\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"orchestrator\":\"";
  body += toString(ApiCapabilityStatus::NotAvailable);
  body += "\",\"uptimeMs\":";
  body += millis();
  body += "}";

  sendJson(200, body);
}

void RestApiServer::handleStatus() {
  logRequest("GET", "/api/status");

  String body;
  body.reserve(192);
  body += "{\"restApi\":\"";
  body += toString(ApiCapabilityStatus::Available);
  body += "\",\"orchestrator\":\"";
  body += toString(ApiCapabilityStatus::NotAvailable);
  body += "\",\"jointStateEndpoint\":\"";
  body += toString(ApiCapabilityStatus::Available);
  body += "\",\"jointMotionEndpoint\":\"";
  body += toString(ApiCapabilityStatus::Available);
  body += "\",\"jointPwmStateEndpoint\":\"";
  body += toString(ApiCapabilityStatus::Available);
  body += "\",\"jointPwmMotionEndpoint\":\"";
  body += toString(ApiCapabilityStatus::Available);
  body += "\",\"jointPwmHardwareOutput\":\"";
  body += toString(servo_driver_ != nullptr ? ApiCapabilityStatus::Available
                                            : ApiCapabilityStatus::NotAvailable);
  body += "\",\"motionEndpoint\":\"reserved\",\"uptimeMs\":";
  body += millis();
  body += "}";

  sendJson(200, body);
}

void RestApiServer::handleJointState() {
  logRequest("GET", "/api/joint-state");

  String body;
  body.reserve(256);
  body += "{\"status\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"code\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"source\":\"assumed_low_level_state\",\"jointState\":";
  appendJointStateJson(body, current_joint_state_);
  body += "}";

  sendJson(200, body);
}

void RestApiServer::handleJointMotionRequest() {
  logRequest("POST", "/api/joint-motion");

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseJointMotionRequestJson(body_arg.c_str());

  String body;
  body.reserve(384);

  if (!parsed.ok) {
    body += "{\"status\":\"rejected\",\"code\":\"";
    body += toString(parsed.code);
    body += "\"";
    if (parsed.field_name[0] != '\0') {
      body += ",\"field\":\"";
      appendJsonEscaped(body, parsed.field_name);
      body += "\"";
    }
    body += ",\"message\":\"";
    appendJsonEscaped(body, parsed.message);
    body += "\"}";
    logResult("[REST] Joint motion request rejected");
    sendJson(400, body);
    return;
  }

  current_joint_state_ = parsed.joint_state;
  logResult("[REST] Joint motion request accepted");

  body += "{\"status\":\"accepted\",\"code\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"mode\":\"joint_space_direct\",\"hardware\":\"";
  body += toString(ApiCapabilityStatus::NotAvailable);
  body += "\",\"jointState\":";
  appendJointStateJson(body, current_joint_state_);
  body += ",\"message\":\"Joint state accepted as assumed low-level target; "
          "hardware output is not connected yet.\"}";

  sendJson(202, body);
}

void RestApiServer::handleJointPwmState() {
  logRequest("GET", "/api/joint-pwm-state");

  String body;
  body.reserve(256);
  body += "{\"status\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"code\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"source\":\"assumed_low_level_pwm_state\",\"jointPwmState\":";
  appendJointPwmStateJson(body, current_joint_pwm_state_);
  body += "}";

  sendJson(200, body);
}

void RestApiServer::handleJointPwmMotionRequest() {
  logRequest("POST", "/api/joint-pwm-motion");

  const auto body_arg = server_.arg("plain");
  const auto parsed = parseJointPwmMotionRequestJson(body_arg.c_str());

  String body;
  body.reserve(384);

  if (!parsed.ok) {
    body += "{\"status\":\"rejected\",\"code\":\"";
    body += toString(parsed.code);
    body += "\"";
    if (parsed.field_name[0] != '\0') {
      body += ",\"field\":\"";
      appendJsonEscaped(body, parsed.field_name);
      body += "\"";
    }
    body += ",\"message\":\"";
    appendJsonEscaped(body, parsed.message);
    body += "\"}";
    logResult("[REST] Joint PWM request rejected");
    sendJson(400, body);
    return;
  }

  if (servo_driver_ != nullptr) {
    if (!servo_driver_->isInitialized()) {
      const auto begin_result = servo_driver_->begin();
      if (begin_result.status != hardware::HardwareDriverStatus::Ok) {
        logResult("[REST] PCA9685 begin failed");
        body += "{\"status\":\"hardware_failed\",\"code\":\"";
        body += toString(ApiResultCode::HardwareDriverFailure);
        body += "\",\"mode\":\"joint_pwm_direct\",\"hardware\":\"";
        body += toString(ApiCapabilityStatus::Available);
        body += "\",\"driver\":";
        appendHardwareDriverResultJson(body, begin_result);
        body += "}";
        sendJson(503, body);
        return;
      }
    }

    const auto write_result = servo_driver_->write(parsed.joint_pwm_state);
    if (write_result.status != hardware::HardwareDriverStatus::Ok) {
      logResult("[REST] PCA9685 write failed");
      body += "{\"status\":\"hardware_failed\",\"code\":\"";
      body += toString(ApiResultCode::HardwareDriverFailure);
      body += "\",\"mode\":\"joint_pwm_direct\",\"hardware\":\"";
      body += toString(ApiCapabilityStatus::Available);
      body += "\",\"driver\":";
      appendHardwareDriverResultJson(body, write_result);
      body += "}";
      sendJson(503, body);
      return;
    }

    current_joint_pwm_state_ = parsed.joint_pwm_state;
    logResult("[REST] Joint PWM request written to hardware");

    body += "{\"status\":\"accepted\",\"code\":\"";
    body += toString(ApiResultCode::Ok);
    body += "\",\"mode\":\"joint_pwm_direct\",\"hardware\":\"";
    body += toString(ApiCapabilityStatus::Available);
    body += "\",\"driver\":";
    appendHardwareDriverResultJson(body, write_result);
    body += ",\"jointPwmState\":";
    appendJointPwmStateJson(body, current_joint_pwm_state_);
    body += "}";
    sendJson(202, body);
    return;
  }

  current_joint_pwm_state_ = parsed.joint_pwm_state;
  logResult("[REST] Joint PWM request accepted without hardware output");

  body += "{\"status\":\"accepted\",\"code\":\"";
  body += toString(ApiResultCode::Ok);
  body += "\",\"mode\":\"joint_pwm_direct\",\"hardware\":\"";
  body += toString(ApiCapabilityStatus::NotAvailable);
  body += "\",\"jointPwmState\":";
  appendJointPwmStateJson(body, current_joint_pwm_state_);
  body += ",\"message\":\"Joint PWM state accepted as assumed low-level "
          "target; hardware output is not connected yet.\"}";

  sendJson(202, body);
}

void RestApiServer::handleMotionRequest() {
  logRequest("POST", "/api/motion");

  String body;
  body.reserve(192);
  body += "{\"status\":\"not_implemented\",\"code\":\"";
  body += toString(ApiResultCode::OrchestratorUnavailable);
  body += "\",\"message\":\"Motion endpoint is reserved for Orchestrator integration.\"}";

  sendJson(501, body);
}

void RestApiServer::handleFavicon() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(204);
}

void RestApiServer::handleNotFound() {
  logRequest(server_.method() == HTTP_POST ? "POST" : "HTTP", server_.uri().c_str());

  String body;
  body.reserve(128);
  body += "{\"status\":\"not_found\",\"code\":\"";
  body += toString(ApiResultCode::UnknownRoute);
  body += "\",\"path\":\"";
  body += server_.uri();
  body += "\"}";

  sendJson(404, body);
}

void RestApiServer::logRequest(const char *method, const char *path) const {
  if (logger_ == nullptr) {
    return;
  }

  logger_->print("[REST] ");
  logger_->print(method);
  logger_->print(" ");
  logger_->println(path);
}

void RestApiServer::logResult(const char *message) const {
  if (logger_ == nullptr) {
    return;
  }

  logger_->println(message);
}

void RestApiServer::sendJson(int status_code, const String &body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status_code, "application/json", body);
}

}  // namespace application
