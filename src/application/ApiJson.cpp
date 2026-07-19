// ArduinoJson-backed request parsing for the REST API.

#include "application/ApiJson.h"

#include <ArduinoJson.h>

#include <cstring>

namespace application
{

namespace
{

constexpr const char *kEmptyField = "";
constexpr const char *kRequestBodyRequired = "Request body must contain a JSON object.";
constexpr const char *kMalformedJson = "Request body must be valid JSON.";

JointMotionParseResult jointMotionError(ApiResultCode code, const char *field_name, const char *message)
{
  return JointMotionParseResult{false, code, field_name, message, common::initialJointState()};
}

JointPwmMotionParseResult jointPwmMotionError(ApiResultCode code, const char *field_name, const char *message)
{
  return JointPwmMotionParseResult{false, code, field_name, message, common::JointPwmState{}};
}

TargetPoseParseResult targetPoseError(ApiResultCode code, const char *field_name, const char *message)
{
  return TargetPoseParseResult{false, code, field_name, message, common::initialTargetPose(),
                               common::defaultMotionProfile()};
}

bool isMissingOrNonNumeric(JsonVariantConst value)
{
  return value.isNull() || !value.is<float>();
}

bool isMissingOrNonInteger(JsonVariantConst value)
{
  return value.isNull() || !value.is<long>();
}

bool parseMotionProfileType(const char *value, common::MotionProfileType &type)
{
  if (value == nullptr)
  {
    return false;
  }

  if (std::strcmp(value, "constant_velocity") == 0)
  {
    type = common::MotionProfileType::ConstantVelocity;
    return true;
  }
  if (std::strcmp(value, "constant_acceleration") == 0)
  {
    type = common::MotionProfileType::ConstantAcceleration;
    return true;
  }
  if (std::strcmp(value, "smooth_start_stop") == 0)
  {
    type = common::MotionProfileType::SmoothStartStop;
    return true;
  }

  return false;
}

}  // namespace

JointMotionParseResult parseJointMotionRequestJson(const char *body)
{
  if (body == nullptr || body[0] == '\0')
  {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField, kRequestBodyRequired);
  }

  StaticJsonDocument<512> doc;
  const auto error = deserializeJson(doc, body);
  if (error)
  {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField, kMalformedJson);
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull())
  {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField, "Request body must be a JSON object.");
  }

  const char *fields[] = {"d_deg", "s_deg", "e_deg", "hp_deg", "hr_deg", "g_pct"};
  for (std::size_t i = 0; i < common::kJointAxisCount; ++i)
  {
    if (isMissingOrNonNumeric(root[fields[i]]))
    {
      return jointMotionError(ApiResultCode::MissingField, fields[i],
                              "Joint motion request is missing a numeric joint field.");
    }
  }

  common::JointState state{root["d_deg"].as<float>(),  root["s_deg"].as<float>(),  root["e_deg"].as<float>(),
                           root["hp_deg"].as<float>(), root["hr_deg"].as<float>(), root["g_pct"].as<float>()};

  if (!common::isFinite(state))
  {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField, "Joint values must be finite numbers.");
  }

  if (const auto *violation = common::findFirstLimitViolation(state))
  {
    return jointMotionError(ApiResultCode::JointLimitViolation, violation->field_name,
                            "Joint value is outside its configured low-level limit.");
  }

  return JointMotionParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField, state};
}

JointPwmMotionParseResult parseJointPwmMotionRequestJson(const char *body)
{
  if (body == nullptr || body[0] == '\0')
  {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField, kRequestBodyRequired);
  }

  StaticJsonDocument<512> doc;
  const auto error = deserializeJson(doc, body);
  if (error)
  {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField, kMalformedJson);
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull())
  {
    return jointPwmMotionError(ApiResultCode::InvalidJson, kEmptyField, "Request body must be a JSON object.");
  }

  common::JointPwmState state{};
  const char *fields[] = {"d_pwm", "s_pwm", "e_pwm", "hp_pwm", "hr_pwm", "g_pwm"};
  uint16_t *values[] = {&state.d_pwm, &state.s_pwm, &state.e_pwm, &state.hp_pwm, &state.hr_pwm, &state.g_pwm};

  for (std::size_t i = 0; i < common::kJointPwmAxisCount; ++i)
  {
    const auto value = root[fields[i]];
    if (isMissingOrNonInteger(value))
    {
      return jointPwmMotionError(ApiResultCode::MissingField, fields[i],
                                 "Joint PWM request is missing a numeric PWM field.");
    }

    const auto parsed_value = value.as<long>();
    if (parsed_value < common::kMinPwm || parsed_value > common::kMaxPwm)
    {
      return jointPwmMotionError(ApiResultCode::JointPwmLimitViolation, fields[i],
                                 "PWM value is outside the PCA9685 12-bit range 0..4095.");
    }

    *values[i] = static_cast<uint16_t>(parsed_value);
  }

  return JointPwmMotionParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField, state};
}

