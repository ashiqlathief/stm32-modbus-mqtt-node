#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "modbus_core.h"

static size_t with_crc(uint8_t *buf, size_t n)
{
    uint16_t crc = modbus_crc16(buf, n);
    buf[n++] = (uint8_t)(crc & 0xFF);
    buf[n++] = (uint8_t)(crc >> 8);
    return n;
}

int main(void)
{
    /* Known CRC from the Modbus spec example: 01 03 00 00 00 01 -> 84 0A */
    const uint8_t spec[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01 };
    assert(modbus_crc16(spec, sizeof spec) == 0x0A84);

    const uint16_t regs[3] = { 215, 42, 7 };
    uint8_t req[16], resp[MODBUS_MAX_FRAME];

    /* read 2 registers starting at 0 */
    memcpy(req, (uint8_t[]){ 1, 3, 0, 0, 0, 2 }, 6);
    size_t n = modbus_handle_request(req, with_crc(req, 6), 1, regs, 3, resp);
    assert(n == 9);
    assert(resp[0] == 1 && resp[1] == 3 && resp[2] == 4);
    assert(resp[3] == 0 && resp[4] == 215 && resp[5] == 0 && resp[6] == 42);
    assert(modbus_crc16(resp, n - 2) == (uint16_t)(resp[n - 2] | (resp[n - 1] << 8)));

    /* bad CRC: no reply */
    memcpy(req, (uint8_t[]){ 1, 3, 0, 0, 0, 1 }, 6);
    size_t rl = with_crc(req, 6);
    req[rl - 1] ^= 0xFF;
    assert(modbus_handle_request(req, rl, 1, regs, 3, resp) == 0);

    /* wrong slave address: no reply */
    memcpy(req, (uint8_t[]){ 2, 3, 0, 0, 0, 1 }, 6);
    assert(modbus_handle_request(req, with_crc(req, 6), 1, regs, 3, resp) == 0);

    /* unsupported function -> exception 01 */
    memcpy(req, (uint8_t[]){ 1, 6, 0, 0, 0, 1 }, 6);
    n = modbus_handle_request(req, with_crc(req, 6), 1, regs, 3, resp);
    assert(n == 5 && resp[1] == 0x86 && resp[2] == 0x01);

    /* address out of range -> exception 02 */
    memcpy(req, (uint8_t[]){ 1, 3, 0, 2, 0, 2 }, 6);
    n = modbus_handle_request(req, with_crc(req, 6), 1, regs, 3, resp);
    assert(n == 5 && resp[1] == 0x83 && resp[2] == 0x02);

    /* zero count -> exception 03 */
    memcpy(req, (uint8_t[]){ 1, 3, 0, 0, 0, 0 }, 6);
    n = modbus_handle_request(req, with_crc(req, 6), 1, regs, 3, resp);
    assert(n == 5 && resp[1] == 0x83 && resp[2] == 0x03);

    puts("modbus_core: all tests passed");
    return 0;
}
