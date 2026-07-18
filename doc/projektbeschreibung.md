# Projektbeschreibung

Dieses Projekt befasst sich mit der Implementierung eines 6-achsigen Roboterarms von Joy-it. Der Produktname lautet „Joy-it Grab-it“, das [Datenblatt](datasheet/Robot02_Datasheet_2019_09_19.pdf) ist abgelegt [[5]](#ref-5). Der Arm besteht aus sechs unabhängigen Servoantrieben des Typs COM-Motor02. Die technischen Daten der Servoantriebe sind im [Datenblatt](datasheet/COM-Motor02-Datasheet.pdf) dokumentiert [[6]](#ref-6).

Ziel des Projekts ist es, den Roboterarm mithilfe inverser Kinematik gleichmäßig und zielorientiert zu steuern. Gleichzeitig soll auf einem ESP32-Modul eine höherwertige Programmiersprache eingesetzt werden, um moderne Programmiertechniken praktisch zu erproben.

Eine weitere zentrale Anforderung ist der Einsatz einer Servokarte mit PCA9685-Chip [[4]](#ref-4). Dadurch können die sechs Servos mithilfe verfügbarer Bibliotheken in Echtzeit angesteuert werden.

![Joy-it Grab-it Roboterarm](datasheet/Robot02-1.png)

## Technisches Konzept

### Einführung in Inverse Kinematik

Unter Kinematik versteht man die Beschreibung von Bewegungen, ohne die dabei wirkenden Kräfte zu betrachten. Für einen Roboterarm sind dabei zwei Richtungen relevant:

* Die Vorwärtskinematik berechnet aus bekannten Gelenkwinkeln die resultierende Position und Orientierung des Greifers.
* Die Inverse Kinematik löst das umgekehrte Problem: Für eine gewünschte Zielposition des Greifers sollen passende Gelenkwinkel bestimmt werden.

Die inverse Kinematik bildet damit die Grundlage, um einen Roboterarm nicht nur achsweise, sondern aufgabenorientiert zu steuern. Anstatt jede Servo-Position einzeln vorzugeben, kann ein Ziel im Arbeitsraum formuliert werden, beispielsweise: "Bewege den Greifer nach (x,y,z) und halte dabei einen bestimmten Pitch-Winkel". Die Steuerung berechnet daraus die erforderlichen Winkel für Drehteller, Schulter, Ellenbogen und Handgelenk.

Dieses Problem ist in der Praxis nicht immer eindeutig lösbar. Für dieselbe Greiferposition können mehrere Gelenkkonfigurationen existieren, beispielsweise eine "Ellenbogen-oben"- und eine "Ellenbogen-unten"-Lösung. Ebenso kann ein Ziel außerhalb des mechanisch erreichbaren Arbeitsraums liegen oder nur unter Verletzung von Gelenkgrenzen erreichbar sein. Eine IK-Implementierung muss daher nicht nur mathematisch eine Lösung finden, sondern auch Randbedingungen wie Servo-Limits, mechanische Offsets und stabile Bewegungsabläufe berücksichtigen.

Für einfache Robotergeometrien lassen sich geschlossene, analytische Lösungen herleiten. Diese sind in der Regel schnell und deterministisch, setzen jedoch ein hinreichend ideales geometrisches Modell voraus. Sobald zusätzliche Offsets, Gelenkgrenzen oder komplexere Freiheitsgrade berücksichtigt werden sollen, sind iterative Verfahren häufig robuster und einfacher erweiterbar. Zwei bekannte Verfahren in diesem Bereich sind CCD (Cyclic Coordinate Descent) und FABRIK (Forward And Backward Reaching Inverse Kinematics). Beide arbeiten schrittweise auf eine Zielposition hin und sind deshalb für spätere Erweiterungen dieses Projekts besonders interessant.

Für den hier betrachteten Roboterarm bietet sich zunächst ein vereinfachtes Modell an, bei dem die Geometrie des Arms in Segmente und Gelenke zerlegt wird. Auf dieser Grundlage kann zunächst eine grundlegende IK für Position und einfache Orientierung implementiert werden. Anschließend kann das Modell schrittweise um reale mechanische Abweichungen ergänzt werden, sodass die inverse Kinematik zunehmend besser zum tatsächlichen Verhalten des Roboters passt.

Einen guten Überblick über die Eigenschaften, Stärken und Schwächen verschiedener IK-Verfahren geben Aristidou et al. in ihrer Survey zu Inverse-Kinematik-Verfahren [[1]](#ref-1). Für FABRIK ist insbesondere die Originalarbeit von Aristidou und Lasenby relevant [[2]](#ref-2), in der das Verfahren beschrieben und gegenüber etablierten iterativen Ansätzen eingeordnet wird. Für CCD kann ergänzend die Einführung von Kenwright herangezogen werden [[3]](#ref-3).

### Beschreibung des idealen Roboterarms

#### Weltkoordinatensystem (task space)

Der Sollzustand des Endeffektors wird durch die Größen
(x,y,z,p,r,g)
beschrieben.
* Dabei beschreiben (x,y,z) die Position des Greifers im Raum.
* p beschreibt die Neigung des Handgelenks
* r beschreibt die Rotation des Handgelenks um seine Längsachse
* g beschreibt die Greiferöffnung

Damit wird festgelegt, an welcher Position (x,y,z) sich der Greifer relativ zum Nullpunkt befinden soll. Zusätzlich werden seine Neigung (p), seine Drehung (r) sowie seine Öffnung (g) beschrieben.

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
Abschließend wird der Gelenkraum definiert. Die Rotationsachsen werden in [°] angegeben, die Greiferöffnung in [%].
* Drehteller d (-180°..90°), 0° zeigt in Richtung der y-Achse des Welt-Koordinatensystems.
* Schulter s (-90°..90°), -90° ist horizontal in Richtung der y-Achse, 0° zeigt vertikal nach oben (Richtung der z-Achse) und 90° kippt die Schulter in Richtung der negativen z-Achse.
* Ellenbogen e (-100°..100°). Hier ist die 0°-Position, wenn Oberarm und Unterarm in dieselbe Richtung zeigen. -100° kippt nach unten, +100° kippt nach oben.
* Handgelenk-Pitch hp (0°..135°) analog zur Ellenbogenachse.
* Handgelenk-Roll hr (-180°..180°): Bei -180° dreht das Handgelenk nach links, bei 180° dreht das Handgelenk nach rechts. Blickrichtung: vom Drehteller nach vorne.
* Greifer g (0%..100%): 0% entspricht vollständig geschlossen, 100% vollständig geöffnet.

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
* die Berücksichtigung eines möglichen Vorzeichenwechsels der Drehrichtung über die Reihenfolge der kalibrierten PWM-Endpunkte
* die Erfassung fester Offsets zwischen idealem Modell und realer Montage

Konzeptionell kann die Kalibration als Transformationsschritt zwischen Gelenkraum und Hardwareansteuerung verstanden werden. Die Kinematik arbeitet dabei weiterhin mit idealisierten Winkeln und Grenzwerten, während die hardwarenahe Ansteuerung diese Werte mithilfe der Kalibrationsdaten in konkrete Servosignale umsetzt.

Darüber hinaus bildet die Kalibration eine wichtige Grundlage für die Wiederholgenauigkeit des Systems. Erst wenn die Zuordnung zwischen Modell und realem Roboterarm konsistent ist, können berechnete Zielpositionen verlässlich angefahren und spätere Erweiterungen wie Bahnplanung oder automatisierte Bewegungsfolgen sinnvoll umgesetzt werden.

### Initialisierung und sichere Startlage

Da der Roboterarm in der ersten Ausbaustufe ohne sensorische Rückmeldung betrieben wird, kommt der Initialisierung des Systems besondere Bedeutung zu. Nach dem Einschalten kann die Software die reale mechanische Stellung des Arms nicht eigenständig verifizieren. Deshalb muss ein definierter Zusammenhang zwischen dem angenommenen Softwarezustand und der tatsächlichen physischen Ausgangslage hergestellt werden.

Für den praktischen Betrieb bedeutet dies, dass der Arm vor dem Start der normalen Ablaufsteuerung in eine bekannte und mechanisch unkritische [Home Position](#home-position) gebracht werden muss. Diese Startlage dient als Bezugspunkt für Kalibration, Testläufe und wiederholbare Bewegungsprogramme. Sie sollte so gewählt werden, dass alle Gelenke ausreichend Abstand zu mechanischen Anschlägen besitzen und der Greifer keine kritische Kollision mit der Standfläche oder der eigenen Struktur verursacht.

Für das vorliegende Projekt wird diese Startlage konkret wie folgt definiert: Mit leichter Krümmung steht der Greifer vertikal vor dem Drehteller. Diese mechanische Ausgangslage ist vor dem Einschalten manuell herzustellen und bildet die praktische Interpretation der [Home Position](#home-position) im stromlosen Zustand.

Ein besonderes Risiko beim Systemstart besteht darin, dass Servoantriebe ohne Positionsrückmeldung beim Aktivieren ihrer Ansteuerung abrupt in eine intern angenommene Sollposition springen können. Ursache ist, dass der Regler des Servos zwar eine Zielstellung erhält, die tatsächliche mechanische Ausgangslage des Arms zu diesem Zeitpunkt jedoch unbekannt sein kann. Für das vorliegende Projekt wird dieses Verhalten als systembedingt akzeptiert und nicht durch zusätzliche Sensorik aufgelöst.

Stattdessen wird eine definierte [Home Position](#home-position) festgelegt, welche aus dem stromlosen Zustand reproduzierbar von Hand eingenommen werden kann. Diese Position bildet die fachliche Grundlage für das Aufstartverhalten des Systems. Erst nachdem der Arm in diese Ausgangslage gebracht wurde, wird die Software mit den entsprechenden angenommenen Gelenkwerten initialisiert und die normale Ablaufsteuerung freigegeben.

Die logische Initialposition des Systems entspricht dabei der im Projekt beschriebenen Nullkonfiguration der Servoachsen. Für die Initialisierung wird somit angenommen, dass sich alle Servoachsen auf ihrer fachlichen 0-Position befinden, nachdem der Arm in die definierte Startlage gebracht wurde.

Konzeptionell sind dafür mindestens folgende Schritte sinnvoll:

* Festlegung einer dokumentierten Startkonfiguration des Arms
* Zuordnung dieser Startkonfiguration zu den angenommenen Gelenkwerten in der Software
* kontrollierte Aktivierung der Servos ohne abrupte oder unplausible Stellwertsprünge
* optionales Anfahren einer sicheren Ruheposition vor Beginn eines eigentlichen Bewegungsablaufs

Für die sichere Inbetriebnahme wird zusätzlich festgelegt, dass die Achsen beginnend beim Greifer rückwärts angesteckt und ihr jeweiliger Fahrbereich kontrolliert werden. Dadurch kann die Zuordnung zwischen mechanischer Achse, elektrischer Ansteuerung und logischem Modell schrittweise geprüft werden, bevor der vollständige Arm im Verbund betrieben wird.

Zur Reduktion ruckartiger Startbewegungen bieten sich insbesondere folgende Maßnahmen an:

* manuelles Platzieren des Arms in einer bekannten Ausgangslage vor dem Aktivieren der Regelung
* Start mit einer sicheren, zur definierten [Home Position](#home-position) passenden Sollkonfiguration
* schrittweises Heranfahren an die eigentliche Initialposition statt unmittelbarer Sprungvorgabe
* Begrenzung von Geschwindigkeit und Stellwertänderung bereits während der Initialisierung
* sequentielle oder gruppierte Aktivierung einzelner Achsen statt gleichzeitiger Freigabe aller Antriebe

Damit bleibt festzuhalten: Ohne Sensorik kann das ruckartige Anspringen nicht mathematisch garantiert vermieden werden. Für das Projekt wird deshalb eine definierte [Home Position](#home-position) als Betriebsannahme festgelegt, die aus dem stromlosen Zustand erreichbar ist, sodass das Aufstartverhalten fachlich klar beschrieben und praktisch beherrschbar bleibt.

Da keine Referenzsensorik vorgesehen ist, bleibt diese Initialisierung auf eine manuell vorbereitete oder konstruktiv definierte Ausgangslage angewiesen. Die Projektbeschreibung sollte diesen Umstand explizit berücksichtigen, damit die Grenzen der Wiederholgenauigkeit und der sichere Systemstart von Anfang an fachlich eingeordnet sind.

Offen bleibt dabei bewusst, wie die Software selbständig erkennt oder verifiziert, dass sich das System tatsächlich in diesem Initialzustand befindet. Für die erste Ausbaustufe wird diese Übereinstimmung nicht technisch nachgewiesen, sondern als Betriebsannahme vorausgesetzt.

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

### Umgang mit mehreren IK-Lösungen und Singularitäten

Für einzelne Zielzustände des Endeffektors können mehrere mathematisch gültige IK-Lösungen existieren. Typische Beispiele sind unterschiedliche Armhaltungen wie "Ellenbogen-oben" und "Ellenbogen-unten". Für das System ist daher nicht nur relevant, ob eine Lösung existiert, sondern auch, nach welchen Regeln zwischen mehreren zulässigen Konfigurationen gewählt wird.

Für eine erste Ausbaustufe bietet sich eine einfache, deterministische Auswahlstrategie an. Diese kann beispielsweise bevorzugen:

* geringe Änderung gegenüber dem aktuellen oder zuletzt angefahrenen Gelenksollzustand
* mechanisch günstige Haltungen mit ausreichendem Abstand zu Gelenkgrenzen
* Konfigurationen mit möglichst geringer Belastung einzelner Achsen

Eine solche Priorisierung erhöht die Vorhersagbarkeit des Systems und reduziert das Risiko unnötiger Sprungwechsel zwischen unterschiedlichen Armhaltungen.

Zusätzlich müssen Singularitäten und numerisch ungünstige Konfigurationen berücksichtigt werden. Dazu zählen beispielsweise fast vollständig gestreckte Armhaltungen, sehr kleine Hebelverhältnisse oder Zielzustände, bei denen kleine Positionsänderungen zu überproportional großen Gelenkänderungen führen. In solchen Fällen kann eine mathematische Lösung zwar formal existieren, praktisch aber nur instabil oder mechanisch ungünstig sein.

Daher sollte das System Konfigurationen dieser Art nicht nur geometrisch bewerten, sondern im Rahmen der Validierung auch auf numerische Plausibilität und Betriebsstabilität prüfen. Im einfachsten Fall können solche Ziele abgelehnt, in ihrer Priorität herabgesetzt oder nur mit einer bevorzugten Ersatzkonfiguration verarbeitet werden.

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

Sanfte Beschleunigungsprofile sind in diesem Zusammenhang besonders relevant. Werden Sollwerte nur linear oder sprunghaft verändert, entstehen insbesondere am Anfang und Ende einer Bewegung hohe Ruckanteile sowie abrupte Lastwechsel. Für Servoantriebe kleiner Roboterarme kann dies zu sichtbarem Nachschwingen, erhöhter mechanischer Belastung und unruhigem Bewegungsverhalten führen.

Daher ist es sinnvoll, perspektivisch nicht nur Geschwindigkeits- und Beschleunigungsgrenzen, sondern auch den zeitlichen Verlauf der Beschleunigung zu betrachten. S-Kurven-Profile stellen hierfür einen geeigneten Ansatz dar. Im Unterschied zu einfachen trapezförmigen Profilen werden Beschleunigung und Verzögerung dabei weicher ein- und ausgeblendet, sodass Bewegungen ruhiger, materialschonender und besser kontrollierbar ausgeführt werden können.

### Bewegungsübergänge und Interpolation

Zwischen zwei aufeinanderfolgenden Zielzuständen stellt sich die Frage, wie die Bewegung des Arms tatsächlich ausgeführt werden soll. Eine einfache Ansteuerung könnte darin bestehen, neue Gelenksollwerte unmittelbar zu übernehmen. In der Praxis kann dies jedoch zu abrupten Bewegungswechseln, unnötigen Lastspitzen oder mechanisch ungünstigen Übergängen führen.

Deshalb ist es sinnvoll, bereits im technischen Konzept zwischen Zielpunktdefinition und Bewegungsübergang zu unterscheiden. Für die erste Ausbaustufe kann eine einfache Interpolation im Gelenkraum ausreichend sein, bei der Zwischenwerte zwischen zwei Gelenksollzuständen gebildet und schrittweise ausgegeben werden. Dadurch lassen sich Bewegungen weicher gestalten und besser an Geschwindigkeits- oder Beschleunigungsgrenzen anpassen.

Langfristig sind unterschiedliche Strategien denkbar:

* direkte Übernahme diskreter Gelenksollwerte
* lineare Interpolation im Gelenkraum
* spätere Erweiterung um kartesische Bahnsegmente oder stärker profilierte Bewegungsmodelle
* Bewegungsprofile mit sanfter Beschleunigung und Verzögerung, beispielsweise auf Basis von S-Kurven

Die konkrete Ausführung eines Bewegungsübergangs beeinflusst nicht nur die mechanische Belastung, sondern auch die Qualität von Demonstrationsabläufen, die Wiederholbarkeit des Systems und die praktische Nutzbarkeit der Run Engine. Auch wenn in der ersten Ausbaustufe noch keine vollständige Bahnplanung vorgesehen ist, sollte daher festgehalten werden, dass zwischen Zielpunkten eine kontrollierte Übergangslogik erforderlich ist.

### Ablaufsteuerung durch eine Run Engine

Da zunächst keine grafische Benutzeroberfläche vorgesehen ist, soll die Anwendungsschicht in einer ersten Ausbaustufe durch eine einfache, programmierbare Ablaufkomponente beschrieben werden. Diese Komponente wird im Folgenden als [Run Engine](#run-engine) bezeichnet. Ihre Aufgabe besteht darin, eine definierte Folge von Bewegungsanweisungen auszuführen und diese nacheinander an die Steuerungslogik zu übergeben.

Die Run Engine beschreibt damit keinen einzelnen Zielpunkt, sondern einen vollständigen Ablauf aus einer oder mehreren Bewegungsstationen. Jede Station kann insbesondere enthalten:

* einen Sollzustand des Endeffektors
* eine optionale Warte- oder Haltezeit
* optionale Zusatzaktionen wie Statussignale über die RGB-LED
* optionale Roboter-Aktionen, die nicht allein durch eine kartesische Zielvorgabe beschrieben werden

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

Diese Schicht nimmt Sollvorgaben entgegen und stellt die Schnittstelle zur Benutzerinteraktion oder zu höheren Programmlogiken dar. In der ersten Ausbaustufe wird sie insbesondere durch eine Run Engine geprägt, welche vordefinierte Bewegungsabläufe verwaltet und schrittweise an die darunterliegende Steuerung übergibt. Die Anwendungsschicht arbeitet dabei ausschließlich mit fachlichen Größen wie Zielposition, Orientierung, Greiferöffnung, Wartezeiten, optionalen Statusaktionen und optionalen Roboter-Aktionen.

#### Orchestrierungs- und Steuerungsschicht

Da es sich um eine Steuerung ohne sensorische Rückkopplung handelt, ist eine zentrale Orchestrierungsinstanz erforderlich, welche den gesamten Ablauf von der Zielvorgabe bis zur Stellwertausgabe verantwortet. Diese Instanz kann als Orchestrator verstanden werden. Die Schicht ruft Prüf- und Berechnungskomponenten in der richtigen Reihenfolge auf, koordiniert die fachlichen Verarbeitungsschritte und entscheidet, ob eine Bewegungsanforderung freigegeben, verworfen oder mit einer Fehlermeldung an die Anwendung zurückgegeben wird. Rückmeldungen dieser Schicht beschreiben dabei den fachlichen und technischen Bearbeitungsstatus einer Anforderung, nicht jedoch eine physisch verifizierte Zielerreichung.

#### Kinematik- und Modellschicht

In dieser Schicht wird das mathematische Modell des Roboterarms beschrieben. Sie enthält die geometrischen Eigenschaften des idealen Arms, die Segmentlängen, die Lage der Gelenke sowie später mögliche Korrekturen für reale mechanische Abweichungen. Zudem bildet sie die Grundlage für Vorwärtskinematik und inverse Kinematik.

#### Prüf- und Freigabeschicht

Zwischen Zielvorgabe und direkter Aktoransteuerung ist eine Prüfung und Freigabe der Bewegung zweckmäßig. In dieser Schicht wird bewertet, ob ein Ziel grundsätzlich erreichbar ist, ob Gelenkgrenzen eingehalten werden und ob die berechnete Lösung mechanisch plausibel ist. Darüber hinaus wird hier zwischen prinzipieller Erreichbarkeit und praktischer Ausführbarkeit unterschieden, beispielsweise im Hinblick auf Geschwindigkeitsgrenzen, Beschleunigungen oder Lastsituationen. Die Schicht hat damit die Aufgabe, Bewegungen fachlich zu bewerten und nur solche Sollwerte zur Ausgabe freizugeben, die unter den definierten Randbedingungen zulässig sind.

#### Hardwareabstraktionsschicht

Diese Schicht setzt freigegebene Gelenksollwerte in konkrete Ansteuersignale für die Servos um. Dazu gehören insbesondere die Umrechnung von Winkeln in hardwaregeeignete Stellgrößen, die Berücksichtigung servoindividueller Kalibrierwerte sowie die Kommunikation mit der Servokarte. Ebenso ist diese Schicht der geeignete Ort, um letzte hardwarenahe Schutzmechanismen wie Signalbegrenzungen, sichere Startpositionen oder die Begrenzung gleichzeitiger Servoaktivitäten zu verankern. Die mathematische Logik der Kinematik soll von diesen Details unabhängig bleiben.

### Komponenten und Verantwortlichkeiten

Innerhalb der beschriebenen Schichten kann die Architektur weiter in logisch getrennte Komponenten unterteilt werden. Diese Unterteilung dient dazu, die Verantwortlichkeiten des Systems präziser zu beschreiben, ohne bereits eine konkrete Implementierungsform festzulegen.

#### Orchestrator

Der Orchestrator ist die zentrale Ablaufkomponente der Softwarearchitektur. Er nimmt [Bewegungsanforderungen](#motion-request) aus der Anwendungsschicht entgegen und steuert deren Verarbeitung durch die übrigen Komponenten. Dabei hält er selbst möglichst wenig fachliche Detaillogik, sondern koordiniert den Ablauf, sammelt Ergebnisse und trifft die abschließende Entscheidung über Freigabe oder Ablehnung einer Bewegung. Im Zusammenspiel mit der [Run Engine](#run-engine) verarbeitet er nicht nur Einzelziele, sondern auch geordnete Folgen mehrerer Ablaufschritte.

Zu den Aufgaben des Orchestrators gehören insbesondere:

* Entgegennahme und Verwaltung von Bewegungsanforderungen
* Entgegennahme einzelner Ablaufschritte aus der Run Engine
* Aufruf von Erreichbarkeitsprüfung, IK-Berechnung und Freigabeprüfung
* Zusammenführung von Teilergebnissen zu einem einheitlichen Bewegungsergebnis
* Rückgabe von Freigaben, Ablehnungen oder Fehlermeldungen an die Anwendung
* Übergabe freigegebener Stellwerte an die Hardwareabstraktion

#### Run Engine

Die [Run Engine](#run-engine) ist die zentrale Anwendungskomponente für die Ausführung vordefinierter Bewegungsabläufe. Sie verwaltet eine Folge von [Ablaufschritten](#sequence-step) und übergibt diese nacheinander an den Orchestrator. Dadurch trennt sie die Beschreibung eines Bewegungsprogramms von dessen fachlicher Prüfung und technischer Ausführung.

Zu den Aufgaben der Run Engine gehören insbesondere:

* Verwaltung eines Ablaufs aus einem oder mehreren Schritten
* sequentielle Übergabe von Bewegungsanforderungen an den Orchestrator
* Berücksichtigung von Haltezeiten zwischen zwei Bewegungsschritten
* Auslösung einfacher Begleitaktionen wie LED-Signalen
* Auslösung einfacher Roboter-Aktionen außerhalb einer reinen kartesischen Zielbeschreibung
* definierte Behandlung von Freigaben, Ablehnungen oder Abbruchbedingungen innerhalb eines Ablaufs

#### Kinematikkomponente

Die Kinematikkomponente (`Kinematics`) stellt die mathematischen Berechnungsverfahren des Systems bereit. Dazu gehören insbesondere die inverse Kinematik zur Berechnung von Gelenksollwerten aus einem Endeffektorziel sowie gegebenenfalls die Vorwärtskinematik zur Analyse oder Plausibilisierung von Gelenkkonfigurationen. Sie arbeitet auf Basis des Robotermodells (`Robot Model`) und bleibt von hardwarebezogenen Details unabhängig.

Werden im Projekt iterative IK-Verfahren wie `CCD` oder `FABRIK` eingesetzt, so gehört auch die dazugehörige Iterationslogik in diese Komponente. Dazu zählen insbesondere die Wahl eines Startzustands, die wiederholte Annäherung an das Ziel, die Prüfung von Toleranzen sowie der Abbruch bei Konvergenz, maximaler Iterationszahl oder erkannter Nichterreichbarkeit. Der Orchestrator stößt die Berechnung nur an und bewertet deren Ergebnis, führt die eigentlichen Iterationsschritte jedoch nicht selbst aus.

#### Prüfkomponente

Die Prüfkomponente (`Validation`) bewertet Bewegungsanforderungen und berechnete Gelenksollzustände unter fachlichen Randbedingungen. Sie führt insbesondere die Erreichbarkeitsprüfung, die Prüfung von Gelenkgrenzen sowie die Bewertung der praktischen Ausführbarkeit durch. Ihre Aufgabe besteht nicht in der Berechnung einer Bewegung, sondern in deren fachlicher Beurteilung.

#### Robot Model Offset

Die Komponente `Robot Model Offset` beschreibt modellbezogene Offsets und Korrekturwerte des realen Roboterarms. Dazu gehören insbesondere Schulter-Offsets sowie weitere Korrekturen, welche die fachliche Zielbeschreibung vor der kinematischen Berechnung an das reale Robotermodell annähern. Konzeptionell kann daraus eine `Offset Target Pose` als modellkorrigierte Zwischenrepräsentation entstehen, welche anschließend von der Kinematik verarbeitet wird.

#### Hardwaretreiber

Die Hardwareabstraktionsschicht (`Hardware`) kann konzeptionell in einen allgemeinen Abstraktionsanteil (`Hardware Abstraction`) und einen konkreten Hardwaretreiber (`Hardware Driver`) unterteilt werden. Die `Hardware Abstraction` übernimmt dabei die Anwendung der `Hardware Calibration` auf einen fachlichen Gelenksollzustand und erzeugt daraus eine PWM-bezogene Ausgabedarstellung (`Joint PWM State`). Der Treiber ist für die tatsächliche Kommunikation mit der Servokarte verantwortlich und setzt diese vorbereiteten Werte in die entsprechenden Ausgabesignale um. Dadurch bleibt die darüberliegende Architektur unabhängig von einer konkreten Ansteuerbibliothek oder Kommunikationsschnittstelle.

### Zentrale Datenmodelle

Unabhängig von der späteren Implementierung bietet sich eine Trennung der wichtigsten fachlichen Datenstrukturen an.

#### Zielpose des Endeffektors

Die [Zielpose des Endeffektors](#target-description) (`Target Pose`) enthält die gewünschte Position des Greifers im Raum, seine Orientierung sowie die Greiferöffnung. Dieses Modell repräsentiert die Eingabe aus Sicht der Anwendung und beschreibt, was erreicht werden soll, jedoch nicht, wie dies auf Gelenkebene umgesetzt wird.

#### Ablaufschritt

Ein [Ablaufschritt](#sequence-step) (`Sequence Step`) beschreibt eine einzelne Anweisung innerhalb eines Bewegungsprogramms. Er enthält mindestens eine [Zielpose des Endeffektors](#target-description) (`Target Pose`) und kann zusätzlich eine Haltezeit, optionale Begleitaktionen oder optionale [Roboter-Aktionen](#robot-action) (`Robot Action`) umfassen. Unter Roboter-Aktionen werden dabei diskrete, fachlich benennbare Aktionen verstanden, die nicht ausschließlich über eine kartesische Zielbeschreibung modelliert werden, beispielsweise das gezielte Öffnen oder Schließen des Greifers. Dadurch bildet der Ablaufschritt die kleinste fachliche Einheit, welche von der [Run Engine](#run-engine) an den Orchestrator übergeben wird.

#### Bewegungsanforderung

Die [Bewegungsanforderung](#motion-request) (`Motion Request`) ist das fachliche Übergabeobjekt zwischen Anwendungsschicht beziehungsweise [Run Engine](#run-engine) und Orchestrator. In der einfachsten Form enthält sie genau einen auszuführenden [Ablaufschritt](#sequence-step) (`Sequence Step`) oder eine daraus abgeleitete [Zielpose](#target-description) (`Target Pose`). Dadurch wird klar zwischen der internen Struktur eines Bewegungsprogramms und der einzelnen fachlichen Anforderung unterschieden, welche der Orchestrator konkret verarbeitet.

#### Ablaufdefinition

Die [Ablaufdefinition](#sequence-definition) (`Sequence Definition`) beschreibt eine geordnete Liste von einem oder mehreren [Ablaufschritten](#sequence-step) (`Sequence Step`). Sie repräsentiert damit das eigentliche Bewegungsprogramm, das von der [Run Engine](#run-engine) verarbeitet wird. Die konkrete Speicherform bleibt bewusst offen, damit das Konzept unabhängig von Dateisystem, Speicherkarte oder externer Konfigurationsquelle bleibt.

#### Gelenkzustand

Der [Gelenkzustand](#joint-target-state) (`Joint State`) beschreibt die berechnete Konfiguration des Roboterarms im Gelenkraum. Dazu gehören die Winkel aller relevanten Achsen sowie die Öffnung des Greifers. Dieses Modell ist das Ergebnis der kinematischen Berechnung und die zentrale Schnittstelle zwischen Kinematik, Freigabe und Hardwareansteuerung.

#### Robotermodell

Das [Robotermodell](#robot-model) (`Robot Model`) enthält die geometrischen und mechanischen Eigenschaften des Arms. Dazu gehören Segmentlängen, Gelenkdefinitionen, Vorzeichenkonventionen, Offsets und zulässige Bewegungsbereiche. Es bildet damit die gemeinsame Grundlage für Berechnung, Validierung und spätere Kalibrierung.

#### Robot Model Offset

Zusätzlich zum idealisierten Robotermodell sind modellbezogene Offsets (`Robot Model Offset`) erforderlich, welche bekannte Abweichungen des realen Arms gegenüber dem idealen Modell beschreiben. Dazu gehören insbesondere Schulter-Offsets oder weitere geometrische Korrekturen, die vor der kinematischen Berechnung zu einer `Offset Target Pose` führen können.

#### Hardware Calibration

Für die hardwarenahe Ausgabe sind zusätzlich [Hardware-Kalibrationsdaten](#calibration) (`Hardware Calibration`) erforderlich, welche die Abbildung zwischen fachlichen Gelenkwinkeln beziehungsweise Greiferwerten und realer Servoansteuerung beschreiben. Dazu gehören beispielsweise Minimal- und Maximalwerte sowie gerichtete PWM-Endpunkte pro Aktor. Eine gegenläufige Servo- oder Greiferrichtung wird dadurch ausgedrückt, dass der PWM-Wert am minimalen Fachwert größer sein kann als der PWM-Wert am maximalen Fachwert.

#### Joint PWM State

Für die Übergabe von der `Hardware Abstraction` an den `Hardware Driver` ist ein PWM-bezogenes Ausgabemodell (`Joint PWM State`) sinnvoll. Dieses Modell enthält die vorbereiteten PWM-Sollwerte pro Aktor und trennt damit die fachliche Gelenkbeschreibung sauber von der konkreten Hardwareausgabe.

#### Bewegungsrandbedingungen

Für die Ausführbarkeit von Bewegungen ist ein weiteres Modell für dynamische und physikalische Randbedingungen ([Motion Constraints](#motion-constraints)) sinnvoll. Dieses umfasst beispielsweise zulässige Geschwindigkeiten, Beschleunigungen, Lastgrenzen oder weitere sicherheitsrelevante Begrenzungen. Auf diese Weise können Positionsberechnung und Bewegungsausführung konzeptionell voneinander getrennt bleiben.

#### Bewegungsergebnis

Da die Architektur als Steuerung ohne sensorische Rückmeldung ausgelegt ist, ist ein explizites [Bewegungsergebnis](#motion-result) (`Motion Result`) sinnvoll. Dieses Modell beschreibt nicht die physisch verifizierte Zielerreichung, sondern den fachlichen und technischen Bearbeitungsstatus einer Anforderung. Es kann beispielsweise ausdrücken, dass ein Ziel nicht erreichbar ist, fachlich abgelehnt wurde, zur Ausführung freigegeben wurde oder dass die vorgesehene Sollwertausgabe vollständig abgearbeitet wurde. Zusätzlich kann es Begründungen enthalten, etwa Nichterreichbarkeit, Verletzung von Gelenkgrenzen, Überschreitung dynamischer Randbedingungen oder technische Fehler in der Ausgabe.

#### Ablaufzustand

Für die Abarbeitung eines mehrschrittigen Programms ist zusätzlich ein Ablaufzustand (`Sequence State`) sinnvoll. Dieser beschreibt beispielsweise, welcher Schritt aktuell bearbeitet wird, ob sich der Ablauf in einer Wartephase befindet und ob ein Programm erfolgreich beendet, angehalten oder abgebrochen wurde. Damit kann die Run Engine ihren internen Fortschritt verwalten, ohne hardwarebezogene Details kennen zu müssen.

### Schnittstellen und Datenflüsse

Die Qualität der Architektur hängt nicht nur von der Trennung der Komponenten, sondern auch von klar definierten Übergaben zwischen ihnen ab. Deshalb ist es sinnvoll, die wesentlichen Schnittstellen auf fachlicher Ebene zu beschreiben.

#### Schnittstelle zwischen Anwendung und Orchestrator

Die Anwendung übergibt dem Orchestrator in der einfachsten Form eine Bewegungsanforderung. Im vorgesehenen Betriebsmodell erfolgt diese Übergabe typischerweise durch die Run Engine, welche einzelne Ablaufschritte aus einer Ablaufdefinition nacheinander in solche Anforderungen überführt. Diese Schnittstelle ist bewusst fachlich formuliert und enthält keine hardwarebezogenen Angaben. Als Ergebnis erhält die Anwendung beziehungsweise die Run Engine ein Bewegungsergebnis, aus dem hervorgeht, ob die Anforderung beispielsweise nicht erreichbar, abgelehnt, freigegeben oder ausgabeseitig vollständig abgearbeitet wurde.

#### Schnittstelle zwischen Orchestrator und Kinematik

Der Orchestrator übergibt der Kinematikseite eine gültige Zielpose. Diese kann zunächst mithilfe des `Robot Model Offset` in eine modellkorrigierte Zwischenrepräsentation (`Offset Target Pose`) überführt werden. Die Kinematik liefert daraufhin einen Gelenkzustand oder meldet zurück, dass unter den gegebenen Annahmen keine geeignete Lösung berechnet werden konnte.

#### Schnittstelle zwischen Orchestrator und Prüfkomponente

Die Prüfkomponente erhält entweder eine Zielpose oder einen berechneten Gelenkzustand zusammen mit den zugehörigen Randbedingungen. Sie liefert ein fachliches Prüfergebnis zurück, etwa als `Target Pose Result` oder `Joint State Result`. Dadurch bleibt die Bewertungslogik von der eigentlichen Bewegungsberechnung getrennt.

#### Schnittstelle zwischen Orchestrator, Kalibration und Hardwareabstraktion

Nach der fachlichen Freigabe übergibt der Orchestrator den Gelenkzustand an die Hardwareabstraktionsseite. Dort werden die idealisierten Sollwerte mithilfe der `Hardware Calibration` in einen `Joint PWM State` überführt und anschließend an den `Hardware Driver` weitergereicht. Erst an diesem Punkt erfolgt der Übergang von der fachlichen Beschreibung zur konkreten Servo-Ansteuerung.

#### Fehler- und Rückgabepfade

Da die Architektur ohne sensorische Rückmeldung arbeitet, kommt den Rückgabepfaden besondere Bedeutung zu. Jede beteiligte Komponente sollte deshalb nicht nur erfolgreiche Ergebnisse, sondern auch klar interpretierbare Ablehnungs-, Abschluss- und Fehlerzustände an den Orchestrator zurückgeben. Dieser bündelt die Resultate und stellt sie der Anwendung beziehungsweise der Run Engine in konsistenter Form zur Verfügung. Auf diese Weise bleibt die Verantwortung für die Ablaufsteuerung zentralisiert, auch wenn die fachlichen Bewertungen in verschiedenen Komponenten stattfinden.

### Fehler- und Sicherheitsverhalten

Neben fachlicher Korrektheit muss die Architektur auch beschreiben, wie das System auf Fehler- und Ausnahmesituationen reagiert. Da keine sensorische Rückmeldung vorhanden ist, können nicht alle physischen Fehlzustände erkannt werden. Umso wichtiger ist ein klares Verhalten bei internen Fehlern, Ablehnungen oder technischen Problemen während der Ausgabe.

Zu unterscheiden sind insbesondere:

* fachliche Ablehnungen, etwa bei Nichterreichbarkeit oder Verletzung von Gelenkgrenzen
* technische Fehler, etwa bei Treiberinitialisierung, Kommunikationsproblemen oder unplausiblen Kalibrationsdaten
* Ablaufabbrüche, beispielsweise durch Benutzeraktion, Sicherheitsbedingung oder Systemreset

Für diese Fälle sollte die Architektur vorsehen, dass keine unkontrollierte Weiterverarbeitung stattfindet. Stattdessen muss der Orchestrator das Ergebnis eindeutig klassifizieren und an die Anwendung zurückmelden. Je nach Fehlerklasse kann dies bedeuten, dass ein einzelner Schritt abgelehnt, ein ganzer Ablauf angehalten oder das System nur noch in einer sicheren Grundfunktion weiterbetrieben wird.

Darüber hinaus ist ein sicheres Verhalten beim Systemstart und nach Unterbrechungen relevant. Nach Reset, Stromunterbruch oder unvollständig ausgeführter Bewegung darf die Software nicht stillschweigend davon ausgehen, dass der physische Zustand des Arms weiterhin dem zuletzt bekannten internen Modell entspricht. In solchen Situationen ist eine erneute Initialisierung oder eine bewusste Rückkehr in eine definierte Startlage erforderlich.

Das Fehler- und Sicherheitsverhalten ergänzt damit die reine Bewegungslogik um eine betriebliche Schutzschicht. Diese Schutzschicht kann zwar mangels Sensorik keine vollständige funktionale Sicherheit im engeren Sinn herstellen, sie reduziert jedoch das Risiko inkonsistenter Softwarezustände und unkontrollierter Bewegungsfolgen.

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

Das folgende High-Level-Diagramm zeigt die grobe Struktur der Softwarearchitektur mit den vier Hauptbereichen `Application`, `Orchestration`, `Robotics` und `Hardware`. Ziel dieser Darstellung ist eine gut lesbare Übersicht über die zentralen fachlichen Zusammenhänge.

```mermaid
flowchart LR
    A[Application]
    B[Orchestration]
    C[Robotics]
    D[Hardware]

    A -->|Programmdefinition / Bedienung| B
    B -->|Bewegungsanforderungen| C
    C -->|Bewegungsergebnisse / Sollwerte| B
    B -->|freigegebene Stellwerte| D
    D -->|Ausgabestatus / Fehler| B
    B -->|Statusinformationen| A
```

Der Baustein `Common` wird in diesem High-Level-Diagramm bewusst nicht separat dargestellt, da er keine eigenständige Verarbeitungsschicht beschreibt, sondern gemeinsam genutzte Datenmodelle für mehrere der gezeigten Hauptbereiche bündelt.

#### Komponentendiagramm

Die folgenden Komponentendiagramme verfeinern die vier Hauptbereiche des High-Level-Diagramms jeweils separat. Dabei orientieren sie sich an den in der statischen Struktur beschriebenen Softwarebausteinen `Application`, `Orchestration`, `Kinematics`, `Robot Model`, `Robot Model Offset`, `Validation`, `Hardware`, `Common` und `HardwareCalibration`. Die Bezeichnungen innerhalb der Diagramme sind bewusst englisch gehalten und in einer schreibweisenahen Form gewählt, damit sie näher an spätere oder bereits vorhandene Modul-, Typ- und Verzeichnisnamen bleiben. Begriffe ohne Leerzeichen wie `JointState`, `JointPwmState`, `RestApiServer` oder `HardwareCalibrationResult` bezeichnen dabei code-nahe Datenmodelle oder Implementierungsbausteine; Begriffe mit Leerzeichen beschreiben weiterhin fachliche Architekturkonzepte.

#### Applikation

```mermaid
flowchart LR
    subgraph APP["<<component>> Application"]
        A1[RunEngine]
        A2[SequenceDefinition]
        A3[SequenceStep]
        A4[SequenceState]
        A5[LedAction]
        A6[RobotAction]
        A7[TargetPose]
        A8[REST Interface]
        A9[RestApiServer]
        A10[Calibration HMI]
        A11[ApiContracts]

        A2 -->|contains| A3
        A1 -->|uses| A2
        A1 -->|processes| A3
        A1 -->|manages| A4
        A3 -->|contains| A7
        A3 -->|may trigger| A5
        A3 -->|may trigger| A6
        A10 -->|calls| A8
        A8 -->|is implemented by| A9
        A9 -->|uses| A11
    end
```

#### Ablaufsteuerung

```mermaid
flowchart LR
    subgraph CTRL["<<component>> Orchestration"]
        B1[Orchestrator]
        B2[MotionRequest]
        B3[MotionResult]
        B4[MotionRequestProcessing]

        B2 -->|is handled in| B4
        B4 -->|is coordinated by| B1
        B1 -->|returns| B3
    end
```

#### Robotik

```mermaid
flowchart LR
    subgraph ROB["<<component>> Robotics"]
        C1[Kinematics]
        C2[Validation]
        C3[RobotModel]
        C4[RobotModelOffset]
        C5[MotionConstraints]
        C6[JointState]
        C7[OffsetTargetPose]
        C8[MathUtilities]
        C9[SegmentLengths,\nOffsets,\nJointLimits]

        C1 -.->|uses| C3
        C4 -->|produces| C7
        C1 -.->|receives| C7
        C1 -.->|uses| C8
        C2 -.->|uses| C3
        C2 -.->|considers| C5
        C3 -->|contains| C9
        C1 -->|computes| C6
        C6 -->|is checked by| C2
    end
```

#### Hardware

```mermaid
flowchart LR
    subgraph HW["<<component>> Hardware"]
        D1[Hardware Abstraction]
        D2[Pca9685ServoDriver]
        D3[PCA9685]
        D4[Servo Actuators]
        D5[HardwareCalibration]
        D6[JointPwmState]
        D7[SerialLogger]
        D8[StatusLed / RGB-LED]
        D9[ServoAxisCalibration]
        D10[GripperCalibration]
        D11[initial_pwm_state]
        D12[HardwareCalibrationResult]

        D1 -->|maps to| D6
        D1 -->|reports| D12
        D1 -->|logs via| D7
        D1 -.->|uses| D5
        D5 -->|contains| D9
        D5 -->|contains| D10
        D5 -->|contains| D11
        D6 -->|is written by| D2
        D2 -->|controls| D3
        D3 -->|generates signals for| D4
        D1 -->|controls| D8
    end
```

#### Gemeinsame Bausteine

```mermaid
flowchart LR
    subgraph COM["<<component>> Common"]
        E1[TargetPose]
        E2[MotionRequest]
        E3[JointState]
        E4[MotionResult]
        E5[SequenceState]
        E6[TargetPoseResult]
        E7[JointStateResult]
        E8[JointPwmState]
        E9[JointLimit]
        E10[PWM Limits]

        E2 -->|contains or references| E1
        E4 -->|refers to| E2
        E4 -->|may contain| E3
        E6 -->|refers to| E1
        E7 -->|refers to| E3
        E3 -->|is constrained by| E9
        E8 -->|is constrained by| E10
    end
```

#### Ablaufdiagramm

Das folgende Ablaufdiagramm beschreibt den typischen fachlichen und technischen Verarbeitungsfluss einer einzelnen Bewegungsanforderung. Dabei wird bewusst zwischen fachlicher Prüfung, kinematischer Berechnung, Freigabe und hardwarenaher Ausgabe unterschieden.

```mermaid
flowchart TD
    A[RunEngine selects next SequenceStep] --> B[Create MotionRequest]
    N[REST Interface receives external request] --> B
    B --> O[Orchestrator coordinates processing]
    O --> C[TargetPose validation]
    C -->|invalid TargetPose| D[MotionResult rejected]
    C -->|valid| E[Apply RobotModelOffset]
    E --> F[OffsetTargetPose]
    F --> G[Kinematics computes JointState]
    G --> H[Validation validates JointState]
    H -->|not approved| D
    H -->|approved| I[Hardware Abstraction applies HardwareCalibration]
    I --> P[HardwareCalibrationResult]
    P -->|ok| J[JointPwmState to Pca9685ServoDriver]
    P -->|invalid calibration| D
    J --> K[Status to Orchestrator]
    K --> L[MotionResult to RunEngine]
    K --> Q[REST response]
    L --> M[RunEngine handles wait]
    M --> A
```

## SW Design

### Entwurfsgrundsätze

Das Softwaredesign soll die im Architekturkapitel beschriebenen Schichten so konkretisieren, dass daraus eine robuste und wartbare Implementierung auf dem ESP32 entstehen kann. Dabei steht nicht die maximale funktionale Breite im Vordergrund, sondern ein schrittweiser Entwurf mit klaren Verantwortlichkeiten und gut testbaren Einheiten.

Für das Design gelten insbesondere folgende Grundsätze:

* fachliche Modelle und Hardwarezugriff werden strikt getrennt
* Datenflüsse werden bevorzugt explizit über klar benannte Modelle beschrieben
* Berechnungslogik bleibt deterministisch und frei von Seiteneffekten, soweit dies praktisch möglich ist
* hardwareabhängige Bibliotheken werden hinter schmalen Abstraktionen gekapselt
* Fehler- und Ablehnungszustände werden als reguläre Ergebnisse modelliert und nicht nur als Ausnahmefall betrachtet
* Bewegungsabläufe werden schrittweise verarbeitet, sodass jeder Schritt nachvollziehbar geprüft und protokolliert werden kann
* eine erste Implementierung darf bewusst einfach bleiben, solange spätere Erweiterungen nicht verbaut werden

Diese Grundsätze unterstützen insbesondere die spätere Erweiterung um alternative IK-Verfahren, detailliertere Kalibration, zusätzliche Sicherheitsprüfungen oder neue Anwendungsszenarien.

### Statische Struktur

Die statische Struktur beschreibt die wichtigsten Softwarebausteine und ihre Abhängigkeiten. Sie leitet sich aus der logischen Architektur ab, konkretisiert diese jedoch stärker im Hinblick auf implementierbare Module.

Eine zweckmäßige Struktur besteht aus den folgenden Bausteinen:

* `Application`: enthält die Run Engine, die Ablaufdefinitionen sowie einfache anwendungsnahe Aktionen
* `Orchestration`: enthält den Orchestrator und die Verarbeitung einzelner Bewegungsanforderungen
* `Kinematics`: enthält Vorwärts- und inverse Kinematik sowie mathematische Hilfsfunktionen
* `Robot Model`: enthält Segmentlängen, Gelenkgrenzen und weitere idealisierte Modellparameter
* `Robot Model Offset`: enthält modellbezogene Offsets und Korrekturwerte des realen Arms
* `Validation`: enthält Erreichbarkeitsprüfungen, Freigabelogik und Ausführbarkeitsregeln
* `Hardware`: kapselt Hardware Abstraction, Hardware Driver, Hardware Calibration, Joint PWM State, PCA9685, Servoausgabe, LED-Ansteuerung und weitere gerätenahe Funktionen
* `Common`: enthält gemeinsam genutzte Datentypen, Ergebnisobjekte und Hilfsstrukturen

Zwischen diesen Bausteinen sollen gerichtete Abhängigkeiten gelten. Die Anwendung hängt von der Orchestrierung ab, die Orchestrierung von fachlichen Modellen und Berechnungskomponenten, und erst die hardwarenahen Bausteine kennen die konkrete Ausgabetechnik. Umgekehrte Abhängigkeiten sollen vermieden werden.

Besonders wichtig ist dabei, dass Modelle wie Zielpose, Bewegungsanforderung, Gelenkzustand, Joint PWM State und Bewegungsergebnis nicht implizit in mehreren Komponenten unterschiedlich interpretiert werden. Sie bilden die verbindenden Vertragsobjekte zwischen den Bausteinen.

### Dynamisches Verhalten

Das dynamische Verhalten beschreibt, wie die statischen Bausteine zur Laufzeit zusammenwirken. Im Mittelpunkt steht die sequenzielle Verarbeitung einzelner Bewegungsanforderungen durch die Run Engine und den Orchestrator.

Ein typischer Laufzeitablauf ist wie folgt aufgebaut:

* Die Run Engine hält einen internen Ablaufzustand und wählt den nächsten Ablaufschritt aus.
* Aus diesem Schritt wird eine Bewegungsanforderung an den Orchestrator übergeben.
* Der Orchestrator stößt zunächst eine fachliche Vorprüfung der Zielpose an.
* Nur bei positiver Vorprüfung wird über den `Robot Model Offset` eine modellkorrigierte Zwischenrepräsentation (`Offset Target Pose`) für die Kinematik erzeugt.
* Die Kinematik berechnet daraus einen `Joint State`.
* Der berechnete Gelenkzustand wird fachlich validiert.
* Die `Hardware Abstraction` überführt den freigegebenen Gelenkzustand mithilfe der `Hardware Calibration` in einen `Joint PWM State`.
* Der `Hardware Driver` gibt diese Werte an die Hardware aus und meldet einen technischen Status zurück.
* Der Orchestrator verdichtet diese Informationen zu einem Bewegungsergebnis, das an die Run Engine zurückgegeben wird.
* Die Run Engine entscheidet anhand dieses Ergebnisses über Fortsetzung, Warten, Wiederholen oder Abbruch des Ablaufs.

Da keine sensorische Positionsrückmeldung vorhanden ist, basiert das dynamische Verhalten auf einer Kombination aus berechneter Plausibilität, Freigaberegeln und technischer Ausgabebestätigung. Die Software darf daher nicht behaupten, dass eine Pose physisch verifiziert erreicht wurde, sondern nur, dass eine Anforderung fachlich akzeptiert und ausgabeseitig abgearbeitet wurde.

### Erweiterungsperspektiven

Das Design soll von Anfang an so gestaltet werden, dass spätere Erweiterungen möglich bleiben, ohne die Grundstruktur neu aufbauen zu müssen.

Wichtige Erweiterungsperspektiven sind:

* mehrere austauschbare IK-Strategien mit gemeinsamem fachlichem Eingabe- und Ergebnisformat
* alternative Bewegungsprofile mit Begrenzung von Geschwindigkeit und Beschleunigung
* feinere Kalibrationsmodelle mit achsspezifischen Kennlinien statt rein linearer Zuordnungen
* zusätzliche Schrittarten in der Run Engine, beispielsweise Referenzfahrt, Initialisierung oder Diagnose
* Protokollierung von Bewegungsanforderungen, Prüfergebnissen und Treiberstatus zu Analysezwecken
* spätere Einbindung von Sensorik, etwa Endschaltern oder Strommessung, ohne die bestehende Fachlogik aufzulösen
* Trennung zwischen Simulationsbetrieb und realer Hardwareausgabe

Die Erweiterbarkeit soll jedoch nicht durch unnötige Abstraktion um ihrer selbst willen erkauft werden. Für die erste Ausbaustufe ist eine einfache, nachvollziehbare und gut testbare Struktur wichtiger als ein vollständig generisches Framework.

## Entwicklungshilfsmittel (Off-the-shelf software)

Für die Umsetzung des Projekts werden bewusst etablierte Entwicklungshilfsmittel eingesetzt, um die technische Komplexität auf die eigentlichen Projektziele zu konzentrieren.

* Entwicklungsumgebung: VSCode 
* VSCode Plugin: PlatformIO
* Schaltplan- und Layoutwerkzeug: KiCad
* Unit-test Framework: Unity
* SBOM: Syft

Als Entwicklungsumgebung bietet sich `VSCode` an. Die Umgebung ist leichtgewichtig, weit verbreitet und lässt sich gut mit eingebetteter Softwareentwicklung verbinden. Für das vorliegende Projekt ist insbesondere die Integration mit `PlatformIO` hilfreich, da Build, Upload, serielle Ausgabe und Testausführung direkt aus einer gemeinsamen Oberfläche angestoßen werden können.

PlatformIO dient als Build-, Konfigurations- und Upload-Umgebung für den ESP32. Dadurch können Abhängigkeiten, Board-Konfigurationen und Build-Schritte reproduzierbar verwaltet werden.

Für die Hardwaredokumentation sowie für spätere Schaltplan- und Layoutarbeiten wird `KiCad` verwendet. Dadurch können elektrische Zusammenhänge strukturiert erfasst, versioniert und bei Bedarf bis auf PCB-Ebene weitergeführt werden.

Ein Unit-Test-Framework wird eingesetzt, um mathematische Logik, Datenmodelle und zentrale Prüfregeln frühzeitig automatisiert abzusichern. Dies ist insbesondere für IK, Kalibration und Validierung von Bedeutung.

Für die vorliegenden Randbedingungen bietet sich als primäres Unit-Test-Framework `Unity` an. Die Wahl begründet sich durch die gute Integration in PlatformIO, die Eignung für ressourcenbeschränkte Embedded-Systeme sowie die Möglichkeit, Tests sowohl nativ auf dem Entwicklungsrechner als auch eingebettet auf dem ESP32 auszuführen. Für das Projekt ist dies besonders passend, da mathematische Kernlogik ohne Hardware lokal getestet werden kann, während hardwarenahe Komponenten bei Bedarf direkt auf dem Zielsystem geprüft werden.

Ein schwergewichtiges C++-Testframework mit umfangreicher Mocking-Unterstützung ist für die erste Ausbaustufe nicht zwingend erforderlich. Stattdessen ist eine leichte Testlösung vorteilhaft, die sich gut mit den begrenzten Ressourcen eines Mikrocontrollers verträgt und ohne zusätzlichen Infrastrukturaufwand in die bestehende Toolchain eingebunden werden kann.

Eine SBOM (Software Bill of Materials) ist sinnvoll, um eingesetzte Bibliotheken und externe Abhängigkeiten nachvollziehbar zu dokumentieren. Das unterstützt Transparenz, Wartbarkeit und spätere Sicherheitsbetrachtungen.

Für die Erstellung einer solchen SBOM bietet sich `Syft` als leichtgewichtiges Werkzeug an. Es kann das Projektverzeichnis analysieren und die enthaltenen Komponenten in etablierten SBOM-Formaten wie SPDX oder CycloneDX ausgeben. Auch wenn die konkrete Umsetzung im Projektverlauf noch offen ist, bleibt der Einsatz eines solchen Werkzeugs in der Projektbeschreibung sinnvoll, um den Aspekt der Abhängigkeits- und Herkunftstransparenz früh zu berücksichtigen.

## Methoden

Die Entwicklung des Systems soll nicht nur funktional, sondern auch methodisch nachvollziehbar erfolgen. Da der Kern des Projekts aus mathematischer Logik, klaren Datenflüssen und sicherer Hardwareansteuerung besteht, eignen sich inkrementelle und testorientierte Vorgehensweisen besonders gut.

* Test driven development
* Unit tests

Testgetriebene Entwicklung ist vor allem dort sinnvoll, wo kleine, klar überprüfbare Regeln vorliegen, beispielsweise bei Koordinatentransformationen, Winkelberechnungen, Grenzwertprüfungen oder Kalibrationsabbildungen.

Unit-Tests bilden die Grundlage, um Regressionsfehler früh zu erkennen und die schrittweise Erweiterung der Architektur sicher zu begleiten. Besonders kritisch sind dabei mathematische Kernfunktionen, Bewegungsfreigaben und Randbedingungen.

Im konkreten Projektkontext soll dafür primär `Unity` verwendet werden. Für fachlich und mathematisch unabhängige Komponenten ist eine Ausführung als Native-Test auf dem Entwicklungsrechner sinnvoll. Für hardwarenahe Komponenten wie Treiberanbindung, Servoausgabe oder serielle Testkommunikation können ergänzend Embedded-Tests auf dem ESP32 eingesetzt werden. Dadurch entsteht ein hybrider Testansatz, der schnelle Rückmeldung in der Entwicklung mit realitätsnahen Zielsystemtests verbindet.

## Anforderungen

### Funktionale Anforderungen

Die funktionalen Anforderungen beschreiben, welche fachlichen Fähigkeiten das System bereitstellen soll.

* Das System muss Zielzustände des Endeffektors im kartesischen Raum entgegennehmen können.
* Das System muss Bewegungsanforderungen auf Erreichbarkeit und fachliche Zulässigkeit prüfen können.
* Das System muss für zulässige Zielzustände passende Gelenksollzustände berechnen können.
* Das System muss Gelenksollzustände unter Berücksichtigung von Kalibrationsdaten in hardwaregeeignete Stellwerte überführen können.
* Das System muss vordefinierte Bewegungsabläufe aus mehreren Ablaufschritten ausführen können.
* Das System muss für jede Bewegungsanforderung ein Bewegungsergebnis bereitstellen können.
* Das System muss Begleitaktionen wie Wartezeiten oder LED-Signale innerhalb eines Ablaufs unterstützen können.
* Das System muss unerreichbare oder unzulässige Zielzustände erkennen und ablehnen können.
* Das System muss auf Basis einer definierten Home Position initialisiert werden können.

### Nichtfunktionale Anforderungen

Zusätzlich zu den fachlichen Funktionen muss das System verschiedene qualitative Anforderungen erfüllen.

* Die Software soll modular aufgebaut und gut testbar sein.
* Die mathematische Berechnung soll nachvollziehbar, deterministisch und ausreichend reproduzierbar sein.
* Hardwarenahe Komponenten sollen austauschbar oder zumindest lokal gekapselt sein.
* Die Bewegungsverarbeitung soll fehlertolerant gegenüber ungültigen Zielvorgaben sein.
* Die Architektur soll spätere Erweiterungen um neue IK-Verfahren oder zusätzliche Prüfregeln unterstützen.
* Die Implementierung soll auf die Ressourcen und Echtzeitanforderungen eines ESP32 abgestimmt sein.
* Die Dokumentation soll die fachlichen Modelle, Datenflüsse und Grenzen der Steuerung verständlich beschreiben.

### Randbedingungen und Annahmen

Die folgenden Randbedingungen und Annahmen prägen den Entwurf des Systems:

* Der Roboterarm wird ohne sensorische Rückmeldung betrieben.
* Die reale Zielerreichung kann daher nicht physisch verifiziert, sondern nur ausgabeseitig und fachlich bewertet werden.
* Die Geometrie des Arms wird in einer ersten Ausbaustufe vereinfacht modelliert.
* Mechanische Offsets und Kalibrationsabweichungen werden schrittweise in das Modell integriert.
* Als Zielhardware wird ein ESP32 mit PCA9685-basierter Servoansteuerung verwendet.
* Auf der Zielhardware steht möglicherweise kein komfortables Dateisystem oder keine Speicherkarte zur Verfügung.
* Bewegungsabläufe werden deshalb zunächst als programmnahe oder zur Build-Zeit bereitgestellte Definitionen betrachtet.
* Die Home Position muss aus dem stromlosen Zustand reproduzierbar erreichbar sein.

### Abgrenzung des Projektumfangs

Die erste Ausbaustufe des Projekts konzentriert sich bewusst auf ein klar abgegrenztes Kernsystem für Kinematik, Ablaufsteuerung und hardwarenahe Ausgabe. Nicht alle denkbaren Erweiterungen eines Robotersystems sind daher Bestandteil des aktuellen Projektumfangs.

Insbesondere nicht vorgesehen oder nur nachrangig betrachtet sind in dieser Phase:

* sensorbasierte Positionsrückmeldung oder geschlossene Regelkreise
* automatische Referenzfahrt über Endschalter oder andere Referenzsensoren
* Kollisionsvermeidung mit externer Umgebung
* vollständige Bahnplanung im kartesischen Raum
* autonome Umgebungswahrnehmung oder kamerabasierte Steuerung
* komplexe Greifstrategien oder objektbezogene Manipulationsplanung

Diese Abgrenzung ist nicht als Ausschluss späterer Erweiterungen zu verstehen, sondern dient dazu, den Projektfokus auf eine tragfähige erste Systemarchitektur und eine beherrschbare Implementierung zu richten.


## Spezifikationen

Die Spezifikationen konkretisieren die im Konzept beschriebenen Modelle und Anforderungen in eine umsetzungsnahe Form. Dabei sollen insbesondere die folgenden Aspekte präzise beschrieben werden:

* Format und Wertebereich der Zielbeschreibung `(x, y, z, p, r, g)`
* Definition des Gelenkraums mit Vorzeichenkonventionen und zulässigen Wertebereichen
* Definition der Home Position und ihrer zugehörigen angenommenen Gelenkwerte
* Struktur von Ablaufschritt, Bewegungsanforderung und Bewegungsergebnis
* mathematische und mechanische Parameter des Robotermodells
* Kalibrationsparameter pro Achse
* Regeln für Freigabe, Ablehnung und technische Fehlerzustände
* minimale Schnittstellen zwischen Orchestrator, Kinematik, Prüfung und Hardwareabstraktion

Dieses Kapitel kann im weiteren Verlauf als Übergang zwischen Projektbeschreibung und technischer Detaildokumentation dienen.

## Test Konzept

Das Testkonzept soll sicherstellen, dass fachliche Modelle, mathematische Berechnungen und zentrale Freigaberegeln bereits vor der Hardwareintegration ausreichend geprüft werden. Aufgrund der Trennung zwischen Berechnungslogik und Hardwareabstraktion bietet sich ein mehrstufiges Testvorgehen an.

* Unit tests
* Unit test coverage >75%
* Hardware in the Loop?

Unit-Tests bilden die erste und wichtigste Ebene. Sie sollen insbesondere für Kinematik, Erreichbarkeitsprüfung, Kalibrationsabbildung und Bewegungsfreigabe eingesetzt werden.

Als Testframework wird hierfür primär `Unity` vorgesehen. Die Testorganisation sollte zwischen Native-Tests und Embedded-Tests unterscheiden. Native-Tests eignen sich vor allem für Robotermodell, Kinematik, Prüfregeln und Kalibrationslogik, während Embedded-Tests zusätzlich das Zusammenspiel mit Plattformbibliotheken, serieller Ausgabe und hardwarenaher Abstraktion prüfen können.

Eine hohe Testabdeckung ist vor allem in den fachlichen Kernkomponenten sinnvoll. Die Zahl allein ist jedoch nicht ausreichend; entscheidend ist, dass kritische Rechen- und Entscheidungslogik durch aussagekräftige Testfälle abgesichert wird.

Hardware-in-the-Loop-Tests können ergänzend sinnvoll sein, um das Zusammenspiel aus Kalibration, Servoausgabe und Ablaufsteuerung unter realen Randbedingungen zu prüfen. Für die erste Ausbaustufe kann dies optional bleiben, sollte aber als späterer Ausbauschritt vorgesehen werden.

## Test Plan

Der Testplan beschreibt, welche Testarten in welcher Reihenfolge durchgeführt werden sollen und welche Ziele damit jeweils verbunden sind.

1. Prüfung der mathematischen Grundfunktionen, etwa Vektorrechnung, Winkelberechnung und Koordinatentransformation.
2. Prüfung der IK-Komponente mit erreichbaren, grenzwertigen und unerreichbaren Zielzuständen.
3. Prüfung der Validierungslogik für Gelenkgrenzen, Offsets und Bewegungsrandbedingungen.
4. Prüfung der Kalibrationskomponente mit repräsentativen Sollwerten je Achse.
5. Prüfung des Orchestrators mit simulierten Bewegungsanforderungen und erwarteten Bewegungsergebnissen.
6. Prüfung der Run Engine mit mehrschrittigen Abläufen, Wartezeiten und Abbruchbedingungen.
7. Optionaler Integrationstest mit realer Hardware oder einer hardwareähnlichen Testumgebung.

Für jeden Testfall sollen Eingabe, erwartetes Ergebnis und fachliche Begründung dokumentiert werden.

## Test Report

Der Test Report dient zur dokumentierten Zusammenfassung der tatsächlich durchgeführten Tests und ihrer Ergebnisse. Er sollte mindestens enthalten:

* getestete Komponenten und Versionen
* durchgeführte Testfälle
* bestandene und fehlgeschlagene Prüfungen
* bekannte Einschränkungen oder offene Defekte
* Bewertung, welche Projektziele bereits ausreichend abgesichert sind und wo weiterer Testbedarf besteht

Dieses Kapitel kann im Projektverlauf schrittweise mit konkreten Resultaten ergänzt werden.

## Anhang

### Glossar und Abkürzungen

| Begriff / Abkürzung | Beschreibung |
| --- | --- |
| <a id="application"></a>Application | Softwarebaustein der Anwendungsebene. Er umfasst im Projekt insbesondere `Run Engine`, `Sequence Definition`, `Sequence Step`, `Sequence State`, `LED Action`, `Robot Action` und `Target Pose`. |
| CCD | Cyclic Coordinate Descent. Iteratives Verfahren zur Lösung inverser Kinematik, bei dem Gelenke nacheinander so angepasst werden, dass sich der Endeffektor schrittweise an ein Ziel annähert. |
| <a id="common"></a>Common | Gemeinsamer Softwarebaustein für fachliche Datenmodelle, die von mehreren anderen Komponenten verwendet werden. |
| <a id="calibration"></a>Hardware Calibration | Datenmodell zur Abbildung fachlicher Gelenk- und Greiferwerte auf hardwarenahe Stellwerte unter Berücksichtigung fachlicher Grenzen und gerichteter PWM-Endpunkte. |
| <a id="calibration-data"></a>Robot Model Offset | Modellbezogene Offsets und Korrekturwerte, welche das reale Robotermodell gegenüber dem idealisierten Modell beschreiben. |
| Endeffektor | Das funktionale Ende des Roboterarms. Im vorliegenden Projekt besteht der Endeffektor aus Handgelenk und Greifer. |
| ESP32 | Mikrocontroller-Plattform, auf der die Steuerungssoftware des Projekts ausgeführt wird. |
| FABRIK | Forward And Backward Reaching Inverse Kinematics. Iteratives IK-Verfahren, bei dem Segmentpunkte abwechselnd vom Ziel und von der Basis aus neu positioniert werden. |
| Greiferöffnung | Öffnungszustand des Greifers. Im Dokument wird dieser Wert mit `g` bezeichnet und in Prozent angegeben. Im Unterschied zu Pitch und Roll bleibt `g` im Task Space und im Joint Space bewusst dieselbe fachliche Größe. |
| <a id="hardware-abstraction"></a>Hardware Abstraction | Softwarebaustein, der freigegebene Sollwerte in konkrete hardwarebezogene Operationen überführt und dabei Treiber und Ausgabekanäle kapselt. |
| <a id="hardware-driver"></a>Hardware Driver | Konkrete hardwarenahe Komponente zur Kommunikation mit Ausgabebausteinen wie dem PCA9685. |
| <a id="home-position"></a>Home Position | Definierte Ausgangslage des Roboterarms, die aus dem stromlosen Zustand reproduzierbar erreicht werden kann und als fachliche Referenz für Initialisierung und Aufstartverhalten dient. |
| IK | Inverse Kinematik. Berechnung von Gelenkwinkeln aus einer gewünschten Position und Orientierung des Endeffektors. |
| Joint Space | Darstellung des Roboterzustands im Gelenkraum. Beschrieben werden dabei die Winkel `d`, `s`, `e`, `hp`, `hr` sowie die Greiferöffnung `g`. |
| Joint PWM State | PWM-bezogenes Ausgabemodell mit den vorbereiteten Stellwerten pro Aktor zwischen `Hardware Abstraction` und `Hardware Driver`. |
| Joint State Result | Fachliches Prüfergebnis zur Bewertung eines berechneten `Joint State`. |
| <a id="joint-target-state"></a>Joint State | Datenmodell der berechneten Gelenkkonfiguration, bestehend aus den relevanten Gelenkwinkeln und der Greiferöffnung. |
| Kalibration | Sammelbegriff für modellbezogene Offsets des realen Arms sowie die hardwarenahe Abbildung von fachlichen Sollwerten auf PWM-bezogene Aktorwerte. |
| <a id="kinematics"></a>Kinematics | Softwarebaustein für Vorwärts- und inverse Kinematik sowie zugehörige mathematische Hilfsfunktionen. |
| Math Utilities | Hilfsfunktionen für mathematische Operationen, die in Kinematik und verwandten Berechnungen verwendet werden. |
| <a id="motion-constraints"></a>Motion Constraints | Datenmodell für dynamische und physikalische Randbedingungen einer Bewegung, etwa Geschwindigkeiten, Lastgrenzen oder Beschleunigungen. |
| <a id="motion-request"></a>Motion Request | Fachliches Übergabeobjekt zwischen Anwendungsschicht und Orchestrator für die Verarbeitung einer einzelnen Bewegungsanforderung. |
| <a id="motion-result"></a>Motion Result | Ergebnisobjekt, das den fachlichen und technischen Bearbeitungsstatus einer Bewegungsanforderung beschreibt. |
| <a id="orchestration"></a>Orchestration | Softwarebaustein zur Koordination von Bewegungsanforderungen, Prüfungen, Berechnungen und Rückgabepfaden. |
| Offset Target Pose | Modellkorrigierte Zwischenrepräsentation einer Zielpose nach Anwendung des `Robot Model Offset` und vor der kinematischen Berechnung. |
| PCA9685 | PWM-Treiberbaustein, der zur Ansteuerung mehrerer Servokanäle verwendet wird. |
| Pitch | Neigung des Endeffektors beziehungsweise des Handgelenks. Im Task Space wird diese Größe mit `p` bezeichnet, im Joint Space als Handgelenk-Pitch mit `hp`. |
| <a id="robot-action"></a>Robot Action | Fachlich benennbare Roboteraktion innerhalb eines Ablaufschritts, die nicht ausschließlich durch eine kartesische Zielbeschreibung beschrieben wird. |
| <a id="robot-model"></a>Robot Model | Daten- und Softwarebaustein zur Beschreibung der Geometrie, Offsets, Segmentlängen und Gelenkgrenzen des Roboterarms. |
| Roll | Rotation um die Längsachse des Endeffektors beziehungsweise des Handgelenks. Im Task Space wird diese Größe mit `r` bezeichnet, im Joint Space als Handgelenk-Roll mit `hr`. |
| Robotics | Sammelbegriff für die fachlichen Bausteine `Kinematics`, `Validation`, `Robot Model`, `Robot Model Offset` und verwandte Datenmodelle. |
| <a id="run-engine"></a>Run Engine | Anwendungskomponente zur sequentiellen Ausführung vordefinierter Bewegungsabläufe. |
| SBOM | Software Bill of Materials. Strukturierte Auflistung eingesetzter Softwarekomponenten und Abhängigkeiten. |
| <a id="sequence-definition"></a>Sequence Definition | Datenmodell oder Baustein zur Beschreibung eines vollständigen Bewegungsablaufs aus mehreren Schritten. |
| Sequence State | Datenmodell zur Beschreibung des aktuellen Fortschritts eines mehrschrittigen Bewegungsablaufs. |
| <a id="sequence-step"></a>Sequence Step | Einzelner Schritt innerhalb eines Bewegungsablaufs mit Zielbeschreibung und optionalen Zusatzaktionen. |
| Serial Output | Hardwarenaher Ausgabebaustein für serielle Kommunikation, Diagnose oder Statusmeldungen. |
| Servo Output | Hardwarenaher Ausgabebaustein zur Erzeugung oder Übergabe von Stellwerten für Servoantriebe. |
| Sollzustand | Gewünschter Zielzustand eines Systems. Im vorliegenden Projekt beschreibt der Sollzustand des Endeffektors insbesondere Position, Orientierung und Greiferöffnung. |
| Task Space | Darstellung des Roboterzustands im Welt- beziehungsweise Arbeitskoordinatensystem. Im Dokument wird der Endeffektorzustand dort durch `(x, y, z, p, r, g)` beschrieben. |
| <a id="target-description"></a>Target Pose | Fachliches Datenmodell zur Beschreibung von Position, Orientierung und Greiferöffnung des Endeffektors. |
| Target Pose Result | Fachliches Prüfergebnis zur Bewertung einer `Target Pose` vor der kinematischen Verarbeitung. |
| Unity | Leichtgewichtiges Unit-Test-Framework, das für native und eingebettete Tests im Projekt vorgesehen ist. |
| Validation | Softwarebaustein zur fachlichen Bewertung von Erreichbarkeit, Gelenkgrenzen und Ausführbarkeit einer Bewegung. |
| VSCode | Visual Studio Code. Entwicklungsumgebung, die im Projekt in Verbindung mit Platform IO eingesetzt werden kann. |

### Literaturverzeichnis

<a id="ref-1"></a>
[1] Aristidou, A., Lasenby, J., Chrysanthou, Y. und Shamir, A.: Inverse Kinematics Techniques in Computer Graphics: A Survey. Computer Graphics Forum, 37(6), 2018. https://onlinelibrary.wiley.com/doi/10.1111/cgf.13310

<a id="ref-2"></a>
[2] Aristidou, A. und Lasenby, J.: FABRIK: A fast, iterative solver for the inverse kinematics problem. Graphical Models, 73(5), 243-260, 2011. https://andreasaristidou.com/FABRIK

<a id="ref-3"></a>
[3] Kenwright, B.: Inverse Kinematics - Cyclic Coordinate Descent (CCD). Journal of Graphics Tools / technische Einführung. https://alogicalmind.com/paper/ik_ccd/

<a id="ref-4"></a>
[4] NXP Semiconductors: PCA9685 - 16-Channel, 12-bit PWM Fm+ I2C-Bus LED Controller, Product Data Sheet, Rev. 4 - 16 April 2015. Lokale Ablage: `datasheet/PCA9685_NXP_Datasheet_Rev4_2015-04-16.pdf`. Original-URL: https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf

<a id="ref-5"></a>
[5] Joy-it: Robot02 Datasheet, Version vom 2019-09-19. Lokale Ablage: `datasheet/Robot02_Datasheet_2019_09_19.pdf`

<a id="ref-6"></a>
[6] Joy-it: COM-Motor02 Datasheet. Lokale Ablage: `datasheet/COM-Motor02-Datasheet.pdf`
