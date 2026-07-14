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

}  // namespace

RestApiServer::RestApiServer(WebServer &server)
    : server_(server),
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
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
}

void RestApiServer::handleClient() {
  server_.handleClient();
}

void RestApiServer::handleHealth() {
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
  body += "\",\"motionEndpoint\":\"reserved\",\"uptimeMs\":";
  body += millis();
  body += "}";

  sendJson(200, body);
}

void RestApiServer::handleJointState() {
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
    sendJson(400, body);
    return;
  }

  current_joint_state_ = parsed.joint_state;

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
    sendJson(400, body);
    return;
  }

  current_joint_pwm_state_ = parsed.joint_pwm_state;

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
  String body;
  body.reserve(192);
  body += "{\"status\":\"not_implemented\",\"code\":\"";
  body += toString(ApiResultCode::OrchestratorUnavailable);
  body += "\",\"message\":\"Motion endpoint is reserved for Orchestrator integration.\"}";

  sendJson(501, body);
}

void RestApiServer::handleNotFound() {
  String body;
  body.reserve(128);
  body += "{\"status\":\"not_found\",\"code\":\"";
  body += toString(ApiResultCode::UnknownRoute);
  body += "\",\"path\":\"";
  body += server_.uri();
  body += "\"}";

  sendJson(404, body);
}

void RestApiServer::sendJson(int status_code, const String &body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status_code, "application/json", body);
}

}  // namespace application
