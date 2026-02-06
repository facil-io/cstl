#define FIO_LOG
#define FIO_MLKEM
#define FIO_SHA3
#define FIO_SHA2
#define FIO_ED25519
#include "../fio-stl/include.h"

#include <stdio.h>
#include <string.h>

/* Helper to convert hex string to bytes */
static void hex_to_bytes(const char *hex, uint8_t *out, size_t len) {
  for (size_t i = 0; i < len; i++) {
    unsigned int val;
    sscanf(hex + 2 * i, "%02x", &val);
    out[i] = (uint8_t)val;
  }
}

/* NIST ACVP ML-KEM-768 KeyGen test vector (tcId: 26) */
static const char *acvp_keygen_d =
    "A2B4BCA315A6EA4600B4A316E09A2578AA1E8BCE919C8DF3A96C71C843F5B38B";
static const char *acvp_keygen_z =
    "D6BF055CB7B375E3271ED131F1BA31F83FEF533A239878A71074578B891265D1";
/* Expected ek (first 48 hex chars for prefix verification) */
static const char *acvp_keygen_ek_prefix =
    "5219C4CC17C35A828F3E21B2AB7496805C99EE041FCA0158";
/* Expected dk (last 64 hex chars = z value at end of secret key) */
static const char *acvp_keygen_dk_suffix =
    "D6BF055CB7B375E3271ED131F1BA31F83FEF533A239878A71074578B891265D1";

/* NIST ACVP ML-KEM-768 KeyGen test vector (tcId: 27) */
static const char *acvp_keygen2_d =
    "6DBB99AE6889AF01DA387D7D99BD4E91BACB11A6051B14AECD4C96F30CD9F9D9";
static const char *acvp_keygen2_z =
    "360557CADDFCF5FEE7C0DE6A363F095757588C35A3FD11C58677AB5E8797C2B8";
static const char *acvp_keygen2_ek_prefix =
    "BC02284C2B36002A8E002311D6FA3ED17088D6157E76867E";

/* NIST ACVP ML-KEM-768 Encapsulation test vector (tcId: 26, tgId: 2) */
/* Input: ek (public key) - first 48 hex chars shown */
static const char *acvp_encap_ek_prefix =
    "F255CE47334283B8622BE7CE76D7354E3C4FE3F6C44F6BB2";
/* Input: m (32-byte randomness) */
static const char *acvp_encap_m =
    "5BD922AF345AB90F297D0A82EA39527A648E4977AB56242E2AC0ED9A2CC66F10";
/* Expected output: k (32-byte shared secret) */
static const char *acvp_encap_k =
    "B2425299020BCF563B8EBE0512F0479941335A75A32B8D10BFF60E5548B64672";
/* Expected output: c (ciphertext) - first 48 hex chars */
static const char *acvp_encap_c_prefix =
    "4EE24D9E0858B36DC755A9389F4FDBF438DB8FBFDDD2E2A4";

