# Controller-Bedienung

Dieses Dokument hält den aktuellen Zwischenstand für die Controller-Bedienung fest. Es beschreibt noch keine freigegebene manuelle Robotersteuerung, sondern die Visualisierung und die aktuell testbare digitale Jog-Zuordnung.

## Visualisierung

```
|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|
|   ZL   |        |        |        |        |        |        |        |        |        |        |   ZR   |
|   L    |        |        |        |        |        |        |        |        |        |        |   R    |
|   GL   |        |        |        |        |        |        |        |        |        |        |   GR   |
|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|

|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|
|        | Up     |        |        |   -    |        |        |   +    |        |        |   X    |        |
| Left   |        | Right  |        |        |  Cap   |  Home  |        |        |   Y    |        |   A    |
|        | Down   |        |        |        |  Cam   |        |        |        |        |   B    |        |
|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|
```

## Aktuelle digitale Jog-Zuordnung

Die aktuelle Default-Zuordnung liegt in `src/application/ControllerJog.h`. Sie ist Native-getestet, aber noch nicht als sichere reale Robotersteuerung freigegeben.

| Eingabe | Achse | Richtung | Geschwindigkeit |
| --- | --- | ---: | ---: |
| `GripL` | `d` | negativ | `40 deg/s` |
| `GripR` | `d` | positiv | `40 deg/s` |
| `D-Pad Up` | `s` | positiv | `40 deg/s` |
| `D-Pad Down` | `s` | negativ | `40 deg/s` |
| `D-Pad Left` | `e` | negativ | `40 deg/s` |
| `D-Pad Right` | `e` | positiv | `40 deg/s` |
| `B` | `hp` | negativ | `40 deg/s` |
| `X` | `hp` | positiv | `40 deg/s` |
| `Y` | `hr` | negativ | `40 deg/s` |
| `A` | `hr` | positiv | `40 deg/s` |
| `L` | `g` | negativ | `40 pct/s` |
| `R` | `g` | positiv | `40 pct/s` |

Die effektive Änderung ergibt sich aus `direction * velocity_per_second * elapsed_ms`. Die Werte werden anschliessend gegen die gemeinsamen `JointState`-Limits begrenzt.

## Sicherheitsentscheidung

Für die nächste Stufe wird bewusst kein zusätzliches Sicherheitsmodell für manuelle Controller-Bedienung eingeführt. Es gibt also vorerst keine gesonderte Hold-, Stop-, Home-, Disconnect-, Rate-Limit- oder Servo-Freigabe-Logik nur für den Controller-Pfad.

Die relevante technische Schutzgrenze sind die gemeinsamen `JointState`-Limits. `applyControllerJog()` begrenzt jede berechnete Änderung gegen diese Achsen-Limits, bevor ein neuer Gelenkzustand zurückgegeben wird. Darüber hinaus gilt: Der Benutzer muss wissen, was er tut, und kann den Roboterarm durch ungünstige, aber innerhalb der Limits liegende Bewegungen bewusst crashen.

Analogsticks bleiben für diese Stufe ebenfalls ausserhalb des Bedienmodells. Die aktuelle Zuordnung nutzt nur digitale Eingaben.
