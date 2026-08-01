#!/usr/bin/env bash

set -u

BASE_URL="${BASE_URL:-http://robot.local}"

printf '\n== GET /api/health ==\n'
curl --max-time 5 -i "$BASE_URL/api/health"
printf '\n'

printf '\n== GET /api/status ==\n'
curl --max-time 5 -i "$BASE_URL/api/status"
printf '\n'

printf '\n== GET /api/joint-state ==\n'
curl --max-time 5 -i "$BASE_URL/api/joint-state"
printf '\n'

printf '\n== GET /api/joint-pwm-state ==\n'
curl --max-time 5 -i "$BASE_URL/api/joint-pwm-state"
printf '\n'

printf '\n== POST /api/joint-motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"s_deg":15,"e_deg":-20,"hp_deg":45,"hr_deg":0,"g_pct":50}'
printf '\n'

printf '\n== GET /api/joint-state after POST /api/joint-motion ==\n'
curl --max-time 5 -i "$BASE_URL/api/joint-state"
printf '\n'

printf '\n== POST /api/servo-driver/init ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/servo-driver/init"
printf '\n'

sleep 2

printf '\n== POST /api/joint-pwm-motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":307,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
printf '\n'

sleep 2

printf '\n== GET /api/joint-pwm-state after POST /api/joint-pwm-motion ==\n'
curl --max-time 5 -i "$BASE_URL/api/joint-pwm-state"
printf '\n'

printf '\n== POST /api/motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/motion" \
  -H 'Content-Type: application/json' \
  -d '{"x_mm":-20,"y_mm":40,"z_mm":60,"p_deg":-85,"r_deg":0,"g_pct":50,"motionProfile":{"type":"smooth_start_stop","target_velocity_deg_s":90,"sample_time_ms":10}}'
printf '\n'

sleep 2

printf '\n== POST /api/sequence/stop after POST /api/motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/sequence/stop"
printf '\n'

printf '\n== POST /api/sequence/start ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/sequence/start" \
  -H 'Content-Type: application/json' \
  -d '{"steps":[{"type":"led","color":"yellow","mode":"on","interval_ms":500},{"type":"pose","name":"home","targetPose":{"x_mm":-20,"y_mm":40,"z_mm":50,"p_deg":-85,"r_deg":0,"g_pct":0},"motionProfile":{"type":"smooth_start_stop","target_velocity_deg_s":30,"sample_time_ms":10}},{"type":"wait","duration_ms":500},{"type":"led","color":"green","mode":"pulsing","interval_ms":500}]}'
printf '\n'

printf '\n== GET /api/sequence/status ==\n'
curl --max-time 5 -i "$BASE_URL/api/sequence/status"
printf '\n'

printf '\n== POST /api/sequence/stop ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/sequence/stop"
printf '\n'

printf '\n== GET /api/unknown ==\n'
curl --max-time 5 -i "$BASE_URL/api/unknown"
printf '\n'

printf '\n== POST /api/joint-motion with missing s_deg ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"e_deg":-20,"hp_deg":45,"hr_deg":0,"g_pct":50}'
printf '\n'

printf '\n== POST /api/joint-pwm-motion with d_pwm outside 0..4095 ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":4096,"s_pwm":307,"e_pwm":307,"hp_pwm":307,"hr_pwm":307,"g_pwm":307}'
printf '\n'

printf '\n== POST /api/sequence/start ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/sequence/start" \
  -H 'Content-Type: application/json' \
  -d '{"steps":[{"type":"led","color":"yellow","mode":"on","interval_ms":500},{"type":"pose","name":"home","targetPose":{"x_mm":-20,"y_mm":40,"z_mm":50,"p_deg":-85,"r_deg":0,"g_pct":0},"motionProfile":{"type":"smooth_start_stop","target_velocity_deg_s":30,"sample_time_ms":10}},{"type":"wait","duration_ms":500},{"type":"led","color":"green","mode":"blinking","interval_ms":500}]}'
printf '\n'
