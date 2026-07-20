// Native tests for joint-space motion profile generation.

#include <unity.h>

#include "orchestration/MotionProfileGenerator.h"

namespace
{

constexpr float kTolerance = 0.001F;

void assertJointStateNear(const common::JointState &expected, const common::JointState &actual)
{
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.d_deg, actual.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.s_deg, actual.s_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.e_deg, actual.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.hp_deg, actual.hp_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.hr_deg, actual.hr_deg);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, expected.g_pct, actual.g_pct);
}

}  // namespace

void test_constant_velocity_motion_plan_contains_start_intermediate_and_target_samples()
{
  const common::JointState start_state{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target_state{60.0F, 30.0F, -30.0F, -60.0F, 15.0F, 90.0F};
  const common::MotionProfile profile{common::MotionProfileType::ConstantVelocity, 30.0F, 1000U};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, profile);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionProfileGeneratorStatus::Ok, result.status);
  TEST_ASSERT_EQUAL_STRING("ok", orchestration::toString(result.status));
  TEST_ASSERT_EQUAL_UINT32(3000U, result.plan.total_duration_ms);
  TEST_ASSERT_EQUAL_UINT32(4U, result.plan.sample_count);
  TEST_ASSERT_EQUAL_UINT32(0U, result.plan.samples[0].time_from_start_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, result.plan.samples[1].time_from_start_ms);
  TEST_ASSERT_EQUAL_UINT32(2000U, result.plan.samples[2].time_from_start_ms);
  TEST_ASSERT_EQUAL_UINT32(3000U, result.plan.samples[3].time_from_start_ms);
  assertJointStateNear(start_state, result.plan.samples[0].joint_state);
  assertJointStateNear(common::JointState{20.0F, 10.0F, -10.0F, -20.0F, 5.0F, 30.0F},
                       result.plan.samples[1].joint_state);
  assertJointStateNear(common::JointState{40.0F, 20.0F, -20.0F, -40.0F, 10.0F, 60.0F},
                       result.plan.samples[2].joint_state);
  assertJointStateNear(target_state, result.plan.samples[3].joint_state);
}

