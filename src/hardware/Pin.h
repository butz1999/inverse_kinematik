// Abstraction class for an Arduino pin

#pragma once

#include "Arduino.h"

namespace hardware
{

template <uint8_t pin_>
class Pin
{
 private:
  uint8_t mode_;
  bool state_{false};

 public:
  void mode(uint8_t mode)
  {
    mode_ = mode;
    pinMode(pin_, mode);
  };

  void write(bool state)
  {
    state_ = state;
    digitalWrite(pin_, state);
  };

  void toggle()
  {
    state_ = !state_;
    digitalWrite(pin_, state_);
  };

  void enable()
  {
    state_ = HIGH;
    digitalWrite(pin_, state_);
  };
  void disable()
  {
    state_ = LOW;
    digitalWrite(pin_, state_);
  };

  void enableLow()
  {
    state_ = LOW;
    digitalWrite(pin_, state_);
  };

  void disableLow()
  {
    state_ = HIGH;
    digitalWrite(pin_, state_);
  };

  bool read()
  {
    return digitalRead(pin_);
  };
};

}  // namespace hardware