# Projektbeschreibung

Dieses Projekt befasst sich mit der Implementierung eines 6-achsigen Roboterarms von Joy-it. Der Produktname lautet „Joy-it Grab-it“; der Arm besteht aus sechs unabhängigen Servoantrieben des Typs COM-Motor02. Die technischen Daten der Servoantriebe sind in der Datei `COM-Motor02-Datasheet.pdf` dokumentiert.

Ziel des Projekts ist es, den Roboterarm mithilfe inverser Kinematik gleichmäßig und zielorientiert zu steuern. Gleichzeitig soll auf einem ESP32-Modul eine höherwertige Programmiersprache eingesetzt werden, um moderne Programmiertechniken praktisch zu erproben.

Eine weitere zentrale Anforderung ist der Einsatz einer Servokarte mit PCA9685-Chip. Dadurch können die sechs Servos mithilfe verfügbarer Bibliotheken in Echtzeit angesteuert werden.

## Anforderungen und Spezifikationen

### Funktionale Anforderungen

### Nichtfunktionale Anforderungen

### Randbedingungen und Annahmen

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

### Servo-Kalibration

Damit die mathematisch berechneten Gelenkwinkel in eine reproduzierbare reale Bewegung überführt werden können, ist eine Kalibration der Servoantriebe erforderlich. Die inverse Kinematik liefert zunächst nur ideale Sollwinkel im Gelenkraum. Zwischen diesen Sollwinkeln und der tatsächlichen mechanischen Stellung des Roboterarms können jedoch systematische Abweichungen bestehen.

Solche Abweichungen entstehen beispielsweise durch unterschiedlich montierte Servohörner, mechanische Toleranzen, Nullpunktverschiebungen oder unterschiedlich nutzbare Winkelbereiche einzelner Antriebe. Ohne Kalibration würde dieselbe berechnete Gelenkkonfiguration in der Praxis nicht zwingend zur erwarteten Pose des Roboterarms führen.

Ziel der Servo-Kalibration ist es daher, für jede Achse eine eindeutige Zuordnung zwischen dem fachlichen Gelenkwinkel und dem realen Ansteuersignal herzustellen. Dazu gehören insbesondere:

* die Bestimmung einer mechanisch sinnvollen Nullstellung
* die Ermittlung von Minimal- und Maximalwerten pro Achse
* die Berücksichtigung eines möglichen Vorzeichenwechsels der Drehrichtung
* die Erfassung fester Offsets zwischen idealem Modell und realer Montage

Konzeptionell kann die Kalibration als Transformationsschritt zwischen Gelenkraum und Hardwareansteuerung verstanden werden. Die Kinematik arbeitet dabei weiterhin mit idealisierten Winkeln und Grenzwerten, während die hardwarenahe Ansteuerung diese Werte mithilfe der Kalibrationsdaten in konkrete Servosignale umsetzt.

Darüber hinaus bildet die Kalibration eine wichtige Grundlage für die Wiederholgenauigkeit des Systems. Erst wenn die Zuordnung zwischen Modell und realem Roboterarm konsistent ist, können berechnete Zielpositionen verlässlich angefahren und spätere Erweiterungen wie Bahnplanung oder automatisierte Bewegungsfolgen sinnvoll umgesetzt werden.

### Erreichbarkeitsprüfung

Nicht jeder vorgegebene Sollzustand des Endeffektors kann vom Roboterarm tatsächlich eingenommen werden. Eine Erreichbarkeitsprüfung ist daher notwendig, um bereits vor oder während der IK-Berechnung zu bewerten, ob ein Zielpunkt unter den gegebenen mechanischen und geometrischen Randbedingungen grundsätzlich realisierbar ist.

Die einfachste Form dieser Prüfung betrachtet zunächst die geometrische Reichweite des Arms. Ein Ziel ist nur dann erreichbar, wenn sich seine Position innerhalb des durch Segmentlängen und Gelenkanordnung definierten Arbeitsraums befindet. Darüber hinaus müssen jedoch weitere Randbedingungen einbezogen werden:

* Gelenkgrenzen der einzelnen Achsen
* feste mechanische Offsets des realen Roboterarms
* Abhängigkeiten zwischen Pitch, Roll und Position des Greifers
* die Frage, ob für einen Zielpunkt mehrere oder gar keine gültigen Gelenkkonfigurationen existieren

