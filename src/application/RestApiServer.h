// Minimal ESP32 REST interface for bring-up and later Orchestrator integration.

#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "application/ApiContracts.h"

namespace application {

class RestApiServer {
 public:
  explicit RestApiServer(WebServer &server);

  void begin();
  void handleClient();

 private:
  void handleHealth();
  void handleStatus();
  void handleMotionRequest();
  void handleNotFound();
  void sendJson(int status_code, const String &body);

  WebServer &server_;
};

}  // namespace application
