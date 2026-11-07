#!/usr/bin/env bash
# End to end: Renode node -> Rust gateway -> Mosquitto (TLS) -> mosquitto_sub. Needs: mosquitto, mosquitto-clients, renode.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
"$ROOT/scripts/make_certs.sh" "$ROOT/build/certs" >/dev/null
sed "s#CERT_DIR#$ROOT/build/certs#" mosquitto/mosquitto.conf.in > build/mosquitto.conf
mosquitto -c build/mosquitto.conf > build/mosquitto.log 2>&1 &
BROKER=$!
# Renode's console quits on end-of-input, so keep its stdin open; run it in its own process group
setsid bash -c "tail -f /dev/null | renode --disable-xwt --console --plain -e '\$boot=@$ROOT/build/bootloader; \$image=@$ROOT/build/firmware_signed.bin; \$log=@$ROOT/build/uart1.log; \$sensor=@$ROOT/sim/si7021.repl; include @$ROOT/sim/serve.resc'" >/dev/null 2>&1 &
RENODE=$!
trap 'kill $BROKER 2>/dev/null || true; kill -- -$RENODE 2>/dev/null || true' EXIT
for _ in $(seq 1 60); do (echo > /dev/tcp/127.0.0.1/3456) 2>/dev/null && break; sleep 0.5; done
sleep 2
mosquitto_sub -h localhost -p 8883 --cafile build/certs/ca.crt -t 'nodes/node1/telemetry' -C 2 -W 20 > build/mqtt_received.txt &
SUB=$!
sleep 1
gateway/target/release/gateway --count 3 --interval-ms 400 --mqtt localhost:8883 --ca build/certs/ca.crt > /dev/null
wait $SUB
cat build/mqtt_received.txt
test "$(wc -l < build/mqtt_received.txt)" -ge 2
echo "mqtt_tls_test: OK"
