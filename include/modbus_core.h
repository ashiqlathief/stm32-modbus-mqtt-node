#pragma once
#include <stddef.h>
#include <stdint.h>

#define MODBUS_MAX_FRAME 256

uint16_t modbus_crc16(const uint8_t *data, size_t len);

/* Handle one Modbus RTU request (address, function, data, CRC) against a read-only holding register map.
 * Supports function 0x03 (read holding registers). Returns the response length written to resp,
 * or 0 if no reply must be sent (bad CRC, wrong slave address). */
size_t modbus_handle_request(const uint8_t *req, size_t len, uint8_t slave_addr,
                             const uint16_t *regs, size_t nregs, uint8_t *resp);
