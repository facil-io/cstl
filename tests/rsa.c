/* *****************************************************************************
Test for 155 rsa.h

Coverage: RSA PKCS#1 v1.5 and RSA-PSS signature verification and signing,
including OpenSSL-CLI known-answer vectors and malformed-key rejection.
Uses OpenSSL as a library to generate a real 2048-bit RSA keypair and to
verify signatures produced by fio, and vice versa. Also includes edge-case
and invalid-input assertions ported from the archived tests.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MATH
#define FIO_SHA2
#define FIO_RAND
#define FIO_RSA
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

/* *****************************************************************************
Hex conversion helper
***************************************************************************** */
FIO_SFUNC size_t fio___rsa_test_hex2bin(uint8_t *out,
                                        size_t out_len,
                                        const char *hex) {
  size_t len = 0;
  while (*hex && len < out_len) {
    int hi = 0, lo = 0;
    if (*hex >= '0' && *hex <= '9')
      hi = *hex - '0';
    else if (*hex >= 'a' && *hex <= 'f')
      hi = *hex - 'a' + 10;
    else if (*hex >= 'A' && *hex <= 'F')
      hi = *hex - 'A' + 10;
    else {
      ++hex;
      continue;
    }
    ++hex;
    if (!*hex)
      break;
    if (*hex >= '0' && *hex <= '9')
      lo = *hex - '0';
    else if (*hex >= 'a' && *hex <= 'f')
      lo = *hex - 'a' + 10;
    else if (*hex >= 'A' && *hex <= 'F')
      lo = *hex - 'A' + 10;
    ++hex;
    out[len++] = (uint8_t)((hi << 4) | lo);
  }
  return len;
}

/* *****************************************************************************
Byte/word conversion helpers (archived coverage)
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, rsa_byte_conversion)(void) {
  uint8_t bytes[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
  uint64_t words[2];

  fio___rsa_bytes_to_words(words, 2, bytes, 16);
  FIO_ASSERT(words[0] == 0x090A0B0C0D0E0F10ULL,
             "RSA LSW conversion failed");
  FIO_ASSERT(words[1] == 0x0102030405060708ULL,
             "RSA MSW conversion failed");

  uint8_t bytes2[16];
  fio___rsa_words_to_bytes(bytes2, 16, words, 2);
  FIO_ASSERT(!FIO_MEMCMP(bytes, bytes2, 16), "RSA round-trip conversion failed");
}

/* *****************************************************************************
Static RSA key for tests that must not depend on OpenSSL runtime generation.
This is the well-known 2048-bit test modulus used in the archived rsa.c tests.
It is NOT a valid keypair for signing, so it is used only for parse/operation
and invalid-signature checks.
***************************************************************************** */
static const char *fio___rsa_test_n_hex =
    "b3510a2bcd4ce644c5b594ae5059e12b2f054b658d5da5959a2fdf1871b808bc"
    "3df3e628d2792e51aad5c124b43bda453cd95f3de3c4b5bfb26c5c9547c9b0c1"
    "b3cd6f8a9c7c1f1b9c9f8b3a9f8d5a7e9b3c5a7e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a01";
static const char *fio___rsa_test_e_hex = "010001";

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_public_op)(void) {
  uint8_t n[256], e[4], sig[256], result[256];
  size_t n_len = fio___rsa_test_hex2bin(n, sizeof(n), fio___rsa_test_n_hex);
  size_t e_len = fio___rsa_test_hex2bin(e, sizeof(e), fio___rsa_test_e_hex);
  FIO_ASSERT(n_len == 256, "test modulus length mismatch");
  FIO_ASSERT(e_len == 3, "test exponent length mismatch");

  FIO_MEMSET(sig, 0, sizeof(sig));
  sig[1] = 0x01;
  for (size_t i = 2; i < 256; ++i)
    sig[i] = (uint8_t)(i & 0xFF);

  fio_rsa_pubkey_s key = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};
  /* the archived modulus is even (not a valid RSA modulus) */
  FIO_ASSERT(fio___rsa_public_op(result, sig, 256, &key) == -1,
             "RSA public op must reject an even modulus");
  n[255] |= 1;
  FIO_ASSERT(fio___rsa_public_op(result, sig, 256, &key) == 0,
             "RSA public op failed");

  int all_zero = 1;
  for (size_t i = 0; i < 256; ++i) {
    if (result[i] != 0) {
      all_zero = 0;
      break;
    }
  }
  FIO_ASSERT(!all_zero, "RSA public op returned all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_mgf1)(void) {
  uint8_t seed[32];
  FIO_MEMSET(seed, 0x42, 32);
  uint8_t mask[64], mask2[64];

  fio___rsa_mgf1(mask, 64, seed, 32, FIO_RSA_HASH_SHA256);
  int all_zero = 1;
  for (size_t i = 0; i < 64; ++i) {
    if (mask[i] != 0) {
      all_zero = 0;
      break;
    }
  }
  FIO_ASSERT(!all_zero, "MGF1 mask should not be all zeros");

  fio___rsa_mgf1(mask2, 64, seed, 32, FIO_RSA_HASH_SHA256);
  FIO_ASSERT(!FIO_MEMCMP(mask, mask2, 64), "MGF1 should be deterministic");

  uint8_t seed2[32];
  FIO_MEMSET(seed2, 0x43, 32);
  fio___rsa_mgf1(mask2, 64, seed2, 32, FIO_RSA_HASH_SHA256);
  FIO_ASSERT(FIO_MEMCMP(mask, mask2, 64) != 0,
             "Different seeds should produce different MGF1 masks");
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_invalid_inputs)(void) {
  uint8_t sig[256], hash[32], n[256], e[3];
  FIO_MEMSET(sig, 0, sizeof(sig));
  FIO_MEMSET(hash, 0xAB, sizeof(hash));
  fio___rsa_test_hex2bin(n, sizeof(n), fio___rsa_test_n_hex);
  fio___rsa_test_hex2bin(e, sizeof(e), fio___rsa_test_e_hex);
  n[255] |= 1; /* odd modulus, so failures below test the intended input */

  fio_rsa_pubkey_s key = {.n = n, .n_len = 256, .e = e, .e_len = 3};

  FIO_ASSERT(fio_rsa_verify_pkcs1(NULL, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL sig should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, NULL, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL hash should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, NULL) == -1,
             "NULL key should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 16,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "bad hash length should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 128, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "sig length mismatch should fail PKCS#1");

  FIO_ASSERT(fio_rsa_verify_pss(NULL, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL sig should fail PSS");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, NULL, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL hash should fail PSS");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, NULL) == -1,
             "NULL key should fail PSS");

  FIO_MEMSET(sig, 0x42, sizeof(sig));
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "random signature should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "random signature should fail PSS");

  fio_rsa_pubkey_s bad_key1 = {.n = n, .n_len = 0, .e = e, .e_len = 3};
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &bad_key1) == -1,
             "empty modulus should fail");
  fio_rsa_pubkey_s bad_key2 = {.n = n, .n_len = 256, .e = e, .e_len = 0};
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &bad_key2) == -1,
             "empty exponent should fail");
}

