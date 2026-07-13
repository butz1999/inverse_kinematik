// Provides small JSON endpoints before the robot control stack exists.

#include "application/RestApiServer.h"

namespace application {

RestApiServer::RestApiServer(WebServer &server) : server_(server) {}

void RestApiServer::begin() {
  server_.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
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
  body += "\",\"motionEndpoint\":\"placeholder\",\"uptimeMs\":";
  body += millis();
  body += "}";

  sendJson(200, body);
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
