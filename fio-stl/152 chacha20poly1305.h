/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_CHACHA             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              ChaCha20 & Poly1305



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_CHACHA) && !defined(H___FIO_CHACHA___H)
#define H___FIO_CHACHA___H 1

/* *****************************************************************************
ChaCha20Poly1305 API
***************************************************************************** */

/**
 * Performs an in-place encryption of `data` using ChaCha20 with additional
 * data, producing a 16 byte message authentication code (MAC) using Poly1305.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce` MUST point to a  96 bit long memory address (12 Bytes).
 * * `ad`     MAY be omitted, will NOT be encrypted.
 * * `data`   MAY be omitted, WILL be encrypted.
 * * `mac`    MUST point to a buffer with (at least) 16 available bytes.
 */
SFUNC void fio_chacha20_poly1305_enc(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad, /* additional data */
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);

/**
 * Performs an in-place decryption of `data` using ChaCha20 after authenticating
 * the message authentication code (MAC) using Poly1305.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce` MUST point to a  96 bit long memory address (12 Bytes).
 * * `ad`     MAY be omitted ONLY IF originally omitted.
 * * `data`   MAY be omitted, WILL be decrypted.
 * * `mac`    MUST point to a buffer where the 16 byte MAC is placed.
 *
 * Returns `-1` on error (authentication failed).
 */
SFUNC int fio_chacha20_poly1305_dec(void *restrict mac,
                                    void *restrict data,
                                    size_t len,
                                    const void *ad, /* additional data */
                                    size_t adlen,
                                    const void *key,
                                    const void *nonce);

/* *****************************************************************************
XChaCha20Poly1305 API (Extended Nonce - 24 bytes)
***************************************************************************** */

/**
 * Performs an in-place encryption of `data` using XChaCha20 with additional
 * data, producing a 16 byte message authentication code (MAC) using Poly1305.
 *
 * XChaCha20 uses a 192-bit (24 byte) nonce, making it safe to use with
 * randomly-generated nonces without risk of nonce collision.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce`  MUST point to a 192 bit long memory address (24 Bytes).
 * * `ad`     MAY be omitted, will NOT be encrypted.
 * * `data`   MAY be omitted, WILL be encrypted.
 * * `mac`    MUST point to a buffer with (at least) 16 available bytes.
 */
SFUNC void fio_xchacha20_poly1305_enc(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad, /* additional data */
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce);

/**
 * Performs an in-place decryption of `data` using XChaCha20 after
 * authenticating the message authentication code (MAC) using Poly1305.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce`  MUST point to a 192 bit long memory address (24 Bytes).
 * * `ad`     MAY be omitted ONLY IF originally omitted.
 * * `data`   MAY be omitted, WILL be decrypted.
 * * `mac`    MUST point to a buffer where the 16 byte MAC is placed.
 *
 * Returns `-1` on error (authentication failed).
 */
SFUNC int fio_xchacha20_poly1305_dec(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad, /* additional data */
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);

/**
 * Performs an in-place encryption/decryption of `data` using XChaCha20.
 *
 * XChaCha20 uses a 192-bit (24 byte) nonce, making it safe to use with
 * randomly-generated nonces without risk of nonce collision.
 *
 * * `key`     MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce`   MUST point to a 192 bit long memory address (24 Bytes).
 * * `counter` is the block counter, usually 0 unless `data` is mid-cyphertext.
 */
SFUNC void fio_xchacha20(void *restrict data,
                         size_t len,
                         const void *key,
                         const void *nonce,
                         uint32_t counter);

/* *****************************************************************************
Using ChaCha20 and Poly1305 separately
***************************************************************************** */

/**
 * Performs an in-place encryption/decryption of `data` using ChaCha20.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nonce` MUST point to a  96 bit long memory address (12 Bytes).
 * * `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.
 */
SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nonce,
                        uint32_t counter);

/**
 * Given a Poly1305 256bit (32 byte) key, writes the authentication code for the
 * poly message and additional data into `mac_dest`.
 *
 * * `mac_dest` MUST point to a buffer with (at least) 16 available bytes.
 * * `message`  MAY be omitted.
 * * `ad`       MAY be omitted (additional data).
 * * `key`      MUST point to a 256 bit long memory address (32 Bytes).
 */
SFUNC void fio_poly1305_auth(void *restrict mac_dest,
                             void *restrict message,
                             size_t len,
                             const void *ad,
                             size_t ad_len,
                             const void *key256bits);

/* *****************************************************************************
ChaCha20Poly1305 Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Poly1305 (authentication)
Prime 2^130-5   = 0x3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB
The math is mostly copied from: https://github.com/floodyberry/poly1305-donna
***************************************************************************** */
/*
 * Math copied from https://github.com/floodyberry/poly1305-donna
 *
 * With thanks to Andrew Moon.
 */
typedef struct {
  /* r (cycle key addition) is 128 bits */
  uint64_t r[3];
  /* s (final key addition) is 128 bits */
  uint64_t s[2];
  /* Accumulator should not exceed 131 bits at the end of every cycle. */
  uint64_t a[3];
} FIO_ALIGN(64) fio___poly_s;

