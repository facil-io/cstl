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
                             void *restrict additional_data,
                             size_t additional_data_len);

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
                                     void *restrict ad, /* additional data */
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
                                    void *restrict ad, /* additional data */
                                    size_t adlen,
                                    const void *key,
                                    const void *nounce);

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
  uint64_t t0, t1;
  /* r &= 0xffffffc0ffffffc0ffffffc0fffffff */
  t0 = fio_buf2u64_little((uint8_t *)key256b + 0);
  t1 = fio_buf2u64_little((uint8_t *)key256b + 8);
  fio___poly_s pl = {
      .r =
          {
              ((t0)&0xffc0fffffff),
              (((t0 >> 44) | (t1 << 20)) & 0xfffffc0ffff),
              (((t1 >> 24)) & 0x00ffffffc0f),
          },
      .s =
          {
              fio_buf2u64_little(((uint8_t *)key256b + 16)),
              fio_buf2u64_little(((uint8_t *)key256b + 24)),
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
    t0 = fio_buf2u64_little(msg);
    t1 = fio_buf2u64_little(((uint8_t *)msg + 8));
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

/*
 * Given a Poly1305 key, writes a MAC into `mac_dest`. */
SFUNC void fio_poly1305_auth(void *restrict mac,
                             const void *key,
                             void *restrict msg,
                             size_t len,
                             void *restrict ad,
                             size_t ad_len) {
  fio___poly_s pl = fio___poly_init(key);
  fio___poly_consume_msg(&pl, (uint8_t *)ad, ad_len);
  fio___poly_consume_msg(&pl, (uint8_t *)msg, len);
  fio___poly_finilize(&pl);
  fio_u2buf64_little(mac, pl.a[0]);
  fio_u2buf64_little(&((char *)mac)[8], pl.a[1]);
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
              fio_buf2u32_little(key),
              fio_buf2u32_little((uint8_t *)key + 4),
              fio_buf2u32_little((uint8_t *)key + 8),
              fio_buf2u32_little((uint8_t *)key + 12),
              fio_buf2u32_little((uint8_t *)key + 16),
              fio_buf2u32_little((uint8_t *)key + 20),
              fio_buf2u32_little((uint8_t *)key + 24),
              fio_buf2u32_little((uint8_t *)key + 28),
              counter,
              fio_buf2u32_little(nounce),
              fio_buf2u32_little((uint8_t *)nounce + 4),
              fio_buf2u32_little((uint8_t *)nounce + 8),
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
                                     void *restrict ad, /* additional data */
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
  fio_u2buf64_little(mac, pl.a[0]);
  fio_u2buf64_little(&((char *)mac)[8], pl.a[1]);
}

SFUNC void fio_chacha20_poly1305_auth(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      void *restrict ad, /* additional data */
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
  fio_u2buf64_little(mac, pl.a[0]);
  fio_u2buf64_little(&((char *)mac)[8], pl.a[1]);
}

SFUNC int fio_chacha20_poly1305_dec(void *restrict mac,
                                    void *restrict data,
                                    size_t len,
                                    void *restrict ad, /* additional data */
                                    size_t adlen,
                                    const void *key,
                                    const void *nounce) {
  uint64_t auth[2];
  fio_chacha20_poly1305_auth(&auth, data, len, ad, adlen, key, nounce);
  if (((auth[0] != fio_buf2u64_little(mac)) ||
       (auth[1] != fio_buf2u64_little(((char *)mac + 8)))))
    return -1;
  fio_chacha20(data, len, key, nounce, 1);
  return 0;
}
/* *****************************************************************************
Module Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

#if HAVE_OPENSSL
// #include <openssl/bio.h>
// #include <openssl/err.h>
// #include <openssl/ssl.h>
// FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __poly1305_open_ssl_wrapper)(char
// *data,
//                                                                   size_t len)
//                                                                   {
// }
#endif /* HAVE_OPENSSL */

FIO_SFUNC uintptr_t fio__poly1305_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe\x42"
                      "\xd5\x06\xa8"
                      "\x01\x03\x80\x8a\xfb\x0d\xb2\xfd\x4a\xbf\xf6\xaf\x41"
                      "\x49\xf5\x1b";
  fio_poly1305_auth(result, key, msg, len, NULL, 0);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20(msg, len, key, nounce, 1);
  result[0] = fio_buf2u64_local(msg);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20poly1305_speed_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20_poly1305_enc(result, msg, len, NULL, 0, key, nounce);
  return (uintptr_t)result[0];
}

FIO_SFUNC uintptr_t fio__chacha20poly1305dec_speed_wrapper(char *msg,
                                                           size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_poly1305_auth(result, key, msg, len, NULL, 0);
  fio_chacha20(msg, len, key, nounce, 1);
  return (uintptr_t)result[0];
}

FIO_SFUNC void FIO_NAME_TEST(stl, chacha)(void) {
  fprintf(stderr, "* Testing ChaCha20 Poly1305\n");
  { /* test ChaCha20 independently */
    struct {
      char key[33];
      char nounce[13];
      char *src;
      char *expected;
    } tests[] = {
        {
            .key = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d"
                   "\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b"
                   "\x1c\x1d\x1e\x1f",
            .nounce = "\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00",
            .src =
                (char *)"\x4c\x61\x64\x69\x65\x73\x20\x61\x6e\x64\x20\x47\x65"
                        "\x6e\x74\x6c\x65\x6d\x65\x6e\x20\x6f\x66\x20\x74\x68"
                        "\x65\x20\x63\x6c\x61\x73\x73\x20\x6f\x66\x20\x27\x39"
                        "\x39\x3a\x20\x49\x66\x20\x49\x20\x63\x6f\x75\x6c\x64"
                        "\x20\x6f\x66\x66\x65\x72\x20\x79\x6f\x75\x20\x6f\x6e"
                        "\x6c\x79\x20\x6f\x6e\x65\x20\x74\x69\x70\x20\x66\x6f"
                        "\x72\x20\x74\x68\x65\x20\x66\x75\x74\x75\x72\x65\x2c"
                        "\x20\x73\x75\x6e\x73\x63\x72\x65\x65\x6e\x20\x77\x6f"
                        "\x75\x6c\x64\x20\x62\x65\x20\x69\x74\x2e",
            .expected =
                (char *)"\x6e\x2e\x35\x9a\x25\x68\xf9\x80\x41\xba\x07\x28\xdd"
                        "\x0d\x69\x81\xe9\x7e\x7a\xec\x1d\x43\x60\xc2\x0a\x27"
                        "\xaf\xcc\xfd\x9f\xae\x0b\xf9\x1b\x65\xc5\x52\x47\x33"
                        "\xab\x8f\x59\x3d\xab\xcd\x62\xb3\x57\x16\x39\xd6\x24"
                        "\xe6\x51\x52\xab\x8f\x53\x0c\x35\x9f\x08\x61\xd8\x07"
                        "\xca\x0d\xbf\x50\x0d\x6a\x61\x56\xa3\x8e\x08\x8a\x22"
                        "\xb6\x5e\x52\xbc\x51\x4d\x16\xcc\xf8\x06\x81\x8c\xe9"
                        "\x1a\xb7\x79\x37\x36\x5a\xf9\x0b\xbf\x74\xa3\x5b\xe6"
                        "\xb4\x0b\x8e\xed\xf2\x78\x5e\x42\x87\x4d",
        },
        {
            .key = {0},
            .nounce = {0},
            .src =
                (char *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            .expected =
                (char *)"\x3a\xeb\x52\x24\xec\xf8\x49\x92\x9b\x9d\x82\x8d\xb1"
                        "\xce\xd4\xdd\x83\x20\x25\xe8\x01\x8b\x81\x60\xb8\x22"
                        "\x84\xf3\xc9\x49\xaa\x5a\x8e\xca\x00\xbb\xb4\xa7\x3b"
                        "\xda\xd1\x92\xb5\xc4\x2f\x73\xf2\xfd\x4e\x27\x36\x44"
                        "\xc8\xb3\x61\x25\xa6\x4a\xdd\xeb\x00\x6c\x13\xa0",
        },
        {.expected = NULL}};
    for (size_t i = 0; tests[i].expected; ++i) {
      size_t len = strlen(tests[i].src);
      char buffer[4096];
      FIO_MEMCPY(buffer, tests[i].src, len);
      fio_chacha20(buffer, len, tests[i].key, tests[i].nounce, 1);
      FIO_ASSERT(!memcmp(buffer, tests[i].expected, len),
                 "ChaCha20 encoding failed");
      fio_chacha20(buffer, len, tests[i].key, tests[i].nounce, 1);
      FIO_ASSERT(!memcmp(buffer, tests[i].src, len),
                 "ChaCha20 decoding failed");
    }
  }
  { /* test Poly1305 independently */
    struct {
      char key[33];
      char *msg;
      char *expected;
    } tests[] = {{
                     .key = "\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe"
                            "\x42\xd5\x06\xa8\x01\x03\x80\x8a\xfb\x0d\xb2\xfd"
                            "\x4a\xbf\xf6\xaf\x41\x49\xf5\x1b",
                     .msg = (char *)"Cryptographic Forum Research Group",
                     .expected =
                         (char *)"\xa8\x06\x1d\xc1\x30\x51\x36\xc6\xc2\x2b\x8b"
                                 "\xaf\x0c\x01\x27\xa9",
                 },
                 {.expected = NULL}};
    char auth[16] = {0};
    char buf1[33] = {0};
    char buf2[33] = {0};
    for (size_t t = 0; tests[t].expected; ++t) {
      fio_poly1305_auth(auth,
                        tests[t].key,
                        tests[t].msg,
                        strlen(tests[t].msg),
                        NULL,
                        0);
      for (int i = 0; i < 16; ++i) {
        buf1[(i << 1)] = fio_i2c(((auth[i] >> 4) & 0xF));
        buf1[(i << 1) + 1] = fio_i2c(((auth[i]) & 0xF));
        buf2[(i << 1)] = fio_i2c(((tests[t].expected[i] >> 4) & 0xF));
        buf2[(i << 1) + 1] = fio_i2c(((tests[t].expected[i]) & 0xF));
      }
      FIO_ASSERT(!memcmp(auth, tests[t].expected, 16),
                 "Poly1305 example authentication failed:\n\t%s != %s",
                 buf1,
                 buf2);
    }
  }
  { /* test ChaCha20Poly1305 */
    struct {
      char key[33];
      char nounce[13];
      char *ad;
      size_t ad_len;
      char *msg;
      char *expected;
      char mac[17];
    } tests[] = {
        {
            .key = "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d"
                   "\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b"
                   "\x9c\x9d\x9e\x9f",
            .nounce = "\x07\x00\x00\x00\x40\x41\x42\x43\x44\x45\x46\x47",
            .ad = (char *)"\x50\x51\x52\x53\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7",
            .ad_len = 12,
            .msg =
                (char *)"\x4c\x61\x64\x69\x65\x73\x20\x61\x6e\x64\x20\x47\x65"
                        "\x6e\x74\x6c\x65\x6d\x65\x6e\x20\x6f\x66\x20\x74\x68"
                        "\x65\x20\x63\x6c\x61\x73\x73\x20\x6f\x66\x20\x27\x39"
                        "\x39\x3a\x20\x49\x66\x20\x49\x20\x63\x6f\x75\x6c\x64"
                        "\x20\x6f\x66\x66\x65\x72\x20\x79\x6f\x75\x20\x6f\x6e"
                        "\x6c\x79\x20\x6f\x6e\x65\x20\x74\x69\x70\x20\x66\x6f"
                        "\x72\x20\x74\x68\x65\x20\x66\x75\x74\x75\x72\x65\x2c"
                        "\x20\x73\x75\x6e\x73\x63\x72\x65\x65\x6e\x20\x77\x6f"
                        "\x75\x6c\x64\x20\x62\x65\x20\x69\x74\x2e",
            .expected =
                (char *)"\xd3\x1a\x8d\x34\x64\x8e\x60\xdb\x7b\x86\xaf\xbc\x53"
                        "\xef\x7e\xc2\xa4\xad\xed\x51\x29\x6e\x08\xfe\xa9\xe2"
                        "\xb5\xa7\x36\xee\x62\xd6\x3d\xbe\xa4\x5e\x8c\xa9\x67"
                        "\x12\x82\xfa\xfb\x69\xda\x92\x72\x8b\x1a\x71\xde\x0a"
                        "\x9e\x06\x0b\x29\x05\xd6\xa5\xb6\x7e\xcd\x3b\x36\x92"
                        "\xdd\xbd\x7f\x2d\x77\x8b\x8c\x98\x03\xae\xe3\x28\x09"
                        "\x1b\x58\xfa\xb3\x24\xe4\xfa\xd6\x75\x94\x55\x85\x80"
                        "\x8b\x48\x31\xd7\xbc\x3f\xf4\xde\xf0\x8e\x4b\x7a\x9d"
                        "\xe5\x76\xd2\x65\x86\xce\xc6\x4b\x61\x16",
            .mac = "\x1a\xe1\x0b\x59\x4f\x09\xe2\x6a\x7e\x90\x2e\xcb\xd0\x60"
                   "\x06\x91",
        },
        {
            .key = "\x1c\x92\x40\xa5\xeb\x55\xd3\x8a\xf3\x33\x88\x86\x04\xf6"
                   "\xb5\xf0\x47\x39\x17\xc1\x40\x2b\x80\x09\x9d\xca\x5c\xbc"
                   "\x20\x70\x75\xc0",
            .nounce = "\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08",
            .ad = (char *)"\xf3\x33\x88\x86\x00\x00\x00\x00\x00\x00\x4e\x91",
            .ad_len = 12,
            .msg =
                (char *)"\x49\x6e\x74\x65\x72\x6e\x65\x74\x2d\x44\x72\x61\x66"
                        "\x74\x73\x20\x61\x72\x65\x20\x64\x72\x61\x66\x74\x20"
                        "\x64\x6f\x63\x75\x6d\x65\x6e\x74\x73\x20\x76\x61\x6c"
                        "\x69\x64\x20\x66\x6f\x72\x20\x61\x20\x6d\x61\x78\x69"
                        "\x6d\x75\x6d\x20\x6f\x66\x20\x73\x69\x78\x20\x6d\x6f"
                        "\x6e\x74\x68\x73\x20\x61\x6e\x64\x20\x6d\x61\x79\x20"
                        "\x62\x65\x20\x75\x70\x64\x61\x74\x65\x64\x2c\x20\x72"
                        "\x65\x70\x6c\x61\x63\x65\x64\x2c\x20\x6f\x72\x20\x6f"
                        "\x62\x73\x6f\x6c\x65\x74\x65\x64\x20\x62\x79\x20\x6f"
                        "\x74\x68\x65\x72\x20\x64\x6f\x63\x75\x6d\x65\x6e\x74"
                        "\x73\x20\x61\x74\x20\x61\x6e\x79\x20\x74\x69\x6d\x65"
                        "\x2e\x20\x49\x74\x20\x69\x73\x20\x69\x6e\x61\x70\x70"
                        "\x72\x6f\x70\x72\x69\x61\x74\x65\x20\x74\x6f\x20\x75"
                        "\x73\x65\x20\x49\x6e\x74\x65\x72\x6e\x65\x74\x2d\x44"
                        "\x72\x61\x66\x74\x73\x20\x61\x73\x20\x72\x65\x66\x65"
                        "\x72\x65\x6e\x63\x65\x20\x6d\x61\x74\x65\x72\x69\x61"
                        "\x6c\x20\x6f\x72\x20\x74\x6f\x20\x63\x69\x74\x65\x20"
                        "\x74\x68\x65\x6d\x20\x6f\x74\x68\x65\x72\x20\x74\x68"
                        "\x61\x6e\x20\x61\x73\x20\x2f\xe2\x80\x9c\x77\x6f\x72"
                        "\x6b\x20\x69\x6e\x20\x70\x72\x6f\x67\x72\x65\x73\x73"
                        "\x2e\x2f\xe2\x80\x9d",
            .expected =
                (char *)"\x64\xa0\x86\x15\x75\x86\x1a\xf4\x60\xf0\x62\xc7\x9b"
                        "\xe6\x43\xbd\x5e\x80\x5c\xfd\x34\x5c\xf3\x89\xf1\x08"
                        "\x67\x0a\xc7\x6c\x8c\xb2\x4c\x6c\xfc\x18\x75\x5d\x43"
                        "\xee\xa0\x9e\xe9\x4e\x38\x2d\x26\xb0\xbd\xb7\xb7\x3c"
                        "\x32\x1b\x01\x00\xd4\xf0\x3b\x7f\x35\x58\x94\xcf\x33"
                        "\x2f\x83\x0e\x71\x0b\x97\xce\x98\xc8\xa8\x4a\xbd\x0b"
                        "\x94\x81\x14\xad\x17\x6e\x00\x8d\x33\xbd\x60\xf9\x82"
                        "\xb1\xff\x37\xc8\x55\x97\x97\xa0\x6e\xf4\xf0\xef\x61"
                        "\xc1\x86\x32\x4e\x2b\x35\x06\x38\x36\x06\x90\x7b\x6a"
                        "\x7c\x02\xb0\xf9\xf6\x15\x7b\x53\xc8\x67\xe4\xb9\x16"
                        "\x6c\x76\x7b\x80\x4d\x46\xa5\x9b\x52\x16\xcd\xe7\xa4"
                        "\xe9\x90\x40\xc5\xa4\x04\x33\x22\x5e\xe2\x82\xa1\xb0"
                        "\xa0\x6c\x52\x3e\xaf\x45\x34\xd7\xf8\x3f\xa1\x15\x5b"
                        "\x00\x47\x71\x8c\xbc\x54\x6a\x0d\x07\x2b\x04\xb3\x56"
                        "\x4e\xea\x1b\x42\x22\x73\xf5\x48\x27\x1a\x0b\xb2\x31"
                        "\x60\x53\xfa\x76\x99\x19\x55\xeb\xd6\x31\x59\x43\x4e"
                        "\xce\xbb\x4e\x46\x6d\xae\x5a\x10\x73\xa6\x72\x76\x27"
                        "\x09\x7a\x10\x49\xe6\x17\xd9\x1d\x36\x10\x94\xfa\x68"
                        "\xf0\xff\x77\x98\x71\x30\x30\x5b\xea\xba\x2e\xda\x04"
                        "\xdf\x99\x7b\x71\x4d\x6c\x6f\x2c\x29\xa6\xad\x5c\xb4"
                        "\x02\x2b\x02\x70\x9b",
            .mac = "\xee\xad\x9d\x67\x89\x0c\xbb\x22\x39\x23\x36\xfe\xa1\x85"
                   "\x1f\x38",
        },
        {.expected = NULL}};
    for (size_t i = 0; tests[i].expected; ++i) {
      size_t len = strlen(tests[i].msg);
      char buffer[1024];
      char mac[16], mac2[16];
      FIO_MEMCPY(buffer, tests[i].msg, len);
      fio_chacha20_poly1305_enc(mac,
                                buffer,
                                len,
                                tests[i].ad,
                                tests[i].ad_len,
                                tests[i].key,
                                tests[i].nounce);
      FIO_ASSERT(!memcmp(buffer, tests[i].expected, len),
                 "ChaCha20Poly1305 encoding failed");
      fio_chacha20_poly1305_auth(mac2,
                                 buffer,
                                 len,
                                 tests[i].ad,
                                 tests[i].ad_len,
                                 tests[i].key,
                                 tests[i].nounce);
      FIO_ASSERT(!memcmp(mac, mac2, 16),
                 "ChaCha20Poly1305 authentication != Poly1305 code");
      FIO_ASSERT(!memcmp(mac, tests[i].mac, 16),
                 "ChaCha20Poly1305 authentication code failed");
      FIO_ASSERT(!fio_chacha20_poly1305_dec(mac,
                                            buffer,
                                            len,
                                            tests[i].ad,
                                            tests[i].ad_len,
                                            tests[i].key,
                                            tests[i].nounce),
                 "fio_chacha20_poly1305_dec returned error for %s",
                 tests[i].msg);
      FIO_ASSERT(
          !memcmp(buffer, tests[i].msg, len),
          "ChaCha20Poly1305 decoding failed for %s\nshould have been %.*s",
          tests[i].msg,
          (int)len,
          buffer);
    }
  }

#if !DEBUG
  fio_test_hash_function(fio__poly1305_speed_wrapper,
                         (char *)"Poly1305",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio__poly1305_speed_wrapper,
                         (char *)"Poly1305",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio__poly1305_speed_wrapper,
                         (char *)"Poly1305 (unaligned)",
                         13,
                         3,
                         0);
#if HAVE_OPENSSL && 0
  fio_test_hash_function(__poly1305_open_ssl_wrapper,
                         (char *)"Poly1305",
                         7,
                         0,
                         0);
  fio_test_hash_function(__poly1305_open_ssl_wrapper,
                         (char *)"Poly1305",
                         13,
                         0,
                         0);
  fio_test_hash_function(__poly1305_open_ssl_wrapper,
                         (char *)"Poly1305 (unaligned)",
                         13,
                         3,
                         0);
#endif /* HAVE_OPENSSL */
  fprintf(stderr, "\n");
  fio_test_hash_function(fio__chacha20_speed_wrapper,
                         (char *)"ChaCha20",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio__chacha20_speed_wrapper,
                         (char *)"ChaCha20",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio__chacha20_speed_wrapper,
                         (char *)"ChaCha20",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio__chacha20_speed_wrapper,
                         (char *)"ChaCha20 (unaligned)",
                         13,
                         3,
                         0);
  fprintf(stderr, "\n");
  fio_test_hash_function(fio__chacha20poly1305dec_speed_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio__chacha20poly1305dec_speed_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         13,
                         0,
                         0);
  fprintf(stderr, "\n");
  fio_test_hash_function(fio__chacha20poly1305_speed_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio__chacha20poly1305_speed_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         13,
                         0,
                         0);
#endif
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
*****************************************************************************
*/

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_CHACHA
#endif /* FIO_CHACHA */