Die Erreichbarkeitsprüfung hat damit nicht nur eine mathematische, sondern auch eine sicherheitsrelevante Funktion. Unerreichbare oder kritische Ziele sollen frühzeitig erkannt werden, bevor unzulässige Ansteuerwerte an die Servos übergeben werden. Das reduziert das Risiko von mechanischer Überlastung, Anschlagfahrten oder unkontrollierten Bewegungen.

Im technischen Konzept kann zwischen einer groben und einer detaillierten Prüfung unterschieden werden. Eine grobe Prüfung bewertet, ob ein Zielpunkt prinzipiell im Arbeitsraum liegt. Eine detaillierte Prüfung berücksichtigt zusätzlich Gelenkgrenzen, Offsets und mögliche Konflikte zwischen Position und Orientierung. Auf diese Weise kann ein Ziel entweder direkt freigegeben, verworfen oder nur in modifizierter Form weiterverarbeitet werden.

Langfristig bildet die Erreichbarkeitsprüfung auch die Grundlage für weitergehende Funktionen wie Bahnplanung, Kollisionsvermeidung oder die Auswahl mehrerer möglicher IK-Lösungen nach zusätzlichen Kriterien, etwa minimaler Gelenkbewegung oder mechanisch günstiger Armhaltung.

### Dynamische und physikalische Randbedingungen

Neben geometrischen und kinematischen Randbedingungen sind auch physikalische Grenzen des realen Systems zu berücksichtigen. Diese Grenzen beeinflussen nicht zwingend, ob ein Zielpunkt prinzipiell erreichbar ist, wohl aber, ob eine Bewegung sicher, reproduzierbar und materialschonend ausgeführt werden kann.

Für das vorliegende System sind insbesondere folgende Randbedingungen relevant:

* maximale Winkelgeschwindigkeiten der einzelnen Servoantriebe
* begrenzte Winkelbeschleunigungen und damit verbundene Lastspitzen bei abrupten Bewegungswechseln
* begrenzte Drehmomente der Servos in Abhängigkeit von Hebelarm und Nutzlast
* mechanisches Spiel, elastische Verformungen und mögliches Nachschwingen des Arms
* Grenzen der Stromversorgung bei gleichzeitiger Belastung mehrerer Antriebe
* thermische Belastung der Servos bei längerem Halten oder wiederholter Bewegung unter Last
* begrenzte Greifkraft und begrenzter Greiferweg

Aus technischer Sicht ist daher zwischen Erreichbarkeit und Ausführbarkeit zu unterscheiden. Ein Ziel kann geometrisch erreichbar sein, gleichzeitig aber nur mit zu hoher Geschwindigkeit, zu hoher Last oder in einer mechanisch ungünstigen Armhaltung angesteuert werden. In solchen Fällen ist die Bewegung zwar theoretisch möglich, praktisch jedoch nur eingeschränkt oder gar nicht sinnvoll ausführbar.

Diese Randbedingungen sind insbesondere für spätere Erweiterungen der Steuerung von Bedeutung. Dazu zählen beispielsweise Bewegungsprofile mit begrenzter Geschwindigkeit und Beschleunigung, die Überwachung kritischer Lastsituationen oder die Begrenzung gleichzeitiger Servoaktivitäten. Bereits im technischen Konzept sollte daher berücksichtigt werden, dass die reine Positionsberechnung allein nicht ausreicht, um ein robustes Gesamtsystem zu erhalten.

### Ablaufsteuerung durch eine Run Engine

Da zunächst keine grafische Benutzeroberfläche vorgesehen ist, soll die Anwendungsschicht in einer ersten Ausbaustufe durch eine einfache, programmierbare Ablaufkomponente beschrieben werden. Diese Komponente wird im Folgenden als Run Engine bezeichnet. Ihre Aufgabe besteht darin, eine definierte Folge von Bewegungsanweisungen auszuführen und diese nacheinander an die Steuerungslogik zu übergeben.

Die Run Engine beschreibt damit keinen einzelnen Zielpunkt, sondern einen vollständigen Ablauf aus einer oder mehreren Bewegungsstationen. Jede Station kann insbesondere enthalten:

* einen Sollzustand des Endeffektors
* eine optionale Warte- oder Haltezeit
* optionale Zusatzaktionen wie Statussignale über die RGB-LED

Konzeptionell kann ein solcher Ablauf als generische Liste von Ablaufschritten verstanden werden. Die konkrete Herkunft dieser Liste bleibt zunächst offen. Sie kann beispielsweise fest im Programm hinterlegt, zur Compile-Zeit generiert oder später durch eine andere Konfigurationsquelle bereitgestellt werden. Da auf der Zielhardware möglicherweise keine Speicherkarte vorhanden ist, wird bewusst keine persistente Dateistruktur vorausgesetzt.

Die Run Engine erweitert damit die Anwendungsebene um eine einfache Form der Bewegungsprogrammierung. Anstatt ausschließlich einzelne Zielpositionen ad hoc zu übergeben, kann ein vollständiger Bewegungsablauf beschrieben, gestartet und in definierter Reihenfolge abgearbeitet werden. Dies ist insbesondere für wiederkehrende Demonstrationsabläufe, einfache Pick-and-Place-Sequenzen oder Testprogramme sinnvoll.

## SW Architektur

Die Softwarearchitektur dieses Projekts soll so aufgebaut sein, dass fachliche Logik, mathematische Modellierung und hardwarenahe Ansteuerung klar voneinander getrennt sind. Dadurch bleibt das System übersichtlich, testbar und später erweiterbar, auch wenn sich einzelne Algorithmen oder Hardwarekomponenten ändern.

### Architekturziele

Die Architektur verfolgt insbesondere folgende Ziele:

* Trennung zwischen fachlicher Beschreibung des Roboters und konkreter Hardwareansteuerung
* Austauschbarkeit der IK-Lösungsverfahren
* klare Datenflüsse zwischen Ablaufdefinition, Zielvorgabe, Modellberechnung und Aktoransteuerung
* Berücksichtigung mechanischer Randbedingungen wie Gelenkgrenzen und Offsets
* Berücksichtigung dynamischer und physikalischer Randbedingungen bei der Bewegungsausführung
* Erweiterbarkeit für spätere Funktionen wie Bahnplanung, Kalibrierung oder alternative Greifer
* klare Trennung zwischen fachlicher Berechnung, Orchestrierung, Bewegungsfreigabe und hardwarenaher Ausgabe
* Unterstützung sequenzieller Bewegungsabläufe durch eine einfache Anwendungskomponente

### Logische Schichten

Eine sinnvolle logische Zerlegung besteht aus mehreren Schichten mit klar abgegrenzten Verantwortlichkeiten.

#### Bedien- und Anwendungsschicht

Diese Schicht nimmt Sollvorgaben entgegen und stellt die Schnittstelle zur Benutzerinteraktion oder zu höheren Programmlogiken dar. In der ersten Ausbaustufe wird sie insbesondere durch eine Run Engine geprägt, welche vordefinierte Bewegungsabläufe verwaltet und schrittweise an die darunterliegende Steuerung übergibt. Die Anwendungsschicht arbeitet dabei ausschließlich mit fachlichen Größen wie Zielposition, Orientierung, Greiferöffnung, Wartezeiten und optionalen Statusaktionen.

#### Orchestrierungs- und Steuerungsschicht

Da es sich um eine Steuerung ohne sensorische Rückkopplung handelt, ist eine zentrale Orchestrierungsinstanz erforderlich, welche den gesamten Ablauf von der Zielvorgabe bis zur Stellwertausgabe verantwortet. Diese Instanz kann als Orchestrator verstanden werden. Die Schicht ruft Prüf- und Berechnungskomponenten in der richtigen Reihenfolge auf, koordiniert die fachlichen Verarbeitungsschritte und entscheidet, ob eine Bewegung freigegeben, verworfen oder mit einer Fehlermeldung an die Anwendung zurückgegeben wird.

#### Kinematik- und Modellschicht

In dieser Schicht wird das mathematische Modell des Roboterarms beschrieben. Sie enthält die geometrischen Eigenschaften des idealen Arms, die Segmentlängen, die Lage der Gelenke sowie später mögliche Korrekturen für reale mechanische Abweichungen. Zudem bildet sie die Grundlage für Vorwärtskinematik und inverse Kinematik.

