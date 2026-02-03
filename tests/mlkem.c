#define FIO_MLKEM
#define FIO_SHA3
#define FIO_TEST
#include "../fio-stl/include.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  uint8_t pk[1184], sk[2400];
  uint8_t ct[1088], ss_enc[32], ss_dec[32];

  printf("ML-KEM-768 tests...\n");

  /* Test 1: Random round-trip */
  fio_mlkem768_keypair(pk, sk);
  fio_mlkem768_encaps(ct, ss_enc, pk);
  fio_mlkem768_decaps(ss_dec, ct, sk);
  if (memcmp(ss_enc, ss_dec, 32) != 0) {
    fprintf(stderr, "FAIL: random round-trip\n");
    return 1;
  }
  printf("  random round-trip: OK\n");

  /* Test 2: Deterministic reproducibility */
  {
    uint8_t coins[64], enc_coins[32];
    uint8_t pk2[1184], sk2[2400], ct2[1088], ss2[32];
    uint8_t pk3[1184], sk3[2400], ct3[1088], ss3[32];
    memset(coins, 0x42, 64);
    memset(enc_coins, 0x37, 32);

    fio_mlkem768_keypair_derand(pk2, sk2, coins);
    fio_mlkem768_keypair_derand(pk3, sk3, coins);
    if (memcmp(pk2, pk3, 1184) || memcmp(sk2, sk3, 2400)) {
      fprintf(stderr, "FAIL: derand keypair\n");
      return 1;
    }

    fio_mlkem768_encaps_derand(ct2, ss2, pk2, enc_coins);
    fio_mlkem768_encaps_derand(ct3, ss3, pk3, enc_coins);
    if (memcmp(ct2, ct3, 1088) || memcmp(ss2, ss3, 32)) {
      fprintf(stderr, "FAIL: derand encaps\n");
      return 1;
    }

    uint8_t ss_d[32];
    fio_mlkem768_decaps(ss_d, ct2, sk2);
    if (memcmp(ss2, ss_d, 32)) {
      fprintf(stderr, "FAIL: derand round-trip\n");
      return 1;
    }
    printf("  deterministic round-trip: OK\n");
  }

  /* Test 3: Implicit rejection */
  {
    uint8_t ss_bad[32], ct_bad[1088];
    memcpy(ct_bad, ct, 1088);
    ct_bad[0] ^= 0xFF;
    fio_mlkem768_decaps(ss_bad, ct_bad, sk);
    if (memcmp(ss_enc, ss_bad, 32) == 0) {
      fprintf(stderr, "FAIL: implicit rejection\n");
      return 1;
    }
    uint8_t ss_bad2[32];
    fio_mlkem768_decaps(ss_bad2, ct_bad, sk);
    if (memcmp(ss_bad, ss_bad2, 32) != 0) {
      fprintf(stderr, "FAIL: implicit rejection not deterministic\n");
      return 1;
    }
    printf("  implicit rejection: OK\n");
  }

  /* Test 4: Multiple round-trips */
  for (int trial = 0; trial < 100; trial++) {
    fio_mlkem768_keypair(pk, sk);
    fio_mlkem768_encaps(ct, ss_enc, pk);
    fio_mlkem768_decaps(ss_dec, ct, sk);
    if (memcmp(ss_enc, ss_dec, 32) != 0) {
      fprintf(stderr, "FAIL: round-trip trial %d\n", trial);
      return 1;
    }
  }
  printf("  100 random round-trips: OK\n");

  printf("ALL TESTS PASSED\n");
  return 0;
}
