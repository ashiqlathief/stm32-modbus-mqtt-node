/* Secure-boot bootloader: only starts an application whose Ed25519 signature checks out.
 *
 * Anti-rollback here is a build-time minimum version (MIN_FW_VERSION). A real product would keep a
 * monotonic counter in OTP or protected flash; that is not modelled in the simulation. */
#include <stdint.h>
#include "image_header.h"
#include "regs.h"
#include "sha256.h"
#include "uart.h"
#include "verify.h"

#define MIN_FW_VERSION 1u

static const char *check_image(const image_header_t *h)
{
    if (h->magic != IMAGE_MAGIC) return "no image (bad magic)";
    if (h->image_size == 0 || h->image_size > IMAGE_MAX_SIZE) return "bad image size";

    sha256_ctx c;
    uint8_t digest[32];
    sha256_init(&c);
    sha256_update(&c, h, 12);                                  /* magic, version, size */
    sha256_update(&c, (const void *)IMAGE_APP_ADDR, h->image_size);
    sha256_final(&c, digest);

    if (boot_verify_signature(digest, h->signature) != 0) return "bad signature";
    if (h->fw_version < MIN_FW_VERSION) return "version too old (rollback)";
    return 0;
}

static void start_app(void)
{
    uint32_t sp = *(volatile uint32_t *)IMAGE_APP_ADDR;
    uint32_t pc = *(volatile uint32_t *)(IMAGE_APP_ADDR + 4u);
    SYST_CSR = 0;
    SCB_VTOR = IMAGE_APP_ADDR;
    __asm volatile("msr msp, %0\n bx %1" :: "r"(sp), "r"(pc));
    for (;;) { }
}

int main(void)
{
    uart_init();
    uart_puts("bootloader: checking image\r\n");
    const image_header_t *h = (const image_header_t *)IMAGE_HEADER_ADDR;
    const char *err = check_image(h);
    if (err) {
        uart_puts("bootloader: REFUSED, ");
        uart_puts(err);
        uart_puts("\r\n");
        for (;;) { }
    }
    uart_puts("bootloader: image verified, version ");
    uart_put_u32(h->fw_version);
    uart_puts(", starting app\r\n");
    start_app();
    return 0;
}
