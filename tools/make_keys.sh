#!/usr/bin/env bash
# Create an Ed25519 signing key pair for the simulated firmware (keys/ is not committed).
# The bootloader gets only the public key (build/generated/pubkey.h).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT/keys" "$ROOT/build/generated"
if [ ! -f "$ROOT/keys/private.pem" ]; then
    openssl genpkey -algorithm ED25519 -out "$ROOT/keys/private.pem"
    chmod 600 "$ROOT/keys/private.pem"
    echo "created keys/private.pem (keep it secret; never commit it)"
fi
openssl pkey -in "$ROOT/keys/private.pem" -pubout -outform DER | tail -c 32 > "$ROOT/keys/public.raw"
python3 "$ROOT/tools/gen_pubkey_header.py" "$ROOT/keys/public.raw" "$ROOT/build/generated/pubkey.h"
