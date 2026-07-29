# Fremdcode-Cleanup

Dieses Dokument beschreibt den aktuellen Umgang mit lokal eingebundenem Fremdcode nach Abschluss des Controller-PoC.

## Aktueller Bestand

Im Repository liegen derzeit mehrere externe Komponenten unter `components/`:

| Pfad | Rolle im Projekt |
| --- | --- |
| `components/ArduinoJson/` | ESP-IDF-Komponente für JSON im Bluepad32-Build |
| `components/AdafruitBusIO/` | Abhängigkeit der Adafruit-PWM-Servo-Driver-Komponente |
| `components/AdafruitPwmServoDriver/` | PCA9685-Treiber im ESP-IDF/Bluepad32-Build |
| `third_party/bluepad32/` | Gepinntes Bluepad32-Submodule; enthält den unveränderten Upstream-Quellbaum |
| `components/bluepad32` | Relativer Symlink auf die gepatchte Bluepad32-Komponente im Submodule |
| `patches/bluepad32/` | Versionierte Switch-2-Pro-Patch-Serie gegen den gepinnten Upstream-Commit |
| `components/bluepad32_arduino/` | Arduino-Wrapper für Bluepad32 |
| `components/btstack/` | Bluetooth-Stack, von Bluepad32 genutzt |
| `components/cmd_nvs/`, `components/cmd_system/` | ESP-IDF-Konsolen-/Hilfskomponenten |

Der Arduino-Build `esp32s3_arduino_native` nutzt weiterhin `lib_deps` aus `platformio.ini` und bleibt als schlankerer Vergleichs- und Bring-up-Build erhalten. Der aktuelle Firmware-Standard ist `esp32s3`; dieser Build nutzt lokale ESP-IDF-Komponenten, weil der Controller-PoC mit Bluepad32 in diesem Aufbau nicht als einfache Arduino-Library integrierbar war.

## Bezug zu den PlatformIO-Environments

Die doppelte Build-Konfiguration ist Teil der vorläufigen Fremdcode-Strategie und kein Versehen:

| Environment | Fremdcode-Bezug | Bewertung |
| --- | --- | --- |
| `esp32s3` | nutzt lokale ESP-IDF-Komponenten sowie Bluepad32 über Submodule und Patch-Serie | aktueller Arbeitsstand für den Controller-PoC |
| `esp32s3_arduino_native` | nutzt PlatformIO-`lib_deps` und schliesst den Bluepad32-Einstieg aus | Referenz für den ursprünglichen Arduino-basierten Firmware-Pfad |

Solange die Patch-Strategie für Bluepad32/BTstack noch nicht entschieden ist, reduziert der Erhalt beider Environments das Risiko beim Aufräumen:

* Der funktionierende Controller-PoC bleibt reproduzierbar.
* Der ursprüngliche Arduino-Pfad bleibt als Vergleich erhalten, falls ein Problem aus dem ESP-IDF-/Bluepad32-Stack stammt.
* Unterschiede zwischen Basishardware-Firmware und Controller-Integration bleiben in `platformio.ini` sichtbar.
* Die spätere Entscheidung "vendored Code, Fork, Submodule oder Patch-Serie" kann mit funktionierenden Referenz-Builds getroffen werden.

Wichtig ist dabei die Grenze: `esp32s3_arduino_native` ist kein zweiter gleichwertiger Produktpfad für Controller-Funktionen. Neue Controller-Arbeit soll gegen `esp32s3` verifiziert werden. `esp32s3_arduino_native` bleibt nur so lange wertvoll, wie er bei Bring-up, Vergleich oder Rückfallanalyse konkret hilft.

## Submodule- und Patch-Strategie

Bluepad32 wird als Submodule auf einen festen Upstream-Commit gepinnt. Das Hauptrepository enthält dadurch keinen vollständigen Bluepad32-Quellbaum mehr, sondern nur den Gitlink, das Submodule-Manifest und die projektbezogenen Patches.

`components/bluepad32` bleibt als relativer Symlink erhalten, damit der ESP-IDF-Build seine bisherige Komponentenstruktur weiterverwenden kann. Der Symlink zeigt auf `third_party/bluepad32/src/components/bluepad32`.

Der Bootstrap `scripts/bootstrap.sh` initialisiert das Submodule und wendet alle Patches unter `patches/bluepad32/` idempotent an. Er ist ein Vorbereitungsschritt für den ESP-IDF-/Bluepad32-Build, nicht für die native PlatformIO-Umgebung.

Für Bluepad32 gilt:

* Neue fachliche Controller-Logik gehört nach `src/application/` oder `src/orchestration/`.
* Änderungen im Submodule erfolgen nicht als Commit im Fremdprojekt, sondern als nummerierter Patch im Hauptrepository.
* Ein Upstream-Update beginnt mit einem neuen gepinnten Submodule-Commit; anschließend wird die Patch-Serie übertragen, angewendet und verifiziert.
* `managed_components/` bleibt generiert und wird nicht versioniert.
