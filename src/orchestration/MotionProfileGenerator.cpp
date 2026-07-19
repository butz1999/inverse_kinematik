#include "orchestration/MotionProfileGenerator.h"

#include <algorithm>
#include <cmath>

namespace orchestration
{

namespace
{

common::MotionPlan initialMotionPlan(const common::MotionProfile &profile)
{
  return common::MotionPlan{profile, 0U, {}, 0U};
}

MotionProfileGeneratorResult error(MotionProfileGeneratorStatus status, const common::MotionProfile &profile,
                                   const char *message)
{
  return MotionProfileGeneratorResult{false, status, initialMotionPlan(profile), message};
}

MotionProfileGenerationStatus generationError(MotionProfileGeneratorStatus status, const char *message)
{
  return MotionProfileGenerationStatus{false, status, message};
}

MotionProfileGeneratorResult ok(const common::MotionPlan &plan)
{
  return MotionProfileGeneratorResult{true, MotionProfileGeneratorStatus::Ok, plan, "ok"};
}

MotionProfileGenerationStatus generationOk()
{
  return MotionProfileGenerationStatus{true, MotionProfileGeneratorStatus::Ok, "ok"};
}

float maxJointDelta(const common::JointState &start_state, const common::JointState &target_state)
{
  auto max_delta = std::fabs(target_state.d_deg - start_state.d_deg);
  max_delta = std::max(max_delta, std::fabs(target_state.s_deg - start_state.s_deg));
  max_delta = std::max(max_delta, std::fabs(target_state.e_deg - start_state.e_deg));
  max_delta = std::max(max_delta, std::fabs(target_state.hp_deg - start_state.hp_deg));
  max_delta = std::max(max_delta, std::fabs(target_state.hr_deg - start_state.hr_deg));
  max_delta = std::max(max_delta, std::fabs(target_state.g_pct - start_state.g_pct));
  return max_delta;
}

common::JointState interpolateJointState(const common::JointState &start_state, const common::JointState &target_state,
                                         float progress)
{
  const auto interpolate = [progress](float start, float target)
  {
    return start + ((target - start) * progress);
  };

  return common::JointState{interpolate(start_state.d_deg, target_state.d_deg),
                            interpolate(start_state.s_deg, target_state.s_deg),
                            interpolate(start_state.e_deg, target_state.e_deg),
                            interpolate(start_state.hp_deg, target_state.hp_deg),
                            interpolate(start_state.hr_deg, target_state.hr_deg),
                            interpolate(start_state.g_pct, target_state.g_pct)};
}

uint32_t durationMsForConstantVelocity(float max_delta, float target_velocity_deg_s)
{
  if (max_delta <= 0.0F)
  {
    return 0U;
  }

  return static_cast<uint32_t>(std::ceil((max_delta / target_velocity_deg_s) * 1000.0F));
}

float progressForProfile(common::MotionProfileType type, float u)
{
  if (u <= 0.0F)
  {
    return 0.0F;
  }
  if (u >= 1.0F)
  {
    return 1.0F;
  }

  switch (type)
  {
    case common::MotionProfileType::ConstantVelocity:
      return u;
    case common::MotionProfileType::ConstantAcceleration:
      if (u < 0.5F)
      {
        return 2.0F * u * u;
      }
      return 1.0F - (2.0F * (1.0F - u) * (1.0F - u));
    case common::MotionProfileType::SmoothStartStop:
      return (3.0F * u * u) - (2.0F * u * u * u);
  }

  return u;
}

std::size_t sampleCountForDuration(uint32_t total_duration_ms, uint32_t sample_time_ms)
{
  if (total_duration_ms == 0U)
  {
    return 1U;
  }

  const auto intermediate_samples = (total_duration_ms - 1U) / sample_time_ms;
  return static_cast<std::size_t>(intermediate_samples + 2U);
}

}  // namespace

MotionProfileGenerationStatus generateMotionPlanInto(const common::JointState &start_state,
                                                     const common::JointState &target_state,
                                                     const common::MotionProfile &profile,
                                                     common::MotionPlan &plan)
{
  plan.profile = profile;
  plan.total_duration_ms = 0U;
  plan.sample_count = 0U;

  if (!common::isFinite(start_state) || !common::isFinite(target_state) ||
      !std::isfinite(profile.target_velocity_deg_s) || profile.target_velocity_deg_s <= 0.0F ||
      profile.sample_time_ms == 0U)
  {
    return generationError(MotionProfileGeneratorStatus::InvalidProfile, "Motion profile is invalid.");
  }

  if (profile.type != common::MotionProfileType::ConstantVelocity &&
      profile.type != common::MotionProfileType::ConstantAcceleration &&
      profile.type != common::MotionProfileType::SmoothStartStop)
  {
    return generationError(MotionProfileGeneratorStatus::UnsupportedProfile,
                           "Motion profile type is not implemented.");
  }

  const auto total_duration_ms = durationMsForConstantVelocity(maxJointDelta(start_state, target_state),
                                                               profile.target_velocity_deg_s);
  const auto sample_count = sampleCountForDuration(total_duration_ms, profile.sample_time_ms);
  if (sample_count > common::kMaxMotionPlanSamples)
  {
    return generationError(MotionProfileGeneratorStatus::TooManySamples, "Motion plan exceeds the sample limit.");
  }

  plan.total_duration_ms = total_duration_ms;
  plan.sample_count = sample_count;

  for (std::size_t i = 0; i < sample_count; ++i)
  {
    auto time_from_start_ms = static_cast<uint32_t>(i) * profile.sample_time_ms;
    if (i == sample_count - 1U)
    {
      time_from_start_ms = total_duration_ms;
    }

    const auto u = total_duration_ms == 0U ? 1.0F : static_cast<float>(time_from_start_ms) / total_duration_ms;
    const auto progress = progressForProfile(profile.type, u);
    plan.samples[i] = common::TimedJointState{interpolateJointState(start_state, target_state, progress),
                                              time_from_start_ms};
  }

  return generationOk();
}

MotionProfileGeneratorResult generateMotionPlan(const common::JointState &start_state,
                                                const common::JointState &target_state,
                                                const common::MotionProfile &profile)
{
  auto plan = initialMotionPlan(profile);
  const auto status = generateMotionPlanInto(start_state, target_state, profile, plan);
  if (!status.ok)
  {
    return error(status.status, profile, status.message.c_str());
  }

  return ok(plan);
}

const char *toString(MotionProfileGeneratorStatus status)
{
  switch (status)
  {
    case MotionProfileGeneratorStatus::Ok:
      return "ok";
    case MotionProfileGeneratorStatus::InvalidProfile:
      return "invalid_profile";
    case MotionProfileGeneratorStatus::UnsupportedProfile:
      return "unsupported_profile";
    case MotionProfileGeneratorStatus::TooManySamples:
      return "too_many_samples";
  }

  return "invalid_profile";
}

}  // namespace orchestration