#### Prüf- und Freigabeschicht

Zwischen Zielvorgabe und direkter Aktoransteuerung ist eine Prüfung und Freigabe der Bewegung zweckmäßig. In dieser Schicht wird bewertet, ob ein Ziel grundsätzlich erreichbar ist, ob Gelenkgrenzen eingehalten werden und ob die berechnete Lösung mechanisch plausibel ist. Darüber hinaus wird hier zwischen prinzipieller Erreichbarkeit und praktischer Ausführbarkeit unterschieden, beispielsweise im Hinblick auf Geschwindigkeitsgrenzen, Beschleunigungen oder Lastsituationen. Die Schicht hat damit die Aufgabe, Bewegungen fachlich zu bewerten und nur solche Sollwerte zur Ausgabe freizugeben, die unter den definierten Randbedingungen zulässig sind.

#### Hardwareabstraktionsschicht

Diese Schicht setzt freigegebene Gelenksollwerte in konkrete Ansteuersignale für die Servos um. Dazu gehören insbesondere die Umrechnung von Winkeln in hardwaregeeignete Stellgrößen, die Berücksichtigung servoindividueller Kalibrierwerte sowie die Kommunikation mit der Servokarte. Ebenso ist diese Schicht der geeignete Ort, um letzte hardwarenahe Schutzmechanismen wie Signalbegrenzungen, sichere Startpositionen oder die Begrenzung gleichzeitiger Servoaktivitäten zu verankern. Die mathematische Logik der Kinematik soll von diesen Details unabhängig bleiben.

### Komponenten und Verantwortlichkeiten

Innerhalb der beschriebenen Schichten kann die Architektur weiter in logisch getrennte Komponenten unterteilt werden. Diese Unterteilung dient dazu, die Verantwortlichkeiten des Systems präziser zu beschreiben, ohne bereits eine konkrete Implementierungsform festzulegen.

#### Orchestrator

Der Orchestrator ist die zentrale Ablaufkomponente der Softwarearchitektur. Er nimmt Bewegungsanforderungen aus der Anwendungsschicht entgegen und steuert deren Verarbeitung durch die übrigen Komponenten. Dabei hält er selbst möglichst wenig fachliche Detaillogik, sondern koordiniert den Ablauf, sammelt Ergebnisse und trifft die abschließende Entscheidung über Freigabe oder Ablehnung einer Bewegung. Im Zusammenspiel mit der Run Engine verarbeitet er nicht nur Einzelziele, sondern auch geordnete Folgen mehrerer Ablaufschritte.

Zu den Aufgaben des Orchestrators gehören insbesondere:

* Entgegennahme und Verwaltung von Bewegungsanforderungen
* Entgegennahme einzelner Ablaufschritte aus der Run Engine
* Aufruf von Erreichbarkeitsprüfung, IK-Berechnung und Freigabeprüfung
* Zusammenführung von Teilergebnissen zu einem einheitlichen Bewegungsergebnis
* Rückgabe von Freigaben, Ablehnungen oder Fehlermeldungen an die Anwendung
* Übergabe freigegebener Stellwerte an die Hardwareabstraktion

#### Run Engine

Die Run Engine ist die zentrale Anwendungskomponente für die Ausführung vordefinierter Bewegungsabläufe. Sie verwaltet eine Folge von Ablaufschritten und übergibt diese nacheinander an den Orchestrator. Dadurch trennt sie die Beschreibung eines Bewegungsprogramms von dessen fachlicher Prüfung und technischer Ausführung.

Zu den Aufgaben der Run Engine gehören insbesondere:

* Verwaltung eines Ablaufs aus einem oder mehreren Schritten
* sequentielle Übergabe von Sollpositionen an den Orchestrator
* Berücksichtigung von Haltezeiten zwischen zwei Bewegungsschritten
* Auslösung einfacher Begleitaktionen wie LED-Signalen
* definierte Behandlung von Freigaben, Ablehnungen oder Abbruchbedingungen innerhalb eines Ablaufs

#### Kinematikkomponente

