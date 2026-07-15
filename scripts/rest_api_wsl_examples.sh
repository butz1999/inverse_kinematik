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

request "GET /api/health" \
  "$BASE_URL/api/health"

request "GET /api/status" \
  "$BASE_URL/api/status"

request "GET /api/joint-state" \
  "$BASE_URL/api/joint-state"

request "POST /api/joint-motion" \
  -X POST "$BASE_URL/api/joint-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"s_deg":15,"e_deg":-20,"hp_deg":45,"hr_deg":0,"g_pct":50}'

request "GET /api/joint-state after POST /api/joint-motion" \
  "$BASE_URL/api/joint-state"

request "GET /api/joint-pwm-state" \
  "$BASE_URL/api/joint-pwm-state"

request "POST /api/joint-pwm-motion" \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":1500,"s_pwm":1500,"e_pwm":1500,"hp_pwm":1500,"hr_pwm":1500,"g_pwm":1500}'

request "GET /api/joint-pwm-state after POST /api/joint-pwm-motion" \
  "$BASE_URL/api/joint-pwm-state"

request "POST /api/motion" \
  -X POST "$BASE_URL/api/motion"

request "GET /api/unknown" \
  "$BASE_URL/api/unknown"

request "POST /api/joint-motion with missing s_deg" \
  -X POST "$BASE_URL/api/joint-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"e_deg":-20,"hp_deg":45,"hr_deg":0,"g_pct":50}'

request "POST /api/joint-pwm-motion with d_pwm outside 0..4095" \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":4096,"s_pwm":1500,"e_pwm":1500,"hp_pwm":1500,"hr_pwm":1500,"g_pwm":1500}'
