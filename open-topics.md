# Offene Themen nach PoC-Abschluss

Der PoC ist abgeschlossen. Der digitale Controller-Jog, der Hardware-Build-up und die Fremdcode-Strategie sind für den aktuellen Aufbau umgesetzt und dokumentiert.

## Parkierte Punkte

* ~~Kalibration der Analogwerte?~~ Wird aktuell über Switch / Steam direkt im Controller behandelt.
* Analogsticks für manuelle Steuerung bleiben vorerst bewusst ausserhalb des nächsten Schritts.
* Ein zusätzliches Sicherheitsmodell für manuelle Steuerung wird bewusst nicht umgesetzt. Die Software begrenzt auf Achsen-Limits; der Benutzer bleibt für bewusst riskante Bewegungen verantwortlich.
* Die feste Controller-MAC bleibt vorerst bewusst als PoC-/Debug-Annahme bestehen und wird nicht konfigurierbar gemacht.
* Authentisierung und Zugriffsschutz gehören nicht zum aktuellen Embedded-Firmware-Umfang. Falls sie erforderlich werden, werden sie in einer späteren Linux-basierten Systemarchitektur bewertet.
* Alternative IK-Solver und kartesische Bahnplanung werden nicht umgesetzt. Die analytische IK und gelenkraumorientierte Bewegungsprofile sind für die aktuelle Hardware ausreichend; die Totzonen der Hardware rechtfertigen keine kartesische Stützpunktplanung.
* Bewegungsprofil-Grenzen bleiben global konfiguriert und werden nicht pro `MotionRequest` übergeben.
* `pio run -e esp32s3 -t metrics` bleibt wegen `esp-idf-size --ng` im PlatformIO-Fork unverändert. Für Firmware-Grössen `scripts/firmware_metrics.sh` verwenden.
* ~~Wahre Events über WebSocket?~~ Aufwändig; für den nächsten Schritt nicht erforderlich.
