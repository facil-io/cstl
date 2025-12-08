/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ED25519            /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Elliptic Curve Cryptography: Ed25519 & X25519




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ED25519) && !defined(H___FIO_ED25519___H)
#define H___FIO_ED25519___H

/* *****************************************************************************
Curve25519 Cryptography Module

This module provides:
- Ed25519: Digital signatures (sign/verify)
- X25519:  Key exchange (ECDH) for deriving shared secrets

These are the minimal building blocks for secure inter-machine communication.
A tested cryptographic library (e.g., OpenSSL) is preferred when available,
but this implementation provides security when no alternative exists.

**Note**: This implementation has not been audited. Use at your own risk.
***************************************************************************** */

/* *****************************************************************************
Ed25519 Digital Signatures API

Ed25519 provides fast, secure digital signatures with 128-bit security level.
- Secret key (sk): 32 bytes (expanded internally to 64 bytes)
- Public key (pk): 32 bytes
- Signature:       64 bytes
***************************************************************************** */

/**
 * Generates a new random Ed25519 key pair.
 *
 * The secret key must be kept secret and securely erased when no longer
 * needed. The public key can be freely shared.
 */
SFUNC void fio_ed25519_keypair(uint8_t secret_key[32], uint8_t public_key[32]);

/**
 * Derives the public key from an Ed25519 secret key.
 *
 * Useful when the secret key is loaded from storage and the public key
 * needs to be recomputed.
 */
SFUNC void fio_ed25519_public_key(uint8_t public_key[32],
                                  const uint8_t secret_key[32]);

/**
 * Signs a message using Ed25519.
 *
 * The signature is 64 bytes and is deterministic (same message + key = same
 * signature).
 */
SFUNC void fio_ed25519_sign(uint8_t signature[64],
                            const void *message,
                            size_t len,
                            const uint8_t secret_key[32],
                            const uint8_t public_key[32]);

/**
 * Verifies an Ed25519 signature.
 *
 * Returns 0 on success (valid signature), -1 on failure (invalid signature).
 */
SFUNC int fio_ed25519_verify(const uint8_t signature[64],
                             const void *message,
                             size_t len,
                             const uint8_t public_key[32]);

/* *****************************************************************************
X25519 Key Exchange (ECDH) API

X25519 provides Elliptic Curve Diffie-Hellman key exchange with 128-bit
security level. Two parties can derive a shared secret using their secret
key and the other party's public key.

- Secret key (sk): 32 bytes
- Public key (pk): 32 bytes
- Shared secret:   32 bytes (should be passed through a KDF before use)
***************************************************************************** */

/**
 * Generates a new random X25519 key pair.
 *
 * The secret key must be kept secret. The public key can be shared with
 * the other party for key exchange.
 */
SFUNC void fio_x25519_keypair(uint8_t secret_key[32], uint8_t public_key[32]);

/**
 * Derives the public key from an X25519 secret key.
 *
 * This performs scalar multiplication of the secret key with the base point.
 */
SFUNC void fio_x25519_public_key(uint8_t public_key[32],
                                 const uint8_t secret_key[32]);

/**
 * Computes a shared secret using X25519 (ECDH).
 *
 * Both parties compute the same shared secret:
 *   shared = X25519(my_secret, their_public)
 *
 * The shared secret should be passed through a KDF (e.g., HKDF with SHA-256)
 * before being used as an encryption key.
 *
 * Returns 0 on success, -1 on failure (e.g., if their_public is a low-order
 * point, which would result in an all-zero shared secret).
 */
SFUNC int fio_x25519_shared_secret(uint8_t shared_secret[32],
                                   const uint8_t secret_key[32],
                                   const uint8_t their_public_key[32]);

/* *****************************************************************************
Key Conversion API

Ed25519 and X25519 use the same underlying curve but with different
representations. These functions convert between the two formats, allowing
a single key pair to be used for both signing and encryption.

Note: Converting keys is generally safe, but using the same key for both
signing and encryption is debated. Consider using separate key pairs for
maximum security.
***************************************************************************** */

/**
 * Converts an Ed25519 secret key to an X25519 secret key.
 *
 * This allows using an Ed25519 signing key for X25519 key exchange.
 */
SFUNC void fio_ed25519_sk_to_x25519(uint8_t x_secret_key[32],
                                    const uint8_t ed_secret_key[32]);

/**
 * Converts an Ed25519 public key to an X25519 public key.
 *
 * This allows encrypting to someone who has only shared their Ed25519
 * signing public key.
 */
SFUNC void fio_ed25519_pk_to_x25519(uint8_t x_public_key[32],
                                    const uint8_t ed_public_key[32]);

