/* *****************************************************************************
RFC 7748 X25519 test vectors — carry-overflow regression test

Verifies that fio_x25519_shared_secret produces correct results, which
exercises fio___gf_mul on the active compilation path.  A carry overflow in
the portable (#else) path silently corrupts field elements and produces a
wrong shared secret.

Test vectors used:
  §5.2 — two independent scalar-multiply test vectors (each checks one
         direction of the X25519 function with a known output)
  §6.1 — DH exchange test vector (Alice and Bob derive the same shared secret)

Build: make tests/test_fio_gf_mul_carry
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPTO
#include "test-helpers.h"
#include FIO_INCLUDE_FILE

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Hex helpers
 * ------------------------------------------------------------------------- */

static void hex_decode(uint8_t *out, const char *hex, size_t len) {
  for (size_t i = 0; i < len; i++) {
    unsigned int hi = (unsigned int)hex[i * 2];
    unsigned int lo = (unsigned int)hex[i * 2 + 1];
    hi = (hi >= '0' && hi <= '9')   ? hi - '0'
         : (hi >= 'a' && hi <= 'f') ? hi - 'a' + 10
                                    : hi - 'A' + 10;
    lo = (lo >= '0' && lo <= '9')   ? lo - '0'
         : (lo >= 'a' && lo <= 'f') ? lo - 'a' + 10
                                    : lo - 'A' + 10;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
}

static void hex_print(const char *label, const uint8_t *buf, size_t len) {
  fprintf(stderr, "  %s: ", label);
  for (size_t i = 0; i < len; i++)
    fprintf(stderr, "%02x", buf[i]);
  fprintf(stderr, "\n");
}

static int check(const char *name,
                 const uint8_t *got,
                 const uint8_t *expected,
                 size_t len) {
  if (memcmp(got, expected, len) == 0) {
    fprintf(stderr, "PASS: %s\n", name);
    return 1;
  }
  fprintf(stderr, "FAIL: %s\n", name);
  hex_print("got     ", got, len);
  hex_print("expected", expected, len);
  return 0;
}

/* ---------------------------------------------------------------------------
 * RFC 7748 §5.2 — X25519 function test vectors
 *
 * Each vector: X25519(scalar, u_coord) == output
 * These are independent scalar-multiply checks (not a DH exchange).
 * ------------------------------------------------------------------------- */

/* §5.2 vector 1 */
static const char TV1_SCALAR_HEX[] =
    "a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4";
static const char TV1_UCOORD_HEX[] =
    "e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c";
static const char TV1_OUTPUT_HEX[] =
    "c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552";

/* §5.2 vector 2 */
static const char TV2_SCALAR_HEX[] =
    "4b66e9d4d1b4673c5ad22691957d6af5c11b6421e0ea01d42ca4169e7918ba0d";
static const char TV2_UCOORD_HEX[] =
    "e5210f12786811d3f4b7959d0538ae2c31dbe7106fc03c3efc4cd549c715a493";
static const char TV2_OUTPUT_HEX[] =
    "95cbde9476e8907d7aade45cb4b873f88b595a68799fa152e6f8f7647aac7957";

/* ---------------------------------------------------------------------------
 * RFC 7748 §6.1 — Curve25519 DH exchange test vector
 *
 * Alice and Bob each compute the shared secret from their own private key
 * and the other party's public key.  Both results must equal K.
 * ------------------------------------------------------------------------- */

/* Alice's private key (random bytes, clamped internally by X25519) */
static const char DH_ALICE_PRIV_HEX[] =
    "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a";
/* Alice's public key = X25519(alice_priv, 9) */
static const char DH_ALICE_PUB_HEX[] =
    "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a";
/* Bob's private key */
static const char DH_BOB_PRIV_HEX[] =
    "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb";
/* Bob's public key = X25519(bob_priv, 9) */
static const char DH_BOB_PUB_HEX[] =
    "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f";
/* Shared secret K = X25519(alice_priv, bob_pub) = X25519(bob_priv, alice_pub)
 */
static const char DH_SHARED_HEX[] =
    "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742";

/* ---------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void) {
  uint8_t scalar[32], ucoord[32], expected[32], got[32];
  int pass = 1;

  /* --- §5.2 vector 1 --- */
  hex_decode(scalar, TV1_SCALAR_HEX, 32);
  hex_decode(ucoord, TV1_UCOORD_HEX, 32);
  hex_decode(expected, TV1_OUTPUT_HEX, 32);
  fio_x25519_shared_secret(got, scalar, ucoord);
  pass &= check("RFC 7748 §5.2 vector 1", got, expected, 32);

  /* --- §5.2 vector 2 --- */
  hex_decode(scalar, TV2_SCALAR_HEX, 32);
  hex_decode(ucoord, TV2_UCOORD_HEX, 32);
  hex_decode(expected, TV2_OUTPUT_HEX, 32);
  fio_x25519_shared_secret(got, scalar, ucoord);
  pass &= check("RFC 7748 §5.2 vector 2", got, expected, 32);

  /* --- §6.1 DH: Alice computes shared secret --- */
  {
    uint8_t alice_priv[32], bob_pub[32], dh_expected[32], alice_shared[32];
    hex_decode(alice_priv, DH_ALICE_PRIV_HEX, 32);
    hex_decode(bob_pub, DH_BOB_PUB_HEX, 32);
    hex_decode(dh_expected, DH_SHARED_HEX, 32);
    fio_x25519_shared_secret(alice_shared, alice_priv, bob_pub);
    pass &=
        check("RFC 7748 §6.1 DH (Alice side)", alice_shared, dh_expected, 32);

    /* --- §6.1 DH: Bob computes shared secret (must match Alice's) --- */
    uint8_t bob_priv[32], alice_pub[32], bob_shared[32];
    hex_decode(bob_priv, DH_BOB_PRIV_HEX, 32);
    hex_decode(alice_pub, DH_ALICE_PUB_HEX, 32);
    fio_x25519_shared_secret(bob_shared, bob_priv, alice_pub);
    pass &= check("RFC 7748 §6.1 DH (Bob side)", bob_shared, dh_expected, 32);

    if (pass && memcmp(alice_shared, bob_shared, 32) != 0) {
      fprintf(stderr, "FAIL: Alice and Bob shared secrets differ\n");
      hex_print("alice", alice_shared, 32);
      hex_print("bob  ", bob_shared, 32);
      pass = 0;
    }
  }

  if (pass) {
    fprintf(stderr, "PASS: all RFC 7748 X25519 test vectors\n");
    return 0;
  }
  return 1;
}
