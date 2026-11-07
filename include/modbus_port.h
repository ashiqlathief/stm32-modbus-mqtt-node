#pragma once
#include <stdint.h>

/* Registers served over Modbus (holding registers, read-only):
 *   0: temperature in 0.1 C   1: sample counter   2: uptime in seconds   3: I2C error count */
void modbus_port_init(void);
void modbus_port_publish(uint16_t value, uint16_t sample_count, uint16_t i2c_errors);
void modbus_port_publish_errors(uint16_t i2c_errors);