/* *****************************************************************************
Public Key Encryption API (ECIES - Elliptic Curve Integrated Encryption Scheme)

This provides asymmetric encryption where anyone can encrypt a message using
only the recipient's public key, and only the recipient can decrypt it using
their private key. No prior key exchange or handshake is required.

The scheme uses:
- X25519 for ephemeral key agreement
- SHA-256 for key derivation (HKDF-like)
- ChaCha20-Poly1305 for authenticated encryption

Ciphertext format: [32-byte ephemeral public key][16-byte MAC][encrypted data]
Total overhead: 48 bytes

Note: Requires FIO_CHACHA to be defined for ChaCha20-Poly1305 support.
***************************************************************************** */

/**
 * Encrypts a message using the recipient's X25519 public key.
 *
 * The ciphertext includes:
 * - 32 bytes: ephemeral public key (for key agreement)
 * - 16 bytes: authentication tag (MAC)
 * - N bytes:  encrypted message
 *
 * Total ciphertext size = message_len + 48 bytes
 *
 * @param ciphertext Output buffer (must be at least message_len + 48 bytes)
 * @param message    The plaintext message to encrypt
 * @param message_len Length of the message
 * @param recipient_pk The recipient's X25519 public key (32 bytes)
 * @return 0 on success, -1 on failure
 */
SFUNC int fio_x25519_encrypt(uint8_t *ciphertext,
                             const void *message,
                             size_t message_len,
                             const uint8_t recipient_pk[32]);

/**
 * Decrypts a message using the recipient's X25519 secret key.
 *
 * @param plaintext   Output buffer (must be at least ciphertext_len - 48 bytes)
 * @param ciphertext  The ciphertext (ephemeral_pk || mac || encrypted_data)
 * @param ciphertext_len Length of the ciphertext (must be >= 48)
 * @param recipient_sk The recipient's X25519 secret key (32 bytes)
 * @return 0 on success, -1 on failure (authentication failed or invalid input)
 */
SFUNC int fio_x25519_decrypt(uint8_t *plaintext,
                             const uint8_t *ciphertext,
                             size_t ciphertext_len,
                             const uint8_t recipient_sk[32]);

/**
 * Returns the ciphertext length for a given plaintext length.
 * Ciphertext = ephemeral_pk (32) + mac (16) + encrypted_message (message_len)
 */
#define FIO_X25519_CIPHERTEXT_LEN(message_len) ((message_len) + 48)

/**
 * Returns the plaintext length for a given ciphertext length.
 * Returns 0 if ciphertext_len < 48 (invalid ciphertext).
 */
#define FIO_X25519_PLAINTEXT_LEN(ciphertext_len)                               \
  ((ciphertext_len) >= 48 ? ((ciphertext_len)-48) : 0)

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Field Arithmetic for GF(2^255 - 19)

We use a radix-2^16 representation with 16 limbs (like TweetNaCl).
Each limb is stored in an int64_t to allow for lazy reduction.

This representation is simpler and well-tested, though slower than
optimized radix-2^51 implementations.
***************************************************************************** */

/* field element: 16 limbs in radix 2^16 */
typedef int64_t fio___gf_s[16];

/* carry propagation and reduction */
FIO_IFUNC void fio___gf_carry(fio___gf_s o) {
  int64_t c;
  for (int i = 0; i < 16; ++i) {
    o[i] += (1LL << 16);
    c = o[i] >> 16;
    o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
    o[i] -= c << 16;
  }
}

/* load a 32-byte little-endian number into field element */
FIO_IFUNC void fio___gf_frombytes(fio___gf_s r, const uint8_t in[32]) {
  for (int i = 0; i < 16; ++i)
    r[i] = in[2 * i] + ((int64_t)in[2 * i + 1] << 8);
  r[15] &= 0x7FFF;
}

/* conditional swap: swap p and q if b is 1, else no-op (constant time) */
FIO_IFUNC void fio___gf_cswap(fio___gf_s p, fio___gf_s q, int b) {
  int64_t t, c = ~((int64_t)b - 1);
  for (int i = 0; i < 16; ++i) {
    t = c & (p[i] ^ q[i]);
    p[i] ^= t;
    q[i] ^= t;
  }
}

/* store field element to 32-byte little-endian output */
FIO_IFUNC void fio___gf_tobytes(uint8_t out[32], fio___gf_s n) {
  int i, j;
  fio___gf_s m, t;
  for (i = 0; i < 16; ++i)
    t[i] = n[i];
  fio___gf_carry(t);
  fio___gf_carry(t);
  fio___gf_carry(t);
  for (j = 0; j < 2; ++j) {
    m[0] = t[0] - 0xffed;
    for (i = 1; i < 15; ++i) {
      m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
      m[i - 1] &= 0xffff;
    }
    m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
    int swap = (m[15] >> 16) & 1;
    m[14] &= 0xffff;
    fio___gf_cswap(t, m, 1 - swap);
  }
  for (i = 0; i < 16; ++i) {
    out[2 * i] = t[i] & 0xff;
    out[2 * i + 1] = t[i] >> 8;
  }
}

