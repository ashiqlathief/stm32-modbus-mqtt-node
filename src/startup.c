#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);

extern void (*const vector_table[])(void);
#define SCB_VTOR (*(volatile uint32_t *)0xE000ED08)

void Reset_Handler(void)
{
    SCB_VTOR = (uint32_t)vector_table;      /* the bootloader sets this too; harmless when started directly */
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    main();
    for (;;) { }
}

static void Default_Handler(void) { for (;;) { } }

/* Core exceptions only; device interrupts get added with the drivers. */
__attribute__((section(".isr_vector"), used))
void (*const vector_table[55])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    Default_Handler,   /* NMI */
    Default_Handler,   /* HardFault */
    Default_Handler,   /* MemManage */
    Default_Handler,   /* BusFault */
    Default_Handler,   /* UsageFault */
    0, 0, 0, 0,
    SVC_Handler,
    Default_Handler,   /* DebugMon */
    0,
    PendSV_Handler,
    SysTick_Handler,
    [16 + 38] = USART2_IRQHandler,   /* IRQ 38 */
};
