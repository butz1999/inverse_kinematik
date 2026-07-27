# PoC-Abschluss

Stand: PoC abgeschlossen.

Dieses Dokument hält den erreichten Proof-of-Concept-Stand knapp fest, damit die nächste Ausbaustufe nicht auf impliziten Bring-up-Notizen aufbaut.

## Erreichter Stand

Der aktuelle PoC zeigt, dass die wesentlichen Projektbausteine zusammengeführt werden können:

* Robotik-Grundmodelle für `TargetPose`, `JointState`, PWM-Zustand und Bewegungsplanung sind vorhanden.
* Validierung, Kinematik, Robot Model und Motion-Profile sind als Native-Logik testbar.
* Der Motion-Orchestrator kann Zielposen in geglättete Gelenkbewegungen überführen.
* Die Hardware-Schicht kapselt Status-LED, serielle Diagnose, PCA9685-Konfiguration, Kalibration und Servoausgabe.
* Die REST API stellt Health-, Status-, Motion-, PWM-, Sequence- und Controller-Debug-Endpunkte bereit.
* Der Nintendo Switch 2 Pro Controller kann im Debug-Pfad über BLE beobachtet werden; Eingaben lösen noch keine Servo-Bewegung aus.

## Bewusste PoC-Annahmen

* Das Waveshare ESP32-S3-Board wird in PlatformIO aktuell über `esp32-s3-devkitc-1` abgebildet.
* WSL nutzt im aktuellen Aufbau `/dev/ttyACM0` für Upload und seriellen Monitor.
* Die reale Roboterposition wird nicht sensorisch verifiziert; die Home Position wird vor dem Start manuell hergestellt.
* Bluepad32 und BTstack liegen lokal unter `components/`, inklusive projektbezogener PoC-Anpassungen.
* Der Controller-Pfad ist read-only und bleibt von Servoausgabe und Motion-Orchestrator getrennt.
* Die feste Controller-Erkennung und GATT-Details sind Bring-up-Annahmen, keine finale Produktabstraktion.
* Ein zusätzliches Sicherheitsmodell für manuelle Controller-Bedienung wird vorerst nicht eingeführt; die Software begrenzt auf Achsen-Limits, der Benutzer bleibt für riskante Bewegungen verantwortlich.

## Nächste sinnvolle Ausbaustufe

Vor neuer Funktionalität sollte der PoC in einen stabileren Projektstand überführt werden:

* Fremdcode-Strategie für Bluepad32/BTstack klären.
* Controller-Debug-Pfad technisch und dokumentarisch bereinigen.
* REST- und BLE-Doku mit dem aktuellen Verhalten abgleichen.
* Danach erst einen `ControllerMapper` entwerfen, der die dokumentierte digitale Jog-Zuordnung in fachliche Kommandos übersetzt.

Die nächste fachliche Entscheidung ist damit nicht "welche Servo-Bewegung bauen wir zuerst", sondern wie der dokumentierte digitale Jog-Zwischenstand sauber an die bestehende Motion- und Hardware-Ausgabe angebunden wird.
