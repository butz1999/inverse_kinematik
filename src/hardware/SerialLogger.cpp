// SerialLogger forwards formatted debug output to the configured serial backend.

#include "hardware/SerialLogger.h"

namespace hardware
{

SerialLogger::SerialLogger(HardwareSerial &serial, unsigned long baudrate)
    : serial_(serial), baudrate_(baudrate), backend_(SerialBackend::HardwareSerial)
{
}

SerialLogger::SerialLogger(HWCDC &serial, unsigned long baudrate)
    : serial_(serial), baudrate_(baudrate), backend_(SerialBackend::UsbCdc)
{
}

void SerialLogger::begin() const
{
  switch (backend_)
  {
    case SerialBackend::HardwareSerial:
      static_cast<HardwareSerial &>(serial_).begin(baudrate_);
      break;
    case SerialBackend::UsbCdc:
      static_cast<HWCDC &>(serial_).begin(baudrate_);
      break;
  }
}

void SerialLogger::print(const char *message) const
{
  serial_.print(message);
  serial_.flush();
}

void SerialLogger::println() const
{
  serial_.println();
  serial_.flush();
}

void SerialLogger::println(const char *message) const
{
  serial_.println(message);
  serial_.flush();
}

}  // namespace hardware
