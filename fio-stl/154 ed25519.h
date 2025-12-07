/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ED25519            /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          Elliptic Curve ED25519 (WIP)
                              NOT YET IMPLEMENTED!




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ED25519) && !defined(H___FIO_ED25519___H) && 0
#define H___FIO_ED25519___H

/* *****************************************************************************
TODO: ED 25519 - NOT YET IMPLEMENTED!

ED-25519 key generation, key exchange and signatures are crucial to complete the
minimal building blocks that would allow to secure inter-machine communication
in mostly secure environments. Of course the use of a tested cryptographic
library (where accessible) might be preferred, but some security is better than
none.
***************************************************************************** */

/* *****************************************************************************
ED25519 API
***************************************************************************** */

/** ED25519 Key Pair */
typedef struct {        /* TODO: FIXME: do we need all the bits? */
  fio_u512 private_key; /* Private key (with extra internal storage?) */
  fio_u256 public_key;  /* Public key */
} fio_ed25519_s;

/* Generates a random ED25519 keypair. */
SFUNC void fio_ed25519_keypair(fio_ed25519_s *keypair);

/* Sign a message using ED25519 */
SFUNC void fio_ed25519_sign(uint8_t *signature,
                            const fio_buf_info_s message,
                            const fio_ed25519_s *keypair);

/* Verify an ED25519 signature */
SFUNC int fio_ed25519_verify(const uint8_t *signature,
                             const fio_buf_info_s message,
                             const fio_u256 *public_key);

/* *****************************************************************************
Implementation - inlined static functions
***************************************************************************** */

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Basic Operations (Helpers)
***************************************************************************** */

/* prevent ED25519 keys from having a small period (cyclic value). */
FIO_IFUNC void fio___ed25519_clamp_on_key(uint8_t *k) {
  k[0] &= 0xF8U;  /* zero out 3 least significant bits (emulate mul by 8) */
  k[31] &= 0x7FU; /* unset most significant bit (constant time fix) */
  k[31] |= 0x40U; /* set the 255th bit (making sure the value is big) */
}

static fio_u256 FIO___ED25519_PRIME = fio_u256_init64(0xFFFFFFFFFFFFFFEDULL,
                                                      0xFFFFFFFFFFFFFFFFULL,
                                                      0xFFFFFFFFFFFFFFFFULL,
                                                      0x7FFFFFFFFFFFFFFFULL);
/* clang-format off */
static fio_u1024 FIO___ED25519_PRIME_UNPACKED =
                 fio_u1024_init64(0xFFEDULL, 0xFFFFULL, 0xFFFFULL, 0xFFFFULL,
                                  0xFFFFULL, 0xFFFFULL, 0xFFFFULL, 0xFFFFULL,
                                  0xFFFFULL, 0xFFFFULL, 0xFFFFULL, 0xFFFFULL,
                                  0xFFFFULL, 0xFFFFULL, 0xFFFFULL, 0x7FFFULL);
/* clang-format on */

/* Obfuscate or recover ED25519 keys to prevent easy memory scraping */
FIO_IFUNC void fio___ed25519_flip(fio_ed25519_s *k) {
  /* Generate a deterministic mask */
  uint64_t msk =
      k->public_key.u64[3] + (uint64_t)(uintptr_t)(void *)&fio_ed25519_keypair;
  /* XOR mask the private key */
  fio_u512_cxor64(&k->private_key, &k->private_key, msk);
  /* XOR mask the first 192 bits of the public key */
  k->public_key.u64[0] ^= msk;
  k->public_key.u64[1] ^= msk;
  k->public_key.u64[2] ^= msk;
}

FIO_IFUNC fio_u1024 fio___ed25519_unpack(uint8_t *u) {
  fio_u1024 r;
  for (size_t i = 0; i < 16; ++i)
    r.u64[i] = (uint64_t)u[(i << 1)] | ((uint64_t)u[(i << 1)] << 8);
  r.u64[15] &= 0x7FFFULL;
  return r;
}

#define fio___ed25519_add     fio_u1024_add64
#define fio___ed25519_sub     fio_u1024_sub64
#define fio___ed25519_swap_if fio_u1024_ct_swap_if

