# Software

## Einleitung

Dieses Dokument beschreibt die umsetzungsnahe Softwaresicht des Projekts. Es ergänzt die Projektbeschreibung um die konkrete Perspektive auf Implementierung, Modulzuschnitt, Datenmodelle, Initialisierung und Testbarkeit.

## Zweck und Abgrenzung

Dieses Dokument beschreibt die Softwaresicht des Projekts in einer umsetzungsnahen Form. Es steht damit zwischen der fachlich und architektonisch geprägten Projektbeschreibung und der konkreten Implementierung in `src/`.

Die Projektbeschreibung definiert insbesondere Ziele, Randbedingungen, Architekturbausteine, Datenflüsse und fachliche Modelle. Dieses Dokument wiederholt diese Inhalte nicht vollständig, sondern konkretisiert sie im Hinblick auf die spätere Umsetzung im Code. Im Mittelpunkt stehen daher vor allem Modulzuschnitt, Verzeichnisstruktur, Datenmodelle, Schnittstellen, Initialisierung, Kalibration, Fehlerbehandlung und Teststruktur.

Bewusst nicht Gegenstand dieses Dokuments sind ausführliche fachliche Herleitungen der Inversen Kinematik, allgemeine Hardwarebeschreibungen oder eine erneute vollständige Darstellung der Softwarearchitektur auf konzeptioneller Ebene. Diese Inhalte bleiben in der Projektbeschreibung beziehungsweise in der Hardwarebeschreibung verankert.

Ebenso ersetzt dieses Dokument nicht den Quellcode selbst. Konkrete Implementierungsdetails, private Hilfsfunktionen, temporäre Workarounds oder kleinteilige technische Entscheidungen sollen nicht vollständig in der Dokumentation dupliziert werden, sondern primär in `src/` sichtbar sein. Das Dokument soll stattdessen diejenige Struktur und Terminologie festhalten, die für ein konsistentes Verständnis und eine nachvollziehbare Weiterentwicklung der Software erforderlich ist.

## Implementationsziele

Die erste Software-Ausbaustufe soll eine fachlich nachvollziehbare und technisch beherrschbare Grundlage für die Steuerung des Roboterarms schaffen. Im Vordergrund steht nicht die maximale funktionale Breite, sondern ein schrittweise erweiterbarer Kern aus Ablaufsteuerung, Kinematik, Prüfung, Kalibration und hardwarenaher Ausgabe.

Konkret soll die Implementierung zunächst insbesondere folgende Ziele erfüllen:

* Abbildung der in der Projektbeschreibung beschriebenen fachlichen Kernmodelle in klar benannte C++-Strukturen
* Umsetzung einer einfachen, testbaren Modulstruktur mit klaren Verantwortlichkeiten
* sequentielle Verarbeitung einzelner Bewegungsanforderungen über `Run Engine`, `Orchestrator`, `Validation`, `Kinematics`, `Calibration` und `Hardware`
* reproduzierbare Initialisierung auf Basis der definierten Startlage und der angenommenen Initialwerte
* nachvollziehbare Abbildung zwischen fachlichen Gelenkwerten und realen Stellwerten
* frühzeitige Testbarkeit mathematischer und fachlicher Kernlogik unabhängig von der realen Hardware

Für die erste Ausbaustufe sollen bestimmte Aspekte bewusst einfach gehalten werden. Dazu gehören insbesondere:

* eine klar lesbare und deterministische Verarbeitung statt frühzeitiger Optimierung
* eine einfache und direkte Datenweitergabe zwischen den Modulen
* eine begrenzte Anzahl gut verständlicher Schnittstellen
* Verzicht auf unnötige Abstraktion, solange sie für die konkrete Implementierung noch keinen praktischen Nutzen bringt

Darüber hinaus verfolgt die Implementierung folgende Qualitätsziele:

* gute Nachvollziehbarkeit der fachlichen Datenflüsse
* klare Trennung zwischen fachlicher Logik und hardwarenaher Ansteuerung
* leichte Erweiterbarkeit für spätere Iterationen
* gute lokale Testbarkeit zentraler Komponenten
* konsistente Benennung zwischen Dokumentation, Verzeichnisstruktur und Quellcode

## Geplante Modulstruktur

In diesem Kapitel sollte beschrieben werden:

* welche Hauptmodule in `src/` erwartet werden
* wie die Trennung zwischen `Application`, `Orchestration`, `Robotics`, `Hardware` und gemeinsamen Datenmodellen im Code abgebildet wird
* welche Abhängigkeitsrichtung zwischen den Modulen erlaubt ist
* welche Teile als Bibliothek, Komponente oder einfacher Quellcodeblock umgesetzt werden könnten
* dass Header- und Implementierungsdateien für den Projektstart bewusst nicht in getrennten Hauptordnern geführt werden, sondern pro Komponente nebeneinander liegen
* dass jede grössere Komponente einen eigenen Ordner in `src/` erhält
* dass die Namen dieser Komponentenordner zugleich als Grundlage für die späteren C++-Namespaces verwendet werden

