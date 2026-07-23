# Design: REST API

Dieses Dokument beschreibt die aktuelle HTTP/JSON-Schnittstelle der ESP32-S3-Firmware.

Die API wird vom `RestApiServer` bereitgestellt. Sie enthält weiterhin Bring-up-Schnittstellen für direkte Gelenk- und PWM-Kommandos; der reguläre Task-Space-Endpunkt `/api/motion` nutzt inzwischen den Orchestrator für Validierung, IK und Motion-Plan-Erzeugung.

## Basisverhalten

- Protokoll: HTTP
- Payload-Format: JSON
- Response-Header: `Content-Type: application/json` für JSON-Antworten
- Cache-Header: `Cache-Control: no-store`
- API-Name: `inverse_kinematic`
- API-Version: `v1`
- Projekt-Hostname: `robot`

Der Server ist nach erfolgreicher WLAN-Initialisierung über die IP-Adresse des ESP32 erreichbar. Wenn mDNS erfolgreich gestartet wurde, ist zusätzlich der lokale Hostname `http://robot.local` verfügbar.

## Übersicht

| Methode | Pfad | Zweck | Status Erfolg |
| --- | --- | --- | --- |
| `GET` | `/api/health` | Minimaler Health-Check | `200` |
| `GET` | `/api/status` | Verfügbare API- und Hardware-Fähigkeiten | `200` |
| `GET` | `/api/joint-state` | Aktueller angenommener Gelenkzustand in Grad/Prozent | `200` |
| `POST` | `/api/joint-motion` | Direkten Gelenkzustand setzen | `202` |
| `GET` | `/api/joint-pwm-state` | Aktueller angenommener PWM-Zustand | `200` |
| `POST` | `/api/servo-driver/init` | PCA9685-Servo-Treiber initialisieren und Ausgänge freigeben | `202` |
| `POST` | `/api/joint-pwm-motion` | Direkten PWM-Zustand setzen und an initialisierte Hardware schreiben | `202` |
| `POST` | `/api/motion` | Task-Space-Zielpose über Orchestrator planen und ausführen | `202` |
| `POST` | `/api/sequence/start` | Sequenz aus mehreren Task-Space-Schritten starten | `202` |
| `POST` | `/api/sequence/stop` | Laufende Sequenz und aktiven Motion-Plan abbrechen | `202` |
| `GET` | `/api/sequence/status` | Sequenzfortschritt und aktive Planposition abfragen | `200` |
| `GET` | `/favicon.ico` | Browser-Favicon unterdrücken | `204` |
| alle | sonstige Pfade | Unbekannter Pfad | `404` |

## Ablauf

```mermaid
flowchart TD
  Client[HTTP Client] --> Server[RestApiServer]
  Server --> Health["GET /api/health"]
  Server --> Status["GET /api/status"]
  Server --> JointState["GET /api/joint-state"]
  Server --> JointMotion["POST /api/joint-motion"]
  Server --> PwmState["GET /api/joint-pwm-state"]
  Server --> ServoInit["POST /api/servo-driver/init"]
  Server --> PwmMotion["POST /api/joint-pwm-motion"]
  Server --> Motion["POST /api/motion"]
  Server --> NotFound[Not Found Handler]

  JointMotion --> AssumedJointState[Assumed joint state]
  ServoInit --> Driver[PCA9685 servo driver]
  PwmMotion --> AssumedPwmState[Assumed PWM state]
  PwmMotion --> Driver
```

## Status- und Fehlerwerte

### Capability-Status

| Wert | Bedeutung |
| --- | --- |
| `available` | Funktion ist verfügbar |
| `not_available` | Funktion ist nicht verfügbar |

### API-Result-Codes

| Wert | Bedeutung |
| --- | --- |
| `ok` | Request wurde akzeptiert oder erfolgreich beantwortet |
| `invalid_json` | Request-Body fehlt, ist kein JSON oder kein JSON-Objekt |
| `missing_field` | Pflichtfeld fehlt oder hat den falschen Typ |
| `joint_limit_violation` | Gelenkwert liegt außerhalb der konfigurierten Grenzen |
| `joint_pwm_limit_violation` | PWM-Wert liegt außerhalb `0..4095` |
| `hardware_driver_failure` | PCA9685-Treiber konnte nicht initialisiert werden, ist noch nicht initialisiert oder konnte nicht beschrieben werden |
| `sequence_busy` | Es läuft bereits eine Sequenz oder ein Motion-Plan |
| `orchestrator_unavailable` | Orchestrator-Endpunkt ist reserviert, aber noch nicht implementiert |
| `unknown_route` | Pfad ist nicht registriert |

