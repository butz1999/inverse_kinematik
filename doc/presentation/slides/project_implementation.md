# Projekt Umsetzung und Hindernisse<span style="float: right">[⬅️](project_hw_description.md) [⬆️](../slides.md) [➡️](project_changes.md) </span>

#### Theorie IK
* Verwandschaft 3D Animation / Robotik
* Forwärts Kinematik: Bewegung von Einzelachsen  
  *Bewegung von Einzelachsen → Punkt im Raum*
* Inverse Kinematik: Gleichzeitige Bewegungen von mehreren Achsen  
  *Punkt im Raum → Koordinierte Bewegung*
* Welt-Koordinatensystem:  
  * Pose [mm] [°] [%]
* Roboter Gelenke
  * Joint Position [°]
  * Greiferöffnung [%]
* IK Ansätze:
  * Analytische IK Lösungen ✅
  * FABRIK (<ins>F</ins>orward <ins>A</ins>nd <ins>B</ins>ackward <ins>R</ins>eaching <ins>I</ins>nverse <ins>K</ins>inematics) ❌
  * CCD (<ins>C</ins>yclic <ins>C</ins>oordinate <ins>D</ins>escent) ❌
  * [Inverse or Forward kinematics Explained under 3 minutes](https://youtu.be/b1arysUSlzo?si=fs7BbtusYuqY-aba) 📺

#### PlatformIO (https://platformio.org/)
* µC Entwicklung in VSCode
* Platformübergreifend: Arduino, ESP32, 🍓π pico, STM32, ...
* C++ und VSCode anstelle Arduino IDE, ESPHome, ...
* Legacy Code via `#include Arduino.h`

#### Unit Tests mit Unity
 * Nicht verwechseln mit Graphics-tool-kit ’Unity‘
 * Test Framwork für C in C geschrieben
 * Empfehlung der KI... 😉
 * Optionale Demo:  
   `pio test -e native`

#### Build & Upload
 * Optionale Demo:  
   `pio run -e esp32s3 -t upload`

#### Logging über /dev/ttyACM0
 * Extrem hilfreich
 * Boot- und Crash Info vom Framework
 * Optionale Demo:  
   `pio device monitor -e esp32s3 -p /dev/ttyACM1 -b 115200`

#### WSL Pitfalls
* USB Ports in WSL mappen: 😡
* Serielles Logging: 🤬
