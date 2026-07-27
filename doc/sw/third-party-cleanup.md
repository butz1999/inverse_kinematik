# Fremdcode-Cleanup

Dieses Dokument beschreibt den aktuellen Umgang mit lokal eingebundenem Fremdcode nach Abschluss des Controller-PoC.

## Aktueller Bestand

Im Repository liegen derzeit mehrere externe Komponenten unter `components/`:

| Pfad | Rolle im Projekt |
| --- | --- |
| `components/ArduinoJson/` | ESP-IDF-Komponente für JSON im Bluepad32-Build |
| `components/AdafruitBusIO/` | Abhängigkeit der Adafruit-PWM-Servo-Driver-Komponente |
| `components/AdafruitPwmServoDriver/` | PCA9685-Treiber im ESP-IDF/Bluepad32-Build |
| `components/bluepad32/` | Bluepad32-Kern inklusive projektbezogener Switch-2-Pro-PoC-Anpassungen |
| `components/bluepad32_arduino/` | Arduino-Wrapper für Bluepad32 |
| `components/btstack/` | Bluetooth-Stack, von Bluepad32 genutzt |
| `components/cmd_nvs/`, `components/cmd_system/` | ESP-IDF-Konsolen-/Hilfskomponenten |

Der Arduino-Build `esp32s3_arduino_native` nutzt weiterhin `lib_deps` aus `platformio.ini` und bleibt als schlankerer Vergleichs- und Bring-up-Build erhalten. Der aktuelle Firmware-Standard ist `esp32s3`; dieser Build nutzt lokale ESP-IDF-Komponenten, weil der Controller-PoC mit Bluepad32 in diesem Aufbau nicht als einfache Arduino-Library integrierbar war.

## Bezug zu den PlatformIO-Environments

Die doppelte Build-Konfiguration ist Teil der vorläufigen Fremdcode-Strategie und kein Versehen:

| Environment | Fremdcode-Bezug | Bewertung |
| --- | --- | --- |
| `esp32s3` | nutzt lokal vendorte ESP-IDF-Komponenten unter `components/` | aktueller Arbeitsstand für den Controller-PoC |
| `esp32s3_arduino_native` | nutzt PlatformIO-`lib_deps` und schliesst den Bluepad32-Einstieg aus | Referenz für den ursprünglichen Arduino-basierten Firmware-Pfad |

Solange die Patch-Strategie für Bluepad32/BTstack noch nicht entschieden ist, reduziert der Erhalt beider Environments das Risiko beim Aufräumen:

* Der funktionierende Controller-PoC bleibt reproduzierbar.
* Der ursprüngliche Arduino-Pfad bleibt als Vergleich erhalten, falls ein Problem aus dem ESP-IDF-/Bluepad32-Stack stammt.
* Unterschiede zwischen Basishardware-Firmware und Controller-Integration bleiben in `platformio.ini` sichtbar.
* Die spätere Entscheidung "vendored Code, Fork, Submodule oder Patch-Serie" kann mit funktionierenden Referenz-Builds getroffen werden.

Wichtig ist dabei die Grenze: `esp32s3_arduino_native` ist kein zweiter gleichwertiger Produktpfad für Controller-Funktionen. Neue Controller-Arbeit soll gegen `esp32s3` verifiziert werden. `esp32s3_arduino_native` bleibt nur so lange wertvoll, wie er bei Bring-up, Vergleich oder Rückfallanalyse konkret hilft.

## Bewertung

Für den aktuellen Stand bleibt die lokale Ablage unter `components/` bewusst erhalten. Sie hat zwei Vorteile:

* Der hardwaregetestete BLE-PoC bleibt reproduzierbar.
* Die projektbezogenen Bluepad32-Änderungen sind im Repository sichtbar und nicht nur als lokale Toolchain-Magie vorhanden.

Der Nachteil ist, dass Upstream-Code und Projektpatches vermischt sind. Dadurch wird ein späteres Update von Bluepad32 oder BTstack schwierig, weil unklar ist, welche Zeilen Projektlogik und welche Zeilen Fremdcode sind.

## Vorläufige Strategie

Bis zur nächsten funktionalen Controller-Stufe gilt:

* `components/` bleibt versioniert und wird nicht automatisch bereinigt.
* `managed_components/` bleibt generiert und wird nicht versioniert.
* Neue fachliche Controller-Logik gehört nach `src/application/`, nicht tiefer in Bluepad32.
* Änderungen in `components/bluepad32/` sollen nur erfolgen, wenn sie für Verbindung, GATT-Discovery oder Notification-Empfang erforderlich sind.
* Jede neue Anpassung in `components/bluepad32/` soll mit einer klaren `Switch 2 Pro`- oder `GATT-POC`-Markierung auffindbar bleiben.

## Empfohlener nächster Schnitt

Vor echter manueller Robotersteuerung sollte der Fremdcode-Schnitt weiter stabilisiert werden:

1. Projektbezogene Stellen in `components/bluepad32/bt/uni_bt_le.c` als kleine Patch-Liste extrahieren.
2. Entscheiden, ob diese Patches langfristig in einem Fork, als Submodule-plus-Patch-Serie oder weiter vendored gehalten werden.
3. Die feste Controller-MAC in eine lokale Konfiguration oder eine robuste Erkennung überführen.
4. Erst danach einen `ControllerMapper` bauen, der Eingaben in fachliche Kommandos übersetzt.

Diese Punkte sind nicht Teil des unmittelbaren nächsten Schritts. Der aktuelle vendored Stand bleibt vorerst die Arbeitsgrundlage.
