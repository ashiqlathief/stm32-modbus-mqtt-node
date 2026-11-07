#pragma once
#include <stdint.h>

/* Signed firmware image layout in flash:
 *   0x08010000  image header (256 bytes)
 *   0x08010100  application: vector table first, then code
 *
 * The signature is an Ed25519 signature over the 32-byte SHA-256 digest of
 *   header bytes 0..11 (magic, fw_version, image_size)  ||  application bytes.
 * Changing the version, the size or any application byte invalidates the signature. */
#define IMAGE_MAGIC        0x4D495746u   /* "FWIM" */
#define IMAGE_HEADER_ADDR  0x08010000u
#define IMAGE_APP_ADDR     0x08010100u
#define IMAGE_MAX_SIZE     (0x100000u - 0x10100u)   /* up to the end of the 1 MiB flash */

typedef struct {
    uint32_t magic;
    uint32_t fw_version;
    uint32_t image_size;      /* bytes of application after the header */
    uint32_t reserved;        /* not signed, must be zero */
    uint8_t  signature[64];
    uint8_t  pad[176];
} image_header_t;

_Static_assert(sizeof(image_header_t) == 256, "image header must be 256 bytes");