FIO_IFUNC void fio___ed25519_normalize_step(fio_u1024 *u) {
  uint64_t c;
  for (size_t i = 0; i < 15; ++i) {
    /* ERR: example code uses illegal / undefined signed shift. */
    c = u->u64[i] >> 16; /* TODO: propagate carry bits of negative numbers? */
    c |= (0ULL - (u->u64[i] >> 63)) & ((1ULL << 48) - 1);
    u->u64[i] &= 0xFFFFU;
    u->u64[i + 1] += c;
  }
  c = u->u64[15] >> 16;
  c |= (0ULL - (u->u64[15] >> 63)) & ((1ULL << 48) - 1);
  u->u64[15] &= 0xFFFFU;
  u->u64[0] += 38 * c;
}

FIO_IFUNC fio_u1024 fio___ed25519_mul(const fio_u1024 *a, const fio_u1024 *b) {
  fio_u1024 r;
  fio_u2048 p = {0};
  for (size_t i = 0; i < 16; ++i)
    for (size_t j = 0; j < 16; ++j)
      p.u64[i + j] += a->u64[i] * b->u64[j];
  for (size_t i = 0; i < 15; ++i)
    p.u64[i] += 38 * p.u64[16 + i];
  for (size_t i = 0; i < 16; ++i)
    r.u64[i] = p.u64[i];
  /* partial normalize / carry (2/3) */
  fio___ed25519_normalize_step(&r);
  fio___ed25519_normalize_step(&r);
  return r;
}

/* *****************************************************************************
Point Math
***************************************************************************** */

/* Elliptic Curve Point Addition for Ed25519 */
FIO_IFUNC void fio___ed25519_point_add(fio_u1024 *R, const fio_u1024 *P) {
  /* Extract coordinates for P1 and P2 (R and P) */
  fio_u256 X1 = R->u256[0], Y1 = R->u256[1], Z1 = R->u256[2], T1 = R->u256[3];
  fio_u256 X2 = P->u256[0], Y2 = P->u256[1], Z2 = P->u256[2], T2 = P->u256[3];

  fio_u256 A, B, C, D, X3, Y3, Z3, T3;

  /* A = (Y1 - X1) * (Y2 - X2) */
  fio_u256 Y1_minus_X1 = fio_u256_sub(Y1, X1);
  fio_u256 Y2_minus_X2 = fio_u256_sub(Y2, X2);
  A = fio_u256_mul(Y1_minus_X1, Y2_minus_X2);

  /* B = (Y1 + X1) * (Y2 + X2) */
  fio_u256 Y1_plus_X1 = fio_u256_add(Y1, X1);
  fio_u256 Y2_plus_X2 = fio_u256_add(Y2, X2);
  B = fio_u256_mul(Y1_plus_X1, Y2_plus_X2);

  /* C = 2 * T1 * T2 * d */
  C = fio_u256_mul(fio_u256_mul(T1, T2), ED25519_D);

  /* D = 2 * Z1 * Z2 */
  D = fio_u256_mul(fio_u256_mul(Z1, Z2), fio_u256_two());

  /* X3 = (B - A) * (D - C) */
  X3 = fio_u256_mul(fio_u256_sub(B, A), fio_u256_sub(D, C));

  /* Y3 = (B + A) * (D + C) */
  Y3 = fio_u256_mul(fio_u256_add(B, A), fio_u256_add(D, C));

  /* Z3 = D * C */
  Z3 = fio_u256_mul(D, C);

  /* T3 = (B - A) * (B + A) */
  T3 = fio_u256_mul(fio_u256_sub(B, A), fio_u256_add(B, A));

  /* Update R with the result */
  R->u256[0] = X3; /* X */
  R->u256[1] = Y3; /* Y */
  R->u256[2] = Z3; /* Z */
  R->u256[3] = T3; /* T */
}

/* Helper function: Scalar multiplication on the elliptic curve */
FIO_IFUNC void fio___ed25519_mul(fio_u512 *result,
                                 const fio_u512 *scalar,
                                 const fio_u512 *point) {
  /* Start with the point */
  fio_u512 R[2] = {{0}, point[0]}; /* Identity point */

  /* Step 2: Perform the Montgomery ladder scalar multiplication */
  for (int i = 255; i >= 0; --i) {
    uint64_t bit = (scalar->u64[i >> 6] >> (i & 63)) & 1U;
    /* Elliptic curve point addition and doubling */
    fio___ed25519_point_add(R, R + 1);
    fio___ed25519_point_double(R + bit);
  }

  /* Step 3: The final result is stored in R0 */
  *result = R[0];
}

