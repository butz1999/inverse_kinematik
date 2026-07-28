// Native tests for controller world-roll locking near the downward tool orientation.

#include <unity.h>

#include "application/ControllerWorldRollLock.h"

void test_world_roll_lock_captures_and_compensates_turntable_rotation()
{
  auto state = application::emptyControllerWorldRollLockState();

  const auto update = application::updateControllerWorldRollLock(true, -90.0F, 15.0F, 20.0F, state);

  TEST_ASSERT_EQUAL(application::ControllerWorldRollLockUpdate::Enabled, update);
  TEST_ASSERT_TRUE(state.enabled);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 35.0F, state.locked_world_roll_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, -25.0F, application::handRollForControllerWorldRollLock(state, 60.0F));
}

void test_world_roll_lock_accepts_pitch_within_20_degrees_of_downward()
{
  auto state = application::emptyControllerWorldRollLockState();

  const auto update = application::updateControllerWorldRollLock(true, -70.0F, 0.0F, 0.0F, state);

  TEST_ASSERT_EQUAL(application::ControllerWorldRollLockUpdate::Enabled, update);
  TEST_ASSERT_TRUE(state.enabled);
}

void test_world_roll_lock_rejects_pitch_outside_downward_range()
{
  auto state = application::emptyControllerWorldRollLockState();

  const auto update = application::updateControllerWorldRollLock(true, -69.9F, 0.0F, 0.0F, state);

  TEST_ASSERT_EQUAL(application::ControllerWorldRollLockUpdate::Rejected, update);
  TEST_ASSERT_FALSE(state.enabled);
}

void test_world_roll_lock_disables_when_pitch_leaves_downward_range()
{
  auto state = application::emptyControllerWorldRollLockState();
  state.enabled = true;
  state.locked_world_roll_deg = 15.0F;

  const auto update = application::updateControllerWorldRollLock(false, -60.0F, 0.0F, 0.0F, state);

  TEST_ASSERT_EQUAL(application::ControllerWorldRollLockUpdate::Disabled, update);
  TEST_ASSERT_FALSE(state.enabled);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_world_roll_lock_captures_and_compensates_turntable_rotation);
  RUN_TEST(test_world_roll_lock_accepts_pitch_within_20_degrees_of_downward);
  RUN_TEST(test_world_roll_lock_rejects_pitch_outside_downward_range);
  RUN_TEST(test_world_roll_lock_disables_when_pitch_leaves_downward_range);
  return UNITY_END();
}
