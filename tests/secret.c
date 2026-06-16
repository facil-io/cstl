/* *****************************************************************************
Test for 160 secret.h

Coverage: fio_secret_is_random, fio_secret_set, fio_secret_set_at,
fio_secret_at, fio_secret retrieval, hex decoding, empty/random generation,
and determinism. No external processes or network access.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SECRET
#include FIO_INCLUDE_FILE

FIO_SFUNC void FIO_NAME_TEST(stl, secret_basic)(void) {
  /* Set a known secret and verify it is not marked random. */
  fio_secret_set((char *)"known-secret", 12, 0);
  FIO_ASSERT(!fio_secret_is_random(), "known secret should not be random");

  fio_u512 s = fio_secret();
  FIO_ASSERT(!fio_u512_is_eq(&s, &(fio_u512){0}),
             "retrieved secret hash should not be zero");

  /* Setting the same secret again should produce the same hash. */
  fio_u512 s2 = fio_secret();
  FIO_ASSERT(fio_u512_is_eq(&s, &s2),
             "same secret should produce the same hash");
}

FIO_SFUNC void FIO_NAME_TEST(stl, secret_set_at)(void) {
  fio_u512 stored;
  fio_secret_set_at(&stored, (char *)"another-secret", 14);

  fio_u512 retrieved = fio_secret_at(&stored);
  FIO_ASSERT(!fio_u512_is_eq(&retrieved, &(fio_u512){0}),
             "stored secret hash should not be zero");

  /* Re-retrieving should be identical. */
  fio_u512 retrieved2 = fio_secret_at(&stored);
  FIO_ASSERT(fio_u512_is_eq(&retrieved, &retrieved2),
             "repeated fio_secret_at should return the same hash");
}

FIO_SFUNC void FIO_NAME_TEST(stl, secret_hex_decoding)(void) {
  /* "deadbeef" hex should decode to the same secret as the raw bytes. */
  fio_u512 hex_secret;
  fio_secret_set_at(&hex_secret, (char *)"deadbeef", 8);
  fio_u512 hex_retrieved = fio_secret_at(&hex_secret);

  fio_u512 raw_secret;
  fio_secret_set_at(&raw_secret, (char *)"\xde\xad\xbe\xef", 4);
  fio_u512 raw_retrieved = fio_secret_at(&raw_secret);

  FIO_ASSERT(fio_u512_is_eq(&hex_retrieved, &raw_retrieved),
             "hex-decoded secret should match raw secret hash");

  /* Hex with separators should also decode. */
  fio_u512 sep_secret;
  fio_secret_set_at(&sep_secret, (char *)"DE AD BE EF", 11);
  fio_u512 sep_retrieved = fio_secret_at(&sep_secret);
  FIO_ASSERT(fio_u512_is_eq(&sep_retrieved, &raw_retrieved),
             "hex with separators should decode correctly");
}

FIO_SFUNC void FIO_NAME_TEST(stl, secret_hex_variations)(void) {
  fio_u512 raw;
  fio_secret_set_at(&raw, (char *)"\xde\xad\xbe\xef", 4);
  fio_u512 expected = fio_secret_at(&raw);

  const char *variants[] = {
      "DEADBEEF",
      "deadbeef",
      "DeAdBeEf",
      "de ad be ef",
      "DE AD BE EF",
  };
  for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); ++i) {
    fio_u512 s;
    fio_secret_set_at(&s, (char *)variants[i], strlen(variants[i]));
    fio_u512 r = fio_secret_at(&s);
    FIO_ASSERT(fio_u512_is_eq(&r, &expected),
               "hex variant %zu should decode to raw secret hash", i);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, secret_empty_becomes_random)(void) {
  fio_secret_set((char *)"", 0, 0);
  FIO_ASSERT(fio_secret_is_random(),
             "empty secret should be replaced by a random secret");

  fio_u512 s1 = fio_secret();
  FIO_ASSERT(!fio_u512_is_eq(&s1, &(fio_u512){0}),
             "random secret hash should not be zero");

  /* A second random setting should almost certainly differ. */
  fio_secret_set(NULL, 0, 1);
  fio_u512 s2 = fio_secret();
  FIO_ASSERT(!fio_u512_is_eq(&s1, &s2),
             "two random secrets should differ");
}

FIO_SFUNC void FIO_NAME_TEST(stl, secret_long_input)(void) {
  char long_secret[9000];
  FIO_MEMSET(long_secret, 'A', sizeof(long_secret));
  fio_secret_set(long_secret, sizeof(long_secret), 0);
  fio_u512 s = fio_secret();
  FIO_ASSERT(!fio_u512_is_eq(&s, &(fio_u512){0}),
             "long secret should hash to a non-zero value");
}

int main(void) {
  FIO_NAME_TEST(stl, secret_basic)();
  FIO_NAME_TEST(stl, secret_set_at)();
  FIO_NAME_TEST(stl, secret_hex_decoding)();
  FIO_NAME_TEST(stl, secret_hex_variations)();
  FIO_NAME_TEST(stl, secret_empty_becomes_random)();
  FIO_NAME_TEST(stl, secret_long_input)();
  return 0;
}