FIO_IFUNC fio___poly_s fio___poly_init(const void *key256b) {
  static const uint64_t defkey[4] = {0};
  if (!key256b)
    key256b = (const void *)defkey;
  uint64_t t0, t1;
  /* r &= 0xffffffc0ffffffc0ffffffc0fffffff */
  t0 = fio_buf2u64_le((uint8_t *)key256b + 0);
  t1 = fio_buf2u64_le((uint8_t *)key256b + 8);
  fio___poly_s pl = {
      .r =
          {
              ((t0)&0xffc0fffffff),
              (((t0 >> 44) | (t1 << 20)) & 0xfffffc0ffff),
              (((t1 >> 24)) & 0x00ffffffc0f),
          },
      .s =
          {
              fio_buf2u64_le(((uint8_t *)key256b + 16)),
              fio_buf2u64_le(((uint8_t *)key256b + 24)),
          },
  };
  return pl;
}
FIO_IFUNC void fio___poly_consume128bit(fio___poly_s *pl,
                                        const void *msg,
                                        uint64_t is_full) {
  uint64_t r0, r1, r2;
  uint64_t s1, s2;
  uint64_t a0, a1, a2;
  uint64_t c;
  uint64_t d0[2], d1[2], d2[2], d[2];

  r0 = pl->r[0];
  r1 = pl->r[1];
  r2 = pl->r[2];

  a0 = pl->a[0];
  a1 = pl->a[1];
  a2 = pl->a[2];

  s1 = r1 * (5 << 2);
  s2 = r2 * (5 << 2);

  {
    uint64_t t0, t1;
    t0 = fio_buf2u64_le(msg);
    t1 = fio_buf2u64_le(((uint8_t *)msg + 8));
    /* a += msg */
    a0 += ((t0)&0xFFFFFFFFFFF);
    a1 += (((t0 >> 44) | (t1 << 20)) & 0xFFFFFFFFFFF);
    a2 += (((t1 >> 24)) & 0x3FFFFFFFFFF) | (is_full << 40);
  }

  /* a *= r */
  d0[0] = fio_math_mulc64(a0, r0, d0 + 1);
  d[0] = fio_math_mulc64(a1, s2, d + 1);
  d0[0] = fio_math_addc64(d0[0], d[0], 0, &c);
  d0[1] += d[1] + c;

  d[0] = fio_math_mulc64(a2, s1, d + 1);
  d0[0] = fio_math_addc64(d0[0], d[0], 0, &c);
  d0[1] += d[1] + c;

  d1[0] = fio_math_mulc64(a0, r1, d1 + 1);
  d[0] = fio_math_mulc64(a1, r0, d + 1);
  d1[0] = fio_math_addc64(d1[0], d[0], 0, &c);
  d1[1] += d[1] + c;

  d[0] = fio_math_mulc64(a2, s2, d + 1);
  d1[0] = fio_math_addc64(d1[0], d[0], 0, &c);
  d1[1] += d[1] + c;

  d2[0] = fio_math_mulc64(a0, r2, d2 + 1);
  d[0] = fio_math_mulc64(a1, r1, d + 1);
  d2[0] = fio_math_addc64(d2[0], d[0], 0, &c);
  d2[1] += d[1] + c;

  d[0] = fio_math_mulc64(a2, r0, d + 1);
  d2[0] = fio_math_addc64(d2[0], d[0], 0, &c);
  d2[1] += d[1] + c;

  /* (partial) a %= p */
  c = (d0[0] >> 44) | (d0[1] << 20);
  a0 = d0[0] & 0xfffffffffff;
  d1[0] = fio_math_addc64(d1[0], c, 0, &c);
  d1[1] += c;

  c = (d1[0] >> 44) | (d1[1] << 20);
  a1 = d1[0] & 0xfffffffffff;
  d2[0] = fio_math_addc64(d2[0], c, 0, &c);
  d2[1] += c;

  c = (d2[0] >> 42) | (d2[1] << 22);
  a2 = d2[0] & 0x3ffffffffff;
  a0 += c * 5;
  c = a0 >> 44;
  a0 = a0 & 0xfffffffffff;
  a1 += c;

  pl->a[0] = a0;
  pl->a[1] = a1;
  pl->a[2] = a2;
}

FIO_IFUNC void fio___poly_finilize(fio___poly_s *pl) {
  uint64_t a0, a1, a2, c;
  uint64_t g0, g1, g2;
  uint64_t t0, t1;

  /* fully carry a */
  a0 = pl->a[0];
  a1 = pl->a[1];
  a2 = pl->a[2];

  c = (a1 >> 44);
  a1 &= 0xFFFFFFFFFFF;
  a2 += c;
  c = (a2 >> 42);
  a2 &= 0x3FFFFFFFFFF;
  a0 += c * 5;
  c = (a0 >> 44);
  a0 &= 0xFFFFFFFFFFF;
  a1 += c;
  c = (a1 >> 44);
  a1 &= 0xFFFFFFFFFFF;
  a2 += c;
  c = (a2 >> 42);
  a2 &= 0x3FFFFFFFFFF;
  a0 += c * 5;
  c = (a0 >> 44);
  a0 &= 0xFFFFFFFFFFF;
  a1 += c;

  /* compute a + -p */
  g0 = a0 + 5;
  c = (g0 >> 44);
  g0 &= 0xFFFFFFFFFFF;
  g1 = a1 + c;
  c = (g1 >> 44);
  g1 &= 0xFFFFFFFFFFF;
  g2 = a2 + c - ((uint64_t)1 << 42);

  /* select h if h < p, or h + -p if h >= p */
  c = (g2 >> ((sizeof(uint64_t) * 8) - 1)) - 1;
  g0 &= c;
  g1 &= c;
  g2 &= c;
  c = ~c;
  a0 = (a0 & c) | g0;
  a1 = (a1 & c) | g1;
  a2 = (a2 & c) | g2;

  /* a = (a + Poly S key) */
  t0 = pl->s[0];
  t1 = pl->s[1];

  a0 += ((t0)&0xFFFFFFFFFFF);
  c = (a0 >> 44);
  a0 &= 0xFFFFFFFFFFF;
  a1 += (((t0 >> 44) | (t1 << 20)) & 0xFFFFFFFFFFF) + c;
  c = (a1 >> 44);
  a1 &= 0xFFFFFFFFFFF;
  a2 += (((t1 >> 24)) & 0x3FFFFFFFFFF) + c;
  a2 &= 0x3FFFFFFFFFF;

  /* mac = a % (2^128) */
  a0 = ((a0) | (a1 << 44));
  a1 = ((a1 >> 20) | (a2 << 24));
  pl->a[0] = a0;
  pl->a[1] = a1;
}