/* field element addition: h = f + g */
FIO_IFUNC void fio___gf_add(fio___gf_s h,
                            const fio___gf_s f,
                            const fio___gf_s g) {
  for (int i = 0; i < 16; ++i)
    h[i] = f[i] + g[i];
}

/* field element subtraction: h = f - g */
FIO_IFUNC void fio___gf_sub(fio___gf_s h,
                            const fio___gf_s f,
                            const fio___gf_s g) {
  for (int i = 0; i < 16; ++i)
    h[i] = f[i] - g[i];
}

/* field element multiplication: o = f * g */
FIO_IFUNC void fio___gf_mul(fio___gf_s o,
                            const fio___gf_s f,
                            const fio___gf_s g) {
  int64_t i, j, t[31];
  for (i = 0; i < 31; ++i)
    t[i] = 0;
  for (i = 0; i < 16; ++i)
    for (j = 0; j < 16; ++j)
      t[i + j] += f[i] * g[j];
  for (i = 0; i < 15; ++i)
    t[i] += 38 * t[i + 16];
  for (i = 0; i < 16; ++i)
    o[i] = t[i];
  fio___gf_carry(o);
  fio___gf_carry(o);
}

/* field element squaring: o = f^2 */
FIO_IFUNC void fio___gf_sqr(fio___gf_s o, const fio___gf_s f) {
  fio___gf_mul(o, f, f);
}

/* field element inversion: o = 1/i using Fermat's little theorem */
/* f^(-1) = f^(p-2) where p = 2^255 - 19 */
FIO_IFUNC void fio___gf_inv(fio___gf_s o, const fio___gf_s i) {
  fio___gf_s c;
  int a;
  for (a = 0; a < 16; ++a)
    c[a] = i[a];
  for (a = 253; a >= 0; --a) {
    fio___gf_sqr(c, c);
    if (a != 2 && a != 4)
      fio___gf_mul(c, c, i);
  }
  for (a = 0; a < 16; ++a)
    o[a] = c[a];
}

/* compute f^((p-5)/8) for square root computation */
FIO_IFUNC void fio___gf_pow_pm5d8(fio___gf_s o, const fio___gf_s i) {
  fio___gf_s c;
  int a;
  for (a = 0; a < 16; ++a)
    c[a] = i[a];
  for (a = 250; a >= 0; --a) {
    fio___gf_sqr(c, c);
    if (a != 1)
      fio___gf_mul(c, c, i);
  }
  for (a = 0; a < 16; ++a)
    o[a] = c[a];
}

/* check if field element is zero */
FIO_IFUNC int fio___gf_iszero(fio___gf_s f) {
  uint8_t s[32];
  fio___gf_tobytes(s, f);
  uint8_t r = 0;
  for (int i = 0; i < 32; ++i)
    r |= s[i];
  return r == 0;
}

/* check if field element is negative (LSB of canonical form) */
FIO_IFUNC int fio___gf_isneg(fio___gf_s f) {
  uint8_t s[32];
  fio___gf_tobytes(s, f);
  return s[0] & 1;
}

/* field element negation: o = -f */
FIO_IFUNC void fio___gf_neg(fio___gf_s o, const fio___gf_s f) {
  for (int i = 0; i < 16; ++i)
    o[i] = -f[i];
}

/* set field element to 1 */
FIO_IFUNC void fio___gf_one(fio___gf_s r) {
  r[0] = 1;
  for (int i = 1; i < 16; ++i)
    r[i] = 0;
}

/* set field element to 0 */
FIO_IFUNC void fio___gf_zero(fio___gf_s r) {
  for (int i = 0; i < 16; ++i)
    r[i] = 0;
}

/* copy field element: r = a */
FIO_IFUNC void fio___gf_copy(fio___gf_s r, const fio___gf_s a) {
  for (int i = 0; i < 16; ++i)
    r[i] = a[i];
}

/* *****************************************************************************
X25519 Implementation - Montgomery Ladder
***************************************************************************** */

/* x25519 base point (u = 9) */
static const uint8_t FIO___X25519_BASEPOINT[32] = {9};

