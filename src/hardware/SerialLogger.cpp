// SerialLogger forwards formatted debug output to the configured UART.

#include "hardware/SerialLogger.h"

namespace hardware {

SerialLogger::SerialLogger(HardwareSerial &serial, unsigned long baudrate)
    : serial_(serial), baudrate_(baudrate) {}

void SerialLogger::begin() const {
  serial_.begin(baudrate_);
}

void SerialLogger::print(const char *message) const {
  serial_.print(message);
  serial_.flush();
}

void SerialLogger::println() const {
  serial_.println();
  serial_.flush();
}

void SerialLogger::println(const char *message) const {
  serial_.println(message);
  serial_.flush();
}

}  // namespace hardware
