/* Regression: base32 encoder/decoder must roundtrip partial final groups. */
#include "test-helpers.h"
#define FIO_STR
#include FIO_INCLUDE_FILE

int main(void) {
  static const size_t sizes[] = {1, 2, 3, 4, 5, 15, 16, 17, 1023, 1024,
                                 2047, 2048, 4095, 4096};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
    const size_t n = sizes[i];
    char *input = (char *)malloc(n);
    FIO_ASSERT(input, "base32 input allocation failed");
    for (size_t j = 0; j < n; ++j)
      input[j] = (char)((j * 73U + 13U) & 255U);
    fio_str_info_s encoded = {0}, decoded = {0};
    int enc_rc = fio_string_write_base32enc(
        &encoded, FIO_STRING_ALLOC_COPY, input, n);
    FIO_ASSERT(!enc_rc && encoded.buf && encoded.len < encoded.capa &&
                   !encoded.buf[encoded.len],
               "base32 encode boundary %zu", n);
    int dec_rc = fio_string_write_base32dec(
        &decoded, FIO_STRING_ALLOC_COPY, encoded.buf, encoded.len);
    int ok = !dec_rc && decoded.buf && decoded.len == n &&
             decoded.len < decoded.capa && !decoded.buf[decoded.len] &&
             !FIO_MEMCMP(decoded.buf, input, n);
    if (!ok)
      fprintf(stderr,
              "base32 boundary %zu: encoded len=%zu, decoded len=%zu, rc=%d\n",
              n,
              encoded.len,
              decoded.len,
              dec_rc);
    FIO_STRING_FREE(decoded.buf);
    FIO_STRING_FREE(encoded.buf);
    free(input);
    if (!ok)
      return 1;
  }
  return 0;
}