Für die erste Ausbaustufe wird damit folgende Strukturentscheidung getroffen:

* die Implementierung liegt vollständig unter `src/`
* `.h`- und `.cpp`-Dateien einer Komponente liegen im selben Komponentenordner
* grössere Bausteine wie `application`, `orchestration`, `robotics`, `hardware` und gegebenenfalls `common` werden als eigene Ordner angelegt
* die Ordnernamen sollen möglichst direkt als C++-Namespaces wiederverwendet werden
* dadurch bleiben fachliche Struktur, Verzeichnisstruktur und Code-Namensräume möglichst deckungsgleich

## Verzeichnisstruktur

Dieses Kapitel beschreibt die konkrete Ablage der Softwareartefakte im Repository. Im Unterschied zur Modulstruktur steht hier nicht die fachliche Verantwortung der Bausteine im Vordergrund, sondern die praktische Frage, wo Quellcode, Dokumentation, Konfiguration und Tests abgelegt werden.

Für die erste Ausbaustufe wird folgende Grundstruktur vorgesehen:

```text
sw/
  software.md

src/
  main.cpp
  application/
  orchestration/
  robotics/
  hardware/
  common/

test/
  native/
  embedded/
```

Dabei gelten zunächst die folgenden Strukturregeln:

* `sw/` enthält die softwarebezogene Dokumentation und keine Implementierung
* `src/` enthält die produktive Implementierung des Projekts
* `main.cpp` bildet den Einstiegspunkt der Firmware und hält selbst möglichst wenig fachliche Logik
* jeder grössere Softwarebaustein erhält unter `src/` einen eigenen Komponentenordner
* `.h`- und `.cpp`-Dateien liegen innerhalb eines Komponentenordners nebeneinander
* `test/` enthält den automatisierten Testcode getrennt nach nativer und eingebetteter Ausführung

Für die Komponentenordner unter `src/` ist in der ersten Ausbaustufe grob folgende Aufteilung vorgesehen:

* `src/application/` für Run Engine und anwendungsnahe Ablaufsteuerung
* `src/orchestration/` für Orchestrator und Koordination der Verarbeitungsschritte
* `src/robotics/` für Kinematik, Validierung, Kalibration und robotiknahe Datenmodelle
* `src/hardware/` für HAL, Treiberanbindung und hardwarebezogene Ausgabe
* `src/common/` für gemeinsame, modulübergreifend verwendete Datentypen und Hilfsstrukturen

Für den ersten lauffähigen Stand werden mindestens folgende Dateien oder gleichwertige Strukturen erwartet:

* `src/main.cpp` als Programmeinstieg
* eine erste Implementierung der Run Engine oder eines einfachen Startablaufs
* ein erster Orchestrator als zentrale Koordinationskomponente
* grundlegende Datenmodelle für Zielbeschreibung, Gelenksollzustand, Bewegungsanforderung und Bewegungsergebnis
* ein erster Hardwarezugang für PCA9685 und Servo-Ausgabe
* erste Testdateien unter `test/native/` für fachliche Kernlogik

Ein separater Hauptordner `include/` ist für die erste Ausbaustufe bewusst nicht vorgesehen. Sollte sich später eine klarere Trennung zwischen öffentlicher Schnittstelle und interner Implementierung als hilfreich erweisen, kann diese Struktur zu einem späteren Zeitpunkt gezielt nachgeschärft werden.

## Zentrale Datenmodelle

Dieses Kapitel konkretisiert die in der Projektbeschreibung beschriebenen fachlichen Modelle in eine softwarebezogene Form. Im Vordergrund steht dabei nicht die endgültige C++-Syntax, sondern eine klare und konsistente Beschreibung der Datenstrukturen, ihrer Beziehungen und ihrer fachlichen Bedeutung.

Für die erste Ausbaustufe sollen die Datenmodelle bewusst nah an den dokumentierten Begriffen aus Task Space, Joint Space, Ablaufsteuerung und Kalibration bleiben. Dadurch kann die spätere Implementierung direkt aus den hier beschriebenen Strukturen abgeleitet werden.

