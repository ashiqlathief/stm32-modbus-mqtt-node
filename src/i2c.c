#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "i2c.h"
#include "regs.h"

#define CR1_PE     (1u << 0)
#define CR1_START  (1u << 8)
#define CR1_STOP   (1u << 9)
#define CR1_ACK    (1u << 10)
#define CR1_POS    (1u << 11)
#define SR1_SB     (1u << 0)
#define SR1_ADDR   (1u << 1)
#define SR1_BTF    (1u << 2)
#define SR1_RXNE   (1u << 6)
#define SR1_TXE    (1u << 7)
#define SR1_AF     (1u << 10)

#define TIMEOUT_TICKS 20

static SemaphoreHandle_t bus_lock;

void i2c_init(void)
{
    RCC_AHB1ENR |= (1u << 1);                                  /* GPIOB clock */
    RCC_APB1ENR |= (1u << 21);                                 /* I2C1 clock */
    GPIOB_MODER = (GPIOB_MODER & ~((3u << 12) | (3u << 14))) | (2u << 12) | (2u << 14); /* PB6 SCL, PB7 SDA alternate */
    GPIOB_OTYPER |= (1u << 6) | (1u << 7);                     /* open drain */
    GPIOB_AFRL = (GPIOB_AFRL & ~((0xFu << 24) | (0xFu << 28))) | (4u << 24) | (4u << 28); /* AF4 = I2C1 */

    I2C1_CR1 = 0;
    I2C1_CR2 = 16;                                             /* APB1 clock in MHz */
    I2C1_CCR = 80;                                             /* 100 kHz standard mode at 16 MHz */
    I2C1_TRISE = 17;
    I2C1_CR1 = CR1_PE;

    bus_lock = xSemaphoreCreateMutex();
    configASSERT(bus_lock != NULL);
}

static int wait_flag(volatile uint32_t *reg, uint32_t mask)
{
    TickType_t start = xTaskGetTickCount();
    while (!(*reg & mask)) {
        if (*reg == 0xFFFFFFFFu) return -1;
        if (I2C1_SR1 & SR1_AF) return -2;                      /* slave did not acknowledge */
        if ((TickType_t)(xTaskGetTickCount() - start) > TIMEOUT_TICKS) return -1;
        taskYIELD();
    }
    return 0;
}

static int start_and_address(uint8_t addr7, uint8_t read)
{
    int r;
    I2C1_CR1 |= CR1_START;
    if ((r = wait_flag(&I2C1_SR1, SR1_SB)) != 0) return r;
    I2C1_DR = (uint32_t)((addr7 << 1) | read);
    if ((r = wait_flag(&I2C1_SR1, SR1_ADDR)) != 0) return r;
    return 0;
}

static void clear_addr(void)
{
    (void)I2C1_SR1;
    (void)I2C1_SR2;                                            /* reading SR1 then SR2 clears ADDR */
}

int i2c_command_read(uint8_t addr7, uint8_t cmd, uint8_t *buf, uint8_t len)
{
    int r;
    xSemaphoreTake(bus_lock, portMAX_DELAY);

    if ((r = start_and_address(addr7, 0)) != 0) goto fail;
    clear_addr();
    I2C1_DR = cmd;
    if ((r = wait_flag(&I2C1_SR1, SR1_TXE | SR1_BTF)) != 0) goto fail;

    if ((r = start_and_address(addr7, 1)) != 0) goto fail;
    I2C1_CR1 |= CR1_ACK;
    clear_addr();
    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1u) I2C1_CR1 = (I2C1_CR1 & ~CR1_ACK) | CR1_STOP;   /* NACK + STOP on the last byte */
        if ((r = wait_flag(&I2C1_SR1, SR1_RXNE)) != 0) goto fail;
        buf[i] = (uint8_t)I2C1_DR;
    }
    xSemaphoreGive(bus_lock);
    return 0;

fail:
    I2C1_SR1 &= ~SR1_AF;
    I2C1_CR1 |= CR1_STOP;
    xSemaphoreGive(bus_lock);
    return r;
}