FIO_IFUNC void fio___poly_consume_msg(fio___poly_s *pl,
                                      uint8_t *msg,
                                      size_t len) {
  /* read 16 byte blocks */
  uint64_t n[2];
  for (size_t i = 31; i < len; i += 32) {
    fio___poly_consume128bit(pl, msg, 1);
    fio___poly_consume128bit(pl, msg + 16, 1);
    msg += 32;
  }
  if ((len & 16)) {
    fio___poly_consume128bit(pl, msg, 1);
    msg += 16;
  }
  if ((len & 15)) {
    n[0] = 0;
    n[1] = 0;
    fio_memcpy15x(n, msg, len);
    n[0] = fio_ltole64(n[0]);
    n[1] = fio_ltole64(n[1]);
    ((uint8_t *)n)[len & 15] = 0x01;
    fio___poly_consume128bit(pl, (void *)n, 0);
  }
}

/* Given a Poly1305 key, writes a MAC into `mac_dest`. */
SFUNC void fio_poly1305_auth(void *restrict mac,
                             void *restrict msg,
                             size_t len,
                             const void *ad,
                             size_t ad_len,
                             const void *key) {
  fio___poly_s pl = fio___poly_init(key);
  fio___poly_consume_msg(&pl, (uint8_t *)ad, ad_len);
  fio___poly_consume_msg(&pl, (uint8_t *)msg, len);
  fio___poly_finilize(&pl);
  fio_u2buf64_le(mac, pl.a[0]);
  fio_u2buf64_le(&((char *)mac)[8], pl.a[1]);
  fio_secure_zero(&pl, sizeof(pl));
}

/* *****************************************************************************
ChaCha20 (encryption)
***************************************************************************** */

FIO_IFUNC fio_u512 fio___chacha_init(const void *key,
                                     const void *nonce,
                                     uint32_t counter) {
  fio_u512 o = {
      .u32 =
          {
              // clang-format off
              0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
              fio_buf2u32_le(key),
              fio_buf2u32_le((uint8_t *)key + 4),
              fio_buf2u32_le((uint8_t *)key + 8),
              fio_buf2u32_le((uint8_t *)key + 12),
              fio_buf2u32_le((uint8_t *)key + 16),
              fio_buf2u32_le((uint8_t *)key + 20),
              fio_buf2u32_le((uint8_t *)key + 24),
              fio_buf2u32_le((uint8_t *)key + 28),
              counter,
              fio_buf2u32_le(nonce),
              fio_buf2u32_le((uint8_t *)nonce + 4),
              fio_buf2u32_le((uint8_t *)nonce + 8),
          }, // clang-format on
  };
  return o;
}

/* Single-block ChaCha20: processes 64 bytes with explicit diagonal indexing. */
FIO_SFUNC void fio___chacha_vround20(const fio_u512 c, uint8_t *restrict data) {
  uint32_t v[16];
  for (size_t i = 0; i < 16; ++i)
    v[i] = c.u32[i];

/* Quarter round macro for a single block with explicit indices */
#define FIO___CHACHA_QR1(a, b, c, d)                                           \
  do {                                                                         \
    v[a] += v[b];                                                              \
    v[d] ^= v[a];                                                              \
    v[d] = fio_lrot32(v[d], 16);                                               \
    v[c] += v[d];                                                              \
    v[b] ^= v[c];                                                              \
    v[b] = fio_lrot32(v[b], 12);                                               \
    v[a] += v[b];                                                              \
    v[d] ^= v[a];                                                              \
    v[d] = fio_lrot32(v[d], 8);                                                \
    v[c] += v[d];                                                              \
    v[b] ^= v[c];                                                              \
    v[b] = fio_lrot32(v[b], 7);                                                \
  } while (0)

  for (size_t round__ = 0; round__ < 10; ++round__) {
    /* Column rounds */
    FIO___CHACHA_QR1(0, 4, 8, 12);
    FIO___CHACHA_QR1(1, 5, 9, 13);
    FIO___CHACHA_QR1(2, 6, 10, 14);
    FIO___CHACHA_QR1(3, 7, 11, 15);
    /* Diagonal rounds */
    FIO___CHACHA_QR1(0, 5, 10, 15);
    FIO___CHACHA_QR1(1, 6, 11, 12);
    FIO___CHACHA_QR1(2, 7, 8, 13);
    FIO___CHACHA_QR1(3, 4, 9, 14);
  }
#undef FIO___CHACHA_QR1

  for (size_t i = 0; i < 16; ++i)
    v[i] += c.u32[i];

#if __BIG_ENDIAN__
  for (size_t i = 0; i < 16; ++i)
    v[i] = fio_bswap32(v[i]);
#endif
  {
    uint32_t d[16];
    fio_memcpy64(d, data);
    for (size_t i = 0; i < 16; ++i)
      d[i] ^= v[i];
    fio_memcpy64(data, d);
  }
}

/* 2-block ChaCha20: processes 128 bytes using explicit diagonal indexing.
 * Avoids all shuffle operations by directly addressing diagonal elements.
 * Layout: v[0..15] = block 0, v[16..31] = block 1 (flat, no interleaving). */
