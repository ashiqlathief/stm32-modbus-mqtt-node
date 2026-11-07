#!/usr/bin/env python3
"""Start the firmware in Renode, run the Rust gateway against it in stdout mode and check the JSON lines."""
import json, os, socket, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    boot = os.path.join(ROOT, "build", "bootloader")
    image = os.path.join(ROOT, "build", "firmware_signed.bin")
    log = os.path.join(ROOT, "build", "uart1.log")
    script = os.path.join(ROOT, "sim", "serve.resc")
    gateway = os.path.join(ROOT, "gateway", "target", "release", "gateway")
    renode = subprocess.Popen(
        ["renode", "--disable-xwt", "--console", "--plain",
         "-e", f"$boot=@{boot}; $image=@{image}; $log=@{log}; $sensor=@{os.path.join(ROOT, 'sim', 'si7021.repl')}; include @{script}"],
        stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)  # open stdin: Renode quits on EOF
    try:
        for _ in range(60):
            try:
                socket.create_connection(("127.0.0.1", 3456), timeout=1).close()
                break
            except OSError:
                time.sleep(0.5)
        else:
            raise SystemExit("Renode UART socket never came up")
        time.sleep(1.5)
        out = subprocess.run([gateway, "--count", "3", "--interval-ms", "300"],
                             capture_output=True, text=True, timeout=60)
        lines = [json.loads(l) for l in out.stdout.splitlines() if l.startswith("{")]
        assert len(lines) == 3, f"expected 3 JSON lines, got {out.stdout!r} / {out.stderr!r}"
        assert all(22.0 <= l["temperature_c"] <= 23.0 for l in lines), lines
        assert all(l["i2c_errors"] == 0 for l in lines), lines
        assert lines[-1]["samples"] > lines[0]["samples"], "sample counter did not advance"
        print(lines)
        print("sim_gateway_test: OK")
    finally:
        renode.terminate()
        try:
            renode.wait(timeout=10)
        except subprocess.TimeoutExpired:
            renode.kill()


if __name__ == "__main__":
    sys.exit(main())