Die Kinematikkomponente stellt die mathematischen Berechnungsverfahren des Systems bereit. Dazu gehören insbesondere die inverse Kinematik zur Berechnung von Gelenksollwerten aus einem Endeffektorziel sowie gegebenenfalls die Vorwärtskinematik zur Analyse oder Plausibilisierung von Gelenkkonfigurationen. Sie arbeitet auf Basis des Robotermodells und bleibt von hardwarebezogenen Details unabhängig.

#### Prüfkomponente

Die Prüfkomponente bewertet Bewegungsanforderungen und berechnete Gelenksollzustände unter fachlichen Randbedingungen. Sie führt insbesondere die Erreichbarkeitsprüfung, die Prüfung von Gelenkgrenzen sowie die Bewertung der praktischen Ausführbarkeit durch. Ihre Aufgabe besteht nicht in der Berechnung einer Bewegung, sondern in deren fachlicher Beurteilung.

#### Kalibrationskomponente

Die Kalibrationskomponente beschreibt die Abbildung zwischen idealisierten Gelenkwinkeln und realen hardwarebezogenen Stellgrößen. Sie nutzt die hinterlegten Kalibrationsdaten, um Sollwerte in eine Form zu überführen, die von der Hardwareabstraktion verarbeitet werden kann. Gleichzeitig sorgt sie dafür, dass Montageabweichungen und servoindividuelle Besonderheiten nicht in die mathematische Kinematiklogik eindringen.

#### Hardwaretreiber

Die Hardwareabstraktionsschicht kann konzeptionell in einen allgemeinen Abstraktionsanteil und einen konkreten Hardwaretreiber unterteilt werden. Der Treiber ist für die tatsächliche Kommunikation mit der Servokarte verantwortlich und setzt vorbereitete Stellwerte in die entsprechenden Ausgabesignale um. Dadurch bleibt die darüberliegende Architektur unabhängig von einer konkreten Ansteuerbibliothek oder Kommunikationsschnittstelle.

### Zentrale Datenmodelle

Unabhängig von der späteren Implementierung bietet sich eine Trennung der wichtigsten fachlichen Datenstrukturen an.

#### Zielbeschreibung des Endeffektors

Die Zielbeschreibung enthält die gewünschte Position des Greifers im Raum, seine Orientierung sowie die Greiferöffnung. Dieses Modell repräsentiert die Eingabe aus Sicht der Anwendung und beschreibt, was erreicht werden soll, jedoch nicht, wie dies auf Gelenkebene umgesetzt wird.

#### Ablaufschritt

Ein Ablaufschritt beschreibt eine einzelne Anweisung innerhalb eines Bewegungsprogramms. Er enthält mindestens eine Zielbeschreibung des Endeffektors und kann zusätzlich eine Haltezeit oder optionale Begleitaktionen umfassen. Dadurch bildet er die kleinste fachliche Einheit, welche von der Run Engine an den Orchestrator übergeben wird.

#### Ablaufdefinition

Die Ablaufdefinition beschreibt eine geordnete Liste von einem oder mehreren Ablaufschritten. Sie repräsentiert damit das eigentliche Bewegungsprogramm, das von der Run Engine verarbeitet wird. Die konkrete Speicherform bleibt bewusst offen, damit das Konzept unabhängig von Dateisystem, Speicherkarte oder externer Konfigurationsquelle bleibt.

#### Gelenksollzustand

Der Gelenksollzustand beschreibt die berechnete Konfiguration des Roboterarms im Gelenkraum. Dazu gehören die Winkel aller relevanten Achsen sowie die Öffnung des Greifers. Dieses Modell ist die zentrale Schnittstelle zwischen Kinematik, Freigabe und Hardwareansteuerung.

#### Robotermodell

Das Robotermodell enthält die geometrischen und mechanischen Eigenschaften des Arms. Dazu gehören Segmentlängen, Gelenkdefinitionen, Vorzeichenkonventionen, Offsets und zulässige Bewegungsbereiche. Es bildet damit die gemeinsame Grundlage für Berechnung, Validierung und spätere Kalibrierung.

#### Kalibrationsdaten

Zusätzlich zu den idealisierten Modellparametern sind Kalibrationsdaten erforderlich, welche die Abbildung zwischen fachlichen Gelenkwinkeln und realer Servoansteuerung beschreiben. Dazu gehören beispielsweise Nullpunktkorrekturen, Minimal- und Maximalwerte, Drehrichtungen und achsspezifische Offsets. Dieses Modell ergänzt das Robotermodell um reale hardwarebezogene Eigenschaften.

