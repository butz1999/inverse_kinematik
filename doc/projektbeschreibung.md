# Projektbeschreibung

Dieses Projekt befasst sich mit der Implementierung eines 6-achsigen Roboterarms von Joy-it. Der Produktname lautet „Joy-it Grab-it“; der Arm besteht aus sechs unabhängigen Servoantrieben des Typs COM-Motor02. Die technischen Daten der Servoantriebe sind in der Datei `COM-Motor02-Datasheet.pdf` dokumentiert.

Ziel des Projekts ist es, den Roboterarm mithilfe inverser Kinematik gleichmäßig und zielorientiert zu steuern. Gleichzeitig soll auf einem ESP32-Modul eine höherwertige Programmiersprache eingesetzt werden, um moderne Programmiertechniken praktisch zu erproben.

Eine weitere zentrale Anforderung ist der Einsatz einer Servokarte mit PCA9685-Chip. Dadurch können die sechs Servos mithilfe verfügbarer Bibliotheken in Echtzeit angesteuert werden.

## Technisches Konzept

### Einführung in Inverse Kinematik

Unter Kinematik versteht man die Beschreibung von Bewegungen, ohne die dabei wirkenden Kräfte zu betrachten. Für einen Roboterarm sind dabei zwei Richtungen relevant:

* Die Vorwärtskinematik berechnet aus bekannten Gelenkwinkeln die resultierende Position und Orientierung des Greifers.
* Die Inverse Kinematik löst das umgekehrte Problem: Für eine gewünschte Zielposition des Greifers sollen passende Gelenkwinkel bestimmt werden.

Die inverse Kinematik bildet damit die Grundlage, um einen Roboterarm nicht nur achsweise, sondern aufgabenorientiert zu steuern. Anstatt jede Servo-Position einzeln vorzugeben, kann ein Ziel im Arbeitsraum formuliert werden, beispielsweise: "Bewege den Greifer nach (x,y,z) und halte dabei einen bestimmten Pitch-Winkel". Die Steuerung berechnet daraus die erforderlichen Winkel für Drehteller, Schulter, Ellenbogen und Handgelenk.

Dieses Problem ist in der Praxis nicht immer eindeutig lösbar. Für dieselbe Greiferposition können mehrere Gelenkkonfigurationen existieren, beispielsweise eine "Ellenbogen-oben"- und eine "Ellenbogen-unten"-Lösung. Ebenso kann ein Ziel außerhalb des mechanisch erreichbaren Arbeitsraums liegen oder nur unter Verletzung von Gelenkgrenzen erreichbar sein. Eine IK-Implementierung muss daher nicht nur mathematisch eine Lösung finden, sondern auch Randbedingungen wie Servo-Limits, mechanische Offsets und stabile Bewegungsabläufe berücksichtigen.

Für einfache Robotergeometrien lassen sich geschlossene, analytische Lösungen herleiten. Diese sind in der Regel schnell und deterministisch, setzen jedoch ein hinreichend ideales geometrisches Modell voraus. Sobald zusätzliche Offsets, Gelenkgrenzen oder komplexere Freiheitsgrade berücksichtigt werden sollen, sind iterative Verfahren häufig robuster und einfacher erweiterbar. Zwei bekannte Verfahren in diesem Bereich sind CCD (Cyclic Coordinate Descent) und FABRIK (Forward And Backward Reaching Inverse Kinematics). Beide arbeiten schrittweise auf eine Zielposition hin und sind deshalb für spätere Erweiterungen dieses Projekts besonders interessant.

Für den hier betrachteten Roboterarm bietet sich zunächst ein vereinfachtes Modell an, bei dem die Geometrie des Arms in Segmente und Gelenke zerlegt wird. Auf dieser Grundlage kann zunächst eine grundlegende IK für Position und einfache Orientierung implementiert werden. Anschließend kann das Modell schrittweise um reale mechanische Abweichungen ergänzt werden, sodass die inverse Kinematik zunehmend besser zum tatsächlichen Verhalten des Roboters passt.

Einen guten Überblick über die Eigenschaften, Stärken und Schwächen verschiedener IK-Verfahren geben Aristidou et al. in ihrer Survey zu Inverse-Kinematik-Verfahren [1]. Für FABRIK ist insbesondere die Originalarbeit von Aristidou und Lasenby relevant [2], in der das Verfahren beschrieben und gegenüber etablierten iterativen Ansätzen eingeordnet wird. Für CCD kann ergänzend die Einführung von Kenwright herangezogen werden [3].

### Beschreibung des idealen Roboterarms

#### Weltkoordinatensystem (task space)

