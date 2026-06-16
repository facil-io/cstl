/* *****************************************************************************
Test for 156 mlkem.h

Coverage: ML-KEM-768 keypair generation, deterministic keypair generation,
encapsulation/decapsulation round-trips, implicit rejection for malformed
ciphertexts, NIST ACVP known-answer vectors, and X25519MLKEM768 hybrid
round-trips with component verification. Performance loops are intentionally
omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MLKEM
#define FIO_SHA3
#define FIO_SHA2
#define FIO_ED25519
#include FIO_INCLUDE_FILE

/* Helper: parse a hex string to bytes (no sscanf, validates every nibble). */
static void fio___test_hex2bin(uint8_t *out, const char *hex, size_t out_len) {
  for (size_t i = 0; i < out_len; ++i) {
    uint8_t hi = fio_c2i((unsigned char)hex[2 * i]);
    uint8_t lo = fio_c2i((unsigned char)hex[2 * i + 1]);
    FIO_ASSERT(hi < 16 && lo < 16, "invalid hex character in test vector");
    out[i] = (uint8_t)((hi << 4) | lo);
  }
}

/* *****************************************************************************
NIST ACVP ML-KEM-768 KeyGen test vector (tcId: 26)
***************************************************************************** */
static const char *acvp_keygen_d =
    "A2B4BCA315A6EA4600B4A316E09A2578AA1E8BCE919C8DF3A96C71C843F5B38B";
static const char *acvp_keygen_z =
    "D6BF055CB7B375E3271ED131F1BA31F83FEF533A239878A71074578B891265D1";
static const char *acvp_keygen_ek_prefix =
    "5219C4CC17C35A828F3E21B2AB7496805C99EE041FCA0158";
static const char *acvp_keygen2_d =
    "6DBB99AE6889AF01DA387D7D99BD4E91BACB11A6051B14AECD4C96F30CD9F9D9";
static const char *acvp_keygen2_z =
    "360557CADDFCF5FEE7C0DE6A363F095757588C35A3FD11C58677AB5E8797C2B8";
static const char *acvp_keygen2_ek_prefix =
    "BC02284C2B36002A8E002311D6FA3ED17088D6157E76867E";

/* NIST ACVP ML-KEM-768 Encapsulation test vector (tcId: 26). */
static const char *acvp_encap_m =
    "5BD922AF345AB90F297D0A82EA39527A648E4977AB56242E2AC0ED9A2CC66F10";
static const char *acvp_encap_k =
    "B2425299020BCF563B8EBE0512F0479941335A75A32B8D10BFF60E5548B64672";
