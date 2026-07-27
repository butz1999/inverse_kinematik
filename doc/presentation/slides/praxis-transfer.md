# Praxis Transfer <span style="float: right">[⬅️](demo.md) [⬆️](../slides.md)</span>


* REST und TCP/IP funktionieren gut auf einem ’leichten‘ ESP32  
  mit viel wiederverwendetem Code
* HW Spezifikation
  * **Platform:** Espressif 32 (54.3.21) > Espressif ESP32-S3-DEV-KIT
  * **Hardware:** ESP32-S3-WROOM-1-N8R8 240MHz, 320KB S-RAM, 8MB PS-RAM, 8MB Flash
  * **Debug:** Current (esp-builtin) On-board (esp-builtin) External (cmsis-dap, esp-bridge, esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
  * **Chip type:** ESP32-S3 (QFN56) (revision v0.2)
  * **Crystal frequency:**  40MHz
* IK Kalkulation von 1024 Stützstellen  
  Zeitbedarf ~1µs pro Stützpunkt
* Vergleich
  * Raspberry Pi Pico (RP2040): Dual ARM Cortex-M0+ mit 133 MHz  
  (~CHF 3 bis 5.-)
  * ESP32-S3-WROOM-1-N8R8: Xtensa Dual-Core LX7 mit bis zu 240 MHz 
  (~CHF 10 bis 15.-)
* ESP32-Konfiguration: It's a mess...