FIO_SFUNC void fio___chacha_vround20x2(fio_u512 c, uint8_t *restrict data) {
  /* Two separate blocks in flat layout */
  uint32_t v0[16], v1[16];
  for (size_t i = 0; i < 16; ++i) {
    v0[i] = c.u32[i];
    v1[i] = c.u32[i];
  }
  ++v1[12]; /* second block has counter+1 */

/* Quarter round macro for a single block with explicit indices */
#define FIO___CHACHA_QR(s, a, b, c, d)                                         \
  do {                                                                         \
    s[a] += s[b];                                                              \
    s[d] ^= s[a];                                                              \
    s[d] = fio_lrot32(s[d], 16);                                               \
    s[c] += s[d];                                                              \
    s[b] ^= s[c];                                                              \
    s[b] = fio_lrot32(s[b], 12);                                               \
    s[a] += s[b];                                                              \
    s[d] ^= s[a];                                                              \
    s[d] = fio_lrot32(s[d], 8);                                                \
    s[c] += s[d];                                                              \
    s[b] ^= s[c];                                                              \
    s[b] = fio_lrot32(s[b], 7);                                                \
  } while (0)

  for (size_t round__ = 0; round__ < 10; ++round__) {
    /* Column rounds */
    FIO___CHACHA_QR(v0, 0, 4, 8, 12);
    FIO___CHACHA_QR(v0, 1, 5, 9, 13);
    FIO___CHACHA_QR(v0, 2, 6, 10, 14);
    FIO___CHACHA_QR(v0, 3, 7, 11, 15);
    FIO___CHACHA_QR(v1, 0, 4, 8, 12);
    FIO___CHACHA_QR(v1, 1, 5, 9, 13);
    FIO___CHACHA_QR(v1, 2, 6, 10, 14);
    FIO___CHACHA_QR(v1, 3, 7, 11, 15);
    /* Diagonal rounds — no shuffle needed, just different indices */
    FIO___CHACHA_QR(v0, 0, 5, 10, 15);
    FIO___CHACHA_QR(v0, 1, 6, 11, 12);
    FIO___CHACHA_QR(v0, 2, 7, 8, 13);
    FIO___CHACHA_QR(v0, 3, 4, 9, 14);
    FIO___CHACHA_QR(v1, 0, 5, 10, 15);
    FIO___CHACHA_QR(v1, 1, 6, 11, 12);
    FIO___CHACHA_QR(v1, 2, 7, 8, 13);
    FIO___CHACHA_QR(v1, 3, 4, 9, 14);
  }
#undef FIO___CHACHA_QR

  /* Add initial state and XOR with data */
  for (size_t i = 0; i < 16; ++i)
    v0[i] += c.u32[i];
  ++c.u32[12];
  for (size_t i = 0; i < 16; ++i)
    v1[i] += c.u32[i];

#if __BIG_ENDIAN__
  for (size_t i = 0; i < 16; ++i) {
    v0[i] = fio_bswap32(v0[i]);
    v1[i] = fio_bswap32(v1[i]);
  }
#endif
  {
    uint32_t d[16];
    fio_memcpy64(d, data);
    for (size_t i = 0; i < 16; ++i)
      d[i] ^= v0[i];
    fio_memcpy64(data, d);

    fio_memcpy64(d, data + 64);
    for (size_t i = 0; i < 16; ++i)
      d[i] ^= v1[i];
    fio_memcpy64(data + 64, d);
  }
}

/* *****************************************************************************
NEON 4-Block ChaCha20 (ARM NEON - processes 256 bytes per call)
***************************************************************************** */
#if FIO___HAS_ARM_INTRIN

/**
 * Quarter round macro for NEON vertical layout.
 * Each uint32x4_t holds the same word position from 4 different blocks.
 *
 * ROT16: vrev32q_u16 is a single REV32.8H instruction.
 * ROT8:  vqtbl1q_u8 with rotation table is a single TBL instruction.
 * ROT12/ROT7: shift+or (NEON has single-cycle shifts).
 */
#define FIO___CHACHA_QR_NEON(a, b, c, d)                                       \
  do {                                                                         \
    a = vaddq_u32(a, b);                                                       \
    d = veorq_u32(d, a);                                                       \
    d = vreinterpretq_u32_u16(vrev32q_u16(vreinterpretq_u16_u32(d)));          \
    c = vaddq_u32(c, d);                                                       \
    b = veorq_u32(b, c);                                                       \
    b = vorrq_u32(vshlq_n_u32(b, 12), vshrq_n_u32(b, 20));                     \
    a = vaddq_u32(a, b);                                                       \
    d = veorq_u32(d, a);                                                       \
    d = vreinterpretq_u32_u8(                                                  \
        vqtbl1q_u8(vreinterpretq_u8_u32(d), fio___chacha_rot8_tbl));           \
    c = vaddq_u32(c, d);                                                       \
    b = veorq_u32(b, c);                                                       \
    b = vorrq_u32(vshlq_n_u32(b, 7), vshrq_n_u32(b, 25));                      \
  } while (0)

/**
 * Processes 4 ChaCha20 blocks (256 bytes) using ARM NEON intrinsics.
 *
 * Uses "vertical" SIMD layout: each uint32x4_t holds the same word position
 * from 4 different blocks. v[w] = {block0[w], block1[w], block2[w], block3[w]}.
 *
 * Counter words get {ctr, ctr+1, ctr+2, ctr+3} for the 4 blocks.
 */
