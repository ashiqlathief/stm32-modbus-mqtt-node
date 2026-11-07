#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "regs.h"
#include "uart.h"
#include "modbus_port.h"
#include "i2c.h"

typedef struct {
    uint32_t tick;
    int32_t  value;
} sample_t;

static QueueHandle_t sample_queue;

#define SI7021_ADDR      0x40u
#define SI7021_MEASURE_T 0xE3u   /* measure temperature, hold master mode */

/* Temperature in tenths of a degree Celsius; returns 0 on success. */
static int read_temperature_x10(int16_t *out)
{
    uint8_t raw[2];
    int r = i2c_command_read(SI7021_ADDR, SI7021_MEASURE_T, raw, 2);
    if (r != 0) return r;
    uint32_t code = ((uint32_t)raw[0] << 8) | raw[1];
    int32_t centi = (int32_t)((17572u * code) / 65536u) - 4685;   /* datasheet: 175.72 * code / 65536 - 46.85 */
    *out = (int16_t)((centi >= 0 ? centi + 5 : centi - 5) / 10);   /* round to 0.1 C */
    return 0;
}

static void sampler_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    uint16_t errors = 0;
    for (uint32_t n = 0;; n++) {
        int16_t t = 0;
        if (read_temperature_x10(&t) != 0) {
            errors++;                      /* keep the last good value, count the failure */
            vTaskDelayUntil(&last, pdMS_TO_TICKS(100));
            modbus_port_publish_errors(errors);
            continue;
        }
        sample_t s = { .tick = (uint32_t)xTaskGetTickCount(), .value = t };
        xQueueSend(sample_queue, &s, 0);
        modbus_port_publish((uint16_t)t, (uint16_t)n, errors);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(100));
    }
}

static void logger_task(void *arg)
{
    (void)arg;
    sample_t s;
    for (;;) {
        if (xQueueReceive(sample_queue, &s, portMAX_DELAY) == pdTRUE) {
            uart_puts("t=");
            uart_put_u32(s.tick);
            uart_puts(" temp_x10=");
            uart_put_u32((uint32_t)s.value);
            uart_puts("\r\n");
        }
    }
}

static void heartbeat_task(void *arg)
{
    (void)arg;
    for (;;) {
        GPIOD_ODR ^= (1u << 12);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    uart_init();
    RCC_AHB1ENR |= (1u << 3);                               /* GPIOD clock */
    GPIOD_MODER = (GPIOD_MODER & ~(3u << 24)) | (1u << 24); /* PD12 output (green LED) */

    uart_puts("app v");
    uart_put_u32(APP_VERSION);
    uart_puts(" running\r\n");

    i2c_init();
    modbus_port_init();
    sample_queue = xQueueCreate(8, sizeof(sample_t));
    configASSERT(sample_queue != NULL);
    xTaskCreate(sampler_task,   "sampler",   256, NULL, 3, NULL);
    xTaskCreate(logger_task,    "logger",    256, NULL, 2, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 128, NULL, 1, NULL);
    vTaskStartScheduler();

    for (;;) { }
}
