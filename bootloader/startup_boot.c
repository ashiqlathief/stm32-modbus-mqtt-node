#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88)

void Reset_Handler(void)
{
    /* The hard-float build may use VFP instructions even in plain C code (memcpy, 64-bit moves). */
    SCB_CPACR |= (0xFu << 20);
    __asm volatile("dsb\n isb");
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    main();
    for (;;) { }
}

static void Default_Handler(void) { for (;;) { } }

__attribute__((section(".isr_vector"), used))
void (*const vector_table[16])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    0, 0, 0, 0,
    Default_Handler, Default_Handler, 0, Default_Handler, Default_Handler,
};
