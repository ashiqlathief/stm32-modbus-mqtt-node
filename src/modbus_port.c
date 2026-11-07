#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "modbus_core.h"
#include "modbus_port.h"
#include "regs.h"

#define MODBUS_SLAVE_ADDR  1u
#define FRAME_GAP_MS       5u     /* silence that ends a frame (simplified 3.5 character times) */
#define NREGS              4u

#define SR_RXNE  (1u << 5)
#define SR_TXE   (1u << 7)
#define CR1_RE   (1u << 2)
#define CR1_TE   (1u << 3)
#define CR1_RXNEIE (1u << 5)
#define CR1_UE   (1u << 13)

static QueueHandle_t rx_queue;
static uint16_t regs[NREGS];

void modbus_port_publish(uint16_t value, uint16_t sample_count, uint16_t i2c_errors)
{
    taskENTER_CRITICAL();
    regs[0] = value;
    regs[1] = sample_count;
    regs[3] = i2c_errors;
    taskEXIT_CRITICAL();
}

void modbus_port_publish_errors(uint16_t i2c_errors)
{
    taskENTER_CRITICAL();
    regs[3] = i2c_errors;
    taskEXIT_CRITICAL();
}

void USART2_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    if (USART2_SR & SR_RXNE) {
        uint8_t b = (uint8_t)USART2_DR;
        xQueueSendFromISR(rx_queue, &b, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

static void tx_bytes(const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        while (!(USART2_SR & SR_TXE)) { }
        USART2_DR = buf[i];
    }
}

static void modbus_task(void *arg)
{
    (void)arg;
    uint8_t frame[MODBUS_MAX_FRAME], resp[MODBUS_MAX_FRAME];
    size_t len = 0;
    for (;;) {
        uint8_t b;
        /* first byte: wait forever; following bytes: end of frame after the gap */
        TickType_t wait = len ? pdMS_TO_TICKS(FRAME_GAP_MS) : portMAX_DELAY;
        if (xQueueReceive(rx_queue, &b, wait) == pdTRUE) {
            if (len < sizeof frame) frame[len++] = b;
            continue;
        }
        uint16_t snapshot[NREGS];
        taskENTER_CRITICAL();
        regs[2] = (uint16_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
        for (size_t i = 0; i < NREGS; i++) snapshot[i] = regs[i];
        taskEXIT_CRITICAL();
        size_t n = modbus_handle_request(frame, len, MODBUS_SLAVE_ADDR, snapshot, NREGS, resp);
        if (n) tx_bytes(resp, n);
        len = 0;
    }
}

void modbus_port_init(void)
{
    rx_queue = xQueueCreate(64, sizeof(uint8_t));
    configASSERT(rx_queue != NULL);

    RCC_AHB1ENR |= (1u << 0);                                /* GPIOA clock */
    RCC_APB1ENR |= (1u << 17);                               /* USART2 clock */
    GPIOA_MODER = (GPIOA_MODER & ~((3u << 4) | (3u << 6))) | (2u << 4) | (2u << 6); /* PA2/PA3 alternate */
    GPIOA_AFRL  = (GPIOA_AFRL & ~((0xFu << 8) | (0xFu << 12))) | (7u << 8) | (7u << 12); /* AF7 */
    USART2_BRR = 16000000u / 115200u;
    USART2_CR1 = CR1_UE | CR1_TE | CR1_RE | CR1_RXNEIE;

    NVIC_IPR(USART2_IRQn) = (uint8_t)(6u << 4);              /* below configMAX_SYSCALL_INTERRUPT_PRIORITY */
    NVIC_ISER1 = 1u << (USART2_IRQn - 32);

    xTaskCreate(modbus_task, "modbus", 384, NULL, 4, NULL);
}
