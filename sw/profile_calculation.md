# Design: Berechnung von Bewegungsprofilen

## Einleitung

Dieses Dokument beschreibt, wie die in der Software vorgesehenen Bewegungsprofile rechnerisch erzeugt werden können. Es ergänzt die allgemeine Softwaresicht in [software.md](./software.md) um eine verständliche und umsetzungsnahe Beschreibung der Profilberechnung.

Ziel ist bewusst nicht eine möglichst mathematische oder akademische Darstellung. Stattdessen soll das Dokument so aufgebaut sein, dass ein technisch interessierter Leser die Grundidee, die Begriffe und die späteren Implementationsschritte nachvollziehen kann, auch wenn mathematische Herleitungen nicht zum beruflichen Alltag gehören.

## Worum geht es überhaupt

Die inverse Kinematik beantwortet die Frage:

* Welche Gelenkwinkel braucht der Arm, um ein Ziel zu erreichen?

Das Bewegungsprofil beantwortet eine andere Frage:

* Wie soll der Arm von seinem aktuellen Gelenkzustand zum Zielgelenkzustand fahren?

Ohne Bewegungsprofil könnte die Software einfach den alten Zustand durch den neuen ersetzen. In der Praxis wäre das aber zu grob:

* Bewegungen würden abrupt starten
* Bewegungen würden abrupt stoppen
* der Arm würde unnötige Lastspitzen und sichtbares Ruckeln erzeugen

Deshalb wird zwischen Startzustand und Zielzustand eine Folge von Zwischenzuständen erzeugt. Diese Folge ist der `MotionPlan`.

## Grundidee des MotionPlan

Für die vereinfachte Ausbaustufe wird angenommen:

* der `Motion Profile Generator` erzeugt Stützstellen in festem Zeitraster
* das Zeitraster ist durch `sample_time_ms` vorgegeben
* die gewünschte Zielgeschwindigkeit ist durch `target_velocity_deg_s` vorgegeben
* der Profiltyp ist einer von:
  * `constant_velocity`
  * `constant_acceleration`
  * `smooth_start_stop`

Damit ist der `MotionPlan` im Kern:

* eine Liste von Zeitpunkten
* und zu jedem Zeitpunkt ein passender `JointState`
* sowie die zugehörige Gesamtdauer der Bewegung

Beispielhaft könnte das so aussehen:

```text
t = 0 ms      -> JointState 0
t = 20 ms     -> JointState 1
t = 40 ms     -> JointState 2
t = 60 ms     -> JointState 3
...
t = T         -> JointState Ziel
```

Wichtig ist: Nicht die Zeitpunkte ändern sich von Profil zu Profil, sondern die Art, wie die Gelenkwerte zwischen Start und Ziel über diese Zeitpunkte verteilt werden.

## Einfache Begriffe

Bevor wir auf die drei Profile eingehen, helfen ein paar einfache Begriffe.

### Startwert und Zielwert

Für jede Achse gibt es:

* einen Startwert `q0`
* einen Zielwert `q1`

Beispiel:

```text
Schulter:
q0 = 10 deg
q1 = 50 deg
```

Dann beträgt die Änderung:

```text
delta_q = q1 - q0 = 40 deg
```

Oft interessiert uns zusätzlich nur die Größe der Änderung:

```text
|delta_q| = 40 deg
```

### Gesamtdauer der Bewegung

Damit aus Start und Ziel ein Bewegungsplan wird, braucht die Bewegung eine Dauer `T`.

In der vereinfachten Variante wird `T` aus der größten Achsbewegung und der gewünschten Zielgeschwindigkeit abgeschätzt.

Wenn zum Beispiel die größte Achsänderung `60 deg` ist und die gewünschte Zielgeschwindigkeit `30 deg/s`, dann gilt näherungsweise:

```text
T = 60 deg / 30 deg/s = 2 s
```

Das ist praktisch, weil dann alle Achsen gemeinsam starten und gemeinsam am Ziel ankommen.

### Feste Stützstellen

Wenn das Zeitraster `sample_time_ms = 20 ms` ist, dann entstehen Stützstellen zu:

```text
0 ms, 20 ms, 40 ms, 60 ms, ...
```

Bei einer Gesamtdauer von `2 s = 2000 ms` wären das:

```text
0, 20, 40, ..., 1980, 2000
```

Für jeden dieser Zeitpunkte muss berechnet werden, wo jede Achse stehen soll.

## Ein gemeinsames Denkmuster für alle Profile

Alle drei Profile lassen sich mit derselben Grundidee beschreiben:

1. Wir betrachten die gesamte Bewegung von Start bis Ziel.
2. Wir normieren die Zeit auf einen Bereich von `0` bis `1`.
3. Wir berechnen, wie weit die Bewegung zu einem bestimmten Zeitpunkt bereits fortgeschritten sein soll.
4. Wir übertragen diesen Fortschritt auf jede Achse.

Die normierte Zeit ist:

