#include "modbus_core.h"

uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
    }
    return crc;
}

static size_t finish(uint8_t *resp, size_t n)
{
    uint16_t crc = modbus_crc16(resp, n);
    resp[n++] = (uint8_t)(crc & 0xFFu);   /* CRC is sent low byte first */
    resp[n++] = (uint8_t)(crc >> 8);
    return n;
}

static size_t exception(uint8_t *resp, uint8_t addr, uint8_t func, uint8_t code)
{
    resp[0] = addr;
    resp[1] = (uint8_t)(func | 0x80u);
    resp[2] = code;
    return finish(resp, 3);
}

size_t modbus_handle_request(const uint8_t *req, size_t len, uint8_t slave_addr,
                             const uint16_t *regs, size_t nregs, uint8_t *resp)
{
    if (len < 4) return 0;
    uint16_t rx_crc = (uint16_t)(req[len - 2] | (req[len - 1] << 8));
    if (modbus_crc16(req, len - 2) != rx_crc) return 0;
    if (req[0] != slave_addr) return 0;

    uint8_t func = req[1];
    if (func != 0x03) return exception(resp, slave_addr, func, 0x01);   /* illegal function */
    if (len != 8) return exception(resp, slave_addr, func, 0x03);       /* illegal data value */

    uint16_t start = (uint16_t)((req[2] << 8) | req[3]);
    uint16_t count = (uint16_t)((req[4] << 8) | req[5]);
    if (count < 1 || count > 125) return exception(resp, slave_addr, func, 0x03);
    if ((size_t)start + count > nregs) return exception(resp, slave_addr, func, 0x02); /* illegal data address */

    resp[0] = slave_addr;
    resp[1] = func;
    resp[2] = (uint8_t)(count * 2u);
    size_t n = 3;
    for (uint16_t i = 0; i < count; i++) {
        resp[n++] = (uint8_t)(regs[start + i] >> 8);
        resp[n++] = (uint8_t)(regs[start + i] & 0xFFu);
    }
    return finish(resp, n);
}
