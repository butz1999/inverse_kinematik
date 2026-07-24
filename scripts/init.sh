#!/usr/bin/env bash

set -u

BASE_URL="${BASE_URL:-http://robot.fritz.box}"

printf '\n== POST /api/servo-driver/init ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/servo-driver/init"
printf '\n'

printf '\n== POST /api/joint-pwm-motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-pwm-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_pwm":310,"s_pwm":290,"e_pwm":290,"hp_pwm":320,"hr_pwm":310,"g_pwm":130}'
printf '\n'

printf '\n== wait 2s ==\n'
sleep 2

printf '\n== POST /api/joint-motion ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/joint-motion" \
  -H 'Content-Type: application/json' \
  -d '{"d_deg":0,"s_deg":45,"e_deg":-45,"hp_deg":-45,"hr_deg":45,"g_pct":50}'
printf '\n'

printf '\n== wait 2s ==\n'
sleep 2

printf '\n== POST /api/sequence/start init pose ==\n'
curl --max-time 5 -i \
  -X POST "$BASE_URL/api/sequence/start" \
  -H 'Content-Type: application/json' \
  -d '{"steps":[{"type":"pose","name":"Init pose","targetPose":{"x_mm":-20,"y_mm":40,"z_mm":60,"p_deg":-85,"r_deg":0,"g_pct":0}}]}'
printf '\n'

