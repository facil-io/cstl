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
 * * `nounce` MUST point to a  96 bit long memory address (12 Bytes).
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
                                     const void *nounce);

/**
 * Performs an in-place decryption of `data` using ChaCha20 after authenticating
 * the message authentication code (MAC) using Poly1305.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nounce` MUST point to a  96 bit long memory address (12 Bytes).
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
                                    const void *nounce);

/* *****************************************************************************
Using ChaCha20 and Poly1305 separately
***************************************************************************** */

/**
 * Performs an in-place encryption/decryption of `data` using ChaCha20.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 * * `nounce` MUST point to a  96 bit long memory address (12 Bytes).
 * * `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.
 */
SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nounce,
                        uint32_t counter);

/**
 * Given a Poly1305 256bit (32 byte) key, writes the authentication code for the
 * poly message and additional data into `mac_dest`.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 */
SFUNC void fio_poly1305_auth(void *restrict mac_dest,
                             const void *key256bits,
                             void *restrict message,
                             size_t len,
                             const void *additional_data,
                             size_t additional_data_len);

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
} FIO_ALIGN(16) fio___poly_s;

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
                             const void *key,
                             void *restrict msg,
                             size_t len,
                             const void *ad,
                             size_t ad_len) {
  fio___poly_s pl = fio___poly_init(key);
  fio___poly_consume_msg(&pl, (uint8_t *)ad, ad_len);
  fio___poly_consume_msg(&pl, (uint8_t *)msg, len);
  fio___poly_finilize(&pl);
  fio_u2buf64_le(mac, pl.a[0]);
  fio_u2buf64_le(&((char *)mac)[8], pl.a[1]);
}

/* *****************************************************************************
ChaCha20 (encryption)
***************************************************************************** */

#define FIO___CHACHA_VROUND(count, a, b, c, d)                                 \
  for (size_t i = 0; i < count; ++i) {                                         \
    a[i] += b[i];                                                              \
    d[i] ^= a[i];                                                              \
    d[i] = (d[i] << 16) | (d[i] >> (32 - 16));                                 \
    c[i] += d[i];                                                              \
    b[i] ^= c[i];                                                              \
    b[i] = (b[i] << 12) | (b[i] >> (32 - 12));                                 \
    a[i] += b[i];                                                              \
    d[i] ^= a[i];                                                              \
    d[i] = (d[i] << 8) | (d[i] >> (32 - 8));                                   \
    c[i] += d[i];                                                              \
    b[i] ^= c[i];                                                              \
    b[i] = (b[i] << 7) | (b[i] >> (32 - 7));                                   \
  }

FIO_IFUNC fio_u512 fio___chacha_init(const void *key,
                                     const void *nounce,
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
              fio_buf2u32_le(nounce),
              fio_buf2u32_le((uint8_t *)nounce + 4),
              fio_buf2u32_le((uint8_t *)nounce + 8),
          }, // clang-format on
  };
  return o;
}

FIO_SFUNC void fio___chacha_vround20(const fio_u512 c, uint8_t *restrict data) {
  uint32_t v[16];
  for (size_t i = 0; i < 16; ++i) {
    v[i] = c.u32[i];
  }
  for (size_t round__ = 0; round__ < 10; ++round__) { /* 2 rounds per loop */
    FIO___CHACHA_VROUND(4, v, (v + 4), (v + 8), (v + 12));
    fio_u32x4_reshuffle((v + 4), 1, 2, 3, 0);
    fio_u32x4_reshuffle((v + 8), 2, 3, 0, 1);
    fio_u32x4_reshuffle((v + 12), 3, 0, 1, 2);
    FIO___CHACHA_VROUND(4, v, (v + 4), (v + 8), (v + 12));
    fio_u32x4_reshuffle((v + 4), 3, 0, 1, 2);
    fio_u32x4_reshuffle((v + 8), 2, 3, 0, 1);
    fio_u32x4_reshuffle((v + 12), 1, 2, 3, 0);
  }
  for (size_t i = 0; i < 16; ++i) {
    v[i] += c.u32[i];
  }

#if __BIG_ENDIAN__
  for (size_t i = 0; i < 16; ++i) {
    v[i] = fio_bswap32(v[i]);
  }
#endif
  {
    uint32_t d[16];
    fio_memcpy64(d, data);
    for (size_t i = 0; i < 16; ++i) {
      d[i] ^= v[i];
    }
    fio_memcpy64(data, d);
  }
}

