#pragma once
#include <stdint.h>

/* Returns 0 if `sig` is a valid Ed25519 signature of the 32-byte `digest` under the built-in public key. */
int boot_verify_signature(const uint8_t digest[32], const uint8_t sig[64]);