static const char *acvp_encap_c_prefix =
    "4EE24D9E0858B36DC755A9389F4FDBF438DB8FBFDDD2E2A4";
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

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_roundtrip)(void) {
  uint8_t pk[1184], sk[2400];
  uint8_t ct[1088], ss_enc[32], ss_dec[32];

  FIO_ASSERT(!fio_mlkem768_keypair(pk, sk), "ML-KEM keypair generation failed");
  FIO_ASSERT(!fio_mlkem768_encaps(ct, ss_enc, pk),
             "ML-KEM encapsulation failed");
  FIO_ASSERT(!fio_mlkem768_decaps(ss_dec, ct, sk),
             "ML-KEM decapsulation failed");
  FIO_ASSERT(!FIO_MEMCMP(ss_enc, ss_dec, 32),
             "ML-KEM shared secret mismatch");
}

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_deterministic)(void) {
  uint8_t coins[64], enc_coins[32];
  uint8_t pk1[1184], sk1[2400], ct1[1088], ss1[32];
  uint8_t pk2[1184], sk2[2400], ct2[1088], ss2[32];
  FIO_MEMSET(coins, 0x42, 64);
  FIO_MEMSET(enc_coins, 0x37, 32);

  FIO_ASSERT(!fio_mlkem768_keypair_derand(pk1, sk1, coins),
             "deterministic ML-KEM keypair failed");
  FIO_ASSERT(!fio_mlkem768_keypair_derand(pk2, sk2, coins),
             "deterministic ML-KEM keypair failed");
  FIO_ASSERT(!FIO_MEMCMP(pk1, pk2, 1184), "deterministic ML-KEM pk mismatch");
  FIO_ASSERT(!FIO_MEMCMP(sk1, sk2, 2400), "deterministic ML-KEM sk mismatch");

  FIO_ASSERT(!fio_mlkem768_encaps_derand(ct1, ss1, pk1, enc_coins),
             "deterministic ML-KEM encaps failed");
  FIO_ASSERT(!fio_mlkem768_encaps_derand(ct2, ss2, pk2, enc_coins),
             "deterministic ML-KEM encaps failed");
  FIO_ASSERT(!FIO_MEMCMP(ct1, ct2, 1088),
             "deterministic ML-KEM ct mismatch");
  FIO_ASSERT(!FIO_MEMCMP(ss1, ss2, 32),
             "deterministic ML-KEM ss mismatch");

  uint8_t ss_d[32];
  FIO_ASSERT(!fio_mlkem768_decaps(ss_d, ct1, sk1),
             "deterministic ML-KEM decaps failed");
  FIO_ASSERT(!FIO_MEMCMP(ss1, ss_d, 32),
             "deterministic ML-KEM decaps mismatch");
}

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_implicit_rejection)(void) {
  uint8_t pk[1184], sk[2400];
  uint8_t ct[1088], ss_enc[32], ss_bad[32], ss_bad2[32];

  FIO_ASSERT(!fio_mlkem768_keypair(pk, sk), "ML-KEM keypair generation failed");
  FIO_ASSERT(!fio_mlkem768_encaps(ct, ss_enc, pk),
             "ML-KEM encapsulation failed");

  ct[0] ^= 0xFF;
  FIO_ASSERT(!fio_mlkem768_decaps(ss_bad, ct, sk),
             "ML-KEM decaps on corrupted ct failed");
  FIO_ASSERT(FIO_MEMCMP(ss_enc, ss_bad, 32),
             "ML-KEM implicit rejection did not change shared secret");

  FIO_ASSERT(!fio_mlkem768_decaps(ss_bad2, ct, sk),
             "ML-KEM second decaps on corrupted ct failed");
  FIO_ASSERT(!FIO_MEMCMP(ss_bad, ss_bad2, 32),
             "ML-KEM implicit rejection is not deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_acvp_keygen)(void) {
  uint8_t d[32], z[32], coins[64];
  uint8_t ek[1184], dk[2400];
  uint8_t expected_ek_prefix[24];

  fio___test_hex2bin(d, acvp_keygen_d, 32);
  fio___test_hex2bin(z, acvp_keygen_z, 32);
  fio___test_hex2bin(expected_ek_prefix, acvp_keygen_ek_prefix, 24);
  FIO_MEMCPY(coins, d, 32);
  FIO_MEMCPY(coins + 32, z, 32);

  FIO_ASSERT(!fio_mlkem768_keypair_derand(ek, dk, coins),
             "ACVP keygen keypair_derand failed");
  FIO_ASSERT(!FIO_MEMCMP(ek, expected_ek_prefix, 24),
             "ACVP keygen ek prefix mismatch (tcId 26)");
  FIO_ASSERT(!FIO_MEMCMP(dk + 2400 - 32, z, 32),
             "ACVP keygen dk suffix mismatch (tcId 26)");

  fio___test_hex2bin(d, acvp_keygen2_d, 32);
  fio___test_hex2bin(z, acvp_keygen2_z, 32);
  fio___test_hex2bin(expected_ek_prefix, acvp_keygen2_ek_prefix, 24);
  FIO_MEMCPY(coins, d, 32);
  FIO_MEMCPY(coins + 32, z, 32);

  FIO_ASSERT(!fio_mlkem768_keypair_derand(ek, dk, coins),
             "ACVP keygen keypair_derand failed (tcId 27)");
  FIO_ASSERT(!FIO_MEMCMP(ek, expected_ek_prefix, 24),
             "ACVP keygen ek prefix mismatch (tcId 27)");
  FIO_ASSERT(!FIO_MEMCMP(dk + 2400 - 32, z, 32),
             "ACVP keygen dk suffix mismatch (tcId 27)");
}

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_acvp_encap)(void) {
  uint8_t ek[1184], m[32];
  uint8_t expected_k[32], expected_c_prefix[24];
  uint8_t acvp_ct[1088], acvp_ss[32];

  fio___test_hex2bin(ek, acvp_encap_ek_full, 1184);
  fio___test_hex2bin(m, acvp_encap_m, 32);
  fio___test_hex2bin(expected_k, acvp_encap_k, 32);
  fio___test_hex2bin(expected_c_prefix, acvp_encap_c_prefix, 24);

  FIO_ASSERT(!fio_mlkem768_encaps_derand(acvp_ct, acvp_ss, ek, m),
             "ACVP encaps failed");
  FIO_ASSERT(!FIO_MEMCMP(acvp_ct, expected_c_prefix, 24),
             "ACVP encaps ciphertext prefix mismatch");
  FIO_ASSERT(!FIO_MEMCMP(acvp_ss, expected_k, 32),
             "ACVP encaps shared secret mismatch");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x25519mlkem768_roundtrip)(void) {
  uint8_t pk[1216], sk[2432];
  uint8_t ct[1120], ss_enc[64], ss_dec[64];

  FIO_ASSERT(!fio_x25519mlkem768_keypair(pk, sk),
             "hybrid keypair generation failed");
  FIO_ASSERT(!fio_x25519mlkem768_encaps(ct, ss_enc, pk),
             "hybrid encapsulation failed");
  FIO_ASSERT(!fio_x25519mlkem768_decaps(ss_dec, ct, sk),
             "hybrid decapsulation failed");
  FIO_ASSERT(!FIO_MEMCMP(ss_enc, ss_dec, 64),
             "hybrid shared secret mismatch");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x25519mlkem768_components)(void) {
  uint8_t pk[1216], sk[2432];
  uint8_t ct[1120], ss[64];
  uint8_t mlkem_ss[32], x25519_ss[32];

  FIO_ASSERT(FIO_X25519MLKEM768_PUBLICKEYBYTES == 1216,
             "hybrid public key size mismatch");
  FIO_ASSERT(FIO_X25519MLKEM768_SECRETKEYBYTES == 2432,
             "hybrid secret key size mismatch");
  FIO_ASSERT(FIO_X25519MLKEM768_CIPHERTEXTBYTES == 1120,
             "hybrid ciphertext size mismatch");
  FIO_ASSERT(FIO_X25519MLKEM768_SSBYTES == 64,
             "hybrid shared secret size mismatch");

  FIO_ASSERT(!fio_x25519mlkem768_keypair(pk, sk),
             "hybrid keypair generation failed");
  FIO_ASSERT(!fio_x25519mlkem768_encaps(ct, ss, pk),
             "hybrid encapsulation failed");

  /* ML-KEM component: ct[0..1087], sk[0..2399]. */
  FIO_ASSERT(!fio_mlkem768_decaps(mlkem_ss, ct, sk),
             "hybrid ML-KEM component decaps failed");
  FIO_ASSERT(!FIO_MEMCMP(ss, mlkem_ss, 32),
             "hybrid ML-KEM component mismatch");

  /* X25519 component: ct[1088..1119], sk[2400..2431]. */
  FIO_ASSERT(!fio_x25519_shared_secret(x25519_ss, sk + 2400, ct + 1088),
             "hybrid X25519 shared secret failed");
  FIO_ASSERT(!FIO_MEMCMP(ss + 32, x25519_ss, 32),
             "hybrid X25519 component mismatch");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x25519mlkem768_implicit_rejection)(void) {
  uint8_t pk[1216], sk[2432];
  uint8_t ct[1120], ss_enc[64], ss_dec[64];

  FIO_ASSERT(!fio_x25519mlkem768_keypair(pk, sk),
             "hybrid keypair generation failed");
  FIO_ASSERT(!fio_x25519mlkem768_encaps(ct, ss_enc, pk),
             "hybrid encapsulation failed");

  /* Corrupt the ML-KEM portion of the ciphertext. */
  ct[500] ^= 0xFF;
  FIO_ASSERT(!fio_x25519mlkem768_decaps(ss_dec, ct, sk),
             "hybrid decaps on corrupted ct failed");
  FIO_ASSERT(FIO_MEMCMP(ss_enc, ss_dec, 32),
             "hybrid ML-KEM portion matched on corrupted ciphertext");
  FIO_ASSERT(!FIO_MEMCMP(ss_enc + 32, ss_dec + 32, 32),
             "hybrid X25519 portion changed on ML-KEM corruption");
}

