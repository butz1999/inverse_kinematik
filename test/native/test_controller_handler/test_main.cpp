// Native tests for transport-independent continuous controller handling.

#include <unity.h>

#include "application/ControllerCommandMapper.h"
#include "orchestration/ControllerHandler.h"

namespace
{

orchestration::ControllerHandler makeHandler()
{
  return orchestration::ControllerHandler(
      orchestration::MotionOrchestrator(robotics::defaultRobotModel(), robotics::defaultRobotModelOffset()));
}

}  // namespace

void test_controller_command_mapper_maps_physical_layout_to_device_neutral_command()
{
  auto input = application::emptyControllerInput();
  input.valid = true;
  input.left_x = 2047;
  input.right_y = -2047;
  input.buttons =
      application::kControllerButtonGripR | application::kControllerButtonA | application::kControllerButtonRightStick;
  input.dpad = application::kControllerDpadLeft;

  const auto command = application::mapControllerInputToJogCommand(input);

  TEST_ASSERT_TRUE(command.valid);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, command.x_input);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, -1.0F, command.z_input);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 31.25F, command.joint_velocity_per_second.d_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, -31.25F, command.joint_velocity_per_second.e_deg);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 31.25F, command.joint_velocity_per_second.hr_deg);
  TEST_ASSERT_TRUE(command.world_roll_toggle_pressed);
}

void test_controller_handler_applies_direct_joint_jog_with_joint_limits()
{
  auto handler = makeHandler();
  auto command = orchestration::JogCommand{};
  command.valid = true;
  command.joint_velocity_per_second.d_deg = 10.0F;

  const auto result = handler.update(command, common::initialJointState(), 100U);

  TEST_ASSERT_TRUE(result.active);
  TEST_ASSERT_TRUE(result.changed);
  TEST_ASSERT_EQUAL(orchestration::ControllerHandlerStatus::Updated, result.status);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, result.joint_state.d_deg);
}

void test_controller_handler_ignores_invalid_command()
{
  auto handler = makeHandler();
  const auto current = common::initialJointState();

  const auto result = handler.update(orchestration::JogCommand{}, current, 5U);

  TEST_ASSERT_FALSE(result.active);
  TEST_ASSERT_FALSE(result.changed);
  TEST_ASSERT_EQUAL(orchestration::ControllerHandlerStatus::Inactive, result.status);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, current.d_deg, result.joint_state.d_deg);
}

void test_controller_handler_state_reports_world_roll_lock_disabled_after_reset()
{
  auto handler = makeHandler();

  const auto state = handler.state();

  TEST_ASSERT_FALSE(state.world_roll_lock_enabled);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.locked_world_roll_deg);
}

void test_controller_hand_roll_jog_disables_world_roll_lock()
{
  auto handler = makeHandler();
  const auto current = common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 0.0F, 50.0F};
  auto toggle = orchestration::JogCommand{};
  toggle.valid = true;
  toggle.world_roll_toggle_pressed = true;
  handler.update(toggle, current, 5U);
  TEST_ASSERT_TRUE(handler.state().world_roll_lock_enabled);

  auto hand_roll_jog = orchestration::JogCommand{};
  hand_roll_jog.valid = true;
  hand_roll_jog.joint_velocity_per_second.hr_deg = 10.0F;
  handler.update(hand_roll_jog, current, 5U);

  TEST_ASSERT_FALSE(handler.state().world_roll_lock_enabled);
}

void test_controller_world_roll_toggle_off_keeps_current_hand_roll()
{
  auto handler = makeHandler();
  const auto current = common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 4.0F, 50.0F};
  auto toggle = orchestration::JogCommand{};
  toggle.valid = true;
  toggle.world_roll_toggle_pressed = true;
  handler.update(toggle, current, 5U);
  TEST_ASSERT_TRUE(handler.state().world_roll_lock_enabled);

  auto released = toggle;
  released.world_roll_toggle_pressed = false;
  handler.update(released, current, 5U);
  const auto result = handler.update(toggle, current, 5U);

  TEST_ASSERT_FALSE(handler.state().world_roll_lock_enabled);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, current.hr_deg, result.joint_state.hr_deg);
}

void test_controller_joint_state_synchronization_disables_world_roll_lock()
{
  auto handler = makeHandler();
  const auto current = common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 0.0F, 50.0F};
  auto toggle = orchestration::JogCommand{};
  toggle.valid = true;
  toggle.world_roll_toggle_pressed = true;
  handler.update(toggle, current, 5U);
  TEST_ASSERT_TRUE(handler.state().world_roll_lock_enabled);

  handler.synchronizeJointState(common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 10.0F, 50.0F});

  TEST_ASSERT_FALSE(handler.state().world_roll_lock_enabled);
}

void test_controller_handler_generates_limited_cartesian_joint_step()
{
  auto handler = makeHandler();
  auto command = orchestration::JogCommand{};
  command.valid = true;
  command.x_input = 1.0F;

  const auto current = common::JointState{-2.765F, -22.122F, -74.927F, -82.952F, 0.0F, 50.0F};
  const auto result = handler.update(command, current, 50U);

  TEST_ASSERT_TRUE(result.active);
  TEST_ASSERT_TRUE(result.changed);
  TEST_ASSERT_EQUAL(orchestration::ControllerHandlerStatus::Updated, result.status);
  TEST_ASSERT_TRUE(robotics::validateJointState(result.joint_state).ok);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_controller_command_mapper_maps_physical_layout_to_device_neutral_command);
  RUN_TEST(test_controller_handler_applies_direct_joint_jog_with_joint_limits);
  RUN_TEST(test_controller_handler_ignores_invalid_command);
  RUN_TEST(test_controller_handler_state_reports_world_roll_lock_disabled_after_reset);
  RUN_TEST(test_controller_hand_roll_jog_disables_world_roll_lock);
  RUN_TEST(test_controller_world_roll_toggle_off_keeps_current_hand_roll);
  RUN_TEST(test_controller_joint_state_synchronization_disables_world_roll_lock);
  RUN_TEST(test_controller_handler_generates_limited_cartesian_joint_step);
  return UNITY_END();
}
