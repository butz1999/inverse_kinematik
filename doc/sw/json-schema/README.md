# REST API JSON Schemas

```text
doc/sw/json-schema/
├── README.md
└── openapi-v1.json
```

`openapi-v1.json` is the machine-readable contract for the current REST API.
It uses OpenAPI 3.1, whose schema objects follow JSON Schema Draft 2020-12.
It contains every registered API route, all request payloads, and the reusable
response objects used by the firmware.

The file is intentionally a local documentation artifact. It is not served by
the ESP32 and does not add a runtime dependency. Validate it with a compatible
OpenAPI or JSON Schema validator when changing the REST handlers.

The source of truth for implementation behavior remains
`src/application/RestApiServer.cpp` and `src/application/ApiJson.cpp`.