FIO_SFUNC void FIO_NAME_TEST(stl, mlkem_null_args)(void) {
  uint8_t pk[1184], sk[2400], ct[1088], ss[32];
  uint8_t coins[64] = {0};

  FIO_ASSERT(fio_mlkem768_keypair(NULL, sk) == -1,
             "keypair should reject NULL pk");
  FIO_ASSERT(fio_mlkem768_keypair(pk, NULL) == -1,
             "keypair should reject NULL sk");

  FIO_ASSERT(fio_mlkem768_keypair_derand(NULL, sk, coins) == -1,
             "keypair_derand should reject NULL pk");
  FIO_ASSERT(fio_mlkem768_keypair_derand(pk, NULL, coins) == -1,
             "keypair_derand should reject NULL sk");
  FIO_ASSERT(fio_mlkem768_keypair_derand(pk, sk, NULL) == -1,
             "keypair_derand should reject NULL coins");

  FIO_ASSERT(fio_mlkem768_encaps(NULL, ss, pk) == -1,
             "encaps should reject NULL ct");
  FIO_ASSERT(fio_mlkem768_encaps(ct, NULL, pk) == -1,
             "encaps should reject NULL ss");
  FIO_ASSERT(fio_mlkem768_encaps(ct, ss, NULL) == -1,
             "encaps should reject NULL pk");

  FIO_ASSERT(fio_mlkem768_encaps_derand(NULL, ss, pk, coins) == -1,
             "encaps_derand should reject NULL ct");
  FIO_ASSERT(fio_mlkem768_encaps_derand(ct, NULL, pk, coins) == -1,
             "encaps_derand should reject NULL ss");
  FIO_ASSERT(fio_mlkem768_encaps_derand(ct, ss, NULL, coins) == -1,
             "encaps_derand should reject NULL pk");
  FIO_ASSERT(fio_mlkem768_encaps_derand(ct, ss, pk, NULL) == -1,
             "encaps_derand should reject NULL coins");

  FIO_ASSERT(fio_mlkem768_decaps(NULL, ct, sk) == -1,
             "decaps should reject NULL ss");
  FIO_ASSERT(fio_mlkem768_decaps(ss, NULL, sk) == -1,
             "decaps should reject NULL ct");
  FIO_ASSERT(fio_mlkem768_decaps(ss, ct, NULL) == -1,
             "decaps should reject NULL sk");

  uint8_t hpk[1216], hsk[2432], hct[1120], hss[64];
  FIO_ASSERT(fio_x25519mlkem768_keypair(NULL, hsk) == -1,
             "hybrid keypair should reject NULL pk");
  FIO_ASSERT(fio_x25519mlkem768_keypair(hpk, NULL) == -1,
             "hybrid keypair should reject NULL sk");

  FIO_ASSERT(fio_x25519mlkem768_encaps(NULL, hss, hpk) == -1,
             "hybrid encaps should reject NULL ct");
  FIO_ASSERT(fio_x25519mlkem768_encaps(hct, NULL, hpk) == -1,
             "hybrid encaps should reject NULL ss");
  FIO_ASSERT(fio_x25519mlkem768_encaps(hct, hss, NULL) == -1,
             "hybrid encaps should reject NULL pk");

  FIO_ASSERT(fio_x25519mlkem768_decaps(NULL, hct, hsk) == -1,
             "hybrid decaps should reject NULL ss");
  FIO_ASSERT(fio_x25519mlkem768_decaps(hss, NULL, hsk) == -1,
             "hybrid decaps should reject NULL ct");
  FIO_ASSERT(fio_x25519mlkem768_decaps(hss, hct, NULL) == -1,
             "hybrid decaps should reject NULL sk");
}

int main(void) {
  FIO_NAME_TEST(stl, mlkem_roundtrip)();
  FIO_NAME_TEST(stl, mlkem_deterministic)();
  FIO_NAME_TEST(stl, mlkem_implicit_rejection)();
  FIO_NAME_TEST(stl, mlkem_acvp_keygen)();
  FIO_NAME_TEST(stl, mlkem_acvp_encap)();
  FIO_NAME_TEST(stl, x25519mlkem768_roundtrip)();
  FIO_NAME_TEST(stl, x25519mlkem768_components)();
  FIO_NAME_TEST(stl, x25519mlkem768_implicit_rejection)();
  FIO_NAME_TEST(stl, mlkem_null_args)();
  return 0;
}
