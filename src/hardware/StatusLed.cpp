// Maps logical LED state to the ESP32-S3 onboard RGB LED.

#include "hardware/StatusLed.h"

#include <cstring>

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
namespace
{
void neopixelWrite(uint8_t, uint8_t, uint8_t, uint8_t)
{
}
}  // namespace
#endif

namespace hardware
{

namespace
{

constexpr uint32_t kDefaultIntervalMs = 500U;
constexpr uint32_t kMinWriteIntervalMs = 10U;

uint8_t scale(uint8_t value, uint8_t brightness)
{
  return static_cast<uint8_t>((static_cast<uint16_t>(value) * brightness) / 255U);
}

}  // namespace

StatusLed::StatusLed(uint8_t pin, uint8_t brightness)
    : pin_(pin),
      brightness_(brightness),
      color_(rgbFromStatusColor(Color::Off)),
      mode_(Mode::Off),
      interval_ms_(kDefaultIntervalMs),
      last_write_ms_(0U),
      last_write_color_{0U, 0U, 0U},
      has_written_(false),
      state_changed_(false)
{
}

void StatusLed::setColor(Color color)
{
  setColor(rgbFromStatusColor(color));
}

void StatusLed::setColor(RgbColor color)
{
  updateColor(color);
  writeCurrentStaticOutputIfNeeded();
}

void StatusLed::setMode(Mode mode)
{
  updateMode(mode);
  writeCurrentStaticOutputIfNeeded();
}

void StatusLed::setIntervalMs(uint32_t interval_ms)
{
  updateIntervalMs(interval_ms);
}

void StatusLed::set(Color color)
{
  updateColor(rgbFromStatusColor(color));
  updateMode(color == Color::Off ? Mode::Off : Mode::On);
  writeCurrentStaticOutputIfNeeded();
}

void StatusLed::set(RgbColor color)
{
  updateColor(color);
  updateMode(Mode::On);
  writeCurrentStaticOutputIfNeeded();
}

void StatusLed::set(Color color, Mode mode, uint32_t interval_ms)
{
  updateColor(rgbFromStatusColor(color));
  updateMode(mode);
  updateIntervalMs(interval_ms);
  writeCurrentStaticOutputIfNeeded();
}

void StatusLed::set(RgbColor color, Mode mode, uint32_t interval_ms)
{
  updateColor(color);
  updateMode(mode);
  updateIntervalMs(interval_ms);
  writeCurrentStaticOutputIfNeeded();
}

bool StatusLed::updateColor(RgbColor color)
{
  if (color_.r == color.r && color_.g == color.g && color_.b == color.b)
  {
    return false;
  }

  color_ = color;
  state_changed_ = true;
  return true;
}

bool StatusLed::updateMode(Mode mode)
{
  if (mode_ == mode)
  {
    return false;
  }

  mode_ = mode;
  state_changed_ = true;
  return true;
}

bool StatusLed::updateIntervalMs(uint32_t interval_ms)
{
  if (interval_ms == 0U || interval_ms_ == interval_ms)
  {
    return false;
  }

  interval_ms_ = interval_ms;
  state_changed_ = true;
  return true;
}

void StatusLed::service(uint32_t now_ms)
{
  const auto brightness = brightnessForNow(now_ms);
  const auto write_color = toNeopixelWriteColor(color_, brightness);
  const auto output_changed = !has_written_ || write_color.red_arg != last_write_color_.red_arg ||
                              write_color.green_arg != last_write_color_.green_arg ||
                              write_color.blue_arg != last_write_color_.blue_arg;

  if (!output_changed)
  {
    state_changed_ = false;
    return;
  }

  switch (mode_)
  {
    case Mode::Off:
    case Mode::On:
    case Mode::Blinking:
      break;
    case Mode::Pulsing:
      if (!state_changed_ && has_written_ && now_ms - last_write_ms_ < kMinWriteIntervalMs)
      {
        return;
      }
      break;
  }

  writeColor(write_color);
  last_write_ms_ = now_ms;
  last_write_color_ = write_color;
  has_written_ = true;
  state_changed_ = false;
}

StatusLed::Color StatusLed::color() const
{
  // Free RGB colors do not necessarily map back to a named status color.
  return Color::Off;
}

StatusLed::RgbColor StatusLed::rgbColor() const
{
  return color_;
}

StatusLed::Mode StatusLed::mode() const
{
  return mode_;
}

uint32_t StatusLed::intervalMs() const
{
  return interval_ms_;
}

StatusLed::RgbColor StatusLed::rgbFromStatusColor(Color color)
{
  switch (color)
  {
    case Color::Off:
      return {0U, 0U, 0U};
    case Color::Red:
      return {255U, 0U, 0U};
    case Color::Orange:
      return {255U, 85U, 0U};
    case Color::Yellow:
      return {255U, 255U, 0U};
    case Color::Green:
      return {0U, 255U, 0U};
    case Color::Cyan:
      return {0U, 255U, 255U};
    case Color::Blue:
      return {0U, 0U, 255U};
    case Color::Violet:
      return {255U, 0U, 255U};
  }

  return {0U, 0U, 0U};
}

StatusLed::NeopixelWriteColor StatusLed::toNeopixelWriteColor(RgbColor color, uint8_t brightness)
{
  return {scale(color.g, brightness), scale(color.r, brightness), scale(color.b, brightness)};
}

uint8_t StatusLed::brightnessForNow(uint32_t now_ms) const
{
  switch (mode_)
  {
    case Mode::Off:
      return 0U;
    case Mode::On:
      return brightness_;
    case Mode::Blinking:
      return ((now_ms / interval_ms_) % 2U) == 0U ? brightness_ : 0U;
    case Mode::Pulsing:
    {
      const auto phase_ms = now_ms % interval_ms_;
      const auto half_interval_ms = interval_ms_ / 2U;
      if (half_interval_ms == 0U)
      {
        return brightness_;
      }
      const auto ramp_ms = phase_ms < half_interval_ms ? phase_ms : interval_ms_ - phase_ms;
      return static_cast<uint8_t>((static_cast<uint32_t>(brightness_) * ramp_ms) / half_interval_ms);
    }
  }

  return 0U;
}

void StatusLed::writeCurrentStaticOutputIfNeeded()
{
  if (mode_ != Mode::Off && mode_ != Mode::On)
  {
    return;
  }

  const auto brightness = mode_ == Mode::On ? brightness_ : 0U;
  const auto write_color = toNeopixelWriteColor(color_, brightness);
  const auto output_changed = !has_written_ || write_color.red_arg != last_write_color_.red_arg ||
                              write_color.green_arg != last_write_color_.green_arg ||
                              write_color.blue_arg != last_write_color_.blue_arg;

  if (!output_changed)
  {
    return;
  }

  writeColor(write_color);
  last_write_color_ = write_color;
  has_written_ = true;
  state_changed_ = false;
}

void StatusLed::writeColor(NeopixelWriteColor color) const
{
  neopixelWrite(pin_, color.red_arg, color.green_arg, color.blue_arg);
}

const char *toString(StatusLed::Mode mode)
{
  switch (mode)
  {
    case StatusLed::Mode::Off:
      return "off";
    case StatusLed::Mode::On:
      return "on";
    case StatusLed::Mode::Blinking:
      return "blinking";
    case StatusLed::Mode::Pulsing:
      return "pulsing";
  }

  return "off";
}

bool parseStatusLedMode(const char *value, StatusLed::Mode &mode)
{
  if (value == nullptr)
  {
    return false;
  }
  if (std::strcmp(value, "off") == 0)
  {
    mode = StatusLed::Mode::Off;
    return true;
  }
  if (std::strcmp(value, "on") == 0)
  {
    mode = StatusLed::Mode::On;
    return true;
  }
  if (std::strcmp(value, "blinking") == 0)
  {
    mode = StatusLed::Mode::Blinking;
    return true;
  }
  if (std::strcmp(value, "pulsing") == 0)
  {
    mode = StatusLed::Mode::Pulsing;
    return true;
  }

  return false;
}

const char *toString(StatusColor color)
{
  switch (color)
  {
    case StatusColor::Off:
      return "off";
    case StatusColor::Red:
      return "red";
    case StatusColor::Orange:
      return "orange";
    case StatusColor::Yellow:
      return "yellow";
    case StatusColor::Green:
      return "green";
    case StatusColor::Cyan:
      return "cyan";
    case StatusColor::Blue:
      return "blue";
    case StatusColor::Violet:
      return "violet";
  }

  return "off";
}

bool parseStatusColor(const char *value, StatusColor &color)
{
  if (value == nullptr)
  {
    return false;
  }
  if (std::strcmp(value, "off") == 0)
  {
    color = StatusColor::Off;
    return true;
  }
  if (std::strcmp(value, "red") == 0)
  {
    color = StatusColor::Red;
    return true;
  }
  if (std::strcmp(value, "orange") == 0)
  {
    color = StatusColor::Orange;
    return true;
  }
  if (std::strcmp(value, "yellow") == 0)
  {
    color = StatusColor::Yellow;
    return true;
  }
  if (std::strcmp(value, "green") == 0)
  {
    color = StatusColor::Green;
    return true;
  }
  if (std::strcmp(value, "cyan") == 0)
  {
    color = StatusColor::Cyan;
    return true;
  }
  if (std::strcmp(value, "blue") == 0)
  {
    color = StatusColor::Blue;
    return true;
  }
  if (std::strcmp(value, "violet") == 0)
  {
    color = StatusColor::Violet;
    return true;
  }

  return false;
}

}  // namespace hardware
