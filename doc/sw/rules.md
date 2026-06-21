# Software-Regeln

## Ziel und Einordnung

Dieses Dokument ergänzt `doc/sw/software.md` um konkrete, pragmatische Arbeitsregeln für die Implementierung im aktuellen ESP32-S3-/PlatformIO-Setup. Es dient als kurze Leitlinie für tägliche Entwicklungsentscheidungen und soll insbesondere helfen, moderne C++-Mittel sinnvoll zu nutzen, ohne die Randbedingungen eines eingebetteten Systems aus dem Blick zu verlieren.

## Grundprinzipien

Für dieses Projekt gelten die folgenden Grundprinzipien:

* Lesbarkeit und Nachvollziehbarkeit gehen vor Cleverness.
* Einfache, robuste Lösungen sind gegenüber unnötig allgemeinen Abstraktionen zu bevorzugen.
* Hardwarenahe Besonderheiten des ESP32-S3 und des aktuellen WSL-Workflows sind bei Implementierungsentscheidungen mitzudenken.
* Moderne C++-Sprachmittel sind erwünscht, sofern sie Build, Laufzeitverhalten und Wartbarkeit nicht unnötig verschlechtern.
* Die fachliche Modulstruktur aus `doc/sw/software.md` bleibt leitend.

## Bevorzugte C++-Mittel

Die folgenden Sprachmittel und Stilmittel sind im Projekt ausdrücklich erwünscht:

* `constexpr` für Konstanten mit Compile-Time-Bedeutung
* `enum class` für klar abgegrenzte Zustände
* `nullptr` statt `NULL` oder `0`
* `using` statt `typedef`
* Konstruktoren zur sauberen Initialisierung kleiner Klassen
* `const`-Korrektheit bei lesenden Methoden
* kleine, klar benannte Klassen mit enger Verantwortung
* `std::array`, wenn eine feste Anzahl von Elementen modelliert wird
* Referenzen und Stack-Allokation statt unnötiger Heap-Nutzung

## Vorsicht bei Standardbibliothek und Heap-Nutzung

Die Nutzung moderner C++-Features ist nicht mit einer unkritischen Nutzung der gesamten C++-Standardbibliothek gleichzusetzen. Insbesondere in der aktuellen Arduino-/ESP32-Toolchain müssen einige Bibliothekstypen bewusst und zurückhaltend eingesetzt werden.

### `std::string`

`std::string` ist in diesem Projekt nicht grundsätzlich verboten, aber mit Vorsicht zu verwenden.

Regeln für den Einsatz:

* `std::string` nicht blind in zeitkritischen Pfaden verwenden
* keine unnötige String-Verkettung in `loop()` oder in häufig aufgerufenen Funktionen
* keine häufigen temporären Strings für serielle Debug-Ausgaben erzeugen
* bei erwartbar wachsendem Inhalt nach Möglichkeit `reserve(...)` verwenden
* Strings möglichst per `const std::string&` weiterreichen, wenn Ownership nicht benötigt wird
* `std::string` und Arduino-`String` nicht unnötig mischen
* bei instabilem Laufzeitverhalten oder Speicherproblemen die Nutzung von `std::string` zuerst kritisch prüfen

`std::string` ist eher geeignet für:

* seltene Konfigurationsverarbeitung
* nicht zeitkritische Hilfslogik
* klar begrenzte Textverarbeitung außerhalb enger Laufzeitschleifen

`std::string` ist eher ungeeignet für:

* dauerhafte Log-Erzeugung in schnellen Schleifen
* häufige Statusformatierung im laufenden Betrieb
* Pfade mit strengen Laufzeit- oder Speicheranforderungen

### Weitere Standardbibliothek

Die folgenden Typen oder Konzepte sollen nur bewusst und nach kurzer Prüfung im echten Projektbuild eingesetzt werden:

* `std::vector`
* `std::function`
* `std::optional`
* `std::variant`
* `std::string_view`
* schwergewichtige Algorithmen oder Container, deren Laufzeit- und Allokationsverhalten für den konkreten Einsatz nicht klar ist

Hintergrund ist nicht, dass diese Mittel grundsätzlich ungeeignet wären, sondern dass Toolchain-Unterstützung, Speicherverhalten und Debugbarkeit im Embedded-Umfeld stärker schwanken als auf einem typischen Desktop-System.

## Regeln für Logging und Textschnittstellen

Für den aktuellen Projektstand gelten die folgenden Leitlinien:

* hardwarenahe serielle Ausgabe gehört in `hardware/`
* fachliche Komponenten sollen keine direkte Kenntnis von `Serial` oder vergleichbaren Hardwareobjekten haben
* einfache Textschnittstellen dürfen zunächst mit `const char *` arbeiten, wenn dies den Code robuster und verständlicher hält
* allgemeinere Logger-Abstraktionen sollen erst dann eingeführt werden, wenn sie durch mehrere Komponenten tatsächlich benötigt werden

## Regeln für Speicher und Lebensdauer

* Stack-Allokation ist gegenüber Heap-Allokation zu bevorzugen
* `new`, `delete`, `malloc` und `free` sollen nach Möglichkeit vermieden werden
* Objektlebensdauern sollen klar aus Konstruktion und Besitzverhältnissen hervorgehen
* globale Objekte sind nur dann sinnvoll, wenn sie stabile, hardwarenahe Infrastruktur repräsentieren und keine versteckten Initialisierungsprobleme erzeugen

## Regeln für `main.cpp`

`src/main.cpp` bleibt der schlanke Einstiegspunkt der Firmware.

Daraus folgen:

* `main.cpp` verdrahtet Komponenten
* `main.cpp` enthält möglichst wenig fachliche Logik
* wiederverwendbare Hardwarelogik wird in `hardware/` ausgelagert
* fachliche oder mathematische Logik wird nicht in `main.cpp` entwickelt

## Verifikation

Wenn neue Sprachmittel, neue Standardbibliothekstypen oder neue Toolchain-abhängige Konstrukte eingeführt werden, sollen sie nicht nur im Editor, sondern im echten PlatformIO-Build geprüft werden.

Für den aktuellen WSL-Workflow ist dafür insbesondere relevant:

* Build und Upload mit dem projektnahen PlatformIO unter `~/.platformio/penv/bin/pio`
* serielle Prüfung über den aktuell bestätigten Port `/dev/ttyACM0`

## Entscheidungsregel im Zweifel

Wenn zwischen einer eleganter wirkenden abstrakten Lösung und einer technisch robusteren einfachen Lösung gewählt werden muss, wird im Zweifel die robustere einfache Lösung bevorzugt.

Erst wenn ein echter Bedarf für mehr Allgemeinheit, Austauschbarkeit oder Wiederverwendung sichtbar wird, soll die Abstraktion gezielt nachgezogen werden.
