// Controls the onboard RGB status LED including color and display mode.

#pragma once

#include <cstdint>

namespace hardware
{

enum class StatusColor
{
  Off,
  Red,
  Orange,
  Yellow,
  Green,
  Cyan,
  Blue,
  Violet,
};

class StatusLed
{
 public:
  enum class Mode
  {
    Off,
    On,
    Blinking,
    Pulsing,
  };

  using Color = StatusColor;

  struct RgbColor
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  struct NeopixelWriteColor
  {
    uint8_t red_arg;
    uint8_t green_arg;
    uint8_t blue_arg;
  };

  StatusLed(uint8_t pin, uint8_t brightness);

  // ToDo: Da gibt es ein paar Duplikate setColor() und set() -> aufräumen!
  void setColor(Color color);
  void setColor(RgbColor color);
  void setMode(Mode mode);
  void setIntervalMs(uint32_t interval_ms);
  void set(Color color);
  void set(RgbColor color);
  void set(Color color, Mode mode, uint32_t interval_ms);
  void set(RgbColor color, Mode mode, uint32_t interval_ms);
  // ToDo: Hier gefällt mir der Name nicht loop() callHandler() ... irgend etwas besseres...
  void service(uint32_t now_ms);

  Color color() const;
  RgbColor rgbColor() const;
  Mode mode() const;
  uint32_t intervalMs() const;

  static RgbColor rgbFromStatusColor(Color color);
  static NeopixelWriteColor toNeopixelWriteColor(RgbColor color, uint8_t brightness);

 private:
  bool updateColor(RgbColor color);
  bool updateMode(Mode mode);
  bool updateIntervalMs(uint32_t interval_ms);
  uint8_t brightnessForNow(uint32_t now_ms) const;
  void writeCurrentStaticOutputIfNeeded();
  void writeColor(NeopixelWriteColor color) const;

  uint8_t pin_;
  uint8_t brightness_;
  RgbColor color_;
  Mode mode_;
  uint32_t interval_ms_;
  uint32_t last_write_ms_;
  NeopixelWriteColor last_write_color_;
  bool has_written_;
  bool state_changed_;
};

const char *toString(StatusLed::Mode mode);
bool parseStatusLedMode(const char *value, StatusLed::Mode &mode);
const char *toString(StatusColor color);
bool parseStatusColor(const char *value, StatusColor &color);

}  // namespace hardware
