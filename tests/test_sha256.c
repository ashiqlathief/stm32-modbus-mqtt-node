#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sha256.h"

static void hex(const uint8_t d[32], char out[65])
{
    for (int i = 0; i < 32; i++) sprintf(out + 2 * i, "%02x", d[i]);
}

static void digest_of(const char *s, char out[65])
{
    sha256_ctx c; uint8_t d[32];
    sha256_init(&c); sha256_update(&c, s, strlen(s)); sha256_final(&c, d);
    hex(d, out);
}

int main(void)
{
    char h[65];
    digest_of("", h);
    assert(!strcmp(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    digest_of("abc", h);
    assert(!strcmp(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    digest_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", h);
    assert(!strcmp(h, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));

    /* one million 'a', fed in odd-sized chunks */
    sha256_ctx c; uint8_t d[32]; char chunk[137];
    memset(chunk, 'a', sizeof chunk);
    sha256_init(&c);
    size_t left = 1000000;
    while (left) { size_t n = left < sizeof chunk ? left : sizeof chunk; sha256_update(&c, chunk, n); left -= n; }
    sha256_final(&c, d); hex(d, h);
    assert(!strcmp(h, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));

    puts("sha256: all tests passed");
    return 0;
}
