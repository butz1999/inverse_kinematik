#!/usr/bin/env bash

set -u

BASE_URL="${BASE_URL:-http://robot.fritz.box}"
CURL_TIMEOUT_SECONDS="${CURL_TIMEOUT_SECONDS:-5}"

request() {
  local title="$1"
  shift

  printf '\n== %s ==\n' "$title"
  curl --max-time "$CURL_TIMEOUT_SECONDS" -i "$@"
  printf '\n'
}

request "POST /api/servo-driver/init" \
  -X POST "$BASE_URL/api/servo-driver/init"

request "POST /api/joint-pwm-motion" \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":207,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