/* x25519 scalar multiplication using montgomery ladder (matches tweetnacl) */
FIO_IFUNC void fio___x25519_scalarmult(uint8_t out[32],
                                       const uint8_t scalar[32],
                                       const uint8_t point[32]) {
  uint8_t z[32];
  FIO_MEMCPY(z, scalar, 32);
  z[31] = (scalar[31] & 127) | 64;
  z[0] &= 248;

  fio___gf_s x, a, b, c, d, e, f;

  fio___gf_frombytes(x, point);

  /* initialize: a=1, b=x, c=0, d=1 */
  for (int i = 0; i < 16; ++i) {
    b[i] = x[i];
    d[i] = a[i] = c[i] = 0;
  }
  a[0] = d[0] = 1;

  /* constant for curve parameter (a-2)/4 = 121665 */
  static const fio___gf_s k121665 =
      {0xDB41, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 254; i >= 0; --i) {
    int64_t r = (z[i >> 3] >> (i & 7)) & 1;
    fio___gf_cswap(a, b, r);
    fio___gf_cswap(c, d, r);
    fio___gf_add(e, a, c);
    fio___gf_sub(a, a, c);
    fio___gf_add(c, b, d);
    fio___gf_sub(b, b, d);
    fio___gf_sqr(d, e);
    fio___gf_sqr(f, a);
    fio___gf_mul(a, c, a);
    fio___gf_mul(c, b, e);
    fio___gf_add(e, a, c);
    fio___gf_sub(a, a, c);
    fio___gf_sqr(b, a);
    fio___gf_sub(c, d, f);
    fio___gf_mul(a, c, k121665);
    fio___gf_add(a, a, d);
    fio___gf_mul(c, c, a);
    fio___gf_mul(a, d, f);
    fio___gf_mul(d, b, x);
    fio___gf_sqr(b, e);
    fio___gf_cswap(a, b, r);
    fio___gf_cswap(c, d, r);
  }

  fio___gf_inv(c, c);
  fio___gf_mul(a, a, c);
  fio___gf_tobytes(out, a);
}

/* *****************************************************************************
X25519 Public API Implementation
***************************************************************************** */

SFUNC void fio_x25519_keypair(uint8_t secret_key[32], uint8_t public_key[32]) {
  fio_rand_bytes(secret_key, 32);
  fio_x25519_public_key(public_key, secret_key);
}

SFUNC void fio_x25519_public_key(uint8_t public_key[32],
                                 const uint8_t secret_key[32]) {
  fio___x25519_scalarmult(public_key, secret_key, FIO___X25519_BASEPOINT);
}

SFUNC int fio_x25519_shared_secret(uint8_t shared_secret[32],
                                   const uint8_t secret_key[32],
                                   const uint8_t their_public_key[32]) {
  fio___x25519_scalarmult(shared_secret, secret_key, their_public_key);

  /* check for all-zero output (low-order point attack) */
  uint8_t zero_check = 0;
  for (int i = 0; i < 32; ++i)
    zero_check |= shared_secret[i];

  return zero_check ? 0 : -1;
}

/* *****************************************************************************
Ed25519 Implementation - Extended Coordinates

Ed25519 uses the twisted Edwards curve: -x^2 + y^2 = 1 + d*x^2*y^2
where d = -121665/121666

Points are represented in extended coordinates (x:y:z:t) where
x = x/z, y = y/z, x*y = t/z
***************************************************************************** */

/* ed25519 group element in extended coordinates (x, y, z, t) */
typedef fio___gf_s fio___ge_p3_s[4];

/* clang-format off */
/* d = -121665/121666 mod p */
static const fio___gf_s FIO___ED25519_D = {
    0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070,
    0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203};

/* 2*d */
static const fio___gf_s FIO___ED25519_D2 = {
    0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0,
    0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406};

/* sqrt(-1) mod p */
static const fio___gf_s FIO___ED25519_SQRTM1 = {
    0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43,
    0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83};

/* ed25519 base point coordinates */
/* y = 4/5 */
static const fio___gf_s FIO___ED25519_BASE_Y = {
    0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
    0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666};

/* x = recovered from y */
static const fio___gf_s FIO___ED25519_BASE_X = {
    0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c,
    0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169};
/* clang-format on */

/* set p to the base point */
FIO_IFUNC void fio___ge_p3_base(fio___ge_p3_s p) {
  fio___gf_copy(p[0], FIO___ED25519_BASE_X);
  fio___gf_copy(p[1], FIO___ED25519_BASE_Y);
  fio___gf_one(p[2]);
  fio___gf_mul(p[3], p[0], p[1]);
}

/* swap two p3 points conditionally (constant time) */
FIO_IFUNC void fio___ge_p3_cswap(fio___ge_p3_s p, fio___ge_p3_s q, int b) {
  fio___gf_cswap(p[0], q[0], b);
  fio___gf_cswap(p[1], q[1], b);
  fio___gf_cswap(p[2], q[2], b);
  fio___gf_cswap(p[3], q[3], b);
}