FIO_SFUNC void fio___chacha_vround20x4(fio_u512 c, uint8_t *restrict data) {
  /* Byte rotation table for ROT8: rotate each 32-bit word left by 8 bits */
  static const uint8_t fio___chacha_rot8_data[] =
      {3, 0, 1, 2, 7, 4, 5, 6, 11, 8, 9, 10, 15, 12, 13, 14};
  const uint8x16_t fio___chacha_rot8_tbl = vld1q_u8(fio___chacha_rot8_data);
  /* 16 state vectors — one per ChaCha20 state word, across 4 blocks */
  uint32x4_t v0, v1, v2, v3, v4, v5, v6, v7;
  uint32x4_t v8, v9, v10, v11, v12, v13, v14, v15;

  /* Initialize: broadcast each state word across 4 lanes */
  v0 = vdupq_n_u32(c.u32[0]);
  v1 = vdupq_n_u32(c.u32[1]);
  v2 = vdupq_n_u32(c.u32[2]);
  v3 = vdupq_n_u32(c.u32[3]);
  v4 = vdupq_n_u32(c.u32[4]);
  v5 = vdupq_n_u32(c.u32[5]);
  v6 = vdupq_n_u32(c.u32[6]);
  v7 = vdupq_n_u32(c.u32[7]);
  v8 = vdupq_n_u32(c.u32[8]);
  v9 = vdupq_n_u32(c.u32[9]);
  v10 = vdupq_n_u32(c.u32[10]);
  v11 = vdupq_n_u32(c.u32[11]);
  /* Counter word: {ctr, ctr+1, ctr+2, ctr+3} */
  {
    static const uint32_t ctr_inc[4] = {0, 1, 2, 3};
    v12 = vaddq_u32(vdupq_n_u32(c.u32[12]), vld1q_u32(ctr_inc));
  }
  v13 = vdupq_n_u32(c.u32[13]);
  v14 = vdupq_n_u32(c.u32[14]);
  v15 = vdupq_n_u32(c.u32[15]);

  /* Save initial state for final addition */
  const uint32x4_t s0 = v0, s1 = v1, s2 = v2, s3 = v3;
  const uint32x4_t s4 = v4, s5 = v5, s6 = v6, s7 = v7;
  const uint32x4_t s8 = v8, s9 = v9, s10 = v10, s11 = v11;
  const uint32x4_t s12 = v12, s13 = v13, s14 = v14, s15 = v15;

  /* 10 double rounds (20 quarter rounds total) */
  for (int i = 0; i < 10; ++i) {
    /* Column round */
    FIO___CHACHA_QR_NEON(v0, v4, v8, v12);
    FIO___CHACHA_QR_NEON(v1, v5, v9, v13);
    FIO___CHACHA_QR_NEON(v2, v6, v10, v14);
    FIO___CHACHA_QR_NEON(v3, v7, v11, v15);
    /* Diagonal round */
    FIO___CHACHA_QR_NEON(v0, v5, v10, v15);
    FIO___CHACHA_QR_NEON(v1, v6, v11, v12);
    FIO___CHACHA_QR_NEON(v2, v7, v8, v13);
    FIO___CHACHA_QR_NEON(v3, v4, v9, v14);
  }

  /* Add initial state */
  v0 = vaddq_u32(v0, s0);
  v1 = vaddq_u32(v1, s1);
  v2 = vaddq_u32(v2, s2);
  v3 = vaddq_u32(v3, s3);
  v4 = vaddq_u32(v4, s4);
  v5 = vaddq_u32(v5, s5);
  v6 = vaddq_u32(v6, s6);
  v7 = vaddq_u32(v7, s7);
  v8 = vaddq_u32(v8, s8);
  v9 = vaddq_u32(v9, s9);
  v10 = vaddq_u32(v10, s10);
  v11 = vaddq_u32(v11, s11);
  v12 = vaddq_u32(v12, s12);
  v13 = vaddq_u32(v13, s13);
  v14 = vaddq_u32(v14, s14);
  v15 = vaddq_u32(v15, s15);

  /* Deinterleave from vertical layout and XOR with data.
   * Each v[w] holds {block0[w], block1[w], block2[w], block3[w]}.
   * We need to store block b at data[b*64 .. b*64+63].
   *
   * Strategy: for each block, extract lane b from all 16 vectors,
   * build 4 uint32x4_t output vectors (words 0-3, 4-7, 8-11, 12-15),
   * XOR with data, and store.
   */
#define FIO___CHACHA_NEON_XORBLOCK(lane, offset)                               \
  do {                                                                         \
    uint32x4_t out0 = {vgetq_lane_u32(v0, lane),                               \
                       vgetq_lane_u32(v1, lane),                               \
                       vgetq_lane_u32(v2, lane),                               \
                       vgetq_lane_u32(v3, lane)};                              \
    uint32x4_t out1 = {vgetq_lane_u32(v4, lane),                               \
                       vgetq_lane_u32(v5, lane),                               \
                       vgetq_lane_u32(v6, lane),                               \
                       vgetq_lane_u32(v7, lane)};                              \
    uint32x4_t out2 = {vgetq_lane_u32(v8, lane),                               \
                       vgetq_lane_u32(v9, lane),                               \
                       vgetq_lane_u32(v10, lane),                              \
                       vgetq_lane_u32(v11, lane)};                             \
    uint32x4_t out3 = {vgetq_lane_u32(v12, lane),                              \
                       vgetq_lane_u32(v13, lane),                              \
                       vgetq_lane_u32(v14, lane),                              \
                       vgetq_lane_u32(v15, lane)};                             \
    out0 = veorq_u32(out0, vld1q_u32((const uint32_t *)(data + (offset))));    \
    out1 =                                                                     \
        veorq_u32(out1, vld1q_u32((const uint32_t *)(data + (offset) + 16)));  \
    out2 =                                                                     \
        veorq_u32(out2, vld1q_u32((const uint32_t *)(data + (offset) + 32)));  \
    out3 =                                                                     \
        veorq_u32(out3, vld1q_u32((const uint32_t *)(data + (offset) + 48)));  \
    vst1q_u32((uint32_t *)(data + (offset)), out0);                            \
    vst1q_u32((uint32_t *)(data + (offset) + 16), out1);                       \
    vst1q_u32((uint32_t *)(data + (offset) + 32), out2);                       \
    vst1q_u32((uint32_t *)(data + (offset) + 48), out3);                       \
  } while (0)

  FIO___CHACHA_NEON_XORBLOCK(0, 0);
  FIO___CHACHA_NEON_XORBLOCK(1, 64);
  FIO___CHACHA_NEON_XORBLOCK(2, 128);
  FIO___CHACHA_NEON_XORBLOCK(3, 192);
#undef FIO___CHACHA_NEON_XORBLOCK
}

#undef FIO___CHACHA_QR_NEON
#endif /* FIO___HAS_ARM_INTRIN */

SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nonce,
                        uint32_t counter) {
  fio_u512 c = fio___chacha_init(key, nonce, counter);
#if FIO___HAS_ARM_INTRIN
  for (size_t pos = 255; pos < len; pos += 256) {
    fio___chacha_vround20x4(c, (uint8_t *)data);
    c.u32[12] += 4; /* block counter */
    data = (void *)((uint8_t *)data + 256);
  }
  if ((len & 128)) {
    fio___chacha_vround20x2(c, (uint8_t *)data);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#else
  for (size_t pos = 127; pos < len; pos += 128) {
    fio___chacha_vround20x2(c, (uint8_t *)data);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#endif
  if ((len & 64)) {
    fio___chacha_vround20(c, (uint8_t *)data);
    data = (void *)((uint8_t *)data + 64);
    ++c.u32[12];
  }
  if ((len & 63)) {
    fio_u512 dest; /* no need to initialize, junk data disregarded. */
    fio_memcpy63x(dest.u64, data, len);
    fio___chacha_vround20(c, dest.u8);
    fio_memcpy63x(data, dest.u64, len);
  }
}

/* *****************************************************************************
ChaCha20Poly1305 Encryption with Authentication
***************************************************************************** */

FIO_IFUNC fio_u512 fio___chacha20_mixround(fio_u512 c) {
  fio_u512 k = {.u64 = {0}};
  fio___chacha_vround20(c, k.u8);
  return k;
}
SFUNC void fio_chacha20_poly1305_enc(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad, /* additional data */
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce) {
  fio_u512 c = fio___chacha_init(key, nonce, 0);
  fio___poly_s pl;
  {
    fio_u512 c2 = fio___chacha20_mixround(c);
    pl = fio___poly_init(&c2);
  }
  ++c.u32[12]; /* block counter */
  for (size_t i = 31; i < adlen; i += 32) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    fio___poly_consume128bit(&pl, (uint8_t *)ad + 16, 1);
    ad = (void *)((uint8_t *)ad + 32);
  }
  if (adlen & 16) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    ad = (void *)((uint8_t *)ad + 16);
  }
  if (adlen & 15) {
    uint64_t tmp[2] = {0}; /* 16 byte pad */
    fio_memcpy15x(tmp, ad, adlen);
    fio___poly_consume128bit(&pl, (uint8_t *)tmp, 1);
  }
#if FIO___HAS_ARM_INTRIN
  for (size_t i = 255; i < len; i += 256) {
    fio___chacha_vround20x4(c, (uint8_t *)data);
    for (size_t j = 0; j < 256; j += 16)
      fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + j), 1);
    c.u32[12] += 4; /* block counter */
    data = (void *)((uint8_t *)data + 256);
  }
  if ((len & 128)) {
    fio___chacha_vround20x2(c, (uint8_t *)data);
    for (size_t j = 0; j < 128; j += 16)
      fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + j), 1);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#else
  for (size_t i = 127; i < len; i += 128) {
    fio___chacha_vround20x2(c, (uint8_t *)data);
    fio___poly_consume128bit(&pl, data, 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 16), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 32), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 48), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 64), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 80), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 96), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 112), 1);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#endif
  if ((len & 64)) {
    fio___chacha_vround20(c, (uint8_t *)data);
    fio___poly_consume128bit(&pl, data, 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 16), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 32), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 48), 1);
    ++c.u32[12]; /* block counter */
    data = (void *)((uint8_t *)data + 64);
  }
  if ((len & 63)) {
    fio_u512 dest;
    fio_memcpy63x(dest.u8, data, len);
    fio___chacha_vround20(c, dest.u8);
    fio_memcpy63x(data, dest.u8, len);
    uint8_t *p = dest.u8;
    if ((len & 32)) {
      fio___poly_consume128bit(&pl, p, 1);
      fio___poly_consume128bit(&pl, (p + 16), 1);
      p += 32;
    }
    if ((len & 16)) {
      fio___poly_consume128bit(&pl, p, 1);
      p += 16;
    }
    if ((len & 15)) {
      /* zero out poly padding */
      for (size_t i = (len & 15UL); i < 16; i++)
        p[i] = 0;
      fio___poly_consume128bit(&pl, p, 1);
    }
  }
  {
    uint64_t mac_data[2] = {fio_ltole64(adlen), fio_ltole64(len)};
    fio___poly_consume128bit(&pl, (uint8_t *)mac_data, 1);
  }
  fio___poly_finilize(&pl);
  fio_u2buf64_le(mac, pl.a[0]);
  fio_u2buf64_le(&((char *)mac)[8], pl.a[1]);
  fio_secure_zero(&pl, sizeof(pl));
}

SFUNC void fio_chacha20_poly1305_auth(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad, /* additional data */
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce) {
  fio___poly_s pl;
  {
    fio_u512 c = fio___chacha_init(key, nonce, 0);
    c = fio___chacha20_mixround(c); /* computes poly1305 key */
    pl = fio___poly_init(&c);
  }
  for (size_t i = 31; i < adlen; i += 32) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    fio___poly_consume128bit(&pl, (uint8_t *)ad + 16, 1);
    ad = (void *)((uint8_t *)ad + 32);
  }
  if (adlen & 16) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    ad = (void *)((uint8_t *)ad + 16);
  }
  if (adlen & 15) {
    uint64_t tmp[2] = {0}; /* 16 byte pad */
    fio_memcpy15x(tmp, ad, adlen);
    fio___poly_consume128bit(&pl, (uint8_t *)tmp, 1);
  }
  fio___poly_consume_msg(&pl, (uint8_t *)data, (len & (~15ULL)));
  if ((len & 15)) {
    fio_u128 dest = {0}; /* 16 byte pad */
    fio_memcpy15x(dest.u64, (uint8_t *)data + (len & (~15ULL)), len);
    fio___poly_consume128bit(&pl, (uint8_t *)(dest.u64), 1);
  }
  {
    uint64_t mac_data[2] = {fio_ltole64(adlen), fio_ltole64(len)};
    fio___poly_consume128bit(&pl, (uint8_t *)mac_data, 1);
  }
  fio___poly_finilize(&pl);
  fio_u2buf64_le(mac, pl.a[0]);
  fio_u2buf64_le(&((char *)mac)[8], pl.a[1]);
  fio_secure_zero(&pl, sizeof(pl));
}