void test_constant_velocity_motion_plan_adds_final_sample_for_non_aligned_duration()
{
  const common::JointState start_state{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target_state{45.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::MotionProfile profile{common::MotionProfileType::ConstantVelocity, 30.0F, 1000U};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, profile);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT32(1500U, result.plan.total_duration_ms);
  TEST_ASSERT_EQUAL_UINT32(3U, result.plan.sample_count);
  TEST_ASSERT_EQUAL_UINT32(0U, result.plan.samples[0].time_from_start_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, result.plan.samples[1].time_from_start_ms);
  TEST_ASSERT_EQUAL_UINT32(1500U, result.plan.samples[2].time_from_start_ms);
  assertJointStateNear(common::JointState{30.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, result.plan.samples[1].joint_state);
  assertJointStateNear(target_state, result.plan.samples[2].joint_state);
}

void test_motion_plan_for_unchanged_state_contains_single_target_sample()
{
  const common::JointState state{10.0F, -20.0F, 30.0F, -40.0F, 50.0F, 60.0F};

  const auto result = orchestration::generateMotionPlan(state, state, common::defaultMotionProfile());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT32(0U, result.plan.total_duration_ms);
  TEST_ASSERT_EQUAL_UINT32(1U, result.plan.sample_count);
  TEST_ASSERT_EQUAL_UINT32(0U, result.plan.samples[0].time_from_start_ms);
  assertJointStateNear(state, result.plan.samples[0].joint_state);
  TEST_ASSERT_EQUAL(common::MotionProfileType::SmoothStartStop, result.plan.profile.type);
  TEST_ASSERT_EQUAL_FLOAT(common::defaultMotionProfile().target_velocity_deg_s,
                          result.plan.profile.target_velocity_deg_s);
  TEST_ASSERT_EQUAL_UINT32(common::defaultMotionProfile().sample_time_ms, result.plan.profile.sample_time_ms);
}

void test_default_motion_profile_covers_full_joint_range_at_fine_sample_time()
{
  const common::JointState start_state{-90.0F, -90.0F, -90.0F, -90.0F, -90.0F, 0.0F};
  const common::JointState target_state{90.0F, 90.0F, 90.0F, 0.0F, 90.0F, 100.0F};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, common::defaultMotionProfile());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT32(2000U, result.plan.total_duration_ms);
  TEST_ASSERT_EQUAL_UINT32(201U, result.plan.sample_count);
  TEST_ASSERT_TRUE(result.plan.sample_count <= common::kMaxMotionPlanSamples);
}

void test_constant_acceleration_motion_plan_eases_start_and_stop()
{
  const common::JointState start_state{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target_state{100.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::MotionProfile profile{common::MotionProfileType::ConstantAcceleration, 25.0F, 1000U};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, profile);

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL_UINT32(4000U, result.plan.total_duration_ms);
  TEST_ASSERT_EQUAL_UINT32(5U, result.plan.sample_count);
  assertJointStateNear(common::JointState{12.5F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, result.plan.samples[1].joint_state);
  assertJointStateNear(common::JointState{50.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, result.plan.samples[2].joint_state);
  assertJointStateNear(common::JointState{87.5F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, result.plan.samples[3].joint_state);
}

void test_smooth_start_stop_motion_plan_uses_s_curve_progress()
{
  const common::JointState start_state{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target_state{100.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::MotionProfile profile{common::MotionProfileType::SmoothStartStop, 25.0F, 1000U};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, profile);

  TEST_ASSERT_TRUE(result.ok);
  assertJointStateNear(common::JointState{10.3515625F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F},
                       result.plan.samples[1].joint_state);
  assertJointStateNear(common::JointState{50.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, result.plan.samples[2].joint_state);
  assertJointStateNear(common::JointState{89.6484375F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F},
                       result.plan.samples[3].joint_state);
}

void test_motion_profile_generator_rejects_invalid_profile()
{
  const auto state = common::initialJointState();
  const common::MotionProfile profile{common::MotionProfileType::ConstantVelocity, 0.0F, 20U};

  const auto result = orchestration::generateMotionPlan(state, state, profile);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionProfileGeneratorStatus::InvalidProfile, result.status);
  TEST_ASSERT_EQUAL_STRING("invalid_profile", orchestration::toString(result.status));
}

void test_motion_profile_generator_rejects_unsupported_profile_type()
{
  const auto state = common::initialJointState();
  const common::MotionProfile profile{static_cast<common::MotionProfileType>(99), 30.0F, 20U};

  const auto result = orchestration::generateMotionPlan(state, state, profile);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionProfileGeneratorStatus::UnsupportedProfile, result.status);
  TEST_ASSERT_EQUAL_STRING("unsupported_profile", orchestration::toString(result.status));
}

void test_motion_profile_generator_rejects_plans_that_exceed_sample_limit()
{
  const common::JointState start_state{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::JointState target_state{90.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const common::MotionProfile profile{common::MotionProfileType::ConstantVelocity, 1.0F, 20U};

  const auto result = orchestration::generateMotionPlan(start_state, target_state, profile);

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionProfileGeneratorStatus::TooManySamples, result.status);
  TEST_ASSERT_EQUAL_STRING("too_many_samples", orchestration::toString(result.status));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_constant_velocity_motion_plan_contains_start_intermediate_and_target_samples);
  RUN_TEST(test_constant_velocity_motion_plan_adds_final_sample_for_non_aligned_duration);
  RUN_TEST(test_motion_plan_for_unchanged_state_contains_single_target_sample);
  RUN_TEST(test_default_motion_profile_covers_full_joint_range_at_fine_sample_time);
  RUN_TEST(test_constant_acceleration_motion_plan_eases_start_and_stop);
  RUN_TEST(test_smooth_start_stop_motion_plan_uses_s_curve_progress);
  RUN_TEST(test_motion_profile_generator_rejects_invalid_profile);
  RUN_TEST(test_motion_profile_generator_rejects_unsupported_profile_type);
  RUN_TEST(test_motion_profile_generator_rejects_plans_that_exceed_sample_limit);
  return UNITY_END();
}
