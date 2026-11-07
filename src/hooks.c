#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    uart_puts("STACK OVERFLOW in ");
    uart_puts(name);
    uart_puts("\r\n");
    for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
    uart_puts("MALLOC FAILED\r\n");
    for (;;) { }
}
