// ArduinoJson-backed request parsing for the REST API.

#include "application/ApiJson.h"

#include <ArduinoJson.h>

#include <cstring>

#include "config/RobotSettings.h"

namespace application
{

namespace
{

constexpr const char *kEmptyField = "";
constexpr const char *kRequestBodyRequired = "Request body must contain a JSON object.";
constexpr const char *kMalformedJson = "Request body must be valid JSON.";
StaticJsonDocument<4096> sequence_json_document;

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
  return TargetPoseParseResult{
      false, code, field_name, message, common::initialTargetPose(), common::defaultMotionProfile()};
}

SequenceDefinitionParseResult sequenceDefinitionError(ApiResultCode code, const char *field_name, const char *message)
{
  return SequenceDefinitionParseResult{false, code, field_name, message};
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
  if (std::strcmp(value, "smooth_start_stop") == 0)
  {
    type = common::MotionProfileType::SmoothStartStop;
    return true;
  }
  if (std::strcmp(value, "constant_acceleration") == 0)
  {
    type = common::MotionProfileType::ConstantAcceleration;
    return true;
  }
  if (std::strcmp(value, "fast_start_stop") == 0)
  {
    type = common::MotionProfileType::FastStartStop;
    return true;
  }

  return false;
}

bool parseMotionProfileObject(JsonObjectConst profile_object, common::MotionProfile &profile, const char *&field_name,
                              const char *&message)
{
  if (profile_object.isNull())
  {
    return true;
  }

  if (!profile_object["type"].isNull())
  {
    if (!profile_object["type"].is<const char *>() ||
        !parseMotionProfileType(profile_object["type"].as<const char *>(), profile.type))
    {
      field_name = "motionProfile.type";
      message =
          "Motion profile type must be constant_velocity, constant_acceleration, smooth_start_stop or "
          "fast_start_stop.";
      return false;
    }
  }

  if (!profile_object["target_velocity_deg_s"].isNull())
  {
    if (isMissingOrNonNumeric(profile_object["target_velocity_deg_s"]))
    {
      field_name = "motionProfile.target_velocity_deg_s";
      message = "Motion profile target velocity must be numeric.";
      return false;
    }
    profile.target_velocity_deg_s = profile_object["target_velocity_deg_s"].as<float>();
  }

  if (!profile_object["sample_time_ms"].isNull())
  {
    if (isMissingOrNonInteger(profile_object["sample_time_ms"]))
    {
      field_name = "motionProfile.sample_time_ms";
      message = "Motion profile sample time must be an integer.";
      return false;
    }
    const auto sample_time_ms = profile_object["sample_time_ms"].as<long>();
    if (sample_time_ms < 0)
    {
      field_name = "motionProfile.sample_time_ms";
      message = "Motion profile sample time must not be negative.";
      return false;
    }
    profile.sample_time_ms = static_cast<uint32_t>(sample_time_ms);
  }

  return true;
}

bool parseTargetPoseObject(JsonObjectConst root, common::TargetPose &pose, const char *&field_name,
                           const char *&message)
{
  const char *fields[] = {"x_mm", "y_mm", "z_mm", "p_deg", "r_deg", "g_pct"};
  for (const auto *field : fields)
  {
    if (isMissingOrNonNumeric(root[field]))
    {
      field_name = field;
      message = "Target pose request is missing a numeric target pose field.";
      return false;
    }
  }

  pose = common::TargetPose{root["x_mm"].as<float>(),  root["y_mm"].as<float>(),  root["z_mm"].as<float>(),
                            root["p_deg"].as<float>(), root["r_deg"].as<float>(), root["g_pct"].as<float>()};

  if (!common::isFinite(pose))
  {
    field_name = kEmptyField;
    message = "Target pose values must be finite numbers.";
    return false;
  }

  if (!common::isWithinTargetGripperLimits(pose))
  {
    field_name = "g_pct";
    message = "Target gripper value is outside 0..100 percent.";
    return false;
  }

  return true;
}

bool parseUint8Field(JsonObjectConst root, const char *name, uint8_t &value)
{
  const auto field = root[name];
  if (isMissingOrNonInteger(field))
  {
    return false;
  }

  const auto parsed = field.as<long>();
  if (parsed < 0 || parsed > 255)
  {
    return false;
  }

  value = static_cast<uint8_t>(parsed);
  return true;
}

bool parseStepType(JsonObjectConst step, steps::StepType &type)
{
  if (step["type"].isNull())
  {
    type = steps::StepType::Pose;
    return true;
  }
  if (!step["type"].is<const char *>())
  {
    return false;
  }

  const auto *value = step["type"].as<const char *>();
  if (std::strcmp(value, "pose") == 0)
  {
    type = steps::StepType::Pose;
    return true;
  }
  if (std::strcmp(value, "wait") == 0)
  {
    type = steps::StepType::Wait;
    return true;
  }
  if (std::strcmp(value, "led") == 0 || std::strcmp(value, "rgb_color") == 0)
  {
    type = steps::StepType::Led;
    return true;
  }

  return false;
}

ApiResultCode targetPoseParseCode(const char *field_name, const char *message)
{
  if (std::strcmp(field_name, kEmptyField) == 0)
  {
    return ApiResultCode::InvalidJson;
  }
  if (std::strcmp(field_name, "g_pct") == 0 &&
      std::strcmp(message, "Target gripper value is outside 0..100 percent.") == 0)
  {
    return ApiResultCode::InvalidTargetPose;
  }
  return ApiResultCode::MissingField;
}

SequenceDefinitionParseResult parseLedStepObject(JsonObjectConst step, steps::LedStep &led_step)
{
  led_step = steps::emptyLedStep();
  if (!led_step.name.assign(step["name"].is<const char *>() ? step["name"].as<const char *>() : ""))
  {
    return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "name", "LED step name is too long.");
  }

