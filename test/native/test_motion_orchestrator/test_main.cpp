// Native tests for motion request orchestration.

#include <unity.h>

#include "orchestration/MotionOrchestrator.h"

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

void test_motion_orchestrator_accepts_reachable_target_and_builds_motion_plan()
{
  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const orchestration::MotionRequest request{
      common::TargetPose{-20.0F, 50.0F, 30.0F, -90.0F, 0.0F, 50.0F},
      common::MotionProfile{common::MotionProfileType::ConstantVelocity, 30.0F, 20U}};

  const auto result = orchestrator.processMotionRequest(request, common::initialJointState());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionStatus::Accepted, result.status);
  TEST_ASSERT_EQUAL_STRING("accepted", orchestration::toString(result.status));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -20.0F, result.offset_target_pose.x_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 155.0F, result.offset_target_pose.y_mm);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -47.5F, result.offset_target_pose.z_mm);
  assertJointStateNear(common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 0.0F, 50.0F}, result.joint_state);
  TEST_ASSERT_TRUE(result.motion_plan.sample_count > 1U);
  assertJointStateNear(common::initialJointState(), result.motion_plan.samples[0].joint_state);
  assertJointStateNear(result.joint_state,
                       result.motion_plan.samples[result.motion_plan.sample_count - 1U].joint_state);
}

void test_motion_orchestrator_rejects_invalid_target_pose()
{
  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const orchestration::MotionRequest request{common::TargetPose{0.0F, 0.0F, 1000.0F, 0.0F, 0.0F, 101.0F},
                                             common::defaultMotionProfile()};

  const auto result = orchestrator.processMotionRequest(request, common::initialJointState());

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionStatus::InvalidTargetPose, result.status);
  TEST_ASSERT_EQUAL(robotics::ValidationStatus::InvalidTargetPose, result.target_validation_status);
  TEST_ASSERT_EQUAL_STRING("g_pct", result.field_name);
}

void test_motion_orchestrator_rejects_unreachable_ik_target()
{
  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const orchestration::MotionRequest request{common::TargetPose{-20.0F, 50.0F, 30.0F, 90.0F, 0.0F, 50.0F},
                                             common::defaultMotionProfile()};

  const auto result = orchestrator.processMotionRequest(request, common::initialJointState());

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionStatus::KinematicsFailure, result.status);
  TEST_ASSERT_EQUAL(robotics::KinematicsStatus::UnreachableTarget, result.kinematics_status);
}

void test_motion_orchestrator_resolves_target_without_creating_motion_plan()
{
  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const auto result = orchestrator.resolveTargetPose(common::TargetPose{-20.0F, 50.0F, 30.0F, -90.0F, 0.0F, 50.0F},
                                                     common::initialJointState());

  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionStatus::Accepted, result.status);
  assertJointStateNear(common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 0.0F, 50.0F}, result.joint_state);
}

void test_motion_orchestrator_rejects_invalid_motion_profile()
{
  const orchestration::MotionOrchestrator orchestrator(robotics::defaultRobotModel(),
                                                       robotics::defaultRobotModelOffset());
  const orchestration::MotionRequest request{
      common::TargetPose{-20.0F, 50.0F, 30.0F, -90.0F, 0.0F, 50.0F},
      common::MotionProfile{common::MotionProfileType::ConstantVelocity, 0.0F, 20U}};

  const auto result = orchestrator.processMotionRequest(request, common::initialJointState());

  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_EQUAL(orchestration::MotionStatus::MotionPlanFailure, result.status);
  TEST_ASSERT_EQUAL(orchestration::MotionProfileGeneratorStatus::InvalidProfile, result.motion_profile_status);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_motion_orchestrator_accepts_reachable_target_and_builds_motion_plan);
  RUN_TEST(test_motion_orchestrator_rejects_invalid_target_pose);
  RUN_TEST(test_motion_orchestrator_rejects_unreachable_ik_target);
  RUN_TEST(test_motion_orchestrator_resolves_target_without_creating_motion_plan);
  RUN_TEST(test_motion_orchestrator_rejects_invalid_motion_profile);
  return UNITY_END();
}