Der Sollzustand des Endeffektors wird durch die Größen
(x,y,z,p,r,g)
beschrieben.
* Dabei beschreiben (x,y,z) die Position des Greifers im Raum.
* p ist die Pitch-Achse des Handgelenks
* r ist die Roll-Achse des Handgelenks
* g beschreibt die Greiferöffnung

Damit wird festgelegt, an welcher Position (x,y,z) sich der Greifer relativ zum Nullpunkt befinden soll. Zusätzlich werden seine Neigung (Pitch p), seine Drehung (Roll r) sowie seine Öffnung beschrieben.

Die Positionen werden in [mm] gemessen. Der Bezugspunkt ist (0,0,0) auf der Standfläche des Motors, genau unterhalb der Achse des Drehtellers.
Pitch und Roll werden in [°] gemessen; Bezugsebene ist die Horizontalebene.
Die Greiferöffnung wird in [%] gemessen, 0% entspricht vollständig geschlossen, 100% entspricht vollständig geöffnet.

#### Definition des Arbeitsraumes (cartesian space)

Der Arbeitsraum beschreibt die Positionen der einzelnen Gelenkpunkte in kartesischen Koordinaten (x,y,z).

Für die Modellierung werden folgende Positionen als kartesische 3D-Vektoren benötigt:
* Drehteller D liegt fest bei (0,0,0)
* Ellenbogen E(x,y,z)
* Handgelenk H(x,y,z)
* Greiferspitze G(x,y,z)

#### Definition des Gelenkraumes (joint space)
Abschließend wird der Gelenkraum definiert. Mit Ausnahme des Greifers werden alle Achsen in [°] angegeben.
* Drehteller d (-90°..90°), 0° zeigt in Richtung der y-Achse des Welt-Koordinatensystems.
* Schulter s (-90°..90°), -90° ist horizontal in Richtung der y-Achse, 0° zeigt vertikal nach oben (Richtung der z-Achse) und 90° kippt die Schulter in Richtung der negativen z-Achse.
* Ellenbogen e (-90°..90°). Hier ist die 0°-Position, wenn Oberarm und Unterarm in dieselbe Richtung zeigen. -90° kippt nach unten, +90° kippt nach oben.
* Handgelenk h (-90°..90°) analog zur Ellenbogenachse.
* Rotation r (-90°..90°): Bei -90° dreht das Handgelenk nach links, bei 90° dreht das Handgelenk nach rechts. Blickrichtung: vom Drehteller nach vorne.

### Beschreibung des realen Roboterarms

Die mechanische Konstruktion des Arms weist folgende Abweichungen vom idealen Modell auf:
* Die Schulter besitzt einen festen Offset nach vorne (positive y-Achse)
* Pitch und Roll haben einen festen Offset zueinander.
* Roll und das Zentrum des Greifers haben einen festen Offset (Achsen) zueinander.

Weitere Offsets werden im aktuellen Modell zunächst vernachlässigt.

## SW Architektur

Die Softwarearchitektur dieses Projekts soll so aufgebaut sein, dass fachliche Logik, mathematische Modellierung und hardwarenahe Ansteuerung klar voneinander getrennt sind. Dadurch bleibt das System übersichtlich, testbar und später erweiterbar, auch wenn sich einzelne Algorithmen oder Hardwarekomponenten ändern.

### Architekturziele

Die Architektur verfolgt insbesondere folgende Ziele:

* Trennung zwischen fachlicher Beschreibung des Roboters und konkreter Hardwareansteuerung
* Austauschbarkeit der IK-Lösungsverfahren
* klare Datenflüsse zwischen Zielvorgabe, Modellberechnung und Aktoransteuerung
* Berücksichtigung mechanischer Randbedingungen wie Gelenkgrenzen und Offsets
* Erweiterbarkeit für spätere Funktionen wie Bahnplanung, Kalibrierung oder alternative Greifer

### Logische Schichten

Eine sinnvolle logische Zerlegung besteht aus mehreren Schichten mit klar abgegrenzten Verantwortlichkeiten.

#### Bedien- und Anwendungsschicht

Diese Schicht nimmt Sollvorgaben entgegen und stellt die Schnittstelle zur Benutzerinteraktion oder zu höheren Programmlogiken dar. Hier wird beschrieben, wohin sich der Greifer bewegen soll oder welche Folge von Bewegungen auszuführen ist. Die Anwendungsschicht arbeitet dabei ausschließlich mit fachlichen Größen wie Zielposition, Orientierung und Greiferöffnung.

#### Kinematik- und Modellschicht

In dieser Schicht wird das mathematische Modell des Roboterarms beschrieben. Sie enthält die geometrischen Eigenschaften des idealen Arms, die Segmentlängen, die Lage der Gelenke sowie später mögliche Korrekturen für reale mechanische Abweichungen. Zudem bildet sie die Grundlage für Vorwärtskinematik und inverse Kinematik.