TargetPoseParseResult parseTargetPoseRequestJson(const char *body)
{
  if (body == nullptr || body[0] == '\0')
  {
    return targetPoseError(ApiResultCode::InvalidJson, kEmptyField, kRequestBodyRequired);
  }

  StaticJsonDocument<512> doc;
  const auto error = deserializeJson(doc, body);
  if (error)
  {
    return targetPoseError(ApiResultCode::InvalidJson, kEmptyField, kMalformedJson);
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull())
  {
    return targetPoseError(ApiResultCode::InvalidJson, kEmptyField, "Request body must be a JSON object.");
  }

  const char *fields[] = {"x_mm", "y_mm", "z_mm", "p_deg", "r_deg", "g_pct"};
  for (const auto *field : fields)
  {
    if (isMissingOrNonNumeric(root[field]))
    {
      return targetPoseError(ApiResultCode::MissingField, field,
                             "Target pose request is missing a numeric target pose field.");
    }
  }

  common::TargetPose pose{root["x_mm"].as<float>(),  root["y_mm"].as<float>(),  root["z_mm"].as<float>(),
                          root["p_deg"].as<float>(), root["r_deg"].as<float>(), root["g_pct"].as<float>()};

  if (!common::isFinite(pose))
  {
    return targetPoseError(ApiResultCode::InvalidJson, kEmptyField, "Target pose values must be finite numbers.");
  }

  if (!common::isWithinTargetGripperLimits(pose))
  {
    return targetPoseError(ApiResultCode::InvalidTargetPose, "g_pct",
                           "Target gripper value is outside 0..100 percent.");
  }

  auto profile = common::defaultMotionProfile();
  JsonObjectConst profile_object = root["motionProfile"].as<JsonObjectConst>();
  if (!profile_object.isNull())
  {
    if (!profile_object["type"].isNull())
    {
      if (!profile_object["type"].is<const char *>() ||
          !parseMotionProfileType(profile_object["type"].as<const char *>(), profile.type))
      {
        return targetPoseError(ApiResultCode::InvalidTargetPose, "motionProfile.type",
                               "Motion profile type must be constant_velocity, constant_acceleration or "
                               "smooth_start_stop.");
      }
    }

    if (!profile_object["target_velocity_deg_s"].isNull())
    {
      if (isMissingOrNonNumeric(profile_object["target_velocity_deg_s"]))
      {
        return targetPoseError(ApiResultCode::InvalidTargetPose, "motionProfile.target_velocity_deg_s",
                               "Motion profile target velocity must be numeric.");
      }
      profile.target_velocity_deg_s = profile_object["target_velocity_deg_s"].as<float>();
    }

    if (!profile_object["sample_time_ms"].isNull())
    {
      if (isMissingOrNonInteger(profile_object["sample_time_ms"]))
      {
        return targetPoseError(ApiResultCode::InvalidTargetPose, "motionProfile.sample_time_ms",
                               "Motion profile sample time must be an integer.");
      }
      const auto sample_time_ms = profile_object["sample_time_ms"].as<long>();
      if (sample_time_ms < 0)
      {
        return targetPoseError(ApiResultCode::InvalidTargetPose, "motionProfile.sample_time_ms",
                               "Motion profile sample time must not be negative.");
      }
      profile.sample_time_ms = static_cast<uint32_t>(sample_time_ms);
    }
  }

  return TargetPoseParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField, pose, profile};
}

}  // namespace application