/* *****************************************************************************
Known-answer tests (keys and signatures generated with the OpenSSL CLI)

k2048: 2048-bit modulus, e = 65537. k2049: 2049-bit modulus (modBits - 1 is a
multiple of 8, so PSS EM is one byte shorter than the modulus), e = 3, with a
129-byte p and a 128-byte q (unequal CRT widths). Signatures are SHA-256 over
fio___rsa_kat_msg (PKCS#1 v1.5 and PSS with 32-byte salt).
***************************************************************************** */
static const char *fio___rsa_kat_msg = "facil.io RSA known-answer message";

/* k2048: 2048-bit modulus, e = 65537 (OpenSSL CLI) */
static const char *fio___rsa_kat_k2048_n =
    "c50a450104a5e3ba7f24675769c76b2106598f3a5e1e3d9eae3fff535b65315eb1607a5a"
    "167e30d8011604e1e9ba1cc278800640c0aa94cb2c65c78585d7fbef3a2bf6745f10618b"
    "529c41ac1d9b31068d45612a2950e9e8e333c30e36481e6e8487399cf357c4c6fc5af37d"
    "6f979a34f1dda2a04119b17512c3f7b300d3532df00f30e3ea176323e1e79341c1231f93"
    "69b440cceba0b1351fc7f29cfc8a9eba4143909fb14db3301fdfaf589ad2a4600aefa777"
    "228e242c45db82b3fdedd2ba8c24ad5e7d925b9db115cce16c3b34a0d86ca9d06dd0c27c"
    "4e667a42b7192e05307be696a7a964b1fb3f5e68a54c016f9e1037dee5d709f061170e73"
    "c8147457";
static const char *fio___rsa_kat_k2048_e =
    "010001";
static const char *fio___rsa_kat_k2048_d =
    "072bb675fc9da28b6d033230ddb39655719265a27fa6ff44ff9799196446b0e795fdc75a"
    "7c981212b6b65033fe2bae42b72da9355284f142ab70a55770dcc0995ef445cf578e25d5"
    "2d396ca4c7ee809ad6ef98abf6fdf5b601c71ae031082a3bb7de5b0ecf80dbe1eee4e218"
    "9c955d552e05d1b96f922b077231b3d3b0a4f48b9c5d0880fbe76455537feb1c8d78f347"
    "130ed2c9a86b53ba1a22240658641fef68349796e06b9a11e937999a3b00a4588227c1c8"
    "c58fd951f8637dce704e75d44720a88b4b902eec66bc6d1b67b7e171d6c1cde718ba954d"
    "466c446899486202e0a48d6d756679e504aa86e1d63d9d74dce05b4de32c9e52bfb45117"
    "c3174919";
static const char *fio___rsa_kat_k2048_p =
    "f1df34f3f5e8c40ddeeec6d1d888ac562e8a0a07a37388285e66fcc6df362acb5f5cf4bb"
    "fca7bcb78098078910f980eaf7e05706acfcef68a1e6c01c7110941798db4dd12c252823"
    "a9df73565765f49a5e022548e1069830f7c8058d703ea9faa17b2d84b708b7e4eca61186"
    "90baa0d3d0ddd5252b63adf21ac81ba217b35f9f";
static const char *fio___rsa_kat_k2048_q =
    "d08cad7ce52da004d25b5712e375ede30746373ef2ea24969f8fc05885858ed7769bfd28"
    "8573c7942e7459cf85b6ff6a5d7cee03b37522ca5116638c8e467b785dffe378e9adebc1"
    "fac029362da187f35ece44d4e67bbd1c3ba4d9c6231e2285fe1542a7495670bcd6855d69"
    "abc00c11d478d9fcf130eb4067ac44eadaa3d049";
static const char *fio___rsa_kat_k2048_dp =
    "8b5e6541c9c9b4e695359b24a193a75205810ee475df856e556f1c6a6f85e3c4ca0f6309"
    "cb3ccff620c7f957968b5e28866a235b3a6488830eff4a65591507704d68d87f15e3d72c"
    "2d9f0ef38a6aba3614c341beabc34400bb55eb35239f480c9dedbf3dc0c5ea48a3bfd14b"
    "6babce2e1f2523c3462c2ab7dd8cbeb778d6a7db";
