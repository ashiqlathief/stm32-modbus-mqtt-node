#include "uart.h"
#include "regs.h"

#define USART_SR_TXE (1u << 7)
#define USART_CR1_TE (1u << 3)
#define USART_CR1_UE (1u << 13)

void uart_init(void)
{
    RCC_AHB1ENR |= (1u << 0);                  /* GPIOA clock */
    RCC_APB2ENR |= (1u << 4);                  /* USART1 clock */
    GPIOA_MODER = (GPIOA_MODER & ~(3u << 18)) | (2u << 18);  /* PA9 alternate function */
    GPIOA_AFRH  = (GPIOA_AFRH & ~(0xFu << 4)) | (7u << 4);   /* AF7 = USART1 */
    USART1_BRR = 16000000u / 115200u;          /* 16 MHz HSI, 115200 baud */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE;
}

void uart_putc(char c)
{
    while (!(USART1_SR & USART_SR_TXE)) { }
    USART1_DR = (uint32_t)c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

void uart_put_u32(unsigned v)
{
    char buf[11];
    int i = 0;
    do { buf[i++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (i) uart_putc(buf[--i]);
}