### Hardware-Driver-Status

| Wert | Bedeutung |
| --- | --- |
| `ok` | Hardware-Operation erfolgreich |
| `driver_begin_failed` | Initialisierung des PCA9685-Treibers fehlgeschlagen |
| `driver_configuration_failed` | PCA9685-Treiberkonfiguration fehlgeschlagen |
| `invalid_channel` | PCA9685-Kanal liegt außerhalb `0..15` |
| `invalid_pwm_value` | PWM-Wert liegt außerhalb `0..4095` |
| `not_initialized` | Treiber wurde vor dem Schreiben nicht initialisiert |

## Datenmodelle

### JointState

Gelenkwerte werden als JSON-Zahlen übertragen. Responses geben Werte mit drei Nachkommastellen aus.

| Feld | Einheit | Minimum | Maximum |
| --- | --- | ---: | ---: |
| `d_deg` | Grad | `-90.0` | `90.0` |
| `s_deg` | Grad | `-90.0` | `90.0` |
| `e_deg` | Grad | `-90.0` | `90.0` |
| `hp_deg` | Grad | `-90.0` | `0.0` |
| `hr_deg` | Grad | `-90.0` | `90.0` |
| `g_pct` | Prozent | `0.0` | `100.0` |

Beispiel:

```json
{
  "d_deg": 0,
  "s_deg": 15.5,
  "e_deg": -20,
  "hp_deg": -45,
  "hr_deg": 0,
  "g_pct": 50
}
```

### JointPwmState

PWM-Werte entsprechen dem 12-Bit-Bereich des PCA9685.

| Feld | Minimum | Maximum |
| --- | ---: | ---: |
| `d_pwm` | `0` | `4095` |
| `s_pwm` | `0` | `4095` |
| `e_pwm` | `0` | `4095` |
| `hp_pwm` | `0` | `4095` |
| `hr_pwm` | `0` | `4095` |
| `g_pwm` | `0` | `4095` |

Beispiel:

```json
{
  "d_pwm": 307,
  "s_pwm": 307,
  "e_pwm": 307,
  "hp_pwm": 307,
  "hr_pwm": 307,
  "g_pwm": 307
}
```

## Endpunkte

Die Beispiele verwenden für Linux und macOS normales `curl` mit dem mDNS-Namen `robot.local`. In WSL funktioniert mDNS je nach Netzwerksetup nicht zuverlässig; die WSL-Beispiele verwenden deshalb den FritzBox-DNS-Namen `robot.fritz.box`. Für PowerShell wird explizit `curl.exe` verwendet, damit nicht versehentlich der PowerShell-Alias `curl` für `Invoke-WebRequest` greift.

Für WSL gibt es zusätzlich ein ausführbares Sammelskript:

```sh
scripts/rest_api_wsl_examples.sh
```

### Health Check

`GET /api/health`

Liefert einen minimalen Health-Check.

#### Ausführen

Linux, macOS:

```sh
curl http://robot.local/api/health
```

WSL:

```sh
curl http://robot.fritz.box/api/health
```

PowerShell:

```powershell
curl.exe http://robot.local/api/health
```

#### Antwort

Response `200`:

```json
{
  "service": "inverse_kinematic",
  "apiVersion": "v1",
  "status": "ok",
  "orchestrator": "available",
  "uptimeMs": 12345
}
```

### Auflisten der HW Fähigkeiten

`GET /api/status`

Liefert die aktuell verfügbaren API- und Hardware-Fähigkeiten.

#### Ausführen

Linux, macOS:

```sh
curl http://robot.local/api/status
```

WSL:

```sh
curl http://robot.fritz.box/api/status
```

PowerShell:

```powershell
curl.exe http://robot.local/api/status
```
#### Antwort

Response `200`:

```json
{
  "restApi": "available",
  "orchestrator": "not_available",
  "jointStateEndpoint": "available",
  "jointMotionEndpoint": "available",
  "jointPwmStateEndpoint": "available",
  "servoDriverInitEndpoint": "available",
  "jointPwmMotionEndpoint": "available",
  "jointPwmHardwareOutput": "available",
  "jointPwmHardwareInitialized": false,
  "motionEndpoint": "available",
  "motionPlanActive": false,
  "motionPlanSampleIndex": 0,
  "motionPlanSampleCount": 0,
  "uptimeMs": 12345
}
```