#### Bewegungsrandbedingungen

Für die Ausführbarkeit von Bewegungen ist ein weiteres Modell für dynamische und physikalische Randbedingungen sinnvoll. Dieses umfasst beispielsweise zulässige Geschwindigkeiten, Beschleunigungen, Lastgrenzen oder weitere sicherheitsrelevante Begrenzungen. Auf diese Weise können Positionsberechnung und Bewegungsausführung konzeptionell voneinander getrennt bleiben.

#### Bewegungsergebnis

Da die Architektur als Steuerung ohne sensorische Rückmeldung ausgelegt ist, ist ein explizites Ergebnis der Bewegungsanforderung sinnvoll. Dieses Modell beschreibt, ob ein Ziel freigegeben, verworfen oder nur eingeschränkt weiterverarbeitet wird. Zusätzlich kann es Begründungen enthalten, etwa Nichterreichbarkeit, Verletzung von Gelenkgrenzen oder Überschreitung dynamischer Randbedingungen.

#### Ablaufzustand

Für die Abarbeitung eines mehrschrittigen Programms ist zusätzlich ein Ablaufzustand sinnvoll. Dieser beschreibt beispielsweise, welcher Schritt aktuell bearbeitet wird, ob sich der Ablauf in einer Wartephase befindet und ob ein Programm erfolgreich beendet, angehalten oder abgebrochen wurde. Damit kann die Run Engine ihren internen Fortschritt verwalten, ohne hardwarebezogene Details kennen zu müssen.

### Schnittstellen und Datenflüsse

Die Qualität der Architektur hängt nicht nur von der Trennung der Komponenten, sondern auch von klar definierten Übergaben zwischen ihnen ab. Deshalb ist es sinnvoll, die wesentlichen Schnittstellen auf fachlicher Ebene zu beschreiben.

#### Schnittstelle zwischen Anwendung und Orchestrator

Die Anwendung übergibt dem Orchestrator in der einfachsten Form eine Zielbeschreibung des Endeffektors. Im vorgesehenen Betriebsmodell erfolgt diese Übergabe typischerweise durch die Run Engine, welche einzelne Ablaufschritte aus einer Ablaufdefinition nacheinander bereitstellt. Diese Schnittstelle ist bewusst fachlich formuliert und enthält keine hardwarebezogenen Angaben. Als Ergebnis erhält die Anwendung beziehungsweise die Run Engine ein Bewegungsergebnis, aus dem hervorgeht, ob die Anforderung freigegeben oder abgelehnt wurde.

#### Schnittstelle zwischen Orchestrator und Kinematik

Der Orchestrator übergibt der Kinematikkomponente eine gültige Zielbeschreibung sowie den Bezug auf das Robotermodell. Die Kinematik liefert daraufhin einen Gelenksollzustand oder meldet zurück, dass unter den gegebenen Annahmen keine geeignete Lösung berechnet werden konnte.

#### Schnittstelle zwischen Orchestrator und Prüfkomponente

Die Prüfkomponente erhält entweder eine Zielbeschreibung oder einen berechneten Gelenksollzustand zusammen mit den zugehörigen Randbedingungen. Sie liefert ein fachliches Prüfergebnis zurück, etwa erreichbar, nicht erreichbar, ausführbar oder nicht freigegeben. Dadurch bleibt die Bewertungslogik von der eigentlichen Bewegungsberechnung getrennt.

#### Schnittstelle zwischen Orchestrator, Kalibration und Hardwareabstraktion

Nach der fachlichen Freigabe übergibt der Orchestrator den Gelenksollzustand an die Kalibrations- und Hardwareabstraktionsseite. Dort werden die idealisierten Sollwerte zunächst in hardwarenahe Stellwerte umgerechnet und anschließend an den Hardwaretreiber weitergereicht. Erst an diesem Punkt erfolgt der Übergang von der fachlichen Beschreibung zur konkreten Servo-Ansteuerung.

#### Fehler- und Rückgabepfade

