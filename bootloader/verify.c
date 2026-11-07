#include <string.h>
#include "pubkey.h"
#include "tweetnacl.h"
#include "verify.h"

/* TweetNaCl references this only for key generation, which the bootloader never calls. */
void randombytes(unsigned char *buf, unsigned long long len)
{
    (void)buf; (void)len;
    for (;;) { }
}

int boot_verify_signature(const uint8_t digest[32], const uint8_t sig[64])
{
    uint8_t sm[64 + 32], m[64 + 32];
    unsigned long long mlen = 0;
    memcpy(sm, sig, 64);
    memcpy(sm + 64, digest, 32);
    if (crypto_sign_open(m, &mlen, sm, sizeof sm, BOOT_PUBLIC_KEY) != 0) return -1;
    if (mlen != 32 || memcmp(m, digest, 32) != 0) return -1;
    return 0;
}