  if (!step["color"].isNull())
  {
    if (!step["color"].is<const char *>() ||
        !hardware::parseStatusColor(step["color"].as<const char *>(), led_step.status_color))
    {
      return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "color",
                                     "LED color must be off, red, orange, yellow, green, cyan, blue or violet.");
    }
    led_step.has_status_color = true;
  }

  const auto rgb_object = step["rgb"].as<JsonObjectConst>();
  const auto has_flat_rgb = !step["r"].isNull() || !step["g"].isNull() || !step["b"].isNull();
  if (!rgb_object.isNull() || has_flat_rgb)
  {
    if (led_step.has_status_color)
    {
      return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "color/rgb",
                                     "LED step must not contain both color and rgb.");
    }

    const auto source = rgb_object.isNull() ? step : rgb_object;
    if (!parseUint8Field(source, "r", led_step.rgb_color.r) || !parseUint8Field(source, "g", led_step.rgb_color.g) ||
        !parseUint8Field(source, "b", led_step.rgb_color.b))
    {
      return sequenceDefinitionError(ApiResultCode::MissingField, "rgb",
                                     "LED rgb must contain integer r, g and b values from 0 to 255.");
    }
    led_step.has_rgb_color = true;
  }

  if (!led_step.has_status_color && !led_step.has_rgb_color)
  {
    return sequenceDefinitionError(ApiResultCode::MissingField, "color", "LED step must contain either color or rgb.");
  }

  if (!step["mode"].is<const char *>())
  {
    return sequenceDefinitionError(ApiResultCode::MissingField, "mode",
                                   "LED mode must be off, on, blinking or pulsing.");
  }
  if (!hardware::parseStatusLedMode(step["mode"].as<const char *>(), led_step.mode))
  {
    return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "mode",
                                   "LED mode must be off, on, blinking or pulsing.");
  }

  if (step["interval_ms"].isNull())
  {
    return sequenceDefinitionError(ApiResultCode::MissingField, "interval_ms", "LED interval must be an integer.");
  }
  if (!step["interval_ms"].is<long>())
  {
    return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "interval_ms", "LED interval must be an integer.");
  }

  const auto interval_ms = step["interval_ms"].as<long>();
  if (interval_ms <= 0)
  {
    return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "interval_ms", "LED interval must be positive.");
  }
  led_step.interval_ms = static_cast<uint32_t>(interval_ms);

  return SequenceDefinitionParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField};
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

  for (const auto axis : common::kJointAxes)
  {
    const auto *field_name = common::jointAxisFieldName(axis);
    if (isMissingOrNonNumeric(root[field_name]))
    {
      return jointMotionError(ApiResultCode::MissingField, field_name,
                              "Joint motion request is missing a numeric joint field.");
    }
  }

  auto state = common::initialJointState();
  for (const auto axis : common::kJointAxes)
  {
    common::jointAxisValue(state, axis) = root[common::jointAxisFieldName(axis)].as<float>();
  }

  if (!common::isFinite(state))
  {
    return jointMotionError(ApiResultCode::InvalidJson, kEmptyField, "Joint values must be finite numbers.");
  }

  if (const auto violation = common::findFirstLimitViolation(state, config::robotSettings().joint_limits))
  {
    return jointMotionError(ApiResultCode::JointLimitViolation, common::jointAxisFieldName(*violation),
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

  const auto &pwm_limits = config::robotSettings().pwm_limits;
  for (std::size_t i = 0; i < common::kJointAxisCount; ++i)
  {
    const auto value = root[fields[i]];
    if (isMissingOrNonInteger(value))
    {
      return jointPwmMotionError(ApiResultCode::MissingField, fields[i],
                                 "Joint PWM request is missing a numeric PWM field.");
    }

    const auto parsed_value = value.as<long>();
    if (parsed_value < pwm_limits.min_value || parsed_value > pwm_limits.max_value)
    {
      return jointPwmMotionError(ApiResultCode::JointPwmLimitViolation, fields[i],
                                 "PWM value is outside the configured PWM limits.");
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

  common::TargetPose pose = common::initialTargetPose();
  const char *field_name = kEmptyField;
  const char *message = kEmptyField;
  if (!parseTargetPoseObject(root, pose, field_name, message))
  {
    return targetPoseError(targetPoseParseCode(field_name, message), field_name, message);
  }

  auto profile = common::defaultMotionProfile();
  JsonObjectConst profile_object = root["motionProfile"].as<JsonObjectConst>();
  if (!profile_object.isNull())
  {
    if (!parseMotionProfileObject(profile_object, profile, field_name, message))
    {
      return targetPoseError(ApiResultCode::InvalidTargetPose, field_name, message);
    }
  }

  return TargetPoseParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField, pose, profile};
}

SequenceDefinitionParseResult parseSequenceDefinitionRequestJson(const char *body, SequenceDefinition &sequence)
{
  sequence = emptySequenceDefinition();
  if (body == nullptr || body[0] == '\0')
  {
    return sequenceDefinitionError(ApiResultCode::InvalidJson, kEmptyField, kRequestBodyRequired);
  }

  sequence_json_document.clear();
  const auto error = deserializeJson(sequence_json_document, body);
  if (error)
  {
    return sequenceDefinitionError(ApiResultCode::InvalidJson, kEmptyField, kMalformedJson);
  }

  JsonObjectConst root = sequence_json_document.as<JsonObjectConst>();
  if (root.isNull())
  {
    return sequenceDefinitionError(ApiResultCode::InvalidJson, kEmptyField, "Request body must be a JSON object.");
  }

  JsonArrayConst steps = root["steps"].as<JsonArrayConst>();
  if (steps.isNull())
  {
    return sequenceDefinitionError(ApiResultCode::MissingField, "steps",
                                   "Sequence request must contain a steps array.");
  }

  for (JsonObjectConst step : steps)
  {
    if (sequence.step_count >= kMaxSequenceSteps)
    {
      return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "steps",
                                     "Sequence request exceeds the maximum number of steps.");
    }

    if (step.isNull())
    {
      return sequenceDefinitionError(ApiResultCode::InvalidJson, "steps", "Each sequence step must be an object.");
    }

    steps::StepType step_type = steps::StepType::Pose;
    if (!parseStepType(step, step_type))
    {
      return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "type",
                                     "Sequence step type must be pose, wait or led.");
    }

    if (step_type == steps::StepType::Pose)
    {
      JsonObjectConst pose_object = step["targetPose"].as<JsonObjectConst>();
      if (pose_object.isNull())
      {
        pose_object = step;
      }

      common::TargetPose pose = common::initialTargetPose();
      const char *field_name = kEmptyField;
      const char *message = kEmptyField;
      if (!parseTargetPoseObject(pose_object, pose, field_name, message))
      {
        return sequenceDefinitionError(targetPoseParseCode(field_name, message), field_name, message);
      }

      auto profile = common::defaultMotionProfile();
      JsonObjectConst profile_object = step["motionProfile"].as<JsonObjectConst>();
      if (!parseMotionProfileObject(profile_object, profile, field_name, message))
      {
        return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, field_name, message);
      }

      steps::PoseStep pose_step{pose, profile, ""};
      if (!pose_step.name.assign(step["name"].is<const char *>() ? step["name"].as<const char *>() : ""))
      {
        return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "name", "Pose step name is too long.");
      }
      sequence.steps[sequence.step_count] = steps::poseSequenceStep(pose_step);
      ++sequence.step_count;
      continue;
    }

    if (step_type == steps::StepType::Wait)
    {
      const auto duration_field = step["duration_ms"].isNull() ? step["wait_ms"] : step["duration_ms"];
      if (isMissingOrNonInteger(duration_field))
      {
        return sequenceDefinitionError(ApiResultCode::MissingField, "duration_ms",
                                       "Wait step must contain an integer duration_ms.");
      }
      const auto duration_ms = duration_field.as<long>();
      if (duration_ms < 0)
      {
        return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "duration_ms",
                                       "Wait step duration must not be negative.");
      }

      sequence.steps[sequence.step_count] =
          steps::waitSequenceStep(steps::WaitStep{static_cast<uint32_t>(duration_ms)});
      ++sequence.step_count;
      continue;
    }

    steps::LedStep led_step = steps::emptyLedStep();
    const auto parsed_led_step = parseLedStepObject(step, led_step);
    if (!parsed_led_step.ok)
    {
      return parsed_led_step;
    }

    sequence.steps[sequence.step_count] = steps::ledSequenceStep(led_step);
    ++sequence.step_count;
  }

  if (sequence.step_count == 0U)
  {
    return sequenceDefinitionError(ApiResultCode::InvalidTargetPose, "steps",
                                   "Sequence request must contain at least one step.");
  }

  return SequenceDefinitionParseResult{true, ApiResultCode::Ok, kEmptyField, kEmptyField};
}

}  // namespace application
