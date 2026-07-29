# Inverse Kinematik

Firmware-Projekt für einen Joy-it Grab-it 6-Achsen-Roboterarm auf einem Waveshare ESP32-S3-Board.

Der aktuelle Stand ist ein abgeschlossener PoC für:

* Robotik-Grundmodelle, Validierung und inverse Kinematik
* Bewegungsprofile und Motion-Orchestrierung
* PCA9685-basierte Servoausgabe mit Kalibration
* REST API für Bring-up, Diagnose und Sequenzen
* read-only Debug-Pfad für einen Nintendo Switch 2 Pro Controller über BLE
* Native-Tests für die zentrale fachliche Logik

## Projektstruktur

```text
src/
  application/
  common/
  hardware/
  orchestration/
  robotics/
test/
  native/
doc/
  hw/
  sw/
```

Die fachliche Projektbeschreibung liegt in `doc/projektbeschreibung.md`. Hardwareannahmen stehen in `doc/hw/hardware.md`; die umsetzungsnahe Softwaresicht steht in `doc/sw/software.md`.

## PlatformIO in WSL

CLI-Aufrufe sollen in diesem Projekt über den vorbereiteten PlatformIO-Interpreter laufen:

```bash
~/.platformio/penv/bin/pio test -e native
~/.platformio/penv/bin/pio run -e esp32s3
~/.platformio/penv/bin/pio run -e esp32s3 -t upload
```

Das Systemkommando `/usr/bin/pio` ist in diesem Setup veraltet und soll nicht verwendet werden.

`esp32s3` ist die aktuelle Firmware-Standardumgebung. Die alte Arduino-Umgebung `esp32s3_arduino_native` bleibt als schlankerer Vergleichs- und Bring-up-Build erhalten, enthält aber nicht den vollständigen Bluepad32-Controller-Pfad.

## Build-Konfigurationen

Das Projekt behält nach Abschluss des PoC bewusst zwei ESP32-S3-Konfigurationen:

| Environment | Rolle |
| --- | --- |
| `esp32s3` | Aktueller Firmware-Default für den Bluepad32-/Controller-Pfad |
| `esp32s3_arduino_native` | Schlanker Arduino-Vergleichs- und Bring-up-Build ohne vollständigen Bluepad32-Pfad |

Der Grund ist pragmatisch: Der Controller-PoC braucht aktuell den ESP-IDF-basierten Bluepad32-Aufbau mit lokalen Komponenten und grösserer Partition. Der ursprüngliche Arduino-Build ist aber weiterhin nützlich, um Basisteile des Projekts, Toolchain-Fragen und hardwarenahe Bring-up-Schritte mit weniger Fremdcode im Build zu prüfen.

Beide Environments teilen sich die gemeinsamen ESP32-S3-Grundwerte in `esp32s3_common`, insbesondere Boardprofil, Flashgrösse `8 MB`, serielle Ports, Monitor-Speed und gemeinsame Build-Flags. Das Board besitzt zusätzlich `8 MB` PSRAM; die Firmware lässt PSRAM aktuell bewusst deaktiviert und nutzt internes SRAM. Dadurch bleiben Unterschiede zwischen den Environments sichtbar und absichtlich:

* `esp32s3` nutzt `framework = espidf`, `partitions_bluepad32.csv` und `-DIK_REQUIRE_BLUEPAD32`.
* `esp32s3_arduino_native` nutzt `framework = arduino`, schliesst `src/bluepad32_app_main.c` aus und bleibt näher am ursprünglichen Firmware-Aufbau.

## WLAN-Konfiguration

Die lokale WLAN-Konfiguration ist bewusst nicht versioniert. Als Vorlage dient:

```text
src/config/WifiCredentials.example.h
```

Die echte Datei `src/config/WifiCredentials.h` bleibt lokal und wird von Git ignoriert.

## Lokale Git-Ausnahmen bei einem Dirty-Worktree

Für bereits versionierte Dateien, deren lokale Änderung vorübergehend nicht in
`git status` erscheinen soll, wird das Git-Flag `skip-worktree` verwendet. Das
ist ausschliesslich eine lokale Arbeitskopie-Einstellung: Es ändert weder
`.gitignore` noch den Repository-Inhalt und darf nicht für fachliche Änderungen
verwendet werden, die später eingecheckt werden sollen.

```bash
# Lokale Änderung einer versionierten Datei ausblenden.
git update-index --skip-worktree -- pfad/zur/datei

# Alle lokal ausgeblendeten versionierten Dateien anzeigen.
git ls-files -v | rg '^S '

# Die Datei wieder beachten; ihre lokale Änderung erscheint wieder in git status.
git update-index --no-skip-worktree -- pfad/zur/datei
git status --short
```

Vor einem Pull, Rebase oder einer Bearbeitung derselben Datei muss das Flag mit
`--no-skip-worktree` entfernt werden. Andernfalls können Änderungen aus dem
Repository für diese Datei übersehen werden. Das Verfahren gilt nur für bereits
versionierte Dateien. Für ausschliesslich lokale, noch nicht versionierte
Dateien kommt ein Eintrag in `.git/info/exclude` infrage; projektweit gültige
Ausnahmen gehören dagegen nach `.gitignore`.
