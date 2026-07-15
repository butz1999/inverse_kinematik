#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required but was not found in PATH." >&2
  exit 1
fi

git ls-files '*.cpp' '*.h' | xargs -r clang-format -i