static const char *fio___rsa_kat_k2048_dq =
    "22774d68b9bb7b99b0663612c212c858dfe7e3d37c73e3ce1cdab6102b28f27d7b75b636"
    "0f301e48c810a4931e672af91a303e69d02bc8af250d8d303a873d7a9c166132a773fde5"
    "961a51f5493f4be795c08331a851c4e4389b3afe7ae94d0d55fb8f8374245c076970ab0a"
    "7ebf037fa2dcc232026e7242b79f89d842801639";
static const char *fio___rsa_kat_k2048_qinv =
    "cd64be077196e04cc7bb082851cef5343a86fc99cc49478404a0abdbc9f1f835aba4ed81"
    "94265d55a4be181da43641de9a956f4d4e97096ef0df9491cecb799d626ab70a39c4c657"
    "c336a72aeaa02b217a8e09ee979d72c504d3503f7add88284a3d5b1046bbac9394cdbd95"
    "107516330a9f828d8b1a294a2d6b9688d6b1fefd";
static const char *fio___rsa_kat_k2048_qinv_swapped =
    "1f73ea95aaf20d01a0a8866cb4890a0992a7b67e444628882df7eeb7fc36e57d255f3f95"
    "a349b52591951b7f1e8f6979f38293cb908607998046e08e26318f56be87000d799ba716"
    "446af62c34a605502f6917baeacca870916ca58952ccddb7eca85974d5d54b2506a7dc44"
    "8e7ba2beb9bc7cb00cce7c3e95db832162e058ed";
static const char *fio___rsa_kat_k2048_sig_pkcs1 =
    "4968e9d47ef9be012db57c4c081024e281305d7361a89ac5226e4b9bad43df66823da528"
    "16fdb94c223d8a63134f21213ff87940ff334ff27706740619e07a544e8fe01b2e6b91f2"
    "947637a0b0aad39c1f5603e990f99e98c577f57f941c32fe7725c5705c473085543b1e72"
    "a309a7d9e6025f919d1a5c5354d56d8ef95eae3572609bbeface3209ce52f142e4d047bf"
    "bfa15f69d8265d7a66853a163e24b4ee18f4227a7e56b5840ba03016d2bc8fbd53753460"
    "a58a4f03da62227b7a97c1764a54f23ae71f931f83e40e2deab64b6a3d374019f6e77bc0"
    "d248ad13ced33fd85356c3d638b85b1ca085aee9de89e4186df59d0384846dc855060e33"
    "e611a73b";
static const char *fio___rsa_kat_k2048_sig_pss =
    "27ada5f7157ed6b286a7a43fc47c48b17a22b5406798a5b81831ba41d219fd37e9610553"
    "53c52c32ccfcd96321f840c2acd887dd5e6287ed8bb09127f235b1aec71deb542a0c76f5"
    "3c2171c7d0afeb9f1e0fcc88bffdb58883b58df4a290ba6e702d50248f10237972db236b"
    "1ef670b66ff70a911d5c5837827b1dd91ee505e452b39a22e9a9b23927d3853c3c96138f"
    "40dad69d416139bbc841c71190318595f7b9b17da00142ea9a56492aabb9b92d27a4c0fb"
    "b2c0e6b1c24e8b6f8c9adc3b0a71bf6e59bfaad5a630d26b9dd53d6413795c44044dd223"
    "34024752270b4f0f25b21f22e635eab95334ace612db07c69f83eaf012129bb5353c6ad2"
    "ebc18a94";

/* k2049: 2049-bit modulus, e = 3 (OpenSSL CLI) */
static const char *fio___rsa_kat_k2049_n =
    "018c8e64614c65aab46c7c97ccfa316db59eb98c74b1cb0d56b014fa60398a91779bac09"
    "80ecc1d8dc15d1c9535a0382e2efa27e0116f6a69210518526f15aca086d244c545dc06a"
    "63418de08ef9c30f477c7fdbdac97177040fe6e43af36a6e6f12da35aa4fda9dc60d2b0a"
    "8e04fedb12ef763755b131ca81c7cbee4943853a4ec2c05914a670da072739499c077e42"
    "ee6fcbac19b2509343223a64c44cadcdc4c3a1ca92016ce881642d09079febda1cabf599"
    "4529f69050a92a7bbe0d6340d5b5eb8c8a45487acc97a660b841881f8788ffef66f722d7"
    "0fda9c805e2334fedd21c28183bb63239a55b35f2314a3bba2c72e183c6571dbad75c041"
    "6249390c01";
static const char *fio___rsa_kat_k2049_e =
    "03";
static const char *fio___rsa_kat_k2049_d =
    "01085eed9632ee71cd9da8653351764923bf265da3213208e4756351957bb1b64fbd1d5b"
    "ab48813b3d63e130e23c025741f516feab64a46f0c0ae1036f4b91dc059e1832e2e92af1"
    "978109405f512cb4da52ffe7e730f64f580a99ed7ca246f44a0c91791c353c692eb37207"
    "0958a9e761f4f97a3920cbdc5685329edb8258d1880a868318c6fc8929759f8a635bae15"
    "b6ceb67ca03860fd6f660a8f3c494391236b9a9b0faa011743db5e0deb195b38d4f1b874"
    "896c28f5959b555db3e6c9cde2311dcec8c3ca18f3ef964d24f32de9292b7bb9f2cdefc5"
    "dc7097d87521087ef8636789e57e158514f8c66ebfd902ff7a85151ab7bdd22883319286"
    "66b74ed4ab";
