# Projekt Umsetzung und Hindernisse<span style="float: right">[⬅️](project_hw_description.md) [⬆️](../slides.md) [➡️](project_changes.md) </span>

#### Theorie IK
* Verwandschaft 3D Animation / Robotik
* Forwärts Kinematik: Bewegung von Einzelachsen  
  Bewegung von Einzelachsen → Punkt im Raum
* Inverse Kinematik: Gleichzeitige Bewegungen von mehreren Achsen  
  Punkt im Raum → *Koordinierte* Bewegung von Einzelachsen
* Welt-Koordinatensystem: Pose [mm] [°] [%]
* Roboter Gelenke
  * Joint Position [°]
  * Greiferöffnung [%]
* Analytische IK Lösungen ✅
* FABRIK (<ins>F</ins>orward <ins>A</ins>nd <ins>B</ins>ackward <ins>R</ins>eaching <ins>I</ins>nverse <ins>K</ins>inematics) ❌
* CCD (<ins>C</ins>yclic <ins>C</ins>oordinate <ins>D</ins>escent) ❌
* [Inverse or Forward kinematics Explained under 3 minutes](https://youtu.be/b1arysUSlzo?si=fs7BbtusYuqY-aba)
* Konkrete Umsetzung

#### PlatformIO
* Platformübergreifende µC Entwicklung
* Geeignet für: Arduino, ESP32, STM32, ...
* C++ und VSCode anstelle Arduino IDE, ESPHome, ...
* Legacy Code via "Arduino.h"

#### Unit Tests mit Unity
 * Nicht verwechseln mit Graphics-tool-kit ’Unity‘
 * Test Framwork für C in C geschrieben
 * Empfehlung der KI... 😉
 * Kurze Demo:
 * `pio test -e native`

#### WSL Pitfalls
* USB Ports in WSL mappen: 😡
* Serielles Logging: 🤬