#### Planungs- und Prüfschicht

Zwischen Zielvorgabe und direkter Aktoransteuerung ist eine Prüfung der Bewegung zweckmäßig. In dieser Schicht wird bewertet, ob ein Ziel grundsätzlich erreichbar ist, ob Gelenkgrenzen eingehalten werden und ob die berechnete Lösung mechanisch plausibel ist. Später können hier auch Strategien für Zwischenpunkte, Geschwindigkeitsprofile oder Kollisionsprüfungen eingeordnet werden.

#### Hardwareabstraktionsschicht

Diese Schicht setzt abstrakte Gelenksollwerte in konkrete Ansteuersignale für die Servos um. Dazu gehören insbesondere die Umrechnung von Winkeln in hardwaregeeignete Stellgrößen, die Berücksichtigung servoindividueller Kalibrierwerte sowie die Kommunikation mit der Servokarte. Die mathematische Logik der Kinematik soll von diesen Details unabhängig bleiben.

### Zentrale Datenmodelle

Unabhängig von der späteren Implementierung bietet sich eine Trennung der wichtigsten fachlichen Datenstrukturen an.

#### Zielbeschreibung des Endeffektors

Die Zielbeschreibung enthält die gewünschte Position des Greifers im Raum, seine Orientierung sowie die Greiferöffnung. Dieses Modell repräsentiert die Eingabe aus Sicht der Anwendung und beschreibt, was erreicht werden soll, jedoch nicht, wie dies auf Gelenkebene umgesetzt wird.

#### Gelenkzustand

Der Gelenkzustand beschreibt die aktuelle oder berechnete Konfiguration des Roboterarms im Gelenkraum. Dazu gehören die Winkel aller relevanten Achsen sowie die Öffnung des Greifers. Dieses Modell ist die zentrale Schnittstelle zwischen Kinematik und Hardwareansteuerung.

#### Robotermodell

Das Robotermodell enthält die geometrischen und mechanischen Eigenschaften des Arms. Dazu gehören Segmentlängen, Gelenkdefinitionen, Vorzeichenkonventionen, Offsets und zulässige Bewegungsbereiche. Es bildet damit die gemeinsame Grundlage für Berechnung, Validierung und spätere Kalibrierung.

### Verarbeitungsablauf

Ein typischer Ablauf innerhalb der Softwarearchitektur kann unabhängig von einer konkreten Implementierung wie folgt beschrieben werden:

1. Eine Anwendung formuliert einen Sollzustand für den Endeffektor.
2. Dieser Sollzustand wird anhand des Robotermodells auf Erreichbarkeit und Randbedingungen geprüft.
3. Ein IK-Verfahren berechnet daraus eine passende Gelenkkonfiguration.
4. Die berechnete Lösung wird erneut gegen Gelenkgrenzen und mechanische Offsets validiert.
5. Die gültigen Sollwerte werden in hardwarenahe Stellgrößen umgerechnet und an die Servoansteuerung übergeben.

### Erweiterbarkeit

Die Architektur soll bewusst offen für spätere Erweiterungen bleiben. Dazu gehören insbesondere:

* Austausch oder Vergleich verschiedener IK-Verfahren wie analytische Lösungen, CCD oder FABRIK
* Einführung von Kalibrierungsdaten für den realen Roboterarm
* Unterstützung für Bewegungsprofile anstelle sprunghafter Sollwertänderungen
* Protokollierung und Diagnose von Zielvorgaben, Berechnungsergebnissen und Servozuständen
* Erweiterung um Sicherheitsmechanismen, beispielsweise zur Begrenzung kritischer Bewegungen

Durch diese Struktur kann die Software schrittweise weiterentwickelt werden, ohne dass fachliche Modellierung, numerische Verfahren und hardwarenahe Steuerung unnötig stark miteinander gekoppelt werden.

## Anhang

### Literaturverzeichnis

[1] Aristidou, A., Lasenby, J., Chrysanthou, Y. und Shamir, A.: Inverse Kinematics Techniques in Computer Graphics: A Survey. Computer Graphics Forum, 37(6), 2018. https://onlinelibrary.wiley.com/doi/10.1111/cgf.13310

[2] Aristidou, A. und Lasenby, J.: FABRIK: A fast, iterative solver for the inverse kinematics problem. Graphical Models, 73(5), 243-260, 2011. http://www.andreasaristidou.com/FABRIK.html

[3] Kenwright, B.: Inverse Kinematics - Cyclic Coordinate Descent (CCD). Journal of Graphics Tools / technische Einführung. https://alogicalmind.com/paper/ik_ccd/
