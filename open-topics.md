# Offene Themen nach PoC-Abschluss

Der PoC ist abgeschlossen. Diese Liste sammelt bewusst die nächsten Aufräum- und Designentscheidungen, bevor Controller-Eingaben echte Roboterbewegungen auslösen.

## Nächster Cleanup-Schnitt

* ~~Fremdcode-Strategie für `components/bluepad32/` und `components/btstack/` entscheiden: vendored Code, Fork, Submodule oder Patch-Serie.~~ Vorläufige Strategie in [doc/sw/third-party-cleanup.md](doc/sw/third-party-cleanup.md) dokumentiert.
* ~~PoC-Anpassungen in `components/bluepad32/` identifizieren und dokumentieren.~~ Patch-Inventar in [doc/sw/ble-design.md](doc/sw/ble-design.md) ergänzt.
* ~~Prüfen, ob `managed_components/` vollständig generiert bleibt und nicht versioniert werden muss.~~ Ist in `.gitignore` ausgeschlossen und bleibt generiert.
* ~~Doku in [doc/sw/ble-design.md](doc/sw/ble-design.md) gegen den aktuellen Code gegenlesen.~~ Aktuelle Bestandsaufnahme ergänzt.
* ~~Kapitel "Offene Punkte" in [doc/sw/ble-design.md](doc/sw/ble-design.md) nach dem Cleanup aktualisieren.~~ Offene Punkte nachgeschärft.

## Controller-Design vor echter Bewegung

* Festlegen, welche Controller-Inputs welche Achsen, Posen oder Aktionen steuern. Digitaler Jog-Zwischenstand in [doc/sw/controller.md](doc/sw/controller.md) dokumentiert.

## Parkierte Punkte

* ~~Kalibration der Analogwerte?~~ Wird aktuell über Switch / Steam direkt im Controller behandelt.
* Analogsticks für manuelle Steuerung bleiben vorerst bewusst ausserhalb des nächsten Schritts.
* Ein zusätzliches Sicherheitsmodell für manuelle Steuerung wird bewusst nicht umgesetzt. Die Software begrenzt auf Achsen-Limits; der Benutzer bleibt für bewusst riskante Bewegungen verantwortlich.
* Die feste Controller-MAC bleibt vorerst bewusst als PoC-/Debug-Annahme bestehen und wird nicht konfigurierbar gemacht.
* Eine saubere Patch-Strategie für die Bluepad32/BTstack-Anpassungen wird später entschieden. Der aktuelle vendored Stand bleibt vorerst bestehen.
* `pio run -e esp32s3 -t metrics` bleibt wegen `esp-idf-size --ng` im PlatformIO-Fork unverändert. Für Firmware-Grössen `scripts/firmware_metrics.sh` verwenden.
* ~~Wahre Events über WebSocket?~~ Aufwändig; für den nächsten Schritt nicht erforderlich.
