# Projektbeschreibung

Dieses Projekt implementiert einen 6-achsigen Roboterarm von Joy-it. Der Produktname lautet „Joy-it Grab-it“ und er besteht aus 6 unabhängigen Servoantrieben (COM-Motor02). Die technischen Daten der Servoantriebe finden Sie in der Datei: COM-Motor02-Datasheet.pdf
Das Ziel ist es, den Arm mithilfe der inversen Kinematik reibungslos zu steuern und eine höhere Programmiersprache auf einem ESP32-Modul zu verwenden, um moderne Programmiertechniken zu üben.
Eine weitere wichtige Anforderung ist die Verwendung einer Servokarte mit einem 

## Technischer Hintergrund

### Einführung in Inverse Kinematik

### Beschreibung des idealen Robotterarms

#### Welt Koordinatensystem (task space)

Das Welt-Koordinatensystem besteht aus 6 Dimensionen:
(x,y,z,p,r,g)
* Dabei beschreiben (x,y,z) die Positionen im Raum.
* p ist die Pitch Achse des Handgelenks
* r ist die Roll Achse des Handgelenks
* g ist der Greifer

Hier wird definiert an welcher Postion (x,y,z) der Greiffer zum Nullpunkt stehen soll. Ausserdem an dieser Position der Greifer eine Neigung (pitch p) eine Drehung (roll r) und einen Öffnung.

Die Positionen werden in [mm] gemessen, Bezugspunkt ist (0,0,0) von der Standfläche des Motors genau unterhalb der Achse des Drehtellers.
Pitch und Roll werden in [°] gemessen, Bezugspunkt ist die Horizontal-Ebene.
Greiferöffnung wird in [%] gemessen, 0% entsprich vollständig geschlossen, 100% entspricht vollständig geöffnet.

#### Definition des Arbeitsraumes (cartesian space)

Der Arbeitsraum definiert die Positionen der einzelnen Servo-Antrieben in (x,y,z).

Wir brauchen folgende Positionen als kartesiche 3D Vektoren:
* Drehteller D liegt fest bei (0,0,0)
* Ellenbogen E(x,y,z)
* Handgelenk H(x,y,z)
* Greifferspitze G(x,y,z)

#### Definition des Gelenkraumes (jont space)
Als letztes definieren wir den Gelenkraum. Bis auf den Greifer haben alle Achsen die Einheit [°].
* Drehteller d (-90°..90°), 0° zeigt in Richtung der y-Achse des Welt-Koordinatensystems.
* Schulter s (-90°..90°), -90° ist horizontal in Richtung der y-Achse, 0° zeigt vertikal nach oben (Richtung der z-Achse) und 90° kippt die Schulter in Richtung der negativen z-Achse.
* Ellenbogen e (-90°..90°). Hier ist die 0° Position wenn Oberarm und Unterarm in die selbe Richtung zeigen. -90° Kippt nach unten, +90° kippt nach oben.
* Handgelenk h (-90°..90°) analog der Ellenbogen Achse.
* Rotation r (-90°..90°) bei -90° dreht das Hangelenk nach links, bei 90° dreht das Handgelenk nach rechts. Blickrichtung: Vom Drehteller nach vorne.

### Beschreibung des realen Robotterarmes

Die mechanische Konstruktion des Armes hat folgende Fehlstellungen:
* Die Schulter besitzt einen festen Offset nach vorne (Positive y-Achse)
* Pitch und Roll haben einen festen Offset zueinander.
* Roll und das Zentrum des Grippers haben einen festen Offset (Achsen) zueinander.

Aktuell werden andere Offsets vernachlässigt.