/* point addition: p = p + q (in-place, matches tweetnacl's add function) */
FIO_IFUNC void fio___ge_p3_add(fio___ge_p3_s p, const fio___ge_p3_s q) {
  fio___gf_s a, b, c, d, t, e, f, g, h;

  fio___gf_sub(a, p[1], p[0]);
  fio___gf_sub(t, q[1], q[0]);
  fio___gf_mul(a, a, t);
  fio___gf_add(b, p[0], p[1]);
  fio___gf_add(t, q[0], q[1]);
  fio___gf_mul(b, b, t);
  fio___gf_mul(c, p[3], q[3]);
  fio___gf_mul(c, c, FIO___ED25519_D2);
  fio___gf_mul(d, p[2], q[2]);
  fio___gf_add(d, d, d);
  fio___gf_sub(e, b, a);
  fio___gf_sub(f, d, c);
  fio___gf_add(g, d, c);
  fio___gf_add(h, b, a);

  fio___gf_mul(p[0], e, f);
  fio___gf_mul(p[1], h, g);
  fio___gf_mul(p[2], g, f);
  fio___gf_mul(p[3], e, h);
}

/* point doubling: p = 2*p (in-place) */
FIO_IFUNC void fio___ge_p3_dbl(fio___ge_p3_s p) {
  fio___gf_s a, b, c, d, t, e, f, g, h;

  fio___gf_sub(a, p[1], p[0]);
  fio___gf_sub(t, p[1], p[0]);
  fio___gf_mul(a, a, t);
  fio___gf_add(b, p[0], p[1]);
  fio___gf_add(t, p[0], p[1]);
  fio___gf_mul(b, b, t);
  fio___gf_mul(c, p[3], p[3]);
  fio___gf_mul(c, c, FIO___ED25519_D2);
  fio___gf_mul(d, p[2], p[2]);
  fio___gf_add(d, d, d);
  fio___gf_sub(e, b, a);
  fio___gf_sub(f, d, c);
  fio___gf_add(g, d, c);
  fio___gf_add(h, b, a);

  fio___gf_mul(p[0], e, f);
  fio___gf_mul(p[1], h, g);
  fio___gf_mul(p[2], g, f);
  fio___gf_mul(p[3], e, h);
}

/* scalar multiplication: r = scalar * base_point (montgomery ladder) */
FIO_IFUNC void fio___ge_scalarmult_base(fio___ge_p3_s r,
                                        const uint8_t scalar[32]) {
  fio___ge_p3_s p, q;

  /* p = identity point (0, 1, 1, 0) */
  fio___gf_zero(p[0]);
  fio___gf_one(p[1]);
  fio___gf_one(p[2]);
  fio___gf_zero(p[3]);

  /* q = base point */
  fio___ge_p3_base(q);

  for (int i = 255; i >= 0; --i) {
    uint8_t b = (scalar[i >> 3] >> (i & 7)) & 1;
    fio___ge_p3_cswap(p, q, b);
    fio___ge_p3_add(q, p);
    fio___ge_p3_dbl(p);
    fio___ge_p3_cswap(p, q, b);
  }

  fio___gf_copy(r[0], p[0]);
  fio___gf_copy(r[1], p[1]);
  fio___gf_copy(r[2], p[2]);
  fio___gf_copy(r[3], p[3]);
}

/* variable-base scalar multiplication: r = scalar * point */
FIO_SFUNC void fio___ge_scalarmult(fio___ge_p3_s r,
                                   const uint8_t scalar[32],
                                   fio___ge_p3_s point) {
  fio___ge_p3_s p, q;

  /* p = identity point */
  fio___gf_zero(p[0]);
  fio___gf_one(p[1]);
  fio___gf_one(p[2]);
  fio___gf_zero(p[3]);

  /* q = input point */
  fio___gf_copy(q[0], point[0]);
  fio___gf_copy(q[1], point[1]);
  fio___gf_copy(q[2], point[2]);
  fio___gf_copy(q[3], point[3]);

  for (int i = 255; i >= 0; --i) {
    uint8_t b = (scalar[i >> 3] >> (i & 7)) & 1;
    fio___ge_p3_cswap(p, q, b);
    fio___ge_p3_add(q, p);
    fio___ge_p3_dbl(p);
    fio___ge_p3_cswap(p, q, b);
  }

  fio___gf_copy(r[0], p[0]);
  fio___gf_copy(r[1], p[1]);
  fio___gf_copy(r[2], p[2]);
  fio___gf_copy(r[3], p[3]);
}

/* encode point to 32 bytes */
FIO_IFUNC void fio___ge_p3_tobytes(uint8_t out[32], fio___ge_p3_s p) {
  fio___gf_s z_inv, x, y;
  fio___gf_inv(z_inv, p[2]);
  fio___gf_mul(x, p[0], z_inv);
  fio___gf_mul(y, p[1], z_inv);
  fio___gf_tobytes(out, y);
  out[31] ^= (fio___gf_isneg(x) << 7);
}