FIO_SFUNC void fio___chacha_vround20x2(fio_u512 c, uint8_t *restrict data) {
  uint32_t v[32];
  for (size_t i = 0; i < 16; ++i) {
    v[i + (i & (4 | 8))] = c.u32[i];
    v[i + 4 + (i & (4 | 8))] = c.u32[i];
  }
  ++v[28];
  for (size_t round__ = 0; round__ < 10; ++round__) { /* 2 rounds per loop */
    FIO___CHACHA_VROUND(8, v, (v + 8), (v + 16), (v + 24));
    fio_u32x8_reshuffle((v + 8), 1, 2, 3, 0, 5, 6, 7, 4);
    fio_u32x8_reshuffle((v + 16), 2, 3, 0, 1, 6, 7, 4, 5);
    fio_u32x8_reshuffle((v + 24), 3, 0, 1, 2, 7, 4, 5, 6);
    FIO___CHACHA_VROUND(8, v, (v + 8), (v + 16), (v + 24));
    fio_u32x8_reshuffle((v + 8), 3, 0, 1, 2, 7, 4, 5, 6);
    fio_u32x8_reshuffle((v + 16), 2, 3, 0, 1, 6, 7, 4, 5);
    fio_u32x8_reshuffle((v + 24), 1, 2, 3, 0, 5, 6, 7, 4);
  }
  for (size_t i = 0; i < 16; ++i) {
    v[i + (i & (4 | 8))] += c.u32[i];
    v[i + 4 + (i & (4 | 8))] += c.u32[i];
  }
  ++v[28];

#if __BIG_ENDIAN__
  for (size_t i = 0; i < 32; ++i) {
    v[i] = fio_bswap32(v[i]);
  }
#endif
  {
    fio_u32x8_reshuffle((v + 4), 4, 5, 6, 7, 0, 1, 2, 3);
    fio_u32x8_reshuffle((v + 20), 4, 5, 6, 7, 0, 1, 2, 3);
    uint32_t d[8];
    fio_memcpy32(d, data);
    for (size_t i = 0; i < 8; ++i) {
      d[i] ^= v[i];
    }
    fio_memcpy32(data, d);

    fio_memcpy32(d, data + 32);
    for (size_t i = 0; i < 8; ++i) {
      d[i] ^= v[16 + i];
    }
    fio_memcpy32(data + 32, d);

    fio_memcpy32(d, data + 64);
    for (size_t i = 0; i < 8; ++i) {
      d[i] ^= v[8 + i];
    }
    fio_memcpy32(data + 64, d);

    fio_memcpy32(d, data + 96);
    for (size_t i = 0; i < 8; ++i) {
      d[i] ^= v[24 + i];
    }
    fio_memcpy32(data + 96, d);
  }
}

SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nounce,
                        uint32_t counter) {
  fio_u512 c = fio___chacha_init(key, nounce, counter);
  for (size_t pos = 127; pos < len; pos += 128) {
    fio___chacha_vround20x2(c, (uint8_t *)data);
    c.u32[12] += 2; /* block counter */
    data = (void *)((uint8_t *)data + 128);
  }
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
                                     const void *nounce) {
  fio_u512 c = fio___chacha_init(key, nounce, 0);
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
}

SFUNC void fio_chacha20_poly1305_auth(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad, /* additional data */
                                      size_t adlen,
                                      const void *key,
                                      const void *nounce) {
  fio___poly_s pl;
  {
    fio_u512 c = fio___chacha_init(key, nounce, 0);
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
}

SFUNC int fio_chacha20_poly1305_dec(void *restrict mac,
                                    void *restrict data,
                                    size_t len,
                                    const void *ad, /* additional data */
                                    size_t adlen,
                                    const void *key,
                                    const void *nounce) {
  uint64_t auth[2];
  fio_chacha20_poly1305_auth(&auth, data, len, ad, adlen, key, nounce);
  if (((auth[0] ^ fio_buf2u64u(mac)) |
       (auth[1] ^ fio_buf2u64u(((char *)mac + 8)))))
    return -1;
  fio_chacha20(data, len, key, nounce, 1);
  return 0;
}
/* *****************************************************************************
Module Cleanup
*****************************************************************************
*/

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_CHACHA
#endif /* FIO_CHACHA */
