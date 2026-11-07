#!/usr/bin/env bash
# Create a throw-away CA and a broker certificate for local TLS tests (never use these outside the simulation).
set -euo pipefail
out="${1:-$(dirname "$0")/../build/certs}"
mkdir -p "$out" && cd "$out"
openssl req -x509 -newkey rsa:2048 -nodes -days 30 -subj "/CN=sim-ca" -keyout ca.key -out ca.crt 2>/dev/null
openssl req -newkey rsa:2048 -nodes -subj "/CN=localhost" -keyout server.key -out server.csr 2>/dev/null
printf "subjectAltName=DNS:localhost,IP:127.0.0.1\n" > san.ext
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -days 30 -extfile san.ext -out server.crt 2>/dev/null
echo "certificates written to $out"