/* Helper function: Modular reduction for Ed25519 */
FIO_IFUNC void fio___ed25519_mod_reduce(fio_u256 *s) {
  /* TODO: Implement modular reduction for Ed25519 scalar */
}

/* ED25519 Base Point (G) */
const fio_u512 FIO___ED25519_BASEPOINT = {
    .u64 =
        {
            0x216936D3CD6E53FEULL, /* x-coordinate (lower 64 bits) */
            0xC0A4E231FDD6DC5CULL, /* x-coordinate (upper 64 bits) */
            0x6666666666666666ULL, /* y-coordinate (lower 64 bits) */
            0x6666666666666666ULL  /* y-coordinate (upper 64 bits) */
        },
};

/* Generate ED25519 keypair */
SFUNC fio_ed25519_s fio_ed25519_keypair(void) {
  fio_ed25519_s keypair;
  /* Generate the 512-bit (clamped) private key */
  keypair.private_key.u64[0] = fio_rand64();
  keypair.private_key.u64[1] = fio_rand64();
  keypair.private_key.u64[2] = fio_rand64();
  keypair.private_key.u64[3] = fio_rand64();
  keypair.private_key = fio_sha512(keypair.private_key.u8, 32);
  fio___ed25519_clamp_on_key(keypair.private_key.u8);
  /* TODO: Derive the public key */
  fio_u256_mul(fio_u512 * result, const fio_u256 *a, const fio_u256 *b)
      fio___ed25519_mul(&keypair.public_key,
                        &keypair.private_key,
                        &FIO___ED25519_BASEPOINT);
  /* Maybe... */

  /* Mask data, so it's harder to scrape in case of a memory dump. */
  fio___ed25519_flip(&keypair);
  return keypair;
}

/* Sign a message using ED25519 */
SFUNC void fio_ed25519_sign(uint8_t *signature,
                            const fio_buf_info_s message,
                            const fio_ed25519_s *keypair) {
  fio_sha512_s sha;
  fio_u512 r, h;
  fio_u256 R;

  /* Step 1: Hash the private key and message */
  sha = fio_sha512_init();
  fio_sha512_consume(&sha,
                     keypair->private_key.u8 + 32,
                     32); /* Hash private key second part */
  fio_sha512_consume(&sha, message.buf, message.len); /* Hash the message */
  r = fio_sha512_finalize(&sha);                      /* Finalize the hash */

  /* Step 2: Clamp and scalar multiply */
  fio___ed25519_clamp_on_key(r.u8);
  fio___ed25519_mul((fio_ed25519_s *)&R, &r, &FIO___ED25519_BASEPOINT);

  /* Step 3: Compute 's' */
  sha = fio_sha512_init();
  fio_sha512_consume(&sha, R.u8, 32);                 /* Hash R */
  fio_sha512_consume(&sha, message.buf, message.len); /* Hash message */
  h = fio_sha512_finalize(&sha);  /* Compute H(R || message) */
  fio___ed25519_mod_reduce(h.u8); /* Modular reduction of the hash */

  /* Step 4: Create the signature */
  memcpy(signature, R.u8, 32);      /* Copy R to the signature */
  memcpy(signature + 32, h.u8, 32); /* Copy the reduced hash 's' */
}

/* Verify an ED25519 signature */
SFUNC int fio_ed25519_verify(const uint8_t *signature,
                             const fio_buf_info_s message,
                             const fio_u256 *public_key) {
  fio_sha512_s sha;
  fio_u512 r, h;
  uint8_t calculated_R[32];

  /* Step 1: Recalculate R */
  sha = fio_sha512_init();
  fio_sha512_consume(&sha, public_key->u8, 32);       /* Hash the public key */
  fio_sha512_consume(&sha, message.buf, message.len); /* Hash the message */
  r = fio_sha512_finalize(&sha);                      /* Finalize the hash */

  fio___ed25519_mul((fio_ed25519_s *)calculated_R, &r, &public_key->u8);

  /* Step 2: Compare calculated R with signature R using FIO_MEMCMP */
  return FIO_MEMCMP(calculated_R, signature, 32) == 0;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_ED25519
#endif /* FIO_ED25519 */