/* decode 32 bytes to point (returns 0 on success, -1 on failure) */
FIO_SFUNC int fio___ge_p3_frombytes(fio___ge_p3_s p, const uint8_t in[32]) {
  uint8_t s[32];
  FIO_MEMCPY(s, in, 32);
  int x_sign = s[31] >> 7;
  s[31] &= 0x7F;

  fio___gf_frombytes(p[1], s);
  fio___gf_one(p[2]);

  /* compute x from y: x^2 = (y^2 - 1) / (d*y^2 + 1) */
  fio___gf_s y2, u, v, one;
  fio___gf_one(one);
  fio___gf_sqr(y2, p[1]);
  fio___gf_sub(u, y2, one); /* y^2 - 1 */
  fio___gf_mul(v, y2, FIO___ED25519_D);
  fio___gf_add(v, v, one); /* d*y^2 + 1 */

  /* x = u * v^3 * (u * v^7)^((p-5)/8) */
  fio___gf_s v3, uv3, v7, uv7, x;
  fio___gf_sqr(v3, v);
  fio___gf_mul(v3, v3, v);
  fio___gf_mul(uv3, u, v3);
  fio___gf_sqr(v7, v3);
  fio___gf_mul(v7, v7, v);
  fio___gf_mul(uv7, u, v7);
  fio___gf_pow_pm5d8(x, uv7);
  fio___gf_mul(x, x, uv3);

  /* check if x^2 * v == u */
  fio___gf_s x2, vx2, check;
  fio___gf_sqr(x2, x);
  fio___gf_mul(vx2, v, x2);
  fio___gf_sub(check, vx2, u);

  if (!fio___gf_iszero(check)) {
    /* try x * sqrt(-1) */
    fio___gf_mul(x, x, FIO___ED25519_SQRTM1);
    fio___gf_sqr(x2, x);
    fio___gf_mul(vx2, v, x2);
    fio___gf_sub(check, vx2, u);
    if (!fio___gf_iszero(check))
      return -1; /* invalid point */
  }

  /* adjust sign */
  if (fio___gf_isneg(x) != x_sign)
    fio___gf_neg(x, x);

  fio___gf_copy(p[0], x);
  fio___gf_mul(p[3], p[0], p[1]);
  return 0;
}

/* *****************************************************************************
Scalar Arithmetic mod L (Ed25519 group order)

L = 2^252 + 27742317777372353535851937790883648493
***************************************************************************** */

/* reduce a 64-byte hash to a scalar mod l */
FIO_SFUNC void fio___sc_reduce(uint8_t s[32], const uint8_t r[64]) {
  /* l = 2^252 + 27742317777372353535851937790883648493 */
  static const unsigned long long l[32] = {
      0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7,
      0xa2, 0xde, 0xf9, 0xde, 0x14, 0,    0,    0,    0,    0,    0,
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0x10};
  long long x[64];
  int i;
  int j;
  for (i = 0; i < 64; ++i)
    x[i] = (unsigned long long)r[i];

  for (i = 63; i >= 32; --i) {
    long long carry = 0;
    for (j = i - 32; j < i - 12; ++j) {
      x[j] += carry - 16 * x[i] * l[j - (i - 32)];
      carry = (x[j] + 128) >> 8;
      x[j] -= carry << 8;
    }
    x[j] += carry;
    x[i] = 0;
  }
  long long carry = 0;
  for (j = 0; j < 32; ++j) {
    x[j] += carry - (x[31] >> 4) * l[j];
    carry = x[j] >> 8;
    x[j] &= 255;
  }
  for (j = 0; j < 32; ++j)
    x[j] -= carry * l[j];
  for (i = 0; i < 32; ++i) {
    x[i + 1] += x[i] >> 8;
    s[i] = x[i] & 255;
  }
}

/* compute s = (a + b*c) mod l */
FIO_SFUNC void fio___sc_muladd(uint8_t s[32],
                               const uint8_t a[32],
                               const uint8_t b[32],
                               const uint8_t c[32]) {
  long long x[65] = {0};
  int i, j;
  for (i = 0; i < 32; ++i)
    x[i] = (unsigned long long)a[i];
  for (i = 0; i < 32; ++i)
    for (j = 0; j < 32; ++j)
      x[i + j] += (unsigned long long)b[i] * (unsigned long long)c[j];
  uint8_t tmp[65];
  for (i = 0; i < 64; ++i) {
    x[i + 1] += x[i] >> 8;
    tmp[i] = x[i] & 255;
  }
  fio___sc_reduce(s, tmp);
}

/* *****************************************************************************
Ed25519 Public API Implementation
***************************************************************************** */

SFUNC void fio_ed25519_keypair(uint8_t secret_key[32], uint8_t public_key[32]) {
  fio_rand_bytes(secret_key, 32);
  fio_ed25519_public_key(public_key, secret_key);
}

