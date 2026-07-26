#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_FILE="${REPO_ROOT}/sbom.cyclonedx.json"

if ! command -v syft >/dev/null 2>&1; then
  echo "Error: syft is not installed or not available in PATH." >&2
  exit 1
fi

cd "${REPO_ROOT}"

syft dir:. \
  --source-name ik \
  --source-version local \
  -o "cyclonedx-json=${OUTPUT_FILE}"

if command -v node >/dev/null 2>&1; then
  node -e "const fs=require('fs'); const path=process.argv[1]; const bom=JSON.parse(fs.readFileSync(path,'utf8')); fs.writeFileSync(path, JSON.stringify(bom, null, 2) + '\n');" "${OUTPUT_FILE}"
elif command -v python3 >/dev/null 2>&1; then
  python3 -m json.tool "${OUTPUT_FILE}" "${OUTPUT_FILE}.tmp"
  mv "${OUTPUT_FILE}.tmp" "${OUTPUT_FILE}"
else
  echo "Warning: neither node nor python3 found; SBOM was generated but not pretty-printed." >&2
fi

echo "SBOM written to ${OUTPUT_FILE}"
