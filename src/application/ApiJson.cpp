// ArduinoJson-backed request parsing for the REST API.

#include "application/ApiJson.h"

#include <ArduinoJson.h>

namespace application {

namespace {

constexpr const char *kEmptyField = "";
constexpr const char *kRequestBodyRequired =
    "Request body must contain a JSON object.";
constexpr const char *kMalformedJson = "Request body must be valid JSON.";

JointMotionParseResult jointMotionError(ApiResultCode code,
                                        const char *field_name,
                                        const char *message) {
  return JointMotionParseResult{false, code, field_name, message,
                                common::initialJointState()};
}

JointPwmMotionParseResult jointPwmMotionError(ApiResultCode code,
                                              const char *field_name,
                                              const char *message) {
  return JointPwmMotionParseResult{false, code, field_name, message,
                                   common::initialJointPwmState()};
}

bool isMissingOrNonNumeric(JsonVariantConst value) {
  return value.isNull() || !value.is<float>();
}

bool isMissingOrNonInteger(JsonVariantConst value) {
  return value.isNull() || !value.is<long>();
}

}  // namespace

JointMotionParseResult parseJointMotionRequestJson(const char *body) {
  if (body == nullptr || body[0] == '\0') {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField,
                            kRequestBodyRequired);
  }

  StaticJsonDocument<384> doc;
  const auto error = deserializeJson(doc, body);
  if (error) {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField,
                            kMalformedJson);
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull()) {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField,
                            "Request body must be a JSON object.");
  }

  const char *fields[] = {"d_deg", "s_deg", "e_deg",
                          "hp_deg", "hr_deg", "g_pct"};
  for (std::size_t i = 0; i < common::kJointAxisCount; ++i) {
    if (isMissingOrNonNumeric(root[fields[i]])) {
      return jointMotionError(
          ApiResultCode::MissingField, fields[i],
          "Joint motion request is missing a numeric joint field.");
    }
  }

  common::JointState state{root["d_deg"].as<float>(), root["s_deg"].as<float>(),
                           root["e_deg"].as<float>(), root["hp_deg"].as<float>(),
                           root["hr_deg"].as<float>(),
                           root["g_pct"].as<float>()};

  if (!common::isFinite(state)) {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField,
                            "Joint values must be finite numbers.");
  }

  if (const auto *violation = common::findFirstLimitViolation(state)) {
    return jointMotionError(
        ApiResultCode::JointLimitViolation, violation->field_name,
        "Joint value is outside its configured low-level limit.");
  }

  return JointMotionParseResult{true, ApiResultCode::Ok, kEmptyField,
                                kEmptyField, state};
}

JointPwmMotionParseResult parseJointPwmMotionRequestJson(const char *body) {
  if (body == nullptr || body[0] == '\0') {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField,
                               kRequestBodyRequired);
  }

  StaticJsonDocument<384> doc;
  const auto error = deserializeJson(doc, body);
  if (error) {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField,
                               kMalformedJson);
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull()) {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField,
                               "Request body must be a JSON object.");
  }

  common::JointPwmState state = common::initialJointPwmState();
  const char *fields[] = {"d_pwm", "s_pwm", "e_pwm",
                          "hp_pwm", "hr_pwm", "g_pwm"};
  uint16_t *values[] = {&state.d_pwm, &state.s_pwm, &state.e_pwm,
                        &state.hp_pwm, &state.hr_pwm, &state.g_pwm};

  for (std::size_t i = 0; i < common::kJointPwmAxisCount; ++i) {
    const auto value = root[fields[i]];
    if (isMissingOrNonInteger(value)) {
      return jointPwmMotionError(
          ApiResultCode::MissingField, fields[i],
          "Joint PWM request is missing a numeric PWM field.");
    }

    const auto parsed_value = value.as<long>();
    if (parsed_value < common::kPca9685MinPwm ||
        parsed_value > common::kPca9685MaxPwm) {
      return jointPwmMotionError(
          ApiResultCode::JointPwmLimitViolation, fields[i],
          "PWM value is outside the PCA9685 12-bit range 0..4095.");
    }

    *values[i] = static_cast<uint16_t>(parsed_value);
  }

  if (const auto *violation = common::findFirstLimitViolation(state)) {
    return jointPwmMotionError(ApiResultCode::JointPwmLimitViolation,
                               violation->field_name,
                               "PWM value is outside its configured low-level "
                               "limit.");
  }

  return JointPwmMotionParseResult{true, ApiResultCode::Ok, kEmptyField,
                                   kEmptyField, state};
}

}  // namespace application