static const char *fio___rsa_kat_k2049_p =
    "01df0b7c16418a3e1ac1b7ee404a1ae79bd63dd46ccc22413acc0f51961ba0725cd129a0"
    "1772466c930c443ee4f30fc7fe32fda5d04135a2eb4675f4c89ef929cc587a2afa0efcfa"
    "935a0667a5c0ab25e3ae777d518dcf783629eb980bf50ccdac3d88ec5e3adc188a67fdf6"
    "b091ffeb3f7a37739113aa27bb6c7f15bcca28c671";
static const char *fio___rsa_kat_k2049_q =
    "d3eb18593a6bce2e35120bc6b3de3ac0637c1cbc919cd5e13d1b3c53c32801b2d11041e3"
    "1024d9088edbb54206d33cdf0e6344a6c6837d04f9b47a67943b623613c4ab63109c5acb"
    "563e855b14181be6194edb29346bb60f07cd23a27c9b72bc4f1e464d4366c370788bc252"
    "bd1f512b8556fc97b50c772d3ee5620b6c1a0691";
static const char *fio___rsa_kat_k2049_dp =
    "013f5cfd642bb17ebc81254980316745128ed3e2f332c180d1dd5f8bb967c04c3de0c66a"
    "ba4c2ef30cb2d829edf75fdaa97753c3e02b791747844ea33069fb7132e5a6c7515f5351"
    "b791599a6e80721942744fa8e10934facec69d1007f8b333c82905f2e97c92bb06effea4"
    "75b6aa9cd4fc24f7b60d1c1a7cf2ff63d33170844b";
static const char *fio___rsa_kat_k2049_dq =
    "8d476590d19d341ece0c07d9cd3ed1d597a8132861133940d3677d8d2cc55677360ad697"
    "601890b05f3d238159e22894b4422dc48457a8adfbcda6efb82796ceb7d872420b12e732"
    "397f03920d6567eebb89e770cd9d240a05336d16fdbcf72834bed988d799d7a0505d2c37"
    "28bf8b7258e4a865235da4c8d498ec079d66af0b";
static const char *fio___rsa_kat_k2049_qinv =
    "7ff213e2295c4727bcbde36cad1fd0c4c4b888900109e970f5c178ed9abefb63a567709a"
    "99a04c791bfdb9eba92c4915c72f3b0a4d8fbcaaa59c83fc557dd03144355dacf57cff17"
    "a85d211f192a96c5065d12e5ed488ed0203ca2cef595c19a811175e72e85dd15944f63d1"
    "12ce3b839539564adce2e3aafd9a6af8661b8729";
static const char *fio___rsa_kat_k2049_qinv_swapped =
    "9b517357e21b48ae953747f8540abcd65f40585f9bf9d52c49f8a758bb19705f5526e79d"
    "41a17ffd94c429e3e73cb669d7fd5cb9a2b55e33d2d20072be609a8f73850992767ea053"
    "fd8415980dde8527991db1f14a7c97ed59b1fc0e3334ba2dc577954eb92ca33c3a1bb2b4"
    "925b435949012ee917477fafc28b304f743e72d9";
static const char *fio___rsa_kat_k2049_sig_pkcs1 =
    "0087e1df20769a32278b5d941f97aa74db7c6a30a90f3194f8ea80fea66e6892554d5eb4"
    "2031e5632a805beffa35955685f573e8dd3e53ee51fdcfc9a35135414039b74169ebf69e"
    "2726e9f7ebcf0699639a33d586f1c3f603fed195e1570b9f21781f4cb7ec34dfbabb0a7a"
    "68e9806c09e56a09edb42eeea1262aef9ac699ab1166b0a51a2e6886246bf97ff68c86eb"
    "e39b627f792cc5287f08eb5eb0c0b58acdcb5379d35b00fcaeec032d39c59eca0ca73723"
    "aa054d33e8990932d4b1c08e20698bb4ca15f83901b2912afb6b26be888372bf126da07d"
    "f4fe6ab589142a67afd9b6475b197ab6c412c4641d4328d0b65047f015de3797685ed3d6"
    "901b1fcf37";
static const char *fio___rsa_kat_k2049_sig_pss =
    "00bf4b23d7bf5d1eb8f9b33be6d20be12383994ff774b7c4bd68ee80a1bf3066d387d4ee"
    "9fef01bff39fb18589d978de443cf9f08de6c68841790f6edc4abcb01a97547bc6bfcd7c"
    "dc38d52c90f388cb638b4273b1860000bf608c91f8172ffd318f8cd1d1d85b6e51d32037"
    "e0dece954c6bc68227aec87de2da1bcba6fa9df6a0368d2ec248c5cdbcb3c5d2be813c34"
    "017ce7512ec0d7a0bc5d7b203c86e6031c26d4d742ae64e09406bfa3cad7597f1cf35c49"
    "c2c3bf20c29e85df0e919a852912d467e18fe9b9163a3bb538b0c6085f5e10eb2330ccb1"
    "8ad01b4d3f197664c53565f2d50fb3c950bbe94a4eec0e14cbe5428fe26eb00d143fc417"
    "a55f2086f5";