Für die Benennung der zentralen Zustandsmodelle werden bewusst die Begriffe `TargetPose` und `JointState` verwendet. Auf Bezeichnungen wie `TargetVector` oder `JointVector` wird verzichtet, obwohl die Modelle jeweils mehrere Werte in fester Reihenfolge zusammenfassen. Der Grund ist, dass es sich hier nicht um mathematische Vektoren im engeren Sinn handelt, sondern um fachliche Zustandscontainer mit unterschiedlichen Bedeutungen und Einheiten. Insbesondere enthält die Zielbeschreibung mit `(x, y, z, p, r, g)` sowohl kartesische Positionen als auch Winkel- und Prozentwerte. `TargetPose` und `JointState` machen deshalb klarer, dass die Modelle eine fachliche Pose beziehungsweise einen Gelenkzustand beschreiben und nicht primär algebraische Vektoroperationen repräsentieren.

Das folgende Übersichtsdiagramm zeigt die zentralen Modelle und ihre Beziehungen:

```mermaid
classDiagram
    class TargetPose {
        +float x_mm
        +float y_mm
        +float z_mm
        +float p_deg
        +float r_deg
        +float g_pct
    }

    class JointState {
        +float d_deg
        +float s_deg
        +float e_deg
        +float hp_deg
        +float hr_deg
        +float g_pct
    }

    class MotionRequest {
        +TargetPose target
        +bool has_wait
        +uint32 wait_ms
    }

    class MotionResult {
        +MotionStatus status
        +bool has_joint_state
        +JointState joint_state
        +ResultCode code
    }

    class CalibrationData {
        +AxisCalibration d
        +AxisCalibration s
        +AxisCalibration e
        +AxisCalibration hp
        +AxisCalibration hr
        +GripperCalibration g
    }

    class AxisCalibration {
        +float zero_deg
        +float min_deg
        +float max_deg
        +uint16 pwm_min
        +uint16 pwm_max
        +bool inverted
    }

    class GripperCalibration {
        +float min_pct
        +float max_pct
        +uint16 pwm_min
        +uint16 pwm_max
        +bool inverted
    }

    MotionRequest --> TargetPose
    MotionResult --> JointState
    CalibrationData *-- AxisCalibration
    CalibrationData *-- GripperCalibration
```

Die fachliche Trennung zwischen Task Space und Joint Space bleibt dabei zentral:

* `TargetPose` beschreibt den Sollzustand des Endeffektors im Task Space mit `(x, y, z, p, r, g)`
* `JointState` beschreibt die berechnete Gelenkkonfiguration im Joint Space mit `(d, s, e, hp, hr, g)`
* `g` bleibt in beiden Modellen bewusst dieselbe fachliche Größe, nämlich die Greiferöffnung in Prozent

### TargetPose

`TargetPose` ist das zentrale Eingabemodell für Bewegungsziele im Task Space.

```mermaid
classDiagram
    class TargetPose {
        +float x_mm
        +float y_mm
        +float z_mm
        +float p_deg
        +float r_deg
        +float g_pct
    }
```

Bedeutung der Felder:

* `x_mm`, `y_mm`, `z_mm`: kartesische Position des Endeffektors in Millimetern
* `p_deg`: Pitch im Task Space in Grad
* `r_deg`: Roll im Task Space in Grad
* `g_pct`: Greiferöffnung in Prozent

### JointState

`JointState` beschreibt das Ergebnis der kinematischen Berechnung im Joint Space.

```mermaid
classDiagram
    class JointState {
        +float d_deg
        +float s_deg
        +float e_deg
        +float hp_deg
        +float hr_deg
        +float g_pct
    }
```

Bedeutung der Felder:

* `d_deg`: Drehtellerwinkel
* `s_deg`: Schulterwinkel
* `e_deg`: Ellenbogenwinkel
* `hp_deg`: Handgelenk-Pitch
* `hr_deg`: Handgelenk-Roll
* `g_pct`: Greiferöffnung

### MotionRequest

`MotionRequest` ist das Übergabemodell zwischen Anwendung und Orchestrierung für eine einzelne Bewegungsanforderung.

```mermaid
classDiagram
    class MotionRequest {
        +TargetPose target
        +bool has_wait
        +uint32 wait_ms
    }
```

Für die erste Ausbaustufe bleibt dieses Modell bewusst einfach. Es enthält mindestens:

* ein fachliches Bewegungsziel als `TargetPose`
* eine optionale Wartezeit nach der Zielverarbeitung

Spätere Erweiterungen wie LED-Aktionen, Roboter-Aktionen oder Prioritäten können auf diesem Modell aufbauen.

### MotionResult

`MotionResult` beschreibt das fachliche und technische Ergebnis einer verarbeiteten Bewegungsanforderung.

```mermaid
classDiagram
    class MotionResult {
        +MotionStatus status
        +bool has_joint_state
        +JointState joint_state
        +ResultCode code
    }
```

Für die erste Ausbaustufe sollte das Modell insbesondere ausdrücken:

* ob eine Bewegungsanforderung akzeptiert, abgelehnt oder nicht erreichbar war
* ob ein berechneter `JointState` vorliegt
* ob ein technischer Fehler oder ein fachlicher Ablehnungsgrund zurückgegeben wurde

