# Projekt Umsetzung und Hindernisse<span style="float: right">[⬅️](project_hw_description.md) [⬆️](../slides.md) [➡️](project_changes.md) </span>
* Theorie IK
  * Welt-Koordinatensystem: Pose [mm] [°] [%]
  * Roboter Gelenke: Joint Position [°]
  * Greiferöffnung [%]
  * Analytische IK Lösungen ✅
  * FABRIK (<ins>F</ins>orward <ins>A</ins>nd <ins>B</ins>ackward <ins>R</ins>eaching <ins>I</ins>nverse <ins>K</ins>inematics) ❌
  * [FABRIK on Youtube](https://www.youtube.com/watch?v=Ihp6tOCYHug)
  * CCD (<ins>C</ins>yclic <ins>C</ins>oordinate <ins>D</ins>escent) ❌
  * Konkrete Umsetzung
* PlatformIO kennenlernen
  * Arduino basiert
  * Reine C++ implementierung
  * Wiederverwendung von "Arduino.h"
* Unit Tests mit Unity
  * Kurze Demo:
  * `pio test -e native`
* USB Ports in WSL mappen: 😡
* Serielles Logging: 🤬