typedef struct {
  uint8_t n[FIO_RSA_MAX_BYTES + 2], e[8], d[FIO_RSA_MAX_BYTES];
  uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
  uint8_t dp[FIO_RSA_MAX_BYTES], dq[FIO_RSA_MAX_BYTES];
  uint8_t qinv[FIO_RSA_MAX_BYTES], qinv_swapped[FIO_RSA_MAX_BYTES];
  uint8_t sig_pkcs1[FIO_RSA_MAX_BYTES], sig_pss[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len, p_len, q_len, dp_len, dq_len, qinv_len,
      qinv_swapped_len, sig_pkcs1_len, sig_pss_len;
  fio_rsa_pubkey_s pub;
  fio_rsa_privkey_s priv;
} fio___rsa_kat_key_s;

#define FIO___RSA_KAT_LOAD(k, name, f)                                         \
  (k)->f##_len =                                                               \
      fio___rsa_test_hex2bin((k)->f, sizeof((k)->f), fio___rsa_kat_##name##_##f)

#define FIO___RSA_KAT_KEY(k, name)                                             \
  do {                                                                         \
    FIO___RSA_KAT_LOAD(k, name, n);                                            \
    FIO___RSA_KAT_LOAD(k, name, e);                                            \
    FIO___RSA_KAT_LOAD(k, name, d);                                            \
    FIO___RSA_KAT_LOAD(k, name, p);                                            \
    FIO___RSA_KAT_LOAD(k, name, q);                                            \
    FIO___RSA_KAT_LOAD(k, name, dp);                                           \
    FIO___RSA_KAT_LOAD(k, name, dq);                                           \
    FIO___RSA_KAT_LOAD(k, name, qinv);                                         \
    FIO___RSA_KAT_LOAD(k, name, qinv_swapped);                                 \
    FIO___RSA_KAT_LOAD(k, name, sig_pkcs1);                                    \
    FIO___RSA_KAT_LOAD(k, name, sig_pss);                                      \
    fio___rsa_kat_bind(k);                                                     \
  } while (0)

FIO_SFUNC void fio___rsa_kat_bind(fio___rsa_kat_key_s *k) {
  k->pub = (fio_rsa_pubkey_s){.n = k->n,
                              .n_len = k->n_len,
                              .e = k->e,
                              .e_len = k->e_len};
  k->priv = (fio_rsa_privkey_s){.n = k->n,
                                .n_len = k->n_len,
                                .d = k->d,
                                .d_len = k->d_len,
                                .e = k->e,
                                .e_len = k->e_len,
                                .p = k->p,
                                .p_len = k->p_len,
                                .q = k->q,
                                .q_len = k->q_len,
                                .dP = k->dp,
                                .dP_len = k->dp_len,
                                .dQ = k->dq,
                                .dQ_len = k->dq_len,
                                .qInv = k->qinv,
                                .qInv_len = k->qinv_len};
}

/** Signs with `priv`, then verifies with `pub`; returns the sign result. */
FIO_SFUNC int fio___rsa_kat_sign_verify(const fio_rsa_privkey_s *priv,
                                        const fio_rsa_pubkey_s *pub,
                                        const uint8_t *hash) {
  uint8_t sig[FIO_RSA_MAX_BYTES];
  size_t sig_len = 0;
  if (fio_rsa_sign_pss(sig, &sig_len, hash, 32, FIO_RSA_HASH_SHA256, priv))
    return -1;
  FIO_ASSERT(fio_rsa_verify_pss(sig,
                                sig_len,
                                hash,
                                32,
                                FIO_RSA_HASH_SHA256,
                                pub) == 0,
             "RSA-PSS signature should verify");
  return 0;
}

FIO_SFUNC void fio___rsa_kat_run(fio___rsa_kat_key_s *k, const char *name) {
  fio_u256 h = fio_sha256(fio___rsa_kat_msg, strlen(fio___rsa_kat_msg));
  fio_u256 bad = h;
  bad.u8[0] ^= 1;

  /* independent signatures (OpenSSL) verify; altered digests do not */
  FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                  k->sig_pkcs1_len,
                                  h.u8,
                                  32,
                                  FIO_RSA_HASH_SHA256,
                                  &k->pub) == 0,
             "%s: OpenSSL PKCS#1 v1.5 KAT should verify",
             name);
  FIO_ASSERT(fio_rsa_verify_pss(k->sig_pss,
                                k->sig_pss_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &k->pub) == 0,
             "%s: OpenSSL PSS KAT should verify",
             name);
  FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                  k->sig_pkcs1_len,
                                  bad.u8,
                                  32,
                                  FIO_RSA_HASH_SHA256,
                                  &k->pub) == -1 &&
                 fio_rsa_verify_pss(k->sig_pss,
                                    k->sig_pss_len,
                                    bad.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &k->pub) == -1,
             "%s: wrong digest must fail",
             name);

  /* DER-style leading zero bytes on n are accepted (length is trimmed) */
  {
    uint8_t n2[FIO_RSA_MAX_BYTES + 2] = {0};
    FIO_MEMCPY(n2 + 2, k->n, k->n_len);
    fio_rsa_pubkey_s pub2 = k->pub;
    pub2.n = n2;
    pub2.n_len = k->n_len + 2;
    FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                    k->sig_pkcs1_len,
                                    h.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &pub2) == 0 &&
                   fio_rsa_verify_pss(k->sig_pss,
                                      k->sig_pss_len,
                                      h.u8,
                                      32,
                                      FIO_RSA_HASH_SHA256,
                                      &pub2) == 0,
               "%s: zero-prefixed modulus should verify",
               name);
  }

  /* signing: CRT, swapped CRT (p < q), non-CRT, and non-CRT without e */
  FIO_ASSERT(!fio___rsa_kat_sign_verify(&k->priv, &k->pub, h.u8),
             "%s: CRT signing failed",
             name);
  {
    fio_rsa_privkey_s sw = k->priv;
    sw.p = k->q, sw.p_len = k->q_len, sw.q = k->p, sw.q_len = k->p_len;
    sw.dP = k->dq, sw.dP_len = k->dq_len, sw.dQ = k->dp, sw.dQ_len = k->dp_len;
    sw.qInv = k->qinv_swapped, sw.qInv_len = k->qinv_swapped_len;
    FIO_ASSERT(!fio___rsa_kat_sign_verify(&sw, &k->pub, h.u8),
               "%s: swapped-prime CRT signing failed",
               name);
  }
  {
    fio_rsa_privkey_s nc = k->priv;
    nc.p_len = nc.q_len = nc.dP_len = nc.dQ_len = nc.qInv_len = 0;
    FIO_ASSERT(!fio___rsa_kat_sign_verify(&nc, &k->pub, h.u8),
               "%s: non-CRT signing failed",
               name);
    nc.e_len = 0;
    FIO_ASSERT(!fio___rsa_kat_sign_verify(&nc, &k->pub, h.u8),
               "%s: non-CRT signing without e failed",
               name);
  }

  /* inconsistent / faulty CRT keys never emit a signature */
  {
    uint8_t sig[FIO_RSA_MAX_BYTES];
    size_t sig_len = 0;
    fio_rsa_privkey_s bad_key = k->priv;
    uint8_t tmp[FIO_RSA_MAX_BYTES];
    FIO_MEMCPY(tmp, k->qinv, k->qinv_len);
    tmp[k->qinv_len - 1] ^= 2;
    bad_key.qInv = tmp;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &bad_key) == -1,
               "%s: corrupted qInv must fail signing",
               name);
    bad_key = k->priv;
    FIO_MEMCPY(tmp, k->dp, k->dp_len);
    tmp[k->dp_len - 1] ^= 2;
    bad_key.dP = tmp;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &bad_key) == -1,
               "%s: corrupted dP must fail signing",
               name);
    bad_key = k->priv;
    bad_key.p_len = k->n_len; /* oversized prime field */
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &bad_key) == -1,
               "%s: oversized prime must fail signing",
               name);
    bad_key = k->priv;
    bad_key.d_len = k->n_len + 1; /* d wider than n */
    bad_key.p_len = 0;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &bad_key) == -1,
               "%s: oversized d must fail signing",
               name);
    FIO_ASSERT(sig_len == 0, "%s: failed signing must not set sig_len", name);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_kat)(void) {
  fio___rsa_kat_key_s *k = (fio___rsa_kat_key_s *)calloc(1, sizeof(*k));
  FIO_ASSERT(k, "allocation failed");
  FIO___RSA_KAT_KEY(k, k2048);
  FIO_ASSERT(k->n_len == 256 && k->e_len == 3, "k2048 vector load failed");
  fio___rsa_kat_run(k, "k2048");
  FIO_MEMSET(k, 0, sizeof(*k));
  FIO___RSA_KAT_KEY(k, k2049);
  FIO_ASSERT(k->n_len == 257 && k->p_len == 129 && k->q_len == 128,
             "k2049 vector load failed");
  fio___rsa_kat_run(k, "k2049");
  free(k);
}