```text
u = t / T
```

Das bedeutet:

* bei `t = 0` ist `u = 0`
* bei `t = T / 2` ist `u = 0.5`
* bei `t = T` ist `u = 1`

Jetzt brauchen wir nur noch eine Funktion `f(u)`, die sagt:

* Wie groß ist der Bewegungsfortschritt zwischen `0` und `1`?

Dann ergibt sich der aktuelle Achswert aus:

```text
q(t) = q0 + (q1 - q0) * f(u)
```

Das sieht vielleicht zuerst mathematisch aus, ist aber inhaltlich schlicht:

* `q0` ist der Startwert
* `q1 - q0` ist die gesamte Wegänderung
* `f(u)` ist der bisher zurückgelegte Anteil

Wenn `f(u) = 0.25`, dann sind eben 25 % des Weges geschafft.

## Profil 1: konstante Geschwindigkeit

### Intuition

Bei konstanter Geschwindigkeit soll die Achse in gleichen Zeiten immer gleich große Wegstücke zurücklegen.

Wenn also alle `20 ms` abgetastet wird, dann soll sich der Winkel bei jeder Stützstelle um denselben Betrag ändern.

### Mathematische Idee

Hier ist der Fortschritt einfach linear:

```text
f(u) = u
```

Damit wird:

```text
q(t) = q0 + (q1 - q0) * u
```

### Anschauliches Beispiel

Nehmen wir:

```text
q0 = 10 deg
q1 = 50 deg
T  = 2 s
```

Dann ist die Gesamtänderung:

```text
40 deg
```

Bei halber Zeit, also `u = 0.5`, ergibt sich:

```text
q = 10 + 40 * 0.5 = 30 deg
```

Bei `u = 0.75`:

```text
q = 10 + 40 * 0.75 = 40 deg
```

### Was ist gut daran

* sehr einfach zu berechnen
* sehr gut testbar
* gleichmäßige Bewegung im Mittelteil

### Was ist daran nicht ideal

* der Start ist hart
* das Stoppen ist hart
* reale Servos und Mechanik reagieren darauf oft mit Ruck

## Profil 2: konstante Beschleunigung

### Intuition

Hier soll die Bewegung nicht sofort mit voller Geschwindigkeit starten. Stattdessen:

* zuerst beschleunigen
* dann eventuell mit hoher Geschwindigkeit weiterfahren
* zum Schluss wieder abbremsen

Das bedeutet:

* am Anfang werden die Wegschritte zwischen den Stützstellen erst klein sein
* dann größer
* dann gegen Ende wieder kleiner

### Einfache mathematische Näherung

Für die erste Ausbaustufe kann eine symmetrische Funktion verwendet werden:

```text
f(u) =
    2 * u^2                 fuer 0 <= u < 0.5
    1 - 2 * (1 - u)^2       fuer 0.5 <= u <= 1
```

Das bedeutet:

* in der ersten Hälfte wächst der Fortschritt zunächst langsam und dann schneller
* in der zweiten Hälfte wächst er zunächst noch weiter, flacht aber gegen Ende wieder ab

### Warum das sinnvoll ist

Diese Funktion ist einfach genug für eine verständliche Implementierung und zeigt bereits das gewünschte Verhalten:

* sanfterer Start als bei `constant_velocity`
* sanfteres Ende als bei `constant_velocity`
* trotzdem noch gut überschaubar

### Wichtiger Hinweis

Streng genommen ist dies noch nicht das vollständige klassische Trapezprofil mit exakt ausmodellierter Konstantfahrtphase. Es ist eher eine einfache, symmetrische Näherung für:

* Beschleunigungsphase
* Bremsphase

Für längere Bewegungen könnte später zusätzlich eine echte Mittelphase mit konstanter Geschwindigkeit eingeführt werden.

## Profil 3: sanftes Losfahren und Abbremsen

### Intuition

Dieses Profil soll noch weicher sein als die Variante mit konstanter Beschleunigung.

Ziel ist:

* der Start soll weich sein
* das Ende soll weich sein
* die Übergänge sollen insgesamt ruhiger wirken

### Einfache mathematische Näherung

Für die erste Ausbaustufe bietet sich eine kubische Blendfunktion an:

```text
f(u) = 3 * u^2 - 2 * u^3
```

Diese Funktion hat eine sehr angenehme Eigenschaft:

* am Anfang ist die Steigung null
* am Ende ist die Steigung null

In einfacher Sprache heißt das:

* die Bewegung läuft weich an
* die Bewegung läuft weich aus

### Warum diese Funktion gut passt

Sie ist:

* mathematisch noch überschaubar
* einfach zu implementieren
* für viele technische Leser noch gut nachvollziehbar
* deutlich ruhiger als eine rein lineare Bewegung

### Was sie noch nicht ist

Auch diese Funktion ist noch kein vollständig physikalisch ausgearbeitetes S-Kurven-Profil mit expliziter Ruckbegrenzung. Für die erste Ausbaustufe reicht sie aber gut als verständliche und nützliche Annäherung.