SFUNC int fio_chacha20_poly1305_dec(void *restrict mac,
                                    void *restrict data,
                                    size_t len,
                                    const void *ad, /* additional data */
                                    size_t adlen,
                                    const void *key,
                                    const void *nonce) {
  void *data_start = data;
  fio_u512 c = fio___chacha_init(key, nonce, 0);
  fio___poly_s pl;
  {
    fio_u512 c2 = fio___chacha20_mixround(c);
    pl = fio___poly_init(&c2);
  }
  ++c.u32[12]; /* block counter */
  /* Authenticate additional data (same as enc) */
  for (size_t i = 31; i < adlen; i += 32) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    fio___poly_consume128bit(&pl, (uint8_t *)ad + 16, 1);
    ad = (void *)((uint8_t *)ad + 32);
  }
  if (adlen & 16) {
    fio___poly_consume128bit(&pl, (uint8_t *)ad, 1);
    ad = (void *)((uint8_t *)ad + 16);
  }
  if (adlen & 15) {
    uint64_t tmp[2] = {0}; /* 16 byte pad */
    fio_memcpy15x(tmp, ad, adlen);
    fio___poly_consume128bit(&pl, (uint8_t *)tmp, 1);
  }
  /* Fused loop: Poly1305 over ciphertext, then ChaCha20 XOR to decrypt */
#if FIO___HAS_ARM_INTRIN
  for (size_t i = 255; i < len; i += 256) {
    for (size_t j = 0; j < 256; j += 16)
      fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + j), 1);
    fio___chacha_vround20x4(c, (uint8_t *)data);
    c.u32[12] += 4; /* block counter */
    data = (void *)((uint8_t *)data + 256);
  }
  if ((len & 128)) {
    for (size_t j = 0; j < 128; j += 16)
      fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + j), 1);
    fio___chacha_vround20x2(c, (uint8_t *)data);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#else
  for (size_t i = 127; i < len; i += 128) {
    fio___poly_consume128bit(&pl, data, 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 16), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 32), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 48), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 64), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 80), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 96), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 112), 1);
    fio___chacha_vround20x2(c, (uint8_t *)data);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
#endif
  if ((len & 64)) {
    fio___poly_consume128bit(&pl, data, 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 16), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 32), 1);
    fio___poly_consume128bit(&pl, (void *)((uint8_t *)data + 48), 1);
    fio___chacha_vround20(c, (uint8_t *)data);
    ++c.u32[12]; /* block counter */
    data = (void *)((uint8_t *)data + 64);
  }
  if ((len & 63)) {
    /* Poly1305 over ciphertext tail (with zero padding) */
    fio_u512 ct;
    FIO_MEMSET(ct.u8, 0, 64);
    fio_memcpy63x(ct.u8, data, len);
    uint8_t *p = ct.u8;
    if ((len & 32)) {
      fio___poly_consume128bit(&pl, p, 1);
      fio___poly_consume128bit(&pl, (p + 16), 1);
      p += 32;
    }
    if ((len & 16)) {
      fio___poly_consume128bit(&pl, p, 1);
      p += 16;
    }
    if ((len & 15)) {
      /* zero padding already set by FIO_MEMSET above */
      fio___poly_consume128bit(&pl, p, 1);
    }
    /* ChaCha20 XOR to decrypt tail in-place */
    fio_u512 dest;
    fio_memcpy63x(dest.u8, data, len);
    fio___chacha_vround20(c, dest.u8);
    fio_memcpy63x(data, dest.u8, len);
  }
  /* Finalize Poly1305 */
  {
    uint64_t mac_data[2] = {fio_ltole64(adlen), fio_ltole64(len)};
    fio___poly_consume128bit(&pl, (uint8_t *)mac_data, 1);
  }
  fio___poly_finilize(&pl);
  uint8_t computed_mac[16];
  fio_u2buf64_le(computed_mac, pl.a[0]);
  fio_u2buf64_le(computed_mac + 8, pl.a[1]);
  fio_secure_zero(&pl, sizeof(pl));
  /* Constant-time MAC comparison */
  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    /* MAC mismatch: zero decrypted output to prevent use of unauthenticated
     * plaintext, then return error. */
    fio_secure_zero(data_start, len);
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));
  return 0;
}

/* *****************************************************************************
XChaCha20 (Extended Nonce Variant)
***************************************************************************** */

/**
 * HChaCha20: derives a 256-bit subkey from a 256-bit key and 128-bit nonce.
 *
 * This is the "hash" variant of ChaCha20 used in the XChaCha20 construction.
 * Unlike regular ChaCha20, it does NOT add the initial state after the rounds.
 * Output: words 0-3 and 12-15 of the final state (256 bits total).
 */
