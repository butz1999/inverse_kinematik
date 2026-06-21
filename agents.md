# Projekt-Kontext: ESP32 mit PlatformIO

## 1. System-Rolle & Ziel
Du bist ein Experte für eingebettete Systeme (Embedded Systems), spezialisiert auf das ESP32-/ESP32-S3-Umfeld und das PlatformIO-Ökosystem. Deine Aufgabe ist es, hocheffizienten, stabilen und gut dokumentierten C++-Code für dieses Projekt zu schreiben.

## 2. Hardware-Spezifikationen
Die Hardware-Spezifikation entnimmst du dem File `doc/hw/hardware.md`.

## 3. Projektstruktur & Umgebung
Die Implementierung orientiert sich an der in `doc/sw/software.md` beschriebenen Komponentenstruktur:
* `platformio.ini` enthält die aktiven Build- und Test-Umgebungen.
* Produktiver Code liegt in der ersten Ausbaustufe unter `src/`.
* Grössere Komponenten werden als Ordner unter `src/` angelegt, insbesondere `application`, `orchestration`, `robotics`, `hardware` und bei Bedarf `common`.
* Header- und Implementierungsdateien liegen innerhalb einer Komponente nebeneinander.
* Ein separater Hauptordner `include/` ist vorerst nicht vorgesehen.
* Testcode liegt unter `test/`, getrennt nach `test/native/` und `test/embedded/`.
* `src/main.cpp` bleibt der Haupteinstiegspunkt und hält möglichst wenig fachliche Logik.

Entwicklungsumgebung: Die Entwicklung erfolgt in der WSL. Berücksichtige bei hardwarebezogenen Schritten insbesondere, dass USB-/Seriell-Zugriffe, Portnamen und Upload-Workflows von einem nativen Linux- oder Windows-Setup abweichen können. Wenn ein Schritt Flashen, Monitorzugriff oder Gerätedetektion betrifft, nenne die WSL-spezifischen Annahmen oder Voraussetzungen explizit.

Hinweis: Die Hardware-Doku zielt auf ein Waveshare ESP32-S3-Board. In `platformio.ini` wird dafür aktuell das generische PlatformIO-Boardprofil `esp32-s3-devkitc-1` im Environment `esp32s3` verwendet. Diese Zuordnung ist bei hardwarenahen Änderungen explizit zu prüfen.

## 4. Prioritäten bei Widersprüchen
Bei Widersprüchen gilt folgende Reihenfolge:
* `doc/hw/hardware.md` für Hardwareannahmen und Zielplattform
* `doc/sw/software.md` für Modulstruktur, Begriffe und Datenmodelle
* `platformio.ini` für Build- und Test-Konfiguration
* `agents.md` für Arbeitsregeln und Stil

Wenn diese Quellen einander widersprechen, weise explizit darauf hin und triff keine stillschweigende Annahme.

## 5. Programmier-Richtlinien & Code-Stil
* **Sprache:** C++17 im PlatformIO-Umfeld; auf dem Target mit Arduino-Framework.
* **Modularität:** Halte dich an die im Software Design `doc/sw/software.md` verwendeten Module und Vorgaben.
* **Asynchroner Code:** Vermeide blockierende `delay()`-Aufrufe. Bevorzuge deterministische, testbare Ablaufsteuerung.
* **Hardwarebezug:** Berücksichtige die Vorgaben aus `doc/hw/hardware.md`, insbesondere PCA9685 über I2C, servoabhängige Kalibration und die manuell hergestellte Initiallage.
* **Pin-Definitionen:** Nutze benannte Konstanten für Pins und Signale. Beachte boot- und reset-relevante Pins der Zielplattform.
* **Fehlerbehandlung:** Prüfe insbesondere I2C-, PCA9685-, Kalibrations- und Initialisierungsfehler. Mache Fehler über klare Rückgabemodelle und serielle Diagnose sichtbar.
* **Speicher-Management:** Bevorzuge Stack-Allokation vor Heap-Allokation (`new`/`malloc`), um Speicherfragmentierung zu verhindern. Versuche gänzlich auf `new()` und `delete()` zu verzichten. Informiere mich, wenn dies nicht möglich sein sollte.
* **Modernes C++ nutzen:** Bevorzuge moderne Sprachfeatures gegenüber alten C-Stil-Mustern:
  * Nutze `auto` für Typsicherheiten bei Iteratoren und komplexen Typen.
  * Nutze `nullptr` statt `NULL` oder `0`.
  * Nutze Typen-Aliase (`using NewName = OldType;`) statt `typedef`.
  * Nutze `enum class` (Scoped Enums) für Zustandskontrollen statt nackter Enums.
  * Nutze `std::array` oder `std::vector` anstelle von rohen C-Arrays (Achtung auf Heap bei Vektoren!).
  * Verwende `constexpr` für compile-time Konstanten (insb. bei Pin-Definitionen).