/** Malformed public keys must be rejected (never forged, never crash). */
FIO_SFUNC void FIO_NAME_TEST(stl, rsa_malformed_keys)(void) {
  fio___rsa_kat_key_s *k = (fio___rsa_kat_key_s *)calloc(1, sizeof(*k));
  FIO_ASSERT(k, "allocation failed");
  FIO___RSA_KAT_KEY(k, k2048);
  fio_u256 h = fio_sha256(fio___rsa_kat_msg, strlen(fio___rsa_kat_msg));
  uint8_t sig[FIO_RSA_MAX_BYTES];
  size_t sig_len = 0;

  /* e = 1 would make sig^e == sig: forging EM must fail */
  {
    static const uint8_t bad_e[][9] = {{0},
                                       {1},
                                       {2},
                                       {0, 1},
                                       {1, 0, 0, 0, 0, 0, 0, 0, 1}};
    static const size_t bad_e_len[] = {1, 1, 1, 2, 9};
    uint8_t em[FIO_RSA_MAX_BYTES];
    FIO_MEMSET(em, 0xFF, sizeof(em));
    em[0] = 0, em[1] = 1;
    for (size_t i = 0; i < sizeof(bad_e_len) / sizeof(bad_e_len[0]); ++i) {
      fio_rsa_pubkey_s pub = k->pub;
      pub.e = bad_e[i];
      pub.e_len = bad_e_len[i];
      FIO_ASSERT(fio___rsa_public_op(sig, em, 256, &pub) == -1,
                 "bad public exponent %zu must be rejected",
                 i);
      FIO_ASSERT(fio_rsa_verify_pss(k->sig_pss,
                                    k->sig_pss_len,
                                    h.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &pub) == -1,
                 "bad public exponent %zu must fail PSS",
                 i);
    }
  }
  /* even modulus, NULL fields, short modulus, oversized prefix */
  {
    fio_rsa_pubkey_s pub = k->pub;
    uint8_t n2[FIO_RSA_MAX_BYTES];
    FIO_MEMCPY(n2, k->n, k->n_len);
    n2[k->n_len - 1] ^= 1;
    pub.n = n2;
    FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                    k->sig_pkcs1_len,
                                    h.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &pub) == -1,
               "even modulus must be rejected");
    pub = k->pub;
    pub.n = NULL;
    FIO_ASSERT(fio_rsa_verify_pss(k->sig_pss,
                                  k->sig_pss_len,
                                  h.u8,
                                  32,
                                  FIO_RSA_HASH_SHA256,
                                  &pub) == -1,
               "NULL modulus must fail PSS");
    pub = k->pub;
    pub.e = NULL;
    FIO_ASSERT(fio_rsa_verify_pss(k->sig_pss,
                                  k->sig_pss_len,
                                  h.u8,
                                  32,
                                  FIO_RSA_HASH_SHA256,
                                  &pub) == -1,
               "NULL exponent must fail PSS");
    /* 512 zero bytes + modulus: trimmed key is fine, oversized buffers are
     * never read past the trimmed length */
    uint8_t *big = (uint8_t *)calloc(1, 512 + k->n_len);
    FIO_ASSERT(big, "allocation failed");
    FIO_MEMCPY(big + 512, k->n, k->n_len);
    pub = k->pub;
    pub.n = big;
    pub.n_len = 512 + k->n_len;
    FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                    k->sig_pkcs1_len,
                                    h.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &pub) == 0,
               "heavily zero-prefixed modulus should verify (trimmed)");
    FIO_ASSERT(fio_rsa_verify_pkcs1(k->sig_pkcs1,
                                    pub.n_len,
                                    h.u8,
                                    32,
                                    FIO_RSA_HASH_SHA256,
                                    &pub) == -1,
               "untrimmed signature length must fail");
    free(big);
    /* all-zero modulus */
    uint8_t zero[256] = {0};
    fio_rsa_privkey_s priv = k->priv;
    priv.n = zero;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &priv) == -1,
               "zero modulus must fail signing");
    priv = k->priv;
    priv.n = NULL;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &priv) == -1,
               "NULL modulus must fail signing");
    priv = k->priv;
    priv.e = (const uint8_t *)"\x02";
    priv.e_len = 1;
    FIO_ASSERT(fio_rsa_sign_pss(sig,
                                &sig_len,
                                h.u8,
                                32,
                                FIO_RSA_HASH_SHA256,
                                &priv) == -1,
               "invalid private-key e must fail signing");
  }
  free(k);
}

