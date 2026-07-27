#!/usr/bin/env bash
set -euo pipefail

ENVIRONMENT="${1:-esp32s3}"
MAP_FILE=".pio/build/${ENVIRONMENT}/firmware.map"

if [[ ! -f "${MAP_FILE}" ]]; then
  echo "Map file not found: ${MAP_FILE}" >&2
  echo "Run first: ~/.platformio/penv/bin/pio run -e ${ENVIRONMENT}" >&2
  exit 1
fi

~/.platformio/penv/bin/python -m esp_idf_size "${MAP_FILE}"