`jointPwmHardwareOutput` ist `available`, wenn der `RestApiServer` mit einem `Pca9685ServoDriver` konstruiert wurde. Andernfalls ist der Wert `not_available`.
`jointPwmHardwareInitialized` ist `true`, nachdem der PCA9685-Treiber erfolgreich initialisiert wurde. Im normalen Firmwarestart geschieht das direkt beim Boot über `servoDriver.init()`.

### Gelenkzustand auslesen

`GET /api/joint-state`

Liefert den aktuell angenommenen Gelenkzustand. Dieser Zustand wird durch erfolgreiche Requests auf `/api/joint-motion` aktualisiert. Aktuell ist er ein angenommener Low-Level-Zielzustand und kein Sensor-Feedback.

#### Ausführen

Linux, macOS:

```sh
curl http://robot.local/api/joint-state
```

WSL:

```sh
curl http://robot.fritz.box/api/joint-state
```

PowerShell:

```powershell
curl.exe http://robot.local/api/joint-state
```

#### Antwort

Response `200`:

```json
{
  "status": "ok",
  "code": "ok",
  "source": "assumed_low_level_state",
  "jointState": {
    "d_deg": 0.000,
    "s_deg": 0.000,
    "e_deg": 0.000,
    "hp_deg": 0.000,
    "hr_deg": 0.000,
    "g_pct": 0.000
  }
}
```

### Gelenkzustand setzen

`POST /api/joint-motion`

Setzt einen direkten Gelenkzustand. Der Request wird validiert, aber noch nicht an Hardware ausgegeben.

Request:

```json
{
  "d_deg": 0,
  "s_deg": 15,
  "e_deg": -20,
  "hp_deg": -45,
  "hr_deg": 0,
  "g_pct": 50
}
```

#### Ausführen

Linux, macOS:

```sh
curl -X POST http://robot.local/api/joint-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"s_deg":15,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/joint-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"s_deg":15,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/joint-motion -H "Content-Type: application/json" -d '{"d_deg":0,"s_deg":15,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

#### Antworten

Response `202`:

```json
{
  "status": "accepted",
  "code": "ok",
  "mode": "joint_space_direct",
  "hardware": "not_available",
  "jointState": {
    "d_deg": 0.000,
    "s_deg": 15.000,
    "e_deg": -20.000,
    "hp_deg": -45.000,
    "hr_deg": 0.000,
    "g_pct": 50.000
  },
  "message": "Joint state accepted as assumed low-level target; hardware output is not connected yet."
}
```

Response `400`, Beispiel bei fehlendem Feld:

```json
{
  "status": "rejected",
  "code": "missing_field",
  "field": "s_deg",
  "message": "Joint motion request is missing a numeric joint field."
}
```

### PWM Zustand auslesen

`GET /api/joint-pwm-state`

Liefert den aktuellen hardwarenahen PWM-Zustand. Wenn der PCA9685-Treiber initialisiert ist, stammt der Zustand aus dem Treiber und entspricht dem zuletzt vollständig erfolgreich geschriebenen `JointPwmState`. Ohne Hardwaretreiber wird der angenommene Low-Level-Zustand des REST-Servers zurückgegeben.

#### Ausführen

Linux, macOS:

```sh
curl http://robot.local/api/joint-pwm-state
```

WSL:

```sh
curl http://robot.fritz.box/api/joint-pwm-state
```

PowerShell:

```powershell
curl.exe http://robot.local/api/joint-pwm-state
```

#### Antwort

Response `200`:

```json
{
  "status": "ok",
  "code": "ok",
  "source": "hardware_driver_pwm_state",
  "jointPwmState": {
    "d_pwm": 307,
    "s_pwm": 307,
    "e_pwm": 307,
    "hp_pwm": 307,
    "hr_pwm": 307,
    "g_pwm": 307
  }
}
```

### Servo-Treiber initialisieren

`POST /api/servo-driver/init`

Initialisiert den PCA9685-Servo-Treiber für Diagnose oder manuelle Initialisierung, falls der Boot-Init nicht erfolgreich war. Im normalen Firmwarestart wird dieser Schritt bereits beim Boot ausgeführt. Wenn der Treiber schon initialisiert ist, schreibt der Endpoint den initialen `JointPwmState` erneut und fährt damit bewusst wieder zur Init-PWM-Position. Nach erfolgreichem Schreiben setzt der REST-Handler den fachlichen `JointState` auf die Init-Position zurück, liest den zuletzt erfolgreich geschriebenen `JointPwmState` aus dem Treiber zurück und liefert beide Zustände in der Antwort. Separate Enable-/Disable-Endpunkte gibt es bewusst nicht mehr.

#### Ausführen

Linux, macOS:

```sh
curl -X POST http://robot.local/api/servo-driver/init
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/servo-driver/init
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/servo-driver/init
```

#### Antworten

Response `202`:

```json
{
  "status": "accepted",
  "code": "ok",
  "hardware": "available",
  "driver": {
    "status": "ok",
    "message": "ok"
  },
  "jointState": {
    "d_deg": 0,
    "s_deg": 0,
    "e_deg": 0,
    "hp_deg": 0,
    "hr_deg": 0,
    "g_pct": 0
  },
  "jointPwmState": {
    "d_pwm": 310,
    "s_pwm": 290,
    "e_pwm": 290,
    "hp_pwm": 320,
    "hr_pwm": 310,
    "g_pwm": 130
  }
}
```

Response `503`, wenn kein Hardwaretreiber eingebunden ist:

```json
{
  "status": "hardware_not_available",
  "code": "hardware_driver_failure",
  "hardware": "not_available",
  "message": "PCA9685 servo driver is not connected."
}
```

### PWM Zustand setzen

`POST /api/joint-pwm-motion`

Setzt einen direkten PWM-Zustand. Wenn ein `Pca9685ServoDriver` verfügbar ist, muss der Treiber initialisiert sein. Im normalen Firmwarestart geschieht das bereits beim Boot; der zusätzliche Aufruf von `POST /api/servo-driver/init` ist dafür nicht mehr erforderlich. Wenn kein Treiber verfügbar ist, wird der Zustand nur als angenommener Low-Level-Zielzustand gespeichert.

Request:

```json
{
  "d_pwm": 307,
  "s_pwm": 307,
  "e_pwm": 307,
  "hp_pwm": 307,
  "hr_pwm": 307,
  "g_pwm": 307
}
```

### Sequenz starten

`POST /api/sequence/start`

Startet eine geordnete Folge von Schritten. Unterstützt werden zunächst `pose`, `wait` und `led`. Während eine Sequenz läuft, werden neue Einzelbewegungen und weitere Sequenzstarts mit `409` abgelehnt.

Request:

```json
{
  "steps": [
    {
      "type": "pose",
      "name": "pick",
      "x_mm": -20,
      "y_mm": 50,
      "z_mm": 30,
      "p_deg": -90,
      "r_deg": 0,
      "g_pct": 0,
      "wait_ms": 500,
      "motionProfile": {
        "type": "smooth_start_stop",
        "target_velocity_deg_s": 90,
        "sample_time_ms": 10
      }
    },
    {
      "type": "wait",
      "duration_ms": 500
    },
    {
      "type": "led",
      "name": "ready",
      "rgb": {
        "r": 0,
        "g": 120,
        "b": 255
      },
      "mode": "blinking",
      "interval_ms": 500
    }
  ]
}
```

Bei `pose` darf die Zielpose alternativ auch als `targetPose`-Objekt übergeben werden. Fehlt `type`, wird der Schritt aus Kompatibilitätsgründen als `pose` interpretiert.

Beim `led`-Schritt muss genau eine Farbe über `color` oder `rgb` gesetzt sein. Zusätzlich sind `mode` und `interval_ms` Pflichtfelder. `color` und `rgb` dürfen nicht gleichzeitig gesetzt sein. Unterstützte Modi sind `off`, `on`, `blinking` und `pulsing`.

Response `202`:

```json
{
  "status": "accepted",
  "code": "ok",
  "execution": "motion_plan_active",
  "sequence": {
    "status": "motion_active",
    "stepIndex": 0,
    "stepCount": 1,
    "message": "motion_active",
    "lastMotionStatus": "accepted",
    "waitRemainingMs": 0
  }
}
```

### Sequenz stoppen

`POST /api/sequence/stop`

Bricht die Run Engine und einen aktiven Motion-Plan ab. Der intern angenommene Gelenkzustand bleibt der zuletzt vollständig abgeschlossene Zustand.

### Sequenzstatus auslesen

`GET /api/sequence/status`

Liefert Sequenzstatus, aktiven Motion-Plan-Fortschritt sowie die aktuell angenommenen Gelenk- und PWM-Zustände.

#### Ausführen

Linux, macOS:

```sh
curl -X POST http://robot.local/api/joint-pwm-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":307,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/joint-pwm-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":307,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/joint-pwm-motion -H "Content-Type: application/json" -d '{"d_pwm":307,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

