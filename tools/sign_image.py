#!/usr/bin/env python3
"""Wrap an application binary in the signed image format used by the bootloader.

  sign_image.py --key keys/private.pem --version 2 --in firmware.bin --out firmware_signed.bin

Layout: 256-byte header (magic, fw_version, image_size, reserved, Ed25519 signature, padding) + application.
The signature covers SHA-256(header bytes 0..11 || application).
"""
import argparse, hashlib, os, struct, subprocess, sys, tempfile

MAGIC = 0x4D495746  # "FWIM"
HEADER_SIZE = 256


def sign_digest(key_path: str, digest: bytes) -> bytes:
    with tempfile.TemporaryDirectory() as d:
        msg = os.path.join(d, "digest.bin")
        open(msg, "wb").write(digest)
        sig = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", key_path, "-rawin", "-in", msg],
                             check=True, capture_output=True).stdout
    assert len(sig) == 64, "unexpected Ed25519 signature length"
    return sig


def build_image(app: bytes, version: int, key_path: str) -> bytes:
    head12 = struct.pack("<III", MAGIC, version, len(app))
    digest = hashlib.sha256(head12 + app).digest()
    sig = sign_digest(key_path, digest)
    header = head12 + struct.pack("<I", 0) + sig
    header += b"\x00" * (HEADER_SIZE - len(header))
    return header + app


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--key", required=True)
    ap.add_argument("--version", type=int, required=True)
    ap.add_argument("--in", dest="inp", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    app = open(a.inp, "rb").read()
    open(a.out, "wb").write(build_image(app, a.version, a.key))
    print(f"signed image: version {a.version}, {len(app)} application bytes -> {a.out}")


if __name__ == "__main__":
    sys.exit(main())