## Wie wird daraus ein kompletter MotionPlan

Jetzt setzen wir die Bausteine zusammen.

### Schritt 1: Start- und Zielzustand festlegen

Der `Motion Profile Generator` erhält:

* Start-`JointState`
* Ziel-`JointState`
* `MotionProfile`

### Schritt 2: Größten Achsweg bestimmen

Für jede Achse wird berechnet:

```text
delta_q_i = |q1_i - q0_i|
```

Dann wird der größte dieser Werte bestimmt:

```text
delta_q_max = max(delta_q_i)
```

Diese Achse bestimmt die Gesamtdauer der Bewegung.

### Schritt 3: Gesamtdauer bestimmen

Mit der vorgegebenen Zielgeschwindigkeit:

```text
target_velocity_deg_s
```

ergibt sich:

```text
T = delta_q_max / target_velocity_deg_s
```

Falls `delta_q_max = 0`, liegt bereits keine Bewegung mehr vor. Dann kann der `MotionPlan` direkt aus einem einzigen Zustand bestehen.

### Schritt 4: Zeitpunkte erzeugen

Nun werden die Stützstellen erzeugt:

```text
t0 = 0
t1 = sample_time_ms
t2 = 2 * sample_time_ms
...
tn = T
```

Falls `T` nicht exakt auf das Raster fällt, wird die letzte Stützstelle trotzdem explizit auf den Zielzeitpunkt gesetzt, damit der Plan sauber am Ziel endet.

### Schritt 5: Für jeden Zeitpunkt den Fortschritt berechnen

Für jeden Zeitpunkt `tk` gilt:

```text
uk = tk / T
```

Dann wird je nach Profiltyp die passende Funktion verwendet:

* `constant_velocity`
* `constant_acceleration`
* `smooth_start_stop`

und daraus `f(uk)` berechnet.

### Schritt 6: Für jede Achse den Sollwert berechnen

Für jede Achse gilt:

```text
q_i(tk) = q0_i + (q1_i - q0_i) * f(uk)
```

Alle Achswerte zusammen ergeben den `JointState` der Stützstelle.

### Schritt 7: `TimedJointState` erzeugen

Für jeden Zeitpunkt wird gespeichert:

* der berechnete `JointState`
* `time_from_start_ms`

Damit entsteht schrittweise der vollständige `MotionPlan`.

Zusätzlich speichert der `MotionPlan` die berechnete Gesamtdauer der Bewegung als `total_duration_ms`. Damit ist die Zeitspanne des Plans nicht nur indirekt über die letzte Stützstelle, sondern auch explizit im Datenmodell sichtbar.

## Kleines Beispiel

Angenommen:

* größte Achsänderung `delta_q_max = 60 deg`
* `target_velocity_deg_s = 30 deg/s`
* `sample_time_ms = 20 ms`

Dann gilt:

```text
T = 60 / 30 = 2 s = 2000 ms
```

Damit ergeben sich Stützstellen bei:

```text
0, 20, 40, 60, ..., 2000 ms
```

Für jeden dieser Zeitpunkte wird:

1. `u = t / 2000`
2. `f(u)` passend zum Profil
3. daraus der Sollwert jeder Achse

berechnet.

## Was an diesem Ansatz bewusst einfach ist

Für die erste Ausbaustufe wird einiges bewusst vereinfacht:

* nur ein globaler Zielgeschwindigkeitswert
* feste Sampling-Zeit
* einfache mathematische Profilfunktionen
* keine getrennte per-Achse-Beschleunigungsgrenze
* kein vollständig physikalisches Servo- oder Lastmodell

Das ist kein Nachteil, sondern eine bewusste Designentscheidung. Der Ansatz bleibt so:

* verständlich
* deterministisch
* leicht zu testen
* gut implementierbar

## Was später erweitert werden kann

Auf dieser Grundlage lassen sich später weitere Schritte ergänzen:

* echte trapezförmige Profile mit definierter Konstantfahrtphase
* getrennte Geschwindigkeits- und Beschleunigungsgrenzen pro Achse
* echte S-Kurven mit expliziter Ruckbegrenzung
* Berücksichtigung servo- oder lastabhängiger Grenzen
* feinere Synchronisation zwischen Profilgenerator und Hardwareausgabe

## Bezug zur Softwarearchitektur

Dieses Dokument beschreibt nur die mathematische und logische Bildung des `MotionPlan`. Es ändert nicht die Verantwortlichkeiten der Softwarearchitektur:

* `Kinematics` berechnet den Zielzustand
* `Motion Profile Generator` erzeugt daraus zusammen mit Startzustand und Profil den `MotionPlan`
* `Hardware Abstraction` setzt die Stützstellen in hardwarenahe Stellwerte um

Für die Einbettung in den Gesamtkontext bleibt [software.md](./software.md) das führende Dokument.
