#pragma once
#include <stdint.h>
#define REG(addr) (*(volatile uint32_t *)(addr))

#define RCC_AHB1ENR   REG(0x40023830)
#define RCC_APB1ENR   REG(0x40023840)

#define GPIOD_MODER   REG(0x40020C00)
#define GPIOD_ODR     REG(0x40020C14)
#define GPIOA_MODER   REG(0x40020000)
#define GPIOA_AFRL    REG(0x40020020)

#define USART2_SR     REG(0x40004400)
#define USART2_DR     REG(0x40004404)
#define USART2_BRR    REG(0x40004408)
#define USART2_CR1    REG(0x4000440C)

#define SYST_CSR      REG(0xE000E010)
#define SYST_RVR      REG(0xE000E014)
#define SYST_CVR      REG(0xE000E018)

#define RCC_APB2ENR   REG(0x40023844)
#define GPIOA_AFRH    REG(0x40020024)

#define USART1_SR     REG(0x40011000)
#define USART1_DR     REG(0x40011004)
#define USART1_BRR    REG(0x40011008)
#define USART1_CR1    REG(0x4001100C)

#define NVIC_ISER1    REG(0xE000E104)
#define NVIC_IPR(n)   (*(volatile uint8_t *)(0xE000E400 + (n)))
#define USART2_IRQn   38

#define GPIOB_MODER   REG(0x40020400)
#define GPIOB_OTYPER  REG(0x40020404)
#define GPIOB_AFRL    REG(0x40020420)

#define I2C1_CR1      REG(0x40005400)
#define I2C1_CR2      REG(0x40005404)
#define I2C1_DR       REG(0x40005410)
#define I2C1_SR1      REG(0x40005414)
#define I2C1_SR2      REG(0x40005418)
#define I2C1_CCR      REG(0x4000541C)
#define I2C1_TRISE    REG(0x40005420)

#define SCB_VTOR      REG(0xE000ED08)