/* Full ek for encap test (1184 bytes = 2368 hex chars) */
static const char *acvp_encap_ek_full =
    "F255CE47334283B8622BE7CE76D7354E3C4FE3F6C44F6BB25C9864EE0BAEB576"
    "5950D88F438263CE8B5A7A4C0FC4C95F10C477A7521F9BB458B8AA55D2E43BDC"
    "86B72F0930EE428B4C5A9C7116310F2AA5CB03AC1603C811959EA9012D69CBCE"
    "40B37CD890999CC74FF375C66F048B240363343CB795998856D560F4C712938C"
    "79466864D20B0BE95419C9EA6A8E7203A1986D10B606691242CEF630941B1164"
    "58A41C83B7DC5B06A97C840B116F2CE9CFA87A1C1AA8C4FAC137DE8498E8749B"
    "3638404271539B247183A32E7E4413B6400E0F295788084EEA93B4A765334100"
    "5672D908C62B64B11B48414B505F3036EE56CC4DA88FEF27B2DA974C9DD38C15"
    "0090B5B8A29BD7C5975A8A959549044B4DAED52A7FA68335308F40C9B768C582"
    "1F78CF068A694978964F597408D09759A19578624C64DC18EAB23082E599EC48"
    "8DFE016E4BA58977E15B715C612496310219B9B4775CB51C5DF03B934F7473AA"
    "58A57C602CF17C5993D30F52D753AC56BACA1A994742BC50435E179A262B3C8E"
    "ECE1513955C593E7508B945F6E95CC4268CBD45B2504082FB8B23D8906946A74"
    "AC2FB676BDBC39DF76B9B8450F49D283C622784565B76B96084DFC099EC2279E"
    "5BC13492561B4439E32324B0050C5FE6451974BF0D72750AC58BAC046D218AC3"
    "97F65532ACC7800246ED1C8094FC807306BF88E2816AD13B06F2898CA87C486A"
    "124B618156A090B1058722ABAE389AB5612CA2C2766DDEF98202A6AB1097B392"
    "404EA151788528B07544325F851B4DEAA2495138F929BBB4026042B0A8CD3CB0"
    "A7D061927A717D4877E0D9A409D6B125361C99090AFDF922A776ACADA2B6A845"
    "22134B089D4B428020C83061A87816C6A59263E636B5B2ECBCA6A64E29600948"
    "D5B0B45600B8D473A65B450B766D0251B6915898BC3C1C2C53B9679121F1F06C"
    "FB9604DE0051FF4B093939C907AB18C2988646A90481BB99F4153611C138BE34"
    "BE163B3ABAC44354A774E9CB54FB29903367C78D275467499D22E83A11CA9B84"
    "45BE9DF3CB612069222A8715A495D115B4BC2457AB731AE7EC1BD8EC9722CA98"
    "0958180AC2BD67898F4A72A675106D66981B2E923C0BA40E3234655D00B25D64"
    "62591C9C9C7A53491489D57A77B2510D08B95B9C61C1784BA752F4A73023742E"
    "CB985DFB37808B16D6C283CD4A06C5A3AC401855E1DABE63F9668BF7A661946B"
    "18230A1A5A7C19DA66ED08151E77A624F579D4E44ABE023A1CD33459FCC3F1A6"
    "589426634D062D0A75A387A0B7B8D802A66B2106E01264500915B97307C85ECF"
    "331BCAC35E4AA243C837876D858AFA8B510C342708B38093B2CD35D1BA68DA05"
    "44794D172C6CA8850A7F847B56998D8E0B0A17144FB6F443E3679767CA91B80A"
    "6CAA8BB0E22BBAC01C0EAE1604B8A243911672B3748C7F18C531E3783D522039"
    "130057198D6F0989E99641AB718DA123710BDB67B3B75EC66BA9CF459FE06C7C"
    "4F7959DD7281FF155940B09FB14AA55CD40B963CA3312C05B36A5207C989428C"
    "16E5D288ADB18A66F74617CA39DB8AA612D706DFEC884C457AECD1AAB598195B"
    "4AC971529FB7A883492235E62112064A0F6FF5BF4F1619A0D03B96B5112009966"
    "B2DF7C2F300B6F295DF7FA2C453E1949DF6405309DF7575C7656C245EDCA9F6";

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

  /* Test 5: NIST ACVP KeyGen test vector (tcId 26, ML-KEM-768) */
  {
    uint8_t d[32], z[32], coins[64];
    uint8_t ek[1184], dk[2400];
    uint8_t expected_ek_prefix[24], expected_dk_suffix[32];

    hex_to_bytes(acvp_keygen_d, d, 32);
    hex_to_bytes(acvp_keygen_z, z, 32);
    hex_to_bytes(acvp_keygen_ek_prefix, expected_ek_prefix, 24);
    hex_to_bytes(acvp_keygen_dk_suffix, expected_dk_suffix, 32);

    /* coins = d || z per FIPS 203 */
    memcpy(coins, d, 32);
    memcpy(coins + 32, z, 32);

    fio_mlkem768_keypair_derand(ek, dk, coins);

    /* Verify ek prefix matches */
    if (memcmp(ek, expected_ek_prefix, 24) != 0) {
      fprintf(stderr, "FAIL: ACVP keygen ek prefix mismatch\n");
      fprintf(stderr, "  Got:      ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", ek[i]);
      fprintf(stderr, "\n  Expected: ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", expected_ek_prefix[i]);
      fprintf(stderr, "\n");
      return 1;
    }

    /* Verify dk suffix (z) matches — z is at end of dk per FIPS 203 sk format
     */
    if (memcmp(dk + 2400 - 32, expected_dk_suffix, 32) != 0) {
      fprintf(stderr, "FAIL: ACVP keygen dk suffix (z) mismatch\n");
      return 1;
    }

    printf("  ACVP keygen tcId 26: OK\n");
  }

  /* Test 6: NIST ACVP KeyGen test vector (tcId 27, ML-KEM-768) */
  {
    uint8_t d[32], z[32], coins[64];
    uint8_t ek[1184], dk[2400];
    uint8_t expected_ek_prefix[24];

    hex_to_bytes(acvp_keygen2_d, d, 32);
    hex_to_bytes(acvp_keygen2_z, z, 32);
    hex_to_bytes(acvp_keygen2_ek_prefix, expected_ek_prefix, 24);

    memcpy(coins, d, 32);
    memcpy(coins + 32, z, 32);

    fio_mlkem768_keypair_derand(ek, dk, coins);

    if (memcmp(ek, expected_ek_prefix, 24) != 0) {
      fprintf(stderr, "FAIL: ACVP keygen tcId 27 ek prefix mismatch\n");
      fprintf(stderr, "  Got:      ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", ek[i]);
      fprintf(stderr, "\n  Expected: ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", expected_ek_prefix[i]);
      fprintf(stderr, "\n");
      return 1;
    }

    /* Also verify z is at end of dk */
    uint8_t expected_z[32];
    hex_to_bytes(acvp_keygen2_z, expected_z, 32);
    if (memcmp(dk + 2400 - 32, expected_z, 32) != 0) {
      fprintf(stderr, "FAIL: ACVP keygen tcId 27 dk suffix (z) mismatch\n");
      return 1;
    }

    printf("  ACVP keygen tcId 27: OK\n");
  }

  /* Test 7: NIST ACVP Encapsulation test vector (tcId 26, ML-KEM-768) */
  {
    uint8_t ek[1184], m[32];
    uint8_t expected_k[32], expected_c_prefix[24];
    uint8_t acvp_ct[1088], acvp_ss[32];

    /* Parse the full public key */
    hex_to_bytes(acvp_encap_ek_full, ek, 1184);
    hex_to_bytes(acvp_encap_m, m, 32);
    hex_to_bytes(acvp_encap_k, expected_k, 32);
    hex_to_bytes(acvp_encap_c_prefix, expected_c_prefix, 24);

    /* Verify ek prefix matches what we expect */
    uint8_t expected_ek_prefix[24];
    hex_to_bytes(acvp_encap_ek_prefix, expected_ek_prefix, 24);
    if (memcmp(ek, expected_ek_prefix, 24) != 0) {
      fprintf(stderr, "FAIL: ACVP encap ek parse error\n");
      return 1;
    }

    /* Deterministic encapsulation */
    fio_mlkem768_encaps_derand(acvp_ct, acvp_ss, ek, m);

    /* Verify ciphertext prefix */
    if (memcmp(acvp_ct, expected_c_prefix, 24) != 0) {
      fprintf(stderr, "FAIL: ACVP encap ciphertext prefix mismatch\n");
      fprintf(stderr, "  Got:      ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", acvp_ct[i]);
      fprintf(stderr, "\n  Expected: ");
      for (int i = 0; i < 24; i++)
        fprintf(stderr, "%02X", expected_c_prefix[i]);
      fprintf(stderr, "\n");
      return 1;
    }

    /* Verify shared secret */
    if (memcmp(acvp_ss, expected_k, 32) != 0) {
      fprintf(stderr, "FAIL: ACVP encap shared secret mismatch\n");
      fprintf(stderr, "  Got:      ");
      for (int i = 0; i < 32; i++)
        fprintf(stderr, "%02X", acvp_ss[i]);
      fprintf(stderr, "\n  Expected: ");
      for (int i = 0; i < 32; i++)
        fprintf(stderr, "%02X", expected_k[i]);
      fprintf(stderr, "\n");
      return 1;
    }

    printf("  ACVP encap tcId 26: OK\n");
  }

  /* =========================================================================
   * X25519MLKEM768 Hybrid Tests
   * =========================================================================
   */
  printf("\nX25519MLKEM768 hybrid tests...\n");

  /* Test 8: Basic hybrid round-trip */
  {
    uint8_t hybrid_pk[1216], hybrid_sk[2432];
    uint8_t hybrid_ct[1120], hybrid_ss_enc[64], hybrid_ss_dec[64];

    if (fio_x25519mlkem768_keypair(hybrid_pk, hybrid_sk) != 0) {
      fprintf(stderr, "FAIL: hybrid keypair generation\n");
      return 1;
    }
    if (fio_x25519mlkem768_encaps(hybrid_ct, hybrid_ss_enc, hybrid_pk) != 0) {
      fprintf(stderr, "FAIL: hybrid encapsulation\n");
      return 1;
    }
    if (fio_x25519mlkem768_decaps(hybrid_ss_dec, hybrid_ct, hybrid_sk) != 0) {
      fprintf(stderr, "FAIL: hybrid decapsulation\n");
      return 1;
    }
    if (memcmp(hybrid_ss_enc, hybrid_ss_dec, 64) != 0) {
      fprintf(stderr, "FAIL: hybrid shared secrets don't match\n");
      fprintf(stderr, "  encaps ss: ");
      for (int i = 0; i < 64; i++)
        fprintf(stderr, "%02X", hybrid_ss_enc[i]);
      fprintf(stderr, "\n  decaps ss: ");
      for (int i = 0; i < 64; i++)
        fprintf(stderr, "%02X", hybrid_ss_dec[i]);
      fprintf(stderr, "\n");
      return 1;
    }
    printf("  hybrid round-trip: OK\n");
  }

  /* Test 8: Multiple hybrid round-trips */
  for (int trial = 0; trial < 50; trial++) {
    uint8_t hybrid_pk[1216], hybrid_sk[2432];
    uint8_t hybrid_ct[1120], hybrid_ss_enc[64], hybrid_ss_dec[64];

    fio_x25519mlkem768_keypair(hybrid_pk, hybrid_sk);
    fio_x25519mlkem768_encaps(hybrid_ct, hybrid_ss_enc, hybrid_pk);
    fio_x25519mlkem768_decaps(hybrid_ss_dec, hybrid_ct, hybrid_sk);
    if (memcmp(hybrid_ss_enc, hybrid_ss_dec, 64) != 0) {
      fprintf(stderr, "FAIL: hybrid round-trip trial %d\n", trial);
      return 1;
    }
  }
  printf("  50 hybrid round-trips: OK\n");

  /* Test 9: Verify hybrid key sizes */
  {
    if (FIO_X25519MLKEM768_PUBLICKEYBYTES != 1216) {
      fprintf(stderr, "FAIL: public key size != 1216\n");
      return 1;
    }
    if (FIO_X25519MLKEM768_SECRETKEYBYTES != 2432) {
      fprintf(stderr, "FAIL: secret key size != 2432\n");
      return 1;
    }
    if (FIO_X25519MLKEM768_CIPHERTEXTBYTES != 1120) {
      fprintf(stderr, "FAIL: ciphertext size != 1120\n");
      return 1;
    }
    if (FIO_X25519MLKEM768_SSBYTES != 64) {
      fprintf(stderr, "FAIL: shared secret size != 64\n");
      return 1;
    }
    printf("  hybrid sizes: OK\n");
  }

  /* Test 10: Hybrid shared secret has both X25519 and ML-KEM components
   * Layout per IETF draft-ietf-tls-ecdhe-mlkem-03 (ML-KEM FIRST):
   *   ct = ML-KEM-768_ct[0..1087] || X25519_eph_pk[1088..1119]
   *   sk = ML-KEM-768_dk[0..2399] || X25519_sk[2400..2431]
   *   ss = ML-KEM-768_ss[0..31] || X25519_ss[32..63]
   */
  {
    uint8_t hybrid_pk[1216], hybrid_sk[2432];
    uint8_t hybrid_ct[1120], hybrid_ss[64];

    fio_x25519mlkem768_keypair(hybrid_pk, hybrid_sk);
    fio_x25519mlkem768_encaps(hybrid_ct, hybrid_ss, hybrid_pk);

    /* Verify ML-KEM component independently */
    uint8_t mlkem_ss[32];
    /* ct[0..1087] is ML-KEM ciphertext, sk[0..2399] is ML-KEM secret key */
    fio_mlkem768_decaps(mlkem_ss, hybrid_ct, hybrid_sk);
    if (memcmp(hybrid_ss, mlkem_ss, 32) != 0) {
      fprintf(stderr, "FAIL: ML-KEM component mismatch\n");
      return 1;
    }

    /* Verify X25519 component independently */
    uint8_t x25519_ss[32];
    /* ct[1088..1119] is ephemeral X25519 public key, sk[2400..2431] is X25519
     * private */
    if (fio_x25519_shared_secret(x25519_ss,
                                 hybrid_sk + 2400,
                                 hybrid_ct + 1088) != 0) {
      fprintf(stderr, "FAIL: couldn't compute X25519 shared secret\n");
      return 1;
    }
    if (memcmp(hybrid_ss + 32, x25519_ss, 32) != 0) {
      fprintf(stderr, "FAIL: X25519 component mismatch\n");
      return 1;
    }

    printf("  hybrid component verification: OK\n");
  }

  /* Test 11: Modified hybrid ciphertext fails gracefully
   * Layout per IETF: ss = ML-KEM-768_ss[0..31] || X25519_ss[32..63]
   */
  {
    uint8_t hybrid_pk[1216], hybrid_sk[2432];
    uint8_t hybrid_ct[1120], hybrid_ss_enc[64], hybrid_ss_dec[64];

    fio_x25519mlkem768_keypair(hybrid_pk, hybrid_sk);
    fio_x25519mlkem768_encaps(hybrid_ct, hybrid_ss_enc, hybrid_pk);

    /* Corrupt ML-KEM portion of ciphertext (first 1088 bytes) */
    hybrid_ct[500] ^= 0xFF;

    /* Decapsulation should still "succeed" but produce different shared secret
     * (ML-KEM uses implicit rejection) */
    int ret = fio_x25519mlkem768_decaps(hybrid_ss_dec, hybrid_ct, hybrid_sk);
    if (ret != 0) {
      fprintf(stderr, "FAIL: hybrid decaps returned error on corrupted ct\n");
      return 1;
    }
    /* The ML-KEM portion (ss[0..31]) should NOT match (implicit rejection) */
    if (memcmp(hybrid_ss_enc, hybrid_ss_dec, 32) == 0) {
      fprintf(stderr, "FAIL: ML-KEM portion matched on corrupted ciphertext\n");
      return 1;
    }
    /* The X25519 portion (ss[32..63]) should still match */
    if (memcmp(hybrid_ss_enc + 32, hybrid_ss_dec + 32, 32) != 0) {
      fprintf(stderr, "FAIL: X25519 portion changed on ML-KEM corruption\n");
      return 1;
    }
    printf("  hybrid implicit rejection: OK\n");
  }

  printf("\nALL TESTS PASSED\n");
  return 0;
}
