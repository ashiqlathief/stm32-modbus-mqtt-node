#!/usr/bin/env python3
"""Start the firmware in Renode, read the holding registers over Modbus RTU (TCP bridge to USART2), check the reply."""
import os, socket, struct, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT = 3456


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def request(sock, addr, func, start, count):
    pdu = struct.pack(">BBHH", addr, func, start, count)
    sock.sendall(pdu + struct.pack("<H", crc16(pdu)))
    sock.settimeout(3)
    buf = b""
    while True:
        try:
            chunk = sock.recv(256)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
        if len(buf) >= 5 and (buf[1] & 0x80 or len(buf) >= 3 + buf[2] + 2):
            break
    return buf


def main():
    boot = os.path.join(ROOT, "build", "bootloader")
    image = os.path.join(ROOT, "build", "firmware_signed.bin")
    log = os.path.join(ROOT, "build", "uart1.log")
    script = os.path.join(ROOT, "sim", "serve.resc")
    renode = subprocess.Popen(
        ["renode", "--disable-xwt", "--console", "--plain",
         "-e", f"$boot=@{boot}; $image=@{image}; $log=@{log}; $sensor=@{os.path.join(ROOT, 'sim', 'si7021.repl')}; include @{script}"],
        stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)  # open stdin: Renode quits on EOF
    try:
        sock = None
        for _ in range(60):
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1)
                break
            except OSError:
                time.sleep(0.5)
        assert sock, "could not connect to the Renode UART socket"
        time.sleep(1.0)  # let the firmware boot and take a few samples

        reply = request(sock, 1, 3, 0, 4)
        assert len(reply) == 3 + 8 + 2, f"unexpected reply length {len(reply)}: {reply.hex()}"
        assert crc16(reply[:-2]) == struct.unpack("<H", reply[-2:])[0], "bad CRC in reply"
        assert reply[0] == 1 and reply[1] == 3 and reply[2] == 8
        value, count, uptime, errors = struct.unpack(">HHHH", reply[3:11])
        print(f"temp_x10={value} samples={count} uptime_s={uptime} i2c_errors={errors}")
        assert 220 <= value <= 230, "temperature differs from the simulated 22.5 C"
        assert errors == 0, "I2C errors reported"
        assert count > 0

        err = request(sock, 1, 6, 0, 1)   # unsupported function
        assert err[1] == 0x86 and err[2] == 0x01, f"expected exception reply, got {err.hex()}"
        print("sim_modbus_test: OK")
    finally:
        renode.terminate()
        try:
            renode.wait(timeout=10)
        except subprocess.TimeoutExpired:
            renode.kill()


if __name__ == "__main__":
    sys.exit(main())