SFUNC void fio_ed25519_public_key(uint8_t public_key[32],
                                  const uint8_t secret_key[32]) {
  /* hash secret key with sha-512 */
  fio_u512 h = fio_sha512(secret_key, 32);

  /* clamp the first 32 bytes */
  h.u8[0] &= 248;
  h.u8[31] &= 127;
  h.u8[31] |= 64;

  /* compute public key = h * b */
  fio___ge_p3_s pk;
  fio___ge_scalarmult_base(pk, h.u8);
  fio___ge_p3_tobytes(public_key, pk);
}

SFUNC void fio_ed25519_sign(uint8_t signature[64],
                            const void *message,
                            size_t len,
                            const uint8_t secret_key[32],
                            const uint8_t public_key[32]) {
  /* hash secret key */
  fio_u512 h = fio_sha512(secret_key, 32);

  /* clamp */
  h.u8[0] &= 248;
  h.u8[31] &= 127;
  h.u8[31] |= 64;

  /* r = h(h[32..63] || message) mod l */
  fio_sha512_s sha = fio_sha512_init();
  fio_sha512_consume(&sha, h.u8 + 32, 32);
  fio_sha512_consume(&sha, message, len);
  fio_u512 r_hash = fio_sha512_finalize(&sha);

  uint8_t r[32];
  fio___sc_reduce(r, r_hash.u8);

  /* r_point = r * b */
  fio___ge_p3_s r_point;
  fio___ge_scalarmult_base(r_point, r);
  fio___ge_p3_tobytes(signature, r_point); /* first 32 bytes of signature */

  /* k = h(r || public_key || message) mod l */
  sha = fio_sha512_init();
  fio_sha512_consume(&sha, signature, 32);
  fio_sha512_consume(&sha, public_key, 32);
  fio_sha512_consume(&sha, message, len);
  fio_u512 k_hash = fio_sha512_finalize(&sha);

  uint8_t k[32];
  fio___sc_reduce(k, k_hash.u8);

  /* s = (r + k * h) mod l */
  fio___sc_muladd(signature + 32, r, k, h.u8);
}

SFUNC int fio_ed25519_verify(const uint8_t signature[64],
                             const void *message,
                             size_t len,
                             const uint8_t public_key[32]) {
  /* decode public key */
  fio___ge_p3_s pk;
  if (fio___ge_p3_frombytes(pk, public_key) != 0)
    return -1;

  /* check s < l */
  static const uint8_t l[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                                0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};
  const uint8_t *s = signature + 32;
  uint8_t c = 0;
  for (int i = 31; i >= 0; --i) {
    c = (s[i] > l[i]) | ((s[i] == l[i]) & c);
  }
  if (c)
    return -1;

  /* k = h(r || public_key || message) mod l */
  fio_sha512_s sha = fio_sha512_init();
  fio_sha512_consume(&sha, signature, 32);
  fio_sha512_consume(&sha, public_key, 32);
  fio_sha512_consume(&sha, message, len);
  fio_u512 k_hash = fio_sha512_finalize(&sha);

  uint8_t k[32];
  fio___sc_reduce(k, k_hash.u8);

  /* compute s*b - k*a */
  /* first compute [s]b */
  fio___ge_p3_s sb;
  fio___ge_scalarmult_base(sb, signature + 32);

  /* compute [k]a (need to negate for subtraction) */
  fio___ge_p3_s ka;
  fio___ge_scalarmult(ka, k, pk);

  /* negate ka: negate x and t coordinates */
  fio___gf_neg(ka[0], ka[0]);
  fio___gf_neg(ka[3], ka[3]);

  /* add sb + (-ka) */
  fio___ge_p3_add(sb, ka);

  /* encode result and compare with r */
  uint8_t check[32];
  fio___ge_p3_tobytes(check, sb);

  /* constant-time comparison */
  uint8_t diff = 0;
  for (int i = 0; i < 32; ++i)
    diff |= check[i] ^ signature[i];

  return diff ? -1 : 0;
}

/* *****************************************************************************
Key Conversion Implementation
***************************************************************************** */

SFUNC void fio_ed25519_sk_to_x25519(uint8_t x_secret_key[32],
                                    const uint8_t ed_secret_key[32]) {
  /* hash ed25519 secret key and use first 32 bytes */
  fio_u512 h = fio_sha512(ed_secret_key, 32);
  FIO_MEMCPY(x_secret_key, h.u8, 32);
  /* x25519 clamping is done in the scalar multiplication */
}