Da die Architektur ohne sensorische Rückmeldung arbeitet, kommt den Rückgabepfaden besondere Bedeutung zu. Jede beteiligte Komponente sollte deshalb nicht nur erfolgreiche Ergebnisse, sondern auch klar interpretierbare Ablehnungs- und Fehlerzustände an den Orchestrator zurückgeben. Dieser bündelt die Resultate und stellt sie der Anwendung beziehungsweise der Run Engine in konsistenter Form zur Verfügung. Auf diese Weise bleibt die Verantwortung für die Ablaufsteuerung zentralisiert, auch wenn die fachlichen Bewertungen in verschiedenen Komponenten stattfinden.

### Verarbeitungsablauf

Ein typischer Ablauf innerhalb der Softwarearchitektur kann unabhängig von einer konkreten Implementierung wie folgt beschrieben werden:

1. Die Run Engine lädt oder verwaltet eine Ablaufdefinition aus einem oder mehreren Ablaufschritten.
2. Die Run Engine übergibt den nächsten Ablaufschritt an den Orchestrator.
3. Der darin enthaltene Sollzustand wird anhand des Robotermodells auf Erreichbarkeit und grundsätzliche Randbedingungen geprüft.
4. Ein IK-Verfahren berechnet daraus eine passende Gelenkkonfiguration.
5. Die berechnete Lösung wird gegen Gelenkgrenzen, mechanische Offsets und Kalibrationsdaten validiert.
6. Zusätzlich wird geprüft, ob die Bewegung unter den geltenden dynamischen und physikalischen Randbedingungen sinnvoll ausführbar ist.
7. Das Ergebnis dieser Prüfung wird als Bewegungsfreigabe oder Ablehnung an den Orchestrator und von dort an die Run Engine zurückgegeben.
8. Nur freigegebene Sollwerte werden in hardwarenahe Stellgrößen umgerechnet und an die Servoansteuerung übergeben.
9. Nach Abschluss eines Schritts verarbeitet die Run Engine gegebenenfalls Wartezeiten oder Begleitaktionen und fährt anschließend mit dem nächsten Ablaufschritt fort.

### Erweiterbarkeit

Die Architektur soll bewusst offen für spätere Erweiterungen bleiben. Dazu gehören insbesondere:

* Austausch oder Vergleich verschiedener IK-Verfahren wie analytische Lösungen, CCD oder FABRIK
* Einführung von Kalibrierungsdaten für den realen Roboterarm
* Unterstützung für Bewegungsprofile anstelle sprunghafter Sollwertänderungen
* Protokollierung und Diagnose von Zielvorgaben, Berechnungsergebnissen und Servozuständen
* Ergänzung um Modelle für Ausführbarkeit, Lastgrenzen und dynamische Begrenzungen
* Ausbau des Orchestrators um komplexere Bewegungsabläufe oder Befehlsfolgen
* Erweiterung der Run Engine um zusätzliche Schrittarten oder alternative Konfigurationsquellen
* Erweiterung um Sicherheitsmechanismen, beispielsweise zur Begrenzung kritischer Bewegungen

Durch diese Struktur kann die Software schrittweise weiterentwickelt werden, ohne dass fachliche Modellierung, numerische Verfahren und hardwarenahe Steuerung unnötig stark miteinander gekoppelt werden.

### Architekturdiagramme

#### Blockschaltbild

#### Komponentendiagramm

#### Ablaufdiagramm

## SW Design

### Entwurfsgrundsätze

### Statische Struktur

### Dynamisches Verhalten

### Erweiterungsperspektiven

## Anhang

### Literaturverzeichnis

[1] Aristidou, A., Lasenby, J., Chrysanthou, Y. und Shamir, A.: Inverse Kinematics Techniques in Computer Graphics: A Survey. Computer Graphics Forum, 37(6), 2018. https://onlinelibrary.wiley.com/doi/10.1111/cgf.13310

[2] Aristidou, A. und Lasenby, J.: FABRIK: A fast, iterative solver for the inverse kinematics problem. Graphical Models, 73(5), 243-260, 2011. http://www.andreasaristidou.com/FABRIK.html

[3] Kenwright, B.: Inverse Kinematics - Cyclic Coordinate Descent (CCD). Journal of Graphics Tools / technische Einführung. https://alogicalmind.com/paper/ik_ccd/
