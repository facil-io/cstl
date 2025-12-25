/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
// #include <openssl/bio.h>
// #include <openssl/err.h>
// #include <openssl/ssl.h>
// FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __poly1305_open_ssl_wrapper)(char
// *data,
//                                                                   size_t len)
//                                                                   {
// }
#endif /* HAVE_OPENSSL */

FIO_SFUNC uintptr_t fio__poly1305_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe\x42"
                      "\xd5\x06\xa8"
                      "\x01\x03\x80\x8a\xfb\x0d\xb2\xfd\x4a\xbf\xf6\xaf\x41"
                      "\x49\xf5\x1b";
  fio_poly1305_auth(result, msg, len, NULL, 0, key);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20(msg, len, key, nounce, 1);
  result[0] = fio_buf2u64u(msg);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20poly1305_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20_poly1305_enc(result, msg, len, NULL, 0, key, nounce);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20poly1305dec_speed_wrapper(char *msg,
                                                           size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_poly1305_auth(result, msg, len, NULL, 0, key);
  fio_chacha20(msg, len, key, nounce, 1);
  return (uintptr_t)result[0];
}

int main(void) {
  { /* test ChaCha20 independently */
    FIO_LOG_DDEBUG("Testing ChaCha20 separately");
    struct {
      char key[33];
      char nounce[13];
      char *src;
      char *expected;
    } tests[] = {
        {
            .key = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d"
                   "\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b"
                   "\x1c\x1d\x1e\x1f",
            .nounce = "\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00",
            .src =
                (char *)"\x4c\x61\x64\x69\x65\x73\x20\x61\x6e\x64\x20\x47\x65"
                        "\x6e\x74\x6c\x65\x6d\x65\x6e\x20\x6f\x66\x20\x74\x68"
                        "\x65\x20\x63\x6c\x61\x73\x73\x20\x6f\x66\x20\x27\x39"
                        "\x39\x3a\x20\x49\x66\x20\x49\x20\x63\x6f\x75\x6c\x64"
                        "\x20\x6f\x66\x66\x65\x72\x20\x79\x6f\x75\x20\x6f\x6e"
                        "\x6c\x79\x20\x6f\x6e\x65\x20\x74\x69\x70\x20\x66\x6f"
                        "\x72\x20\x74\x68\x65\x20\x66\x75\x74\x75\x72\x65\x2c"
                        "\x20\x73\x75\x6e\x73\x63\x72\x65\x65\x6e\x20\x77\x6f"
                        "\x75\x6c\x64\x20\x62\x65\x20\x69\x74\x2e",
            .expected =
                (char *)"\x6e\x2e\x35\x9a\x25\x68\xf9\x80\x41\xba\x07\x28\xdd"
                        "\x0d\x69\x81\xe9\x7e\x7a\xec\x1d\x43\x60\xc2\x0a\x27"
                        "\xaf\xcc\xfd\x9f\xae\x0b\xf9\x1b\x65\xc5\x52\x47\x33"
                        "\xab\x8f\x59\x3d\xab\xcd\x62\xb3\x57\x16\x39\xd6\x24"
                        "\xe6\x51\x52\xab\x8f\x53\x0c\x35\x9f\x08\x61\xd8\x07"
                        "\xca\x0d\xbf\x50\x0d\x6a\x61\x56\xa3\x8e\x08\x8a\x22"
                        "\xb6\x5e\x52\xbc\x51\x4d\x16\xcc\xf8\x06\x81\x8c\xe9"
                        "\x1a\xb7\x79\x37\x36\x5a\xf9\x0b\xbf\x74\xa3\x5b\xe6"
                        "\xb4\x0b\x8e\xed\xf2\x78\x5e\x42\x87\x4d",
        },
        {
            .key = {0},
            .nounce = {0},
            .src =
                (char *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            .expected =
                (char *)"\x3a\xeb\x52\x24\xec\xf8\x49\x92\x9b\x9d\x82\x8d\xb1"
                        "\xce\xd4\xdd\x83\x20\x25\xe8\x01\x8b\x81\x60\xb8\x22"
                        "\x84\xf3\xc9\x49\xaa\x5a\x8e\xca\x00\xbb\xb4\xa7\x3b"
                        "\xda\xd1\x92\xb5\xc4\x2f\x73\xf2\xfd\x4e\x27\x36\x44"
                        "\xc8\xb3\x61\x25\xa6\x4a\xdd\xeb\x00\x6c\x13\xa0",
        },
        {.expected = NULL}};
    for (size_t i = 0; tests[i].expected; ++i) {
      size_t len = FIO_STRLEN(tests[i].src);
      char buffer[4096];
      FIO_MEMCPY(buffer, tests[i].src, len);
      fio_chacha20(buffer, len, tests[i].key, tests[i].nounce, 1);
      FIO_ASSERT(!memcmp(buffer, tests[i].expected, len),
                 "ChaCha20 encoding failed");
      fio_chacha20(buffer, len, tests[i].key, tests[i].nounce, 1);
      FIO_ASSERT(!memcmp(buffer, tests[i].src, len),
                 "ChaCha20 decoding failed");
    }
  }
  { /* test Poly1305 independently */
    FIO_LOG_DDEBUG("Testing Poly1305 separately");
    struct {
      char key[33];
      char *msg;
      char *expected;
    } tests[] = {{
                     .key = "\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe"
                            "\x42\xd5\x06\xa8\x01\x03\x80\x8a\xfb\x0d\xb2\xfd"
                            "\x4a\xbf\xf6\xaf\x41\x49\xf5\x1b",
                     .msg = (char *)"Cryptographic Forum Research Group",
                     .expected =
                         (char *)"\xa8\x06\x1d\xc1\x30\x51\x36\xc6\xc2\x2b\x8b"
                                 "\xaf\x0c\x01\x27\xa9",
                 },
                 {.expected = NULL}};
    char auth[24] = {0};
    char buf1[33] = {0};
    char buf2[33] = {0};
    for (size_t t = 0; tests[t].expected; ++t) {
      fio_poly1305_auth(auth,
                        tests[t].msg,
                        FIO_STRLEN(tests[t].msg),
                        NULL,
                        0,
                        tests[t].key);
      for (size_t i = 0; i < 16; ++i) {
        buf1[(i << 1)] = fio_i2c(((auth[i] >> 4) & 0xF));
        buf1[(i << 1) + 1] = fio_i2c(((auth[i]) & 0xF));
        buf2[(i << 1)] = fio_i2c(((tests[t].expected[i] >> 4) & 0xF));
        buf2[(i << 1) + 1] = fio_i2c(((tests[t].expected[i]) & 0xF));
      }
      FIO_ASSERT(!memcmp(auth, tests[t].expected, 16),
                 "Poly1305 example authentication failed:\n\t%s != %s",
                 buf1,
                 buf2);
      FIO_ASSERT(!fio_buf2u64u(auth + 16),
                 "Poly1305 authentication code overflow!");
    }
  }
  { /* test ChaCha20Poly1305 */
    FIO_LOG_DDEBUG("Testing ChaCha20Poly1305 together");
    struct {
      char key[33];
      char nounce[13];
      char *ad;
      size_t ad_len;
      char *msg;
      char *expected;
      char mac[17];
    } tests[] = {
        {
            .key = "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d"
                   "\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b"
                   "\x9c\x9d\x9e\x9f",
            .nounce = "\x07\x00\x00\x00\x40\x41\x42\x43\x44\x45\x46\x47",
            .ad = (char *)"\x50\x51\x52\x53\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7",
            .ad_len = 12,
            .msg =
                (char *)"\x4c\x61\x64\x69\x65\x73\x20\x61\x6e\x64\x20\x47\x65"
                        "\x6e\x74\x6c\x65\x6d\x65\x6e\x20\x6f\x66\x20\x74\x68"
                        "\x65\x20\x63\x6c\x61\x73\x73\x20\x6f\x66\x20\x27\x39"
                        "\x39\x3a\x20\x49\x66\x20\x49\x20\x63\x6f\x75\x6c\x64"
                        "\x20\x6f\x66\x66\x65\x72\x20\x79\x6f\x75\x20\x6f\x6e"
                        "\x6c\x79\x20\x6f\x6e\x65\x20\x74\x69\x70\x20\x66\x6f"
                        "\x72\x20\x74\x68\x65\x20\x66\x75\x74\x75\x72\x65\x2c"
                        "\x20\x73\x75\x6e\x73\x63\x72\x65\x65\x6e\x20\x77\x6f"
                        "\x75\x6c\x64\x20\x62\x65\x20\x69\x74\x2e",
            .expected =
                (char *)"\xd3\x1a\x8d\x34\x64\x8e\x60\xdb\x7b\x86\xaf\xbc\x53"
                        "\xef\x7e\xc2\xa4\xad\xed\x51\x29\x6e\x08\xfe\xa9\xe2"
                        "\xb5\xa7\x36\xee\x62\xd6\x3d\xbe\xa4\x5e\x8c\xa9\x67"
                        "\x12\x82\xfa\xfb\x69\xda\x92\x72\x8b\x1a\x71\xde\x0a"
                        "\x9e\x06\x0b\x29\x05\xd6\xa5\xb6\x7e\xcd\x3b\x36\x92"
                        "\xdd\xbd\x7f\x2d\x77\x8b\x8c\x98\x03\xae\xe3\x28\x09"
                        "\x1b\x58\xfa\xb3\x24\xe4\xfa\xd6\x75\x94\x55\x85\x80"
                        "\x8b\x48\x31\xd7\xbc\x3f\xf4\xde\xf0\x8e\x4b\x7a\x9d"
                        "\xe5\x76\xd2\x65\x86\xce\xc6\x4b\x61\x16",
            .mac = "\x1a\xe1\x0b\x59\x4f\x09\xe2\x6a\x7e\x90\x2e\xcb\xd0\x60"
                   "\x06\x91",
        },
        {
            .key = "\x1c\x92\x40\xa5\xeb\x55\xd3\x8a\xf3\x33\x88\x86\x04\xf6"
                   "\xb5\xf0\x47\x39\x17\xc1\x40\x2b\x80\x09\x9d\xca\x5c\xbc"
                   "\x20\x70\x75\xc0",
            .nounce = "\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08",
            .ad = (char *)"\xf3\x33\x88\x86\x00\x00\x00\x00\x00\x00\x4e\x91",
            .ad_len = 12,
            .msg =
                (char *)"\x49\x6e\x74\x65\x72\x6e\x65\x74\x2d\x44\x72\x61\x66"
                        "\x74\x73\x20\x61\x72\x65\x20\x64\x72\x61\x66\x74\x20"
                        "\x64\x6f\x63\x75\x6d\x65\x6e\x74\x73\x20\x76\x61\x6c"
                        "\x69\x64\x20\x66\x6f\x72\x20\x61\x20\x6d\x61\x78\x69"
                        "\x6d\x75\x6d\x20\x6f\x66\x20\x73\x69\x78\x20\x6d\x6f"
                        "\x6e\x74\x68\x73\x20\x61\x6e\x64\x20\x6d\x61\x79\x20"
                        "\x62\x65\x20\x75\x70\x64\x61\x74\x65\x64\x2c\x20\x72"
                        "\x65\x70\x6c\x61\x63\x65\x64\x2c\x20\x6f\x72\x20\x6f"
                        "\x62\x73\x6f\x6c\x65\x74\x65\x64\x20\x62\x79\x20\x6f"
                        "\x74\x68\x65\x72\x20\x64\x6f\x63\x75\x6d\x65\x6e\x74"
                        "\x73\x20\x61\x74\x20\x61\x6e\x79\x20\x74\x69\x6d\x65"
                        "\x2e\x20\x49\x74\x20\x69\x73\x20\x69\x6e\x61\x70\x70"
                        "\x72\x6f\x70\x72\x69\x61\x74\x65\x20\x74\x6f\x20\x75"
                        "\x73\x65\x20\x49\x6e\x74\x65\x72\x6e\x65\x74\x2d\x44"
                        "\x72\x61\x66\x74\x73\x20\x61\x73\x20\x72\x65\x66\x65"
                        "\x72\x65\x6e\x63\x65\x20\x6d\x61\x74\x65\x72\x69\x61"
                        "\x6c\x20\x6f\x72\x20\x74\x6f\x20\x63\x69\x74\x65\x20"
                        "\x74\x68\x65\x6d\x20\x6f\x74\x68\x65\x72\x20\x74\x68"
                        "\x61\x6e\x20\x61\x73\x20\x2f\xe2\x80\x9c\x77\x6f\x72"
                        "\x6b\x20\x69\x6e\x20\x70\x72\x6f\x67\x72\x65\x73\x73"
                        "\x2e\x2f\xe2\x80\x9d",
            .expected =
                (char *)"\x64\xa0\x86\x15\x75\x86\x1a\xf4\x60\xf0\x62\xc7\x9b"
                        "\xe6\x43\xbd\x5e\x80\x5c\xfd\x34\x5c\xf3\x89\xf1\x08"
                        "\x67\x0a\xc7\x6c\x8c\xb2\x4c\x6c\xfc\x18\x75\x5d\x43"
                        "\xee\xa0\x9e\xe9\x4e\x38\x2d\x26\xb0\xbd\xb7\xb7\x3c"
                        "\x32\x1b\x01\x00\xd4\xf0\x3b\x7f\x35\x58\x94\xcf\x33"
                        "\x2f\x83\x0e\x71\x0b\x97\xce\x98\xc8\xa8\x4a\xbd\x0b"
                        "\x94\x81\x14\xad\x17\x6e\x00\x8d\x33\xbd\x60\xf9\x82"
                        "\xb1\xff\x37\xc8\x55\x97\x97\xa0\x6e\xf4\xf0\xef\x61"
                        "\xc1\x86\x32\x4e\x2b\x35\x06\x38\x36\x06\x90\x7b\x6a"
                        "\x7c\x02\xb0\xf9\xf6\x15\x7b\x53\xc8\x67\xe4\xb9\x16"
                        "\x6c\x76\x7b\x80\x4d\x46\xa5\x9b\x52\x16\xcd\xe7\xa4"
                        "\xe9\x90\x40\xc5\xa4\x04\x33\x22\x5e\xe2\x82\xa1\xb0"
                        "\xa0\x6c\x52\x3e\xaf\x45\x34\xd7\xf8\x3f\xa1\x15\x5b"
                        "\x00\x47\x71\x8c\xbc\x54\x6a\x0d\x07\x2b\x04\xb3\x56"
                        "\x4e\xea\x1b\x42\x22\x73\xf5\x48\x27\x1a\x0b\xb2\x31"
                        "\x60\x53\xfa\x76\x99\x19\x55\xeb\xd6\x31\x59\x43\x4e"
                        "\xce\xbb\x4e\x46\x6d\xae\x5a\x10\x73\xa6\x72\x76\x27"
                        "\x09\x7a\x10\x49\xe6\x17\xd9\x1d\x36\x10\x94\xfa\x68"
                        "\xf0\xff\x77\x98\x71\x30\x30\x5b\xea\xba\x2e\xda\x04"
                        "\xdf\x99\x7b\x71\x4d\x6c\x6f\x2c\x29\xa6\xad\x5c\xb4"
                        "\x02\x2b\x02\x70\x9b",
            .mac = "\xee\xad\x9d\x67\x89\x0c\xbb\x22\x39\x23\x36\xfe\xa1\x85"
                   "\x1f\x38",
        },
        {.expected = NULL}};
    for (size_t i = 0; tests[i].expected; ++i) {
      size_t len = strlen(tests[i].msg);
      char buffer[1024];
      char mac[24] = {0}, mac2[24] = {0};
      FIO_MEMCPY(buffer, tests[i].msg, len);
      fio_chacha20_poly1305_enc(mac,
                                buffer,
                                len,
                                tests[i].ad,
                                tests[i].ad_len,
                                tests[i].key,
                                tests[i].nounce);
      FIO_ASSERT(!memcmp(buffer, tests[i].expected, len),
                 "ChaCha20Poly1305 encoding failed");
      fio_chacha20_poly1305_auth(mac2,
                                 buffer,
                                 len,
                                 tests[i].ad,
                                 tests[i].ad_len,
                                 tests[i].key,
                                 tests[i].nounce);
      FIO_ASSERT(!memcmp(mac, mac2, 16),
                 "ChaCha20Poly1305 authentication != Poly1305 code");
      FIO_ASSERT(!memcmp(mac, tests[i].mac, 16),
                 "ChaCha20Poly1305 authentication code failed");
      FIO_ASSERT(!fio_chacha20_poly1305_dec(mac,
                                            buffer,
                                            len,
                                            tests[i].ad,
                                            tests[i].ad_len,
                                            tests[i].key,
                                            tests[i].nounce),
                 "fio_chacha20_poly1305_dec returned error for %s",
                 tests[i].msg);
      FIO_ASSERT(!fio_buf2u64u(mac + 16),
                 "ChaCha20Poly1305 authentication code overflow!");
      FIO_ASSERT(!fio_buf2u64u(mac2 + 16),
                 "ChaCha20Poly1305 authentication code (2) overflow!");
      FIO_ASSERT(
          !memcmp(buffer, tests[i].msg, len),
          "ChaCha20Poly1305 decoding failed for %s\nshould have been %.*s",
          tests[i].msg,
          (int)len,
          buffer);
    }
  }
  { /* test roundtrip */
    FIO_LOG_DDEBUG("Testing ChaCha20Poly1305 round-trip.");
    fio_u256 key =
        fio_u256_init64(fio_rand64(), fio_rand64(), fio_rand64(), fio_rand64());
    FIO_STR_INFO_TMP_VAR(ad, 128);
    FIO_STR_INFO_TMP_VAR(plaintext, 1024);
    FIO_STR_INFO_TMP_VAR(cyphertext, 1024);
    FIO_STR_INFO_TMP_VAR(decrypted, 1024);
    fio_string_write2(&ad,
                      NULL,
                      FIO_STRING_WRITE_STR1(
                          "This is unencrypted additional data with a nonce:"),
                      FIO_STRING_WRITE_HEX(fio_rand64()));
    fio_string_write2(
        &plaintext,
        NULL,
        FIO_STRING_WRITE_STR1(
            "This is unencrypted text that will eventually be encrypted, the "
            "following are the whole 0-255 byte values:"));
    for (size_t i = 0; i < 256; ++i) {
      plaintext.buf[plaintext.len++] = (char)i;
    }
    plaintext.buf[plaintext.len] = 0;
    FIO_MEMCPY(cyphertext.buf, plaintext.buf, plaintext.len);
    cyphertext.len = plaintext.len;
    fio_chacha20_poly1305_enc(ad.buf + ad.len,
                              cyphertext.buf,
                              cyphertext.len,
                              ad.buf, /* additional data */
                              ad.len,
                              key.u8,
                              ad.buf + ad.len - 12);
    FIO_MEMCPY(decrypted.buf, cyphertext.buf, cyphertext.len);
    decrypted.len = cyphertext.len;
    FIO_ASSERT(!fio_chacha20_poly1305_dec(ad.buf + ad.len,
                                          decrypted.buf,
                                          decrypted.len,
                                          ad.buf, /* additional data */
                                          ad.len,
                                          key.u8,
                                          ad.buf + ad.len - 12),
               "fio_chacha20_poly1305_dec failed!");
    FIO_ASSERT(FIO_MEMCMP(cyphertext.buf, plaintext.buf, plaintext.len),
               "chacha20 cypher-text should be different than plain-text.");
    FIO_ASSERT(!FIO_MEMCMP(decrypted.buf, plaintext.buf, plaintext.len),
               "chacha20_poly1305 roundtrip error!");
  }

  /* Performance tests moved to tests/performance-crypto.c */

  /* **************************************************************************
   * Edge Case Tests for ChaCha20-Poly1305 AEAD
   * *************************************************************************/
  FIO_LOG_DDEBUG("Testing ChaCha20-Poly1305 edge cases...");

  /* Test: Empty plaintext (0 bytes) - should still produce valid tag */
  {
    FIO_LOG_DDEBUG("  Testing empty plaintext...");
    char key[32] = {0};
    char nonce[12] = {0};
    char mac[16] = {0};
    char aad[] = "additional data";

    /* Encrypt empty message */
    fio_chacha20_poly1305_enc(mac, NULL, 0, aad, sizeof(aad) - 1, key, nonce);

    /* MAC should not be all zeros */
    int all_zero = 1;
    for (int i = 0; i < 16; ++i) {
      if (mac[i] != 0) {
        all_zero = 0;
        break;
      }
    }
    FIO_ASSERT(!all_zero, "Empty plaintext should produce non-zero MAC");

    /* Decrypt should succeed */
    int ret = fio_chacha20_poly1305_dec(mac,
                                        NULL,
                                        0,
                                        aad,
                                        sizeof(aad) - 1,
                                        key,
                                        nonce);
    FIO_ASSERT(ret == 0, "Empty plaintext decryption should succeed");
  }

  /* Test: Empty AAD (0 bytes) */
  {
    FIO_LOG_DDEBUG("  Testing empty AAD...");
    char key[32] = {0};
    char nonce[12] = {0};
    char plaintext[] = "Hello, World!";
    char buffer[64];
    char mac[16] = {0};

    FIO_MEMCPY(buffer, plaintext, sizeof(plaintext));
    fio_chacha20_poly1305_enc(mac,
                              buffer,
                              sizeof(plaintext),
                              NULL,
                              0,
                              key,
                              nonce);

    int ret = fio_chacha20_poly1305_dec(mac,
                                        buffer,
                                        sizeof(plaintext),
                                        NULL,
                                        0,
                                        key,
                                        nonce);
    FIO_ASSERT(ret == 0, "Empty AAD decryption should succeed");
    FIO_ASSERT(!memcmp(buffer, plaintext, sizeof(plaintext)),
               "Empty AAD roundtrip failed");
  }

  /* Test: Both plaintext and AAD empty */
  {
    FIO_LOG_DDEBUG("  Testing both plaintext and AAD empty...");
    char key[32] = {0};
    char nonce[12] = {0};
    char mac[16] = {0};

    fio_chacha20_poly1305_enc(mac, NULL, 0, NULL, 0, key, nonce);

    int ret = fio_chacha20_poly1305_dec(mac, NULL, 0, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "Both empty decryption should succeed");
  }

  /* Test: Plaintext at block boundary (64 bytes for ChaCha) */
  {
    FIO_LOG_DDEBUG("  Testing plaintext at block boundary (64 bytes)...");
    char key[32];
    char nonce[12];
    char plaintext[64];
    char buffer[64];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 64);
    FIO_MEMCPY(buffer, plaintext, 64);

    fio_chacha20_poly1305_enc(mac, buffer, 64, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 64, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "64-byte plaintext decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 64), "64-byte roundtrip failed");
  }

  /* Test: Plaintext one byte before block boundary (63 bytes) */
  {
    FIO_LOG_DDEBUG("  Testing plaintext at 63 bytes...");
    char key[32];
    char nonce[12];
    char plaintext[63];
    char buffer[63];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 63);
    FIO_MEMCPY(buffer, plaintext, 63);

    fio_chacha20_poly1305_enc(mac, buffer, 63, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 63, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "63-byte plaintext decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 63), "63-byte roundtrip failed");
  }

  /* Test: Plaintext one byte after block boundary (65 bytes) */
  {
    FIO_LOG_DDEBUG("  Testing plaintext at 65 bytes...");
    char key[32];
    char nonce[12];
    char plaintext[65];
    char buffer[65];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 65);
    FIO_MEMCPY(buffer, plaintext, 65);

    fio_chacha20_poly1305_enc(mac, buffer, 65, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 65, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "65-byte plaintext decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 65), "65-byte roundtrip failed");
  }

  /* Test: AAD at various sizes (1, 16, 17, 256) */
  {
    FIO_LOG_DDEBUG("  Testing various AAD sizes...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];
    char aad[256];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    fio_rand_bytes(aad, 256);

    size_t aad_sizes[] = {1, 16, 17, 256};
    for (size_t i = 0; i < sizeof(aad_sizes) / sizeof(aad_sizes[0]); ++i) {
      FIO_MEMCPY(buffer, plaintext, 32);
      fio_chacha20_poly1305_enc(mac, buffer, 32, aad, aad_sizes[i], key, nonce);
      int ret = fio_chacha20_poly1305_dec(mac,
                                          buffer,
                                          32,
                                          aad,
                                          aad_sizes[i],
                                          key,
                                          nonce);
      FIO_ASSERT(ret == 0, "AAD size %zu decryption failed", aad_sizes[i]);
      FIO_ASSERT(!memcmp(buffer, plaintext, 32),
                 "AAD size %zu roundtrip failed",
                 aad_sizes[i]);
    }
  }

  /* Test: Tag verification failure (corrupted ciphertext) */
  {
    FIO_LOG_DDEBUG("  Testing corrupted ciphertext detection...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);

    /* Corrupt ciphertext */
    buffer[0] ^= 0x01;

    int ret = fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(ret != 0, "Corrupted ciphertext should fail verification");
  }

  /* Test: Tag verification failure (corrupted tag) */
  {
    FIO_LOG_DDEBUG("  Testing corrupted tag detection...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);

    /* Corrupt tag */
    mac[0] ^= 0x01;

    int ret = fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(ret != 0, "Corrupted tag should fail verification");
  }

  /* Test: Tag verification failure (corrupted AAD) */
  {
    FIO_LOG_DDEBUG("  Testing corrupted AAD detection...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];
    char aad[16];
    char bad_aad[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    fio_rand_bytes(aad, 16);
    FIO_MEMCPY(bad_aad, aad, 16);
    bad_aad[0] ^= 0x01;
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, aad, 16, key, nonce);

    /* Decrypt with corrupted AAD */
    int ret =
        fio_chacha20_poly1305_dec(mac, buffer, 32, bad_aad, 16, key, nonce);
    FIO_ASSERT(ret != 0, "Corrupted AAD should fail verification");
  }

  /* Test: Wrong key decryption */
  {
    FIO_LOG_DDEBUG("  Testing wrong key detection...");
    char key1[32];
    char key2[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];

    fio_rand_bytes(key1, 32);
    fio_rand_bytes(key2, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key1, nonce);

    /* Decrypt with wrong key */
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key2, nonce);
    FIO_ASSERT(ret != 0, "Wrong key should fail verification");
  }

  /* Test: Single byte plaintext */
  {
    FIO_LOG_DDEBUG("  Testing single byte plaintext...");
    char key[32];
    char nonce[12];
    char plaintext[1] = {0x42};
    char buffer[1];
    char mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    FIO_MEMCPY(buffer, plaintext, 1);

    fio_chacha20_poly1305_enc(mac, buffer, 1, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 1, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "Single byte decryption failed");
    FIO_ASSERT(buffer[0] == plaintext[0], "Single byte roundtrip failed");
  }

  /* Test: Large plaintext (16KB - TLS max) */
  {
    FIO_LOG_DDEBUG("  Testing large plaintext (16KB)...");
    char key[32];
    char nonce[12];
    size_t len = 16384;
    char *plaintext = (char *)malloc(len);
    char *buffer = (char *)malloc(len);
    char mac[16];

    FIO_ASSERT(plaintext && buffer, "Memory allocation failed");

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buffer, plaintext, len);

    fio_chacha20_poly1305_enc(mac, buffer, len, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "16KB decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, len), "16KB roundtrip failed");

    free(plaintext);
    free(buffer);
  }

  /* Test: Deterministic encryption (same inputs = same outputs) */
  {
    FIO_LOG_DDEBUG("  Testing deterministic encryption...");
    char key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
    char nonce[12] = {0x00,
                      0x01,
                      0x02,
                      0x03,
                      0x04,
                      0x05,
                      0x06,
                      0x07,
                      0x08,
                      0x09,
                      0x0a,
                      0x0b};
    char plaintext[32] = "deterministic test message!!!!!";
    char buffer1[32], buffer2[32];
    char mac1[16], mac2[16];

    FIO_MEMCPY(buffer1, plaintext, 32);
    FIO_MEMCPY(buffer2, plaintext, 32);

    fio_chacha20_poly1305_enc(mac1, buffer1, 32, NULL, 0, key, nonce);
    fio_chacha20_poly1305_enc(mac2, buffer2, 32, NULL, 0, key, nonce);

    FIO_ASSERT(!memcmp(buffer1, buffer2, 32),
               "Encryption should be deterministic");
    FIO_ASSERT(!memcmp(mac1, mac2, 16), "MAC should be deterministic");
  }

  /* Test: All-zero key and nonce */
  {
    FIO_LOG_DDEBUG("  Testing all-zero key and nonce...");
    char key[32] = {0};
    char nonce[12] = {0};
    char plaintext[32];
    char buffer[32];
    char mac[16];

    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "All-zero key/nonce decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 32),
               "All-zero key/nonce roundtrip failed");
  }

  /* Test: All-ones key and nonce */
  {
    FIO_LOG_DDEBUG("  Testing all-ones key and nonce...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];

    FIO_MEMSET(key, 0xFF, 32);
    FIO_MEMSET(nonce, 0xFF, 12);
    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);
    int ret = fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(ret == 0, "All-ones key/nonce decryption failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 32),
               "All-ones key/nonce roundtrip failed");
  }

  /* Test: Flip each byte of signature to ensure detection */
  {
    FIO_LOG_DDEBUG("  Testing tag bit-flip detection...");
    char key[32];
    char nonce[12];
    char plaintext[32];
    char buffer[32];
    char mac[16];
    char bad_mac[16];

    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);

    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);

    /* Flip each byte of the MAC and verify detection */
    for (int i = 0; i < 16; ++i) {
      FIO_MEMCPY(bad_mac, mac, 16);
      bad_mac[i] ^= 0x01;
      char test_buffer[32];
      FIO_MEMCPY(test_buffer, buffer, 32);
      int ret = fio_chacha20_poly1305_dec(bad_mac,
                                          test_buffer,
                                          32,
                                          NULL,
                                          0,
                                          key,
                                          nonce);
      FIO_ASSERT(ret != 0, "Flipped MAC byte %d should fail verification", i);
    }
  }

  FIO_LOG_DDEBUG("ChaCha20-Poly1305 edge case tests passed!");
}
