# Projekt Umsetzung und Hindernisse<span style="float: right">[⬅️](project_hw_description.md) [⬆️](../slides.md) [➡️](project_changes.md) </span>

#### Inverse Kinematik
* Verwandschaft 3D Animation / Robotik  
  <img src="../youtube.svg" style="height:1em; background-color: #888"> [Sebastian Lague](https://www.youtube.com/@SebastianLague), [Freya Holmér](https://www.youtube.com/@acegikmo)
* Forwärts Kinematik: Bewegung von Einzelachsen  
  *Bewegung von Einzelachsen → Punkt im Raum*
* Roboter-Koordinatensystem  
  Joint Position [°] [%]
* Inverse Kinematik: Gleichzeitige Bewegungen von mehreren Achsen  
  *2 Punkte im Raum → Koordinierte Bewegung*
* Welt-Koordinatensystem  
  Pose [mm] [°] [%]
* IK Ansätze:
  * Analytische IK Lösungen  
    <img src="../youtube.svg" style="height:1em; background-color: #888"> [Inverse or Forward kinematics Explained under 3 minutes](https://youtu.be/b1arysUSlzo?si=fs7BbtusYuqY-aba)
  * FABRIK (<ins>F</ins>orward <ins>A</ins>nd <ins>B</ins>ackward <ins>R</ins>eaching <ins>I</ins>nverse <ins>K</ins>inematics) ❌  
    <img src="../youtube.svg" style="height:1em; background-color: #888"> [Programming Chaos](https://youtu.be/NfuO66wsuRg?si=a5t_cy8z0aZeiHL4)
  * CCD (<ins>C</ins>yclic <ins>C</ins>oordinate <ins>D</ins>escent) ❌

#### [PlatformIO](https://platformio.org/)
* µC Entwicklung in VSCode
* Platformübergreifend: Arduino, ESP32, 🍓π pico, STM32, ...
* C++ und VSCode anstelle Arduino IDE, ESPHome, ...
* Legacy Code via `#include Arduino.h`
* ein ukrainische Projekt 🫡

#### Unit Tests mit Unity
 * Nicht verwechseln mit Graphics-tool-kit ’Unity‘
 * Test Framwork für C in C geschrieben
 * Empfehlung der KI... 😉
 * Demo: `pio test -e native`

#### SBOM mit Syft
 * [Skript](../../../scripts/generate-sbom.sh) ⏳
 * [Output](../../../sbom.cyclonedx.json) ⌛

#### Build & Upload
 * Demo: `pio run -e esp32s3 -t upload`

#### Logging über /dev/ttyACM0 oder 1
 * Extrem hilfreich
 * Boot- und Crash Info vom Framework
 * Demo:  
 `pio device monitor -e esp32s3 -p /dev/ttyACM1 -b 115200`

#### WSL Pitfalls
* USB Ports in WSL mappen: 😡
* Serielles Logging: 🤬
* Netzwerk: 🤢
