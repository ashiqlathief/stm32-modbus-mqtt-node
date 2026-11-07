#pragma once
#include <stdint.h>

void i2c_init(void);

/* Send one command byte, then read `len` bytes (repeated start). Returns 0 on success, negative on error:
 * -1 timeout, -2 no acknowledge. */
int i2c_command_read(uint8_t addr7, uint8_t cmd, uint8_t *buf, uint8_t len);