/* *****************************************************************************
OpenSSL cross-validation tests
***************************************************************************** */
#if HAVE_OPENSSL

FIO_SFUNC EVP_PKEY *fio___rsa_test_gen_key(void) {
  EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
  FIO_ASSERT(ctx, "EVP_PKEY_CTX_new_id failed");
  FIO_ASSERT(EVP_PKEY_keygen_init(ctx) == 1, "EVP_PKEY_keygen_init failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) == 1,
             "EVP_PKEY_CTX_set_rsa_keygen_bits failed");
  EVP_PKEY *pkey = NULL;
  FIO_ASSERT(EVP_PKEY_keygen(ctx, &pkey) == 1, "EVP_PKEY_keygen failed");
  EVP_PKEY_CTX_free(ctx);
  return pkey;
}

FIO_SFUNC void fio___rsa_test_extract_key(EVP_PKEY *pkey,
                                          uint8_t *n,
                                          size_t *n_len,
                                          uint8_t *e,
                                          size_t *e_len,
                                          uint8_t *d,
                                          size_t *d_len,
                                          uint8_t *p,
                                          size_t *p_len,
                                          uint8_t *q,
                                          size_t *q_len,
                                          uint8_t *dP,
                                          size_t *dP_len,
                                          uint8_t *dQ,
                                          size_t *dQ_len,
                                          uint8_t *qInv,
                                          size_t *qInv_len) {
  RSA *rsa = EVP_PKEY_get1_RSA(pkey);
  FIO_ASSERT(rsa, "EVP_PKEY_get1_RSA failed");
  const BIGNUM *bn_n = NULL, *bn_e = NULL, *bn_d = NULL;
  const BIGNUM *bn_p = NULL, *bn_q = NULL, *bn_dP = NULL, *bn_dQ = NULL,
               *bn_qInv = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
  RSA_get0_key(rsa, &bn_n, &bn_e, &bn_d);
  RSA_get0_factors(rsa, &bn_p, &bn_q);
  RSA_get0_crt_params(rsa, &bn_dP, &bn_dQ, &bn_qInv);
#else
  bn_n = rsa->n;
  bn_e = rsa->e;
  bn_d = rsa->d;
  bn_p = rsa->p;
  bn_q = rsa->q;
  bn_dP = rsa->dmp1;
  bn_dQ = rsa->dmq1;
  bn_qInv = rsa->iqmp;
#endif

  *n_len = (size_t)BN_num_bytes(bn_n);
  *e_len = (size_t)BN_num_bytes(bn_e);
  *d_len = (size_t)BN_num_bytes(bn_d);
  FIO_ASSERT(*n_len <= FIO_RSA_MAX_BYTES, "modulus too large");
  FIO_ASSERT(*e_len <= FIO_RSA_MAX_BYTES, "exponent too large");
  FIO_ASSERT(*d_len <= FIO_RSA_MAX_BYTES, "private exponent too large");
  BN_bn2bin(bn_n, n);
  BN_bn2bin(bn_e, e);
  BN_bn2bin(bn_d, d);

  if (bn_p) {
    *p_len = (size_t)BN_num_bytes(bn_p);
    BN_bn2bin(bn_p, p);
  } else
    *p_len = 0;
  if (bn_q) {
    *q_len = (size_t)BN_num_bytes(bn_q);
    BN_bn2bin(bn_q, q);
  } else
    *q_len = 0;
  if (bn_dP) {
    *dP_len = (size_t)BN_num_bytes(bn_dP);
    BN_bn2bin(bn_dP, dP);
  } else
    *dP_len = 0;
  if (bn_dQ) {
    *dQ_len = (size_t)BN_num_bytes(bn_dQ);
    BN_bn2bin(bn_dQ, dQ);
  } else
    *dQ_len = 0;
  if (bn_qInv) {
    *qInv_len = (size_t)BN_num_bytes(bn_qInv);
    BN_bn2bin(bn_qInv, qInv);
  } else
    *qInv_len = 0;

  RSA_free(rsa);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pss_fio_sign_openssl_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
  uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES], qInv[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len, p_len, q_len, dP_len, dQ_len, qInv_len;

  fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                             p, &p_len, q, &q_len, dP, &dP_len,
                             dQ, &dQ_len, qInv, &qInv_len);

  fio_rsa_privkey_s priv = {.n = n,
                            .n_len = n_len,
                            .d = d,
                            .d_len = d_len,
                            .e = e,
                            .e_len = e_len,
                            .p = p,
                            .p_len = p_len,
                            .q = q,
                            .q_len = q_len,
                            .dP = dP,
                            .dP_len = dP_len,
                            .dQ = dQ,
                            .dQ_len = dQ_len,
                            .qInv = qInv,
                            .qInv_len = qInv_len};
  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "RSA-PSS cross-validation message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  uint8_t sig[FIO_RSA_MAX_BYTES];
  size_t sig_len = 0;
  FIO_ASSERT(fio_rsa_sign_pss(sig, &sig_len, h.u8, 32,
                              FIO_RSA_HASH_SHA256, &priv) == 0,
             "fio RSA-PSS signing failed");
  FIO_ASSERT(sig_len == n_len, "signature length mismatch");
  FIO_ASSERT(fio_rsa_verify_pss(sig, sig_len, h.u8, 32,
                                FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio RSA-PSS self-verification failed");

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestVerifyInit failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_padding(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), RSA_PKCS1_PSS_PADDING) == 1,
             "EVP_PKEY_CTX_set_rsa_padding failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_pss_saltlen(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), 32) == 1,
             "EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
  int rc = EVP_DigestVerify(mdctx, sig, sig_len, msg, sizeof(msg) - 1);
  FIO_ASSERT(rc == 1, "OpenSSL should verify fio RSA-PSS signature");
  EVP_MD_CTX_free(mdctx);

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pss_openssl_sign_fio_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
  uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES], qInv[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len, p_len, q_len, dP_len, dQ_len, qInv_len;

  fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                             p, &p_len, q, &q_len, dP, &dP_len,
                             dQ, &dQ_len, qInv, &qInv_len);

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "OpenSSL RSA-PSS signed message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_padding(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), RSA_PKCS1_PSS_PADDING) == 1,
             "EVP_PKEY_CTX_set_rsa_padding failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_pss_saltlen(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), 32) == 1,
             "EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  FIO_ASSERT(sig_len == n_len, "OpenSSL signature length mismatch");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL RSA-PSS signing failed");
  EVP_MD_CTX_free(mdctx);

  FIO_ASSERT(fio_rsa_verify_pss(sig, sig_len, h.u8, 32,
                                FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio should verify OpenSSL RSA-PSS signature");

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pkcs1_openssl_sign_fio_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len;
  {
    uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
    uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES],
        qInv[FIO_RSA_MAX_BYTES];
    size_t p_len, q_len, dP_len, dQ_len, qInv_len;
    fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                               p, &p_len, q, &q_len, dP, &dP_len,
                               dQ, &dQ_len, qInv, &qInv_len);
  }

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "OpenSSL PKCS#1 v1.5 signed message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  FIO_ASSERT(sig_len == n_len, "OpenSSL PKCS#1 signature length mismatch");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL PKCS#1 signing failed");
  EVP_MD_CTX_free(mdctx);

  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, sig_len, h.u8, 32,
                                  FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio should verify OpenSSL PKCS#1 v1.5 signature");

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pkcs1_fio_rejects_bad_sig)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len;
  {
    uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
    uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES],
        qInv[FIO_RSA_MAX_BYTES];
    size_t p_len, q_len, dP_len, dQ_len, qInv_len;
    fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                               p, &p_len, q, &q_len, dP, &dP_len,
                               dQ, &dQ_len, qInv, &qInv_len);
  }

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "Message with corrupted signature";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL signing failed");
  EVP_MD_CTX_free(mdctx);

  sig[sig_len - 1] ^= 0xFF;
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, sig_len, h.u8, 32,
                                  FIO_RSA_HASH_SHA256, &pub) == -1,
             "fio should reject corrupted PKCS#1 signature");

  EVP_PKEY_free(pkey);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_NAME_TEST(stl, rsa_byte_conversion)();
  FIO_NAME_TEST(stl, rsa_public_op)();
  FIO_NAME_TEST(stl, rsa_mgf1)();
  FIO_NAME_TEST(stl, rsa_invalid_inputs)();
  FIO_NAME_TEST(stl, rsa_kat)();
  FIO_NAME_TEST(stl, rsa_malformed_keys)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, rsa_pss_fio_sign_openssl_verify)();
  FIO_NAME_TEST(stl, rsa_pss_openssl_sign_fio_verify)();
  FIO_NAME_TEST(stl, rsa_pkcs1_openssl_sign_fio_verify)();
  FIO_NAME_TEST(stl, rsa_pkcs1_fio_rejects_bad_sig)();
#endif
  return 0;
}