```mermaid
sequenceDiagram
  participant Client
  participant REST as RestApiServer
  participant Parser as ApiJson
  participant Driver as PCA9685 Driver

  Client->>REST: POST /api/joint-pwm-motion
  REST->>Parser: parseJointPwmMotionRequestJson(body)
  alt invalid JSON or invalid field
    Parser-->>REST: error
    REST-->>Client: 400 rejected
  else valid request and driver unavailable
    Parser-->>REST: JointPwmState
    REST-->>Client: 202 accepted, hardware not_available
  else valid request and driver available but not initialized
    Parser-->>REST: JointPwmState
    REST-->>Client: 503 hardware_not_initialized
  else valid request and initialized driver available
    Parser-->>REST: JointPwmState
    REST->>Driver: write(state)
    alt hardware failure
      Driver-->>REST: failure
      REST-->>Client: 503 hardware_failed
    else write ok
      Driver-->>REST: ok
      REST-->>Client: 202 accepted, driver ok
    end
  end
```

#### Antworten

Response `202`, ohne Hardware-Treiber:

```json
{
  "status": "accepted",
  "code": "ok",
  "mode": "joint_pwm_direct",
  "hardware": "not_available",
  "jointPwmState": {
    "d_pwm": 307,
    "s_pwm": 307,
    "e_pwm": 307,
    "hp_pwm": 307,
    "hr_pwm": 307,
    "g_pwm": 307
  },
  "message": "Joint PWM state accepted as assumed low-level target; hardware output is not connected yet."
}
```

Response `202`, mit erfolgreichem Hardware-Schreibzugriff:

```json
{
  "status": "accepted",
  "code": "ok",
  "mode": "joint_pwm_direct",
  "hardware": "available",
  "driver": {
    "status": "ok",
    "message": "ok"
  },
  "jointPwmState": {
    "d_pwm": 307,
    "s_pwm": 307,
    "e_pwm": 307,
    "hp_pwm": 307,
    "hr_pwm": 307,
    "g_pwm": 307
  }
}
```

Response `400`, Beispiel bei Grenzwertverletzung:

```json
{
  "status": "rejected",
  "code": "joint_pwm_limit_violation",
  "field": "d_pwm",
  "message": "PWM value is outside the PCA9685 12-bit range 0..4095."
}
```

Response `503`, Beispiel bei noch nicht initialisiertem Hardwaretreiber:

```json
{
  "status": "hardware_not_initialized",
  "code": "hardware_driver_failure",
  "mode": "joint_pwm_direct",
  "hardware": "available",
  "message": "PCA9685 servo driver is not initialized; check the boot log or call POST /api/servo-driver/init for diagnostics."
}
```

Response `503`, Beispiel bei Hardwarefehler:

```json
{
  "status": "hardware_failed",
  "code": "hardware_driver_failure",
  "mode": "joint_pwm_direct",
  "hardware": "available",
  "driver": {
    "status": "invalid_channel",
    "message": "PCA9685 channel is outside 0..15."
  }
}
```

### Orchestrator

`POST /api/motion`

Nimmt eine Zielpose im Weltkoordinatensystem entgegen. Der Endpunkt validiert die Zielpose über den Orchestrator, wendet den `RobotModelOffset` an, berechnet per analytischer IK einen `JointState`, erzeugt einen `MotionPlan` mit `TimedJointState`-Samples und führt diesen bei verfügbarer Hardware nicht-blockierend aus. Optional kann über `motionProfile.type` eines der Profile `constant_velocity`, `constant_acceleration` oder `smooth_start_stop` gewählt werden. Ohne Angabe gilt das Defaultprofil der Firmware.

#### Ausführen

Linux, macOS:

```sh
curl -X POST http://robot.local/api/motion \
  -H "Content-Type: application/json" \
  -d '{"x_mm":-20,"y_mm":50,"z_mm":30,"p_deg":-90,"r_deg":0,"g_pct":50,"motionProfile":{"type":"smooth_start_stop"}}'
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/motion \
  -H "Content-Type: application/json" \
  -d '{"x_mm":-20,"y_mm":50,"z_mm":30,"p_deg":-90,"r_deg":0,"g_pct":50,"motionProfile":{"type":"smooth_start_stop"}}'
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/motion -H "Content-Type: application/json" -d '{"x_mm":-20,"y_mm":50,"z_mm":30,"p_deg":-90,"r_deg":0,"g_pct":50,"motionProfile":{"type":"smooth_start_stop"}}'
```

