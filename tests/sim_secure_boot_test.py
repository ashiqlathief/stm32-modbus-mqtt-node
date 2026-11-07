#!/usr/bin/env python3
"""Secure-boot tests in Renode: the bootloader must start only correctly signed, new-enough images.

Scenarios: valid image, tampered application, tampered version field, image signed with another key,
rollback to an old version, blank flash, and an update to a newer signed version.
"""
import os, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
WORK = os.path.join(BUILD, "boot_tests")
SIGN = os.path.join(ROOT, "tools", "sign_image.py")
KEY = os.path.join(ROOT, "keys", "private.pem")
RUN_SECONDS = 6


def run_node(image: str) -> str:
    """Boot the simulated STM32 with the bootloader and the given image in flash; return the debug log."""
    log = os.path.join(WORK, "uart1.log")
    if os.path.exists(log):
        os.remove(log)
    cmd = (f"$boot=@{os.path.join(BUILD, 'bootloader')}; $image=@{image}; $log=@{log}; "
           f"$sensor=@{os.path.join(ROOT, 'sim', 'si7021.repl')}; include @{os.path.join(ROOT, 'sim', 'serve.resc')}; "
           f"sleep {RUN_SECONDS}; quit")
    subprocess.run(["renode", "--disable-xwt", "--console", "--plain", "-e", cmd],
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=120)
    return open(log).read() if os.path.exists(log) else ""


def sign(app_bin: str, version: int, key: str, out: str) -> str:
    subprocess.run([sys.executable, SIGN, "--key", key, "--version", str(version), "--in", app_bin, "--out", out],
                   check=True, capture_output=True)
    return out


def expect(name, log, must, must_not=()):
    ok = all(m in log for m in must) and not any(m in log for m in must_not)
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
    if not ok:
        print("       log was:\n       " + "\n       ".join(log.splitlines()[:6]))
    return ok


def main():
    os.makedirs(WORK, exist_ok=True)
    app_bin = os.path.join(BUILD, "firmware.bin")
    valid = os.path.join(BUILD, "firmware_signed.bin")
    results = []

    results.append(expect("valid image boots", run_node(valid),
                          ["image verified, version 1", "app v1 running", "temp_x10=225"], ["REFUSED"]))

    img = bytearray(open(valid, "rb").read())
    img[256 + 100] ^= 0x01                         # flip one bit in the application
    p = os.path.join(WORK, "tampered_app.bin"); open(p, "wb").write(img)
    results.append(expect("tampered application is refused", run_node(p),
                          ["REFUSED, bad signature"], ["app v", "starting app"]))

    img = bytearray(open(valid, "rb").read())
    img[4:8] = struct.pack("<I", 9)                # claim version 9 without re-signing
    p = os.path.join(WORK, "tampered_version.bin"); open(p, "wb").write(img)
    results.append(expect("edited version field is refused", run_node(p),
                          ["REFUSED, bad signature"], ["app v", "starting app"]))

    with tempfile.TemporaryDirectory() as d:
        other = os.path.join(d, "other.pem")
        subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", other], check=True, capture_output=True)
        p = sign(app_bin, 1, other, os.path.join(WORK, "wrong_key.bin"))
    results.append(expect("image signed with another key is refused", run_node(p),
                          ["REFUSED, bad signature"], ["app v", "starting app"]))

    p = sign(app_bin, 0, KEY, os.path.join(WORK, "rollback_v0.bin"))
    results.append(expect("old version is refused (rollback)", run_node(p),
                          ["REFUSED, version too old"], ["app v", "starting app"]))

    p = os.path.join(WORK, "blank.bin"); open(p, "wb").write(b"\x00" * 256)
    results.append(expect("blank flash is refused", run_node(p),
                          ["REFUSED, no image"], ["app v", "starting app"]))

    # update: build the application again as version 2 and boot that image
    v2_dir = os.path.join(BUILD, "v2")
    subprocess.run(["cmake", "-S", ROOT, "-B", v2_dir, "-DAPP_VERSION=2",
                    f"-DCMAKE_TOOLCHAIN_FILE={os.path.join(ROOT, 'cmake', 'arm-none-eabi.cmake')}",
                    "-DCMAKE_BUILD_TYPE=Release"], check=True, capture_output=True)
    subprocess.run(["cmake", "--build", v2_dir, "--target", "firmware"], check=True, capture_output=True)
    results.append(expect("signed update to version 2 boots", run_node(os.path.join(v2_dir, "firmware_signed.bin")),
                          ["image verified, version 2", "app v2 running"], ["REFUSED"]))

    failed = results.count(False)
    print(f"sim_secure_boot_test: {'OK' if not failed else str(failed) + ' FAILED'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
