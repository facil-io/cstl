/* *****************************************************************************
Split FIO_EXTERN build - declarations-only translation unit.

Linked against `impl.c` (see `make tests/extern-split`). Compiled with
missing-prototype / implicit-declaration warnings promoted to errors, so any
public function lacking a declaration (or any internal helper leaking out of
its translation unit) fails the build.
***************************************************************************** */
#define FIO_LOG
#define FIO_EVERYTHING
#define FIO_EXTERN
#include "fio-stl/include.h"

#define EXTERN_SPLIT_CHECK(cond, ...)                                          \
  do {                                                                         \
    if (!(cond)) {                                                             \
      FIO_LOG_ERROR("(extern-split) " __VA_ARGS__);                            \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(int argc, char const *argv[]) {
  (void)argc, (void)argv;

  /* FIOBJ big numbers, floats and data (static ref-counted helper types) */
  FIOBJ n = fiobj_num_new((intptr_t)1 << 62);
  EXTERN_SPLIT_CHECK(fiobj_num2i(n) == ((intptr_t)1 << 62), "fiobj bignum");
  fiobj_free(n);
  FIOBJ f = fiobj_float_new(1.5);
  EXTERN_SPLIT_CHECK(fiobj_float2f(f) == 1.5, "fiobj float");
  fiobj_free(f);
  FIOBJ d = fiobj_data_new(1024);
  EXTERN_SPLIT_CHECK(d != FIOBJ_INVALID, "fiobj data");
  fiobj_free(d);

  /* HMAC-SHA256 (RFC 4231, test case 2) */
  fio_u256 mac = fio_sha256_hmac("Jefe", 4, "what do ya want for nothing?", 28);
  EXTERN_SPLIT_CHECK(mac.u8[0] == 0x5b && mac.u8[31] == 0x43, "sha256 hmac");

  /* HKDF: valid and over-limit requests */
  uint8_t okm[64];
  EXTERN_SPLIT_CHECK(!fio_hkdf(okm, 42, NULL, 0, "ikm", 3, NULL, 0, 0),
                     "hkdf failed");
  EXTERN_SPLIT_CHECK(fio_hkdf(okm, 255 * 32 + 1, NULL, 0, "ikm", 3, NULL, 0, 0),
                     "hkdf accepted oversized output");

  /* ChaCha20-Poly1305: detached auth must match the encryption MAC */
  uint8_t key[32] = {1}, nonce[12] = {2}, msg[48] = {3}, mac1[16], mac2[16];
  int r =
      fio_chacha20_poly1305_enc(mac1, msg, sizeof(msg), NULL, 0, key, nonce);
  EXTERN_SPLIT_CHECK(r == 0, "chacha20poly1305 enc failed");
  r = fio_chacha20_poly1305_auth(mac2, msg, sizeof(msg), NULL, 0, key, nonce);
  EXTERN_SPLIT_CHECK(r == 0, "chacha20poly1305 auth failed");
  EXTERN_SPLIT_CHECK(!FIO_MEMCMP(mac1, mac2, 16), "chacha20poly1305 auth");

  /* system-allocator string reallocation */
  fio_str_info_s s = FIO_STR_INFO2(NULL, 0);
  EXTERN_SPLIT_CHECK(!fio_string_sys_reallocate(&s, 100) && s.buf &&
                         s.capa >= 100,
                     "string sys realloc");
  free(s.buf);

  /* TLS 1.3 IO function table */
  fio_io_functions_s tls = fio_tls13_io_functions();
  EXTERN_SPLIT_CHECK(tls.build_context != NULL, "tls13 io functions");

  /* formerly always-static public helpers must link from this unit */
  char num[] = "-17", *pnum = num;
  fio_aton_s an = fio_aton(&pnum);
  EXTERN_SPLIT_CHECK(!an.err && !an.is_float && an.i == -17, "fio_aton");

  fio_u512 b2b = fio_blake2b("abc", 3); /* RFC 7693 Appendix A */
  EXTERN_SPLIT_CHECK(b2b.u8[0] == 0xBA && b2b.u8[63] == 0x23, "blake2b");
  fio_u256 b2s = fio_blake2s("abc", 3); /* RFC 7693 Appendix B */
  EXTERN_SPLIT_CHECK(b2s.u8[0] == 0x50 && b2s.u8[31] == 0x82, "blake2s");

  EXTERN_SPLIT_CHECK(fio_deflate_compress_bound(100) >= 100 &&
                         fio_deflate_decompress_bound(100) >= 100,
                     "deflate bounds");

  uint8_t tiv[12] = {0}, tnonce[12];
  fio_tls13_build_nonce(tnonce, tiv, 0x0102);
  EXTERN_SPLIT_CHECK(tnonce[10] == 1 && tnonce[11] == 2 && !tnonce[0],
                     "tls13 nonce");
  fio_tls13_record_keys_s tkeys;
  FIO_MEMSET(&tkeys, 0xAA, sizeof(tkeys));
  fio_tls13_record_keys_clear(&tkeys);
  EXTERN_SPLIT_CHECK(!tkeys.key[0] && !tkeys.key[31] && !tkeys.iv[11],
                     "tls13 keys clear");

  /* opaque types (no allocator in this unit): prove the symbols link */
  int (*volatile poll_init)(fio_poll_s *, fio_poll_settings_s) = fio_poll_init;
  void (*volatile poll_destroy)(fio_poll_s *) = fio_poll_destroy;
  int (*volatile ipc_decrypt)(fio_ipc_s *) = fio_ipc_decrypt;
  EXTERN_SPLIT_CHECK(poll_init && poll_destroy && ipc_decrypt, "symbols");

  FIO_LOG_INFO("(extern-split) split FIO_EXTERN build passed.");
  return 0;
}