Die genaue Ausgestaltung von `MotionStatus` und `ResultCode` wird im Kapitel zur Fehlerbehandlung weiter konkretisiert.

### CalibrationData

`CalibrationData` beschreibt die Abbildung zwischen fachlichen Sollwerten und realen hardwarebezogenen Stellwerten.

```mermaid
classDiagram
    class CalibrationData {
        +AxisCalibration d
        +AxisCalibration s
        +AxisCalibration e
        +AxisCalibration hp
        +AxisCalibration hr
        +GripperCalibration g
    }
```

Für Rotationsachsen wird jeweils ein `AxisCalibration`-Eintrag verwendet:

```mermaid
classDiagram
    class AxisCalibration {
        +float zero_deg
        +float min_deg
        +float max_deg
        +uint16 pwm_min
        +uint16 pwm_max
        +bool inverted
    }
```

Für den Greifer wird ein separates Modell mit Prozentbezug verwendet:

```mermaid
classDiagram
    class GripperCalibration {
        +float min_pct
        +float max_pct
        +uint16 pwm_min
        +uint16 pwm_max
        +bool inverted
    }
```

Damit bleibt die Kalibration konsistent mit der fachlichen Entscheidung, dass `g` in Task Space und Joint Space dieselbe Größe beschreibt, während die interne Abbildung auf PWM-Werte dennoch separat dokumentiert wird.


## Schnittstellen der Kernmodule

In diesem Kapitel sollte beschrieben werden:

* welche öffentlichen Schnittstellen die zentralen Module besitzen
* welche Eingaben und Ausgaben `Run Engine`, `Orchestrator`, `Validation`, `Kinematics`, `Calibration` und `Hardware` haben
* welche Rückgabe- und Fehlerformen erwartet werden
* welche Teile synchron, einfach gehalten und deterministisch bleiben sollen

## Initialisierung und Laufzeitmodell

In diesem Kapitel sollte beschrieben werden:

* wie die Software startet und welche Initialisierungsreihenfolge vorgesehen ist
* wie die angenommene Home Position beziehungsweise Init Position in die Software übernommen wird
* wie aus Initialisierung, Kalibration und Ablaufsteuerung ein konsistenter Laufzeitfluss entsteht
* wie mit Reset, Neustart oder unklarem physischem Zustand umgegangen werden soll

## Kalibration und Hardwareabbildung

In diesem Kapitel sollte beschrieben werden:

* wie fachliche Sollwerte in hardwarenahe Stellwerte überführt werden
* welche Kalibrationsparameter pro Aktor benötigt werden
* wie Drehrichtung, PWM-Minimum, PWM-Maximum und Offsets abgebildet werden
* welche Grenzen im Software HAL durchgesetzt werden sollen

## Fehlerbehandlung und Rückmeldungen

In diesem Kapitel sollte beschrieben werden:

* welche Fehlerzustände fachlich und technisch unterschieden werden
* wie `Motion Result` strukturiert ist
* wie nicht erreichbare Ziele, ungültige Zustände und Hardwarefehler behandelt werden
* welche Fehler nur gemeldet und welche direkt zur Bewegungsablehnung führen

## Teststruktur

In diesem Kapitel sollte beschrieben werden:

* welche Teile nativ getestet werden
* welche Teile als Embedded-Test auf dem ESP32 geprüft werden
* wie `Unity` in die Softwarestruktur eingebunden wird
* welche Kernkomponenten zuerst mit Tests abgesichert werden sollen

## Benennung und Konventionen

In diesem Kapitel sollte beschrieben werden:

* welche Regeln für Dateinamen, Typnamen und Modulnamen gelten
* wie Task-Space- und Joint-Space-Größen konsistent benannt werden
* wie mit Einheiten, Winkeln, Prozentwerten und Grenzwerten umgegangen wird
* welche Begriffe aus der Dokumentation direkt in Codebezeichner übernommen werden sollen

## Offene Softwarepunkte

In diesem Kapitel sollte beschrieben werden:

* welche Implementationsentscheidungen noch offen sind
* welche Punkte vor dem Start der eigentlichen Codierung noch festgelegt werden sollten
* welche Themen bewusst erst in einer späteren Ausbaustufe behandelt werden

## Anhang

### Glossar und Abkürzungen

In diesem Kapitel sollte beschrieben werden:

* softwarebezogene Begriffe und Abkürzungen
* zentrale Modulnamen und ihre Bedeutung
* wiederkehrende technische Kurzformen

### Dokumentenverweise

In diesem Kapitel sollte beschrieben werden:

* Verweise auf die Projektbeschreibung
* Verweise auf die Hardwarebeschreibung
* spätere Verweise auf Quellcode, Tests oder ergänzende technische Dokumente