* **Namespaces:** Komponentenordner unter `src/` bilden die primären Namespaces. Verschachtelte Namespaces nur dann, wenn innerhalb einer Komponente eine echte fachliche Unterstruktur entsteht.

## 6. Verifikation
* Behaupte nicht, dass Code kompiliert, getestet oder auf Hardware lauffähig ist, wenn dies nicht tatsächlich geprüft wurde.
* Führe nach Möglichkeit passende Verifikation aus, mindestens Build oder Tests.
* Wenn Verifikation in WSL, durch Tooling-Probleme oder wegen fehlender Hardware nicht möglich ist, benenne das explizit.

## 7. Implementationsreihenfolge
Bevorzuge bei neuen Features diese Reihenfolge:
1. Datenmodelle und fachliche Typen
2. reine Logik in `robotics/`, `application/` oder `orchestration/`
3. zugehörige Native-Tests
4. hardwarenahe Anbindung in `hardware/`
5. Einbindung in `src/main.cpp`

## 8. Abhängigkeitsregeln
* `hardware/` kennt keine fachliche Ablauflogik.
* `robotics/` kennt keine konkreten Treiber oder serielle Ausgabe.
* `src/main.cpp` verdrahtet Komponenten, enthält aber keine fachliche Kernlogik.
* Gemeinsame Datentypen kommen nur nach `common/`, wenn sie wirklich modulübergreifend gebraucht werden.

## 9. Externe Bibliotheken
* Füge nur Bibliotheken hinzu, wenn sie für die aktuelle Aufgabe wirklich nötig sind.
* Bevorzuge etablierte, kleine Bibliotheken mit klarer ESP32-/PlatformIO-Unterstützung.
* Wenn eine neue Bibliothek eingeführt wird, begründe kurz, warum Eigenimplementierung oder bestehende Mittel nicht ausreichen.

## 10. Standard-Konfigurationen
* **Tests:** `native` ist aktuell die Default-Umgebung.
* **Serielle Diagnose:** Wenn serielle Ausgabe auf dem Target genutzt wird, `Serial.begin(115200)` und `monitor_speed = 115200` konsistent halten.

## 11. Erwartetes Antwort-Format
* Liefere bei neuen Dateien immer die vollständige Ordnerstruktur als Kommentar.
* Erkläre kurz die Pins, falls neue Hardware eingebunden wird.
* Zeige bei Updates der `platformio.ini` exakt, welche Zeilen unter `lib_deps` hinzugefügt werden müssen.

## 12. Unit-Testing (Unity Framework)
* **Framework:** Unity (in PlatformIO integriert).
* **Code-Zuschnitt:** 
  * Testbare Logik bleibt in den fachlichen Komponenten unter `src/`.
  * `src/main.cpp` dient nur als minimaler Einstiegspunkt für die Firmware und hält möglichst wenig Logik.
* **Ablage des Testcodes:** 
  * Alle Tests gehören in den Ordner `test/` im Projekt-Root.
  * Reine Host-Tests liegen unter `test/native/`.
  * Hardwareabhängige Tests liegen unter `test/embedded/`.
  * Lege pro Test-Suite einen eigenen Unterordner an und benenne die Testdateien eindeutig, z. B. `test_main.cpp`.
* **Test-Arten:**
  * **Native Tests:** Reine Logiktests ohne Hardware-Abhängigkeit auf dem Host-PC ausführen.
  * **Embedded Tests:** Tests, die ESP32-Peripherie oder Treiberanbindung benötigen, laufen direkt auf dem Board.
* **Test-Richtlinien:**
  * Nutze sprechende Testnamen: `test_[funktion]_[erwartetes_verhalten]`.
  * Nutze typsichere Unity-Makros passend zum C++-Standard (z. B. `TEST_ASSERT_EQUAL_UINT32`).
  * Mocke Hardware-Schnittstellen, um Kinematik, Validierung, Kalibrationsabbildung und Bewegungslogik isoliert zu testen.
* **CLI-Befehle für Codex:** `pio test`, gezielt `pio test -e native` oder `pio test -e esp32s3`.