SFUNC void fio_ed25519_pk_to_x25519(uint8_t x_public_key[32],
                                    const uint8_t ed_public_key[32]) {
  /* ed25519 public key is (x, y) on edwards curve
   * x25519 public key is u on montgomery curve
   * conversion: u = (1 + y) / (1 - y)
   */
  fio___ge_p3_s p;
  if (fio___ge_p3_frombytes(p, ed_public_key) != 0) {
    /* invalid point - return zeros */
    FIO_MEMSET(x_public_key, 0, 32);
    return;
  }

  /* compute u = (z + y) / (z - y) */
  fio___gf_s one_plus_y, one_minus_y, one_minus_y_inv, u;
  fio___gf_add(one_plus_y, p[2], p[1]);
  fio___gf_sub(one_minus_y, p[2], p[1]);
  fio___gf_inv(one_minus_y_inv, one_minus_y);
  fio___gf_mul(u, one_plus_y, one_minus_y_inv);

  fio___gf_tobytes(x_public_key, u);
}

/* *****************************************************************************
ECIES Public Key Encryption Implementation

Encrypts data using X25519 key agreement + ChaCha20-Poly1305.
Format: [32-byte ephemeral public key][16-byte MAC][encrypted data]
***************************************************************************** */

SFUNC int fio_x25519_encrypt(uint8_t *ciphertext,
                             const void *message,
                             size_t message_len,
                             const uint8_t recipient_pk[32]) {
  /* generate ephemeral key pair */
  uint8_t eph_sk[32], eph_pk[32];
  fio_x25519_keypair(eph_sk, eph_pk);

  /* compute shared secret */
  uint8_t shared[32];
  if (fio_x25519_shared_secret(shared, eph_sk, recipient_pk) != 0) {
    fio_secure_zero(eph_sk, 32);
    return -1; /* invalid recipient public key */
  }

  /* derive encryption key using sha-256(shared_secret || ephemeral_pk) */
  fio_sha256_s sha = fio_sha256_init();
  fio_sha256_consume(&sha, shared, 32);
  fio_sha256_consume(&sha, eph_pk, 32);
  fio_u256 key = fio_sha256_finalize(&sha);

  /* clear sensitive data */
  fio_secure_zero(eph_sk, 32);
  fio_secure_zero(shared, 32);

  /* copy ephemeral public key to output */
  FIO_MEMCPY(ciphertext, eph_pk, 32);

  /* copy plaintext to ciphertext buffer (after eph_pk and mac space) */
  if (message_len > 0)
    FIO_MEMCPY(ciphertext + 48, message, message_len);

  /* encrypt with chacha20-poly1305 */
  /* nonce: first 12 bytes of ephemeral public key (unique per encryption) */
  fio_chacha20_poly1305_enc(ciphertext + 32, /* mac output */
                            ciphertext + 48, /* data to encrypt */
                            message_len,     /* data length */
                            ciphertext,      /* additional data: eph_pk */
                            32,              /* ad length */
                            key.u8,          /* encryption key */
                            eph_pk);         /* nonce (first 12 bytes) */

  fio_secure_zero(&key, sizeof(key));
  return 0;
}

SFUNC int fio_x25519_decrypt(uint8_t *plaintext,
                             const uint8_t *ciphertext,
                             size_t ciphertext_len,
                             const uint8_t recipient_sk[32]) {
  /* validate minimum ciphertext length */
  if (ciphertext_len < 48)
    return -1;

  size_t message_len = ciphertext_len - 48;
  const uint8_t *eph_pk = ciphertext;
  const uint8_t *mac = ciphertext + 32;
  const uint8_t *encrypted = ciphertext + 48;

  /* compute shared secret */
  uint8_t shared[32];
  if (fio_x25519_shared_secret(shared, recipient_sk, eph_pk) != 0) {
    return -1; /* invalid ephemeral public key */
  }

  /* derive decryption key using sha-256(shared_secret || ephemeral_pk) */
  fio_sha256_s sha = fio_sha256_init();
  fio_sha256_consume(&sha, shared, 32);
  fio_sha256_consume(&sha, eph_pk, 32);
  fio_u256 key = fio_sha256_finalize(&sha);

  fio_secure_zero(shared, 32);

  /* copy ciphertext to output buffer for in-place decryption */
  if (message_len > 0)
    FIO_MEMCPY(plaintext, encrypted, message_len);

  /* copy mac for in-place verification (chacha20_poly1305_dec modifies it) */
  uint8_t mac_copy[16];
  FIO_MEMCPY(mac_copy, mac, 16);

  /* decrypt and verify with chacha20-poly1305 */
  int result = fio_chacha20_poly1305_dec(mac_copy,    /* mac to verify */
                                         plaintext,   /* data to decrypt */
                                         message_len, /* data length */
                                         eph_pk,      /* additional data */
                                         32,          /* ad length */
                                         key.u8,      /* decryption key */
                                         eph_pk);     /* nonce */

  fio_secure_zero(&key, sizeof(key));
  return result;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_ED25519
#endif /* FIO_ED25519 */