FIO_IFUNC void fio___hchacha20(void *restrict subkey,
                               const void *key,
                               const void *nonce16) {
  /* Initialize state: constants + key + nonce (no counter for HChaCha20) */
  uint32_t v[16] = {
      // clang-format off
      0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,  /* "expand 32-byte k" */
      fio_buf2u32_le(key),
      fio_buf2u32_le((uint8_t *)key + 4),
      fio_buf2u32_le((uint8_t *)key + 8),
      fio_buf2u32_le((uint8_t *)key + 12),
      fio_buf2u32_le((uint8_t *)key + 16),
      fio_buf2u32_le((uint8_t *)key + 20),
      fio_buf2u32_le((uint8_t *)key + 24),
      fio_buf2u32_le((uint8_t *)key + 28),
      fio_buf2u32_le(nonce16),
      fio_buf2u32_le((uint8_t *)nonce16 + 4),
      fio_buf2u32_le((uint8_t *)nonce16 + 8),
      fio_buf2u32_le((uint8_t *)nonce16 + 12),
      // clang-format on
  };

/* Quarter round macro for HChaCha20 */
#define FIO___HCHACHA_QR(a, b, c, d)                                           \
  do {                                                                         \
    v[a] += v[b];                                                              \
    v[d] ^= v[a];                                                              \
    v[d] = fio_lrot32(v[d], 16);                                               \
    v[c] += v[d];                                                              \
    v[b] ^= v[c];                                                              \
    v[b] = fio_lrot32(v[b], 12);                                               \
    v[a] += v[b];                                                              \
    v[d] ^= v[a];                                                              \
    v[d] = fio_lrot32(v[d], 8);                                                \
    v[c] += v[d];                                                              \
    v[b] ^= v[c];                                                              \
    v[b] = fio_lrot32(v[b], 7);                                                \
  } while (0)

  /* Run 10 double-rounds (20 quarter-rounds total) */
  for (size_t round__ = 0; round__ < 10; ++round__) {
    /* Column rounds */
    FIO___HCHACHA_QR(0, 4, 8, 12);
    FIO___HCHACHA_QR(1, 5, 9, 13);
    FIO___HCHACHA_QR(2, 6, 10, 14);
    FIO___HCHACHA_QR(3, 7, 11, 15);
    /* Diagonal rounds */
    FIO___HCHACHA_QR(0, 5, 10, 15);
    FIO___HCHACHA_QR(1, 6, 11, 12);
    FIO___HCHACHA_QR(2, 7, 8, 13);
    FIO___HCHACHA_QR(3, 4, 9, 14);
  }
#undef FIO___HCHACHA_QR

  /* Output words 0-3 and 12-15 as the 256-bit subkey (NO state addition!) */
  fio_u2buf32_le(subkey, v[0]);
  fio_u2buf32_le((uint8_t *)subkey + 4, v[1]);
  fio_u2buf32_le((uint8_t *)subkey + 8, v[2]);
  fio_u2buf32_le((uint8_t *)subkey + 12, v[3]);
  fio_u2buf32_le((uint8_t *)subkey + 16, v[12]);
  fio_u2buf32_le((uint8_t *)subkey + 20, v[13]);
  fio_u2buf32_le((uint8_t *)subkey + 24, v[14]);
  fio_u2buf32_le((uint8_t *)subkey + 28, v[15]);
}

/**
 * XChaCha20: ChaCha20 with extended 192-bit nonce.
 *
 * 1. Derive subkey using HChaCha20 with first 16 bytes of nonce
 * 2. Use remaining 8 bytes of nonce (prefixed with 4 zero bytes) as 12-byte
 *    nonce for standard ChaCha20
 */
SFUNC void fio_xchacha20(void *restrict data,
                         size_t len,
                         const void *key,
                         const void *nonce,
                         uint32_t counter) {
  uint8_t subkey[32];
  uint8_t sub_nonce[12];

  /* Derive subkey from first 16 bytes of nonce */
  fio___hchacha20(subkey, key, nonce);

  /* Build sub-nonce: 4 zero bytes + last 8 bytes of original nonce */
  sub_nonce[0] = 0;
  sub_nonce[1] = 0;
  sub_nonce[2] = 0;
  sub_nonce[3] = 0;
  FIO_MEMCPY(sub_nonce + 4, (const uint8_t *)nonce + 16, 8);

  /* Apply standard ChaCha20 with derived subkey and sub-nonce */
  fio_chacha20(data, len, subkey, sub_nonce, counter);

  /* Secure cleanup */
  fio_secure_zero(subkey, sizeof(subkey));
}

/**
 * XChaCha20-Poly1305 encryption with 192-bit nonce.
 */
SFUNC void fio_xchacha20_poly1305_enc(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce) {
  uint8_t subkey[32];
  uint8_t sub_nonce[12];

  /* Derive subkey from first 16 bytes of nonce */
  fio___hchacha20(subkey, key, nonce);

  /* Build sub-nonce: 4 zero bytes + last 8 bytes of original nonce */
  sub_nonce[0] = 0;
  sub_nonce[1] = 0;
  sub_nonce[2] = 0;
  sub_nonce[3] = 0;
  FIO_MEMCPY(sub_nonce + 4, (const uint8_t *)nonce + 16, 8);

  /* Apply standard ChaCha20-Poly1305 with derived subkey and sub-nonce */
  fio_chacha20_poly1305_enc(mac, data, len, ad, adlen, subkey, sub_nonce);

  /* Secure cleanup */
  fio_secure_zero(subkey, sizeof(subkey));
}

/**
 * XChaCha20-Poly1305 decryption with 192-bit nonce.
 */
SFUNC int fio_xchacha20_poly1305_dec(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad,
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce) {
  uint8_t subkey[32];
  uint8_t sub_nonce[12];
  int result;

  /* Derive subkey from first 16 bytes of nonce */
  fio___hchacha20(subkey, key, nonce);

  /* Build sub-nonce: 4 zero bytes + last 8 bytes of original nonce */
  sub_nonce[0] = 0;
  sub_nonce[1] = 0;
  sub_nonce[2] = 0;
  sub_nonce[3] = 0;
  FIO_MEMCPY(sub_nonce + 4, (const uint8_t *)nonce + 16, 8);

  /* Apply standard ChaCha20-Poly1305 decryption with derived key/nonce */
  result =
      fio_chacha20_poly1305_dec(mac, data, len, ad, adlen, subkey, sub_nonce);

  /* Secure cleanup */
  fio_secure_zero(subkey, sizeof(subkey));

  return result;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_CHACHA
#endif /* FIO_CHACHA */
