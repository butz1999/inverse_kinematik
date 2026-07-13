// String conversions for externally visible API status values.

#include "application/ApiContracts.h"

namespace application {

const char *toString(ApiCapabilityStatus status) {
  switch (status) {
    case ApiCapabilityStatus::Available:
      return "available";
    case ApiCapabilityStatus::NotAvailable:
      return "not_available";
  }

  return "not_available";
}

const char *toString(ApiResultCode code) {
  switch (code) {
    case ApiResultCode::Ok:
      return "ok";
    case ApiResultCode::OrchestratorUnavailable:
      return "orchestrator_unavailable";
    case ApiResultCode::UnknownRoute:
      return "unknown_route";
  }

  return "unknown_route";
}

}  // namespace application
