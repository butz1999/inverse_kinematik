// Small wrapper around Arduino serial streams to keep logging setup in one place.

#pragma once

#include <Arduino.h>

namespace hardware
{

class SerialLogger
{
 public:
  SerialLogger(HardwareSerial &serial, unsigned long baudrate);
  SerialLogger(HWCDC &serial, unsigned long baudrate);

  void begin() const;
  void print(const char *message) const;
  void print(int number) const;
  void print(unsigned int number) const;
  void print(long number) const;
  void print(unsigned long number) const;
  void println() const;
  void println(const char *message) const;
  void println(int number) const;
  void println(unsigned int number) const;
  void println(long number) const;
  void println(unsigned long number) const;

 private:
  enum class SerialBackend
  {
    HardwareSerial,
    UsbCdc,
  };

  Stream &serial_;
  unsigned long baudrate_;
  SerialBackend backend_;
};

}  // namespace hardware
