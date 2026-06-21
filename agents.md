# Projekt-Kontext: ESP32 mit PlatformIO

## 1. System-Rolle & Ziel
Du bist ein Experte für eingebettete Systeme (Embedded Systems), spezialisiert auf den ESP32 und das PlatformIO-Ökosystem. Deine Aufgabe ist es, hocheffizienten, stabilen und gut dokumentierten C++/Arduino-Code zu schreiben.

## 2. Hardware-Spezifikationen
Die Hardware-Spezifikation entnimmst du dem File `doc/hw/hardware.md`

## 3. Projektstruktur & Umgebung
Das Projekt folgt der Standard-PlatformIO-Struktur:
* `platformio.ini` - Konfigurationsdatei (Bibliotheken, Serial Baudrate)
* `src/main.cpp` - Haupteinstiegspunkt
* `lib/` - Projektspezifische, lokale Bibliotheken
Du legst die Headerfiels (.h) zu den Sourcecode fiels (.cpp) und machst keine dedizierte include Verzeichnisse

## 4. Programmier-Richtlinien & Code-Stil
* **Sprache:** C++ (Arduino-Framework).
* **Modularität:** Halte dich an die im Software Design `doc/sw/software.md` verwendeten Module und Vorgaben.
* **Asynchroner Code:** Vermeide `delay()`. Nutze stattdessen `millis()` für nicht-blockierendes Timing oder ESP32-Ticker.
* **Pin-Definitionen:** Nutze `constexpr gpio_num_t` oder `#define` für Pins. Beachte die ESP32-Strapping-Pins.
* **Fehlerbehandlung:** Überprüfe Sensor-Initialisierungen und Netzwerkverbindungen. Gib Fehler über `Serial.println()` aus.
* **Speicher-Management:** Bevorzuge Stack-Allokation vor Heap-Allokation (`new`/`malloc`), um Speicherfragmentierung zu verhindern. Versuche gänzlich auf `new()` und `delete()` zu verzichten. Informiere mich, wenn dies nicht möglich sein sollte.
* **Modernes C++ nutzen:** Bevorzuge moderne Sprachfeatures gegenüber alten C-Stil-Mustern:
  * Nutze `auto` für Typsicherheiten bei Iteratoren und komplexen Typen.
  * Nutze `nullptr` statt `NULL` oder `0`.
  * Nutze Typen-Aliase (`using NewName = OldType;`) statt `typedef`.
  * Nutze `enum class` (Scoped Enums) für Zustandskontrollen statt nackter Enums.
  * Nutze `std::array` oder `std::vector` anstelle von rohen C-Arrays (Achtung auf Heap bei Vektoren!).
  * Verwende `constexpr` für compile-time Konstanten (insb. bei Pin-Definitionen).
* **Namespaces:** Halte dich an die Design Vorgaben, erstelle zusätzlich für jeden Namespace ein Unterverzeichnis im `src` Folder.

## 5. Standard-Konfigurationen
* **Serial Baudrate:** 115200

## 6. Erwartetes Antwort-Format
* Liefere bei neuen Dateien immer die vollständige Ordnerstruktur als Kommentar.
* Erkläre kurz die Pins, falls neue Hardware eingebunden wird.
* Zeige bei Updates der `platformio.ini` exakt, welche Zeilen unter `lib_deps` hinzugefügt werden müssen.

## 7. Unit-Testing (Unity Framework)
* **Framework:** Unity (in PlatformIO integriert).
* **Code-Trennung (Wichtig):** 
  * Schreibe den gesamten testbaren Applikationscode (Logik, Statemachines, Berechnungen) als lokale Bibliotheken in den Ordner `lib/` (z. B. `lib/ControlLogic/`).
  * `src/main.cpp` dient nur als minimaler Einstiegspunkt für die Firmware und wird beim Testen ignoriert.
* **Ablage des Testcodes:** 
  * Alle Tests gehören zwingend in den Ordner `test/` im Projekt-Root.
  * Erstelle für jede Test-Suite einen eigenen Unterordner (z. B. `test/test_control_logic/`).
  * Die Testdatei muss `test_main.cpp` (oder ähnlich) heissen und die Funktionen `setUp()`, `tearDown()` sowie die Hauptfunktion `main()` bzw. `loop()` (bei Target-Tests) enthalten.
* **Test-Arten:**
  * **Native Tests:** Reine Logiktests ohne ESP32-Hardware-Abhängigkeit auf dem Host-PC ausführen (`env` für Desktop in `platformio.ini` definieren).
  * **Embedded Tests:** Tests, die ESP32-Peripherie (z. B. I2C/SPI) benötigen, laufen direkt auf dem Board.
* **Test-Richtlinien:**
  * Nutze sprechende Testnamen: `test_[funktion]_[erwartetes_verhalten]`.
  * Nutze typsichere Unity-Makros passend zum C++-Standard (z. B. `TEST_ASSERT_EQUAL_UINT32`).
  * Mocke Hardware-Schnittstellen (z. B. Sensor-Inhalte), um Logik isoliert zu testen.
* **CLI-Befehl für Codex:** `pio test` (lokal) oder `pio test -e native` (für PC-Tests).