#### Antwort

Response `202`:

```json
{
  "status": "accepted",
  "code": "ok",
  "mode": "task_space_ik",
  "hardware": "available",
  "execution": "motion_plan_active",
  "targetPose": {
    "x_mm": -20.0,
    "y_mm": 50.0,
    "z_mm": 30.0,
    "p_deg": -90.0,
    "r_deg": 0.0,
    "g_pct": 50.0
  },
  "motionPlan": {
    "profile": "smooth_start_stop",
    "totalDurationMs": 2765,
    "sampleCount": 140,
    "sampleTimeMs": 20,
    "targetVelocityDegS": 30.0
  }
}
```

Wenn keine Hardware angebunden ist, wird derselbe MotionPlan berechnet und in der Antwort zusammengefasst, aber nicht real ausgegeben. Während ein Hardware-MotionPlan aktiv ist, lehnt `/api/motion` weitere Task-Space-Bewegungen mit `409` und Status `busy` ab.

### Unbekannte Pfade

Alle nicht registrierten Pfade liefern `404`.

#### Ausführen

Linux, macOS:

```sh
curl http://robot.local/api/unknown
```

WSL:

```sh
curl http://robot.fritz.box/api/unknown
```

PowerShell:

```powershell
curl.exe http://robot.local/api/unknown
```

#### Antwort

Response `404`:

```json
{
  "status": "not_found",
  "code": "unknown_route",
  "path": "/api/unknown"
}
```

## Prüfung exemplarischer Fehlerfälle

Die folgenden Requests sind absichtlich ungültig. Sie dienen dazu, die Fehlerantworten der API zu prüfen.

### Fehlendes Joint-Feld

Dieser Request lässt `s_deg` aus und soll deshalb `missing_field` für `s_deg` liefern.

Linux, macOS:

```sh
curl -X POST http://robot.local/api/joint-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/joint-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/joint-motion -H "Content-Type: application/json" -d '{"d_deg":0,"e_deg":-20,"hp_deg":-45,"hr_deg":0,"g_pct":50}'
```

Erwartete Response:

```json
{
  "status": "rejected",
  "code": "missing_field",
  "field": "s_deg",
  "message": "Joint motion request is missing a numeric joint field."
}
```

### PWM-Grenzwertverletzung

Dieser Request setzt `d_pwm` auf `4096`. Der gültige PCA9685-Bereich ist `0..4095`, deshalb soll `joint_pwm_limit_violation` für `d_pwm` geliefert werden.

#### Ausführen

Linux, macOS:

```sh
curl -X POST http://robot.local/api/joint-pwm-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":4096,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

WSL:

```sh
curl -X POST http://robot.fritz.box/api/joint-pwm-motion \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":4096,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

PowerShell:

```powershell
curl.exe -X POST http://robot.local/api/joint-pwm-motion -H "Content-Type: application/json" -d '{"d_pwm":4096,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
```

#### Antwort

Response:

```json
{
  "status": "rejected",
  "code": "joint_pwm_limit_violation",
  "field": "d_pwm",
  "message": "PWM value is outside the PCA9685 12-bit range 0..4095."
}
```

## Validierungsregeln

```mermaid
flowchart TD
  Body[Request body] --> Empty{Body vorhanden?}
  Empty -- nein --> InvalidJson[400 invalid_json]
  Empty -- ja --> Parse{Valides JSON?}
  Parse -- nein --> InvalidJson
  Parse -- ja --> Object{JSON object?}
  Object -- nein --> InvalidJson
  Object -- ja --> Fields{Alle Pflichtfelder numerisch?}
  Fields -- nein --> MissingField[400 missing_field]
  Fields -- ja --> Limits{Werte innerhalb der Grenzen?}
  Limits -- nein --> LimitViolation[400 joint_limit_violation oder joint_pwm_limit_violation]
  Limits -- ja --> Accepted[202 accepted]
```

- Request-Bodies müssen JSON-Objekte sein.
- Alle Pflichtfelder müssen vorhanden sein.
- Joint-Werte müssen numerisch und endlich sein.
- PWM-Werte müssen ganzzahlig sein.
- PWM-Werte müssen im Bereich `0..4095` liegen.
- Bei Fehlern enthält die Response nach Möglichkeit das Feld `field` mit dem ersten betroffenen Feldnamen.
