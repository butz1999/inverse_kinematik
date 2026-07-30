# HTTP-Server-Design

## Zweck

Dieses Dokument beschreibt die technischen Randbedingungen des HTTP-Servers auf dem ESP32-S3. Er stellt dieselbe Anwendungsschnittstelle auf Port `80` bereit:

* REST-Endpunkte unter `/api/...`
* die statische Browser-Oberfläche aus dem SPIFFS-Dateisystem

Die fachliche API ist in [rest_api.md](./rest_api.md) definiert. Dieses Dokument beschreibt dagegen Transport-, Auslieferungs- und Laufzeitverhalten.

## Aktueller Aufbau

Der `RestApiServer` verwendet den synchronen Arduino-`WebServer`. `main.cpp` mountet die vorhandene SPIFFS-Partition `storage` ohne automatisches Formatieren. PlatformIO baut deren Inhalt aus `web/` und lädt ihn mit `uploadfs`.

`GET /` liefert `dilbert.html`; weitere `GET`-Anfragen, die nicht zu einer REST-Route gehören, werden als statische Dateien aus SPIFFS aufgelöst. Die REST-API und die Browser-Oberfläche bleiben damit unter derselben Origin erreichbar. CORS bleibt für lokal geöffnete oder separat gehostete Entwicklungsoberflächen aktiviert.

## Laufzeit-Constraint: ein synchroner Bedienpfad

`WebServer::handleClient()`, Motion-Samples, Run-Engine-Service, Controller-Service und Status-LED werden nacheinander durch die Arduino-`loop()` bedient. Es gibt keine parallele fachliche Ausführung und keine ausgehende HTTP-Response-Queue.

Eine laufende Bewegungssequenz führt pro Loop-Durchlauf höchstens ein fälliges Motion-Sample mit zugehöriger PCA9685-Ausgabe aus. Eine laufende Dateiauslieferung verzögert daher diese Services; sie dürfen ihrerseits nicht voraussetzen, während einer HTTP-Antwort gleichzeitig zu laufen.

## TCP-Backpressure und Dateiauslieferung

Browser laden HTML, CSS, JavaScript, Favicon und anschließend REST-Zustände normalerweise parallel. In Verbindung mit dem synchronen Server führte dies zu `EAGAIN`-Backpressure und unvollständigen Antworten trotz HTTP-Status `200` und `Content-Length`.

Die statische Auslieferung verwendet deshalb kleine, nichtblockierende TCP-Schreibblöcke. Bei fehlendem Sendepuffer gibt die Loop-Task dem Netzwerk-Task für einen FreeRTOS-Tick Zeit. Nach fünf Sekunden wird die Übertragung abgebrochen und mit Pfad sowie gesendeter und erwarteter Bytezahl seriell protokolliert. Dieser Pfad ist ein begrenzter Zuverlässigkeits-Workaround; er ersetzt keinen asynchronen HTTP-Server.

Der direkte Socket-Zugriff bleibt auf statische Antwortdaten beschränkt. REST-Antworten werden weiterhin durch `WebServer` erzeugt.

## Browser-Strategie

`dilbert.html` lädt ihre Assets absichtlich seriell in dieser Reihenfolge:

1. `dilbert.css`
2. `controller-reporting.js`
3. `dilbert.js`
4. `favicon.png`

REST-Anfragen starten erst, nachdem das Hauptskript geladen wurde. Ein fehlgeschlagenes CSS- oder JavaScript-Asset wird bis zu dreimal erneut angefordert. Diese Reihenfolge reduziert gleichzeitig offene TCP-Verbindungen und verhindert konkurrierende Asset- und Statusanfragen beim Start.

Die Browser-Oberfläche verwendet relative `/api/...`-URLs nur, wenn sie von `robot.local`, `robot.fritz.box` oder einer IPv4-Adresse ausgeliefert wird. Bei lokalem `file:`-Zugriff oder einem Entwicklungsserver bleibt die manuelle Ziel-URL sichtbar.

## Cache und Deployment

HTML wird mit `Cache-Control: no-store` ausgeliefert. Versionierte statische Assets dürfen langfristig gecacht werden. Die Versionskennung `assetVersion` in `web/dilbert.html` ist daher bei jeder Änderung von CSS, JavaScript oder Favicon zu erhöhen. Die Asset-Anfragen enthalten diese Kennung als Query-Parameter; der Server entfernt sie vor dem SPIFFS-Dateizugriff.

Für ein vollständiges Deployment sind Firmware und Dateisystem getrennt zu laden:

```bash
~/.platformio/penv/bin/pio run -e esp32s3 -t upload
~/.platformio/penv/bin/pio run -e esp32s3 -t uploadfs
```

Reine Asset-Änderungen benötigen nur `uploadfs`; Änderungen am HTTP-Server benötigen zusätzlich den Firmware-Upload. In WSL muss der konfigurierte serielle Port zum Board durchgereicht sein.

## Grenzen und Weiterentwicklung

Der synchrone Server ist für die aktuelle kleine HMI akzeptabel, solange die serielle Browser-Ladestrategie eingehalten wird. Bei größeren Assets, mehreren gleichzeitigen Bedienclients, Server-Sent Events, WebSockets oder anspruchsvoller Telemetrie ist eine Migration der gesamten HTTP-Schicht auf einen asynchronen Server erforderlich. Sie muss alle REST-Routen auf Port `80` gemeinsam migrieren; getrennte Static- und REST-Server auf unterschiedlichen Ports würden die Same-Origin-Schnittstelle verändern.
