/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_CHACHA     /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                              ChaCha20 & Poly1305



Copyright and License: see header file (000 header.h) or top of file
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
SFUNC void fio_chacha20(void *data,
                        size_t len,
                        void *key,
                        void *nounce,
                        uint32_t counter);

/**
 * Given a Poly1305 256bit (32 byte) key, writes the authentication code for the
 * poly message and additional data into `mac_dest`.
 *
 * * `key`    MUST point to a 256 bit long memory address (32 Bytes).
 */
SFUNC void fio_poly1305_auth(void *mac_dest,
                             void *key256bits,
                             void *message,
                             size_t len,
                             void *additional_data,
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
SFUNC void fio_chacha20_poly1305_enc(void *mac,
                                     void *data,
                                     size_t len,
                                     void *ad, /* additional data */
                                     size_t adlen,
                                     void *key,
                                     void *nounce);

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
SFUNC int fio_chacha20_poly1305_dec(void *mac,
                                    void *data,
                                    size_t len,
                                    void *ad, /* additional data */
                                    size_t adlen,
                                    void *key,
                                    void *nounce);

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

FIO_IFUNC fio___poly_s fio___poly_init(void *key256b) {
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
                                        void *msg,
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
  for (size_t i = 15; i < len; i += 16) {
    fio___poly_consume128bit(pl, msg, 1);
    msg += 16;
  }
  if (!(len & 15))
    return;
  n[0] = 0;
  n[1] = 0;
  fio_memcpy15x(n, msg, len);
  n[0] = fio_ltole64(n[0]);
  n[1] = fio_ltole64(n[1]);
  ((uint8_t *)n)[len & 15] = 0x01;
  fio___poly_consume128bit(pl, (void *)n, 0);
}

/*
 * Given a Poly1305 key, writes a MAC into `mac_dest`. */
SFUNC void fio_poly1305_auth(void *mac,
                             void *key,
                             void *msg,
                             size_t len,
                             void *ad,
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

FIO_IFUNC fio_512u fio___chacha_init(void *key,
                                     void *nounce,
                                     uint32_t counter) {
  fio_512u o = {
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

// clang-format off
#define FIO___CHACHA_QROUND(a, b, c, d)                                        \
    a += b; d ^= a; d = fio_lrot32(d, 16);                                     \
    c += d; b ^= c; b = fio_lrot32(b, 12);                                     \
    a += b; d ^= a; d = fio_lrot32(d, 8);                                      \
    c += d; b ^= c; b = fio_lrot32(b, 7);
// clang-format on

FIO_IFUNC void fio___chacha_xor(fio_512u *dest, fio_512u *c) {
  for (size_t i = 0; i < 16; ++i) {
    dest->u32[i] ^= fio_ltole32(c->u32[i]);
  }
}

FIO_IFUNC void fio___chacha_round20(fio_512u *c) {
  fio_512u c2 = *c;
  for (size_t round = 0; round < 10; ++round) {
    for (size_t i = 0; i < 4; ++i) {
      FIO___CHACHA_QROUND(c2.u32[i],
                          c2.u32[i + 4],
                          c2.u32[i + 8],
                          c2.u32[i + 12]);
    }
    for (size_t i = 0; i < 4; ++i) {
      FIO___CHACHA_QROUND(c2.u32[i],
                          c2.u32[((i + 1) & 3) + 4],
                          c2.u32[((i + 2) & 3) + 8],
                          c2.u32[((i + 3) & 3) + 12]);
    }
  }
  for (size_t i = 0; i < 16; ++i) {
    c->u32[i] += c2.u32[i];
  }
}

SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        void *key,
                        void *nounce,
                        uint32_t counter) {
  fio_512u c = fio___chacha_init(key, nounce, counter);
  fio_512u dest;
  for (size_t i = 0; i < len; i += 64) {
    fio_512u c2 = c;
    ++c.u32[12]; /* block counter */
    fio___chacha_round20(&c2);
    fio_memcpy64(dest.u64, data);
    fio___chacha_xor(&dest, &c2);
    fio_memcpy64(data, dest.u64);
    data = (void *)((uint8_t *)data + 64);
  }
  if (!(len & 63))
    return;
  FIO_MEMSET(dest.u64, 0, 64);
  fio___chacha_round20(&c);
  fio_memcpy63x(dest.u64, data, len);
  fio___chacha_xor(&dest, &c);
  fio_memcpy63x(data, dest.u64, len);
}

/* *****************************************************************************
ChaCha20Poly1305 Encryption with Authentication
***************************************************************************** */

SFUNC void fio_chacha20_poly1305_enc(void *mac,
                                     void *data,
                                     size_t len,
                                     void *ad, /* additional data */
                                     size_t adlen,
                                     void *key,
                                     void *nounce) {
  fio_512u c = fio___chacha_init(key, nounce, 0);
  fio_512u c2 = c;
  fio_512u dest;
  fio___chacha_round20(&c2); /* computes poly1305 key */
  fio___poly_s pl = fio___poly_init(&c2);
  if (adlen)
    fio___poly_consume_msg(&pl, (uint8_t *)ad, adlen);
  ++c.u32[12]; /* block counter */
  for (size_t i = 0; i < len; i += 64) {
    c2 = c;
    ++c.u32[12]; /* block counter */
    fio___chacha_round20(&c2);
    fio_memcpy64(dest.u64, data);
    fio___chacha_xor(&dest, &c2);
    fio___poly_consume128bit(&pl, dest.u64, 1);
    fio___poly_consume128bit(&pl, dest.u64 + 2, 1);
    fio___poly_consume128bit(&pl, dest.u64 + 4, 1);
    fio___poly_consume128bit(&pl, dest.u64 + 6, 1);
    fio_memcpy64(data, dest.u64);
    data = (void *)((uint8_t *)data + 64);
  }
  if (!(len & 63))
    return;
  fio___chacha_round20(&c);
  FIO_MEMSET(dest.u64, 0, 64);
  fio_memcpy63x(dest.u64, data, len);
  fio___chacha_xor(&dest, &c);
  fio___poly_consume_msg(&pl, (uint8_t *)&dest, (len & 63));
  fio_memcpy63x(data, dest.u64, len);
  fio___poly_finilize(&pl);
  fio_u2buf64_little(mac, pl.a[0]);
  fio_u2buf64_little(&((char *)mac)[8], pl.a[1]);
}

SFUNC void fio_chacha20_poly1305_auth(void *mac,
                                      void *data,
                                      size_t len,
                                      void *ad, /* additional data */
                                      size_t adlen,
                                      void *key,
                                      void *nounce) {
  fio_512u c = fio___chacha_init(key, nounce, 0);
  fio___chacha_round20(&c); /* computes poly1305 key */
  fio___poly_s pl = fio___poly_init(&c);
  if (adlen)
    fio___poly_consume_msg(&pl, (uint8_t *)ad, adlen);
  if (len)
    fio___poly_consume_msg(&pl, (uint8_t *)data, len);
  fio___poly_finilize(&pl);
  fio_u2buf64_little(mac, pl.a[0]);
  fio_u2buf64_little(&((char *)mac)[8], pl.a[1]);
}

SFUNC int fio_chacha20_poly1305_dec(void *mac,
                                    void *data,
                                    size_t len,
                                    void *ad, /* additional data */
                                    size_t adlen,
                                    void *key,
                                    void *nounce) {
  uint64_t auth[2];
  fio_poly1305_auth(&auth, key, data, len, ad, adlen);
  if (((auth[0] != fio_buf2u64_little(mac)) |
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
  {
    uint32_t tv1[4] = {
        0x11111111,
        0x01020304,
        0x9b8d6f43,
        0x01234567,
    };
    FIO___CHACHA_QROUND(tv1[0], tv1[1], tv1[2], tv1[3]);
    FIO_ASSERT((tv1[0] == 0xea2a92f4 && tv1[1] == 0xcb1cf8ce &&
                tv1[2] == 0x4581472e && tv1[3] == 0x5881c4bb),
               "ChaCha quarter round example error");
  }
  { /* test ChaCha20 independently */
    char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                        "\x0d\x0e\x0f"
                        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                        "\x1d\x1e\x1f";
    char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
    char *src = (char *)"\x4c\x61\x64\x69\x65\x73\x20\x61\x6e\x64\x20\x47\x65"
                        "\x6e\x74\x6c"
                        "\x65\x6d\x65\x6e\x20\x6f\x66\x20\x74\x68\x65\x20\x63"
                        "\x6c\x61\x73"
                        "\x73\x20\x6f\x66\x20\x27\x39\x39\x3a\x20\x49\x66\x20"
                        "\x49\x20\x63"
                        "\x6f\x75\x6c\x64\x20\x6f\x66\x66\x65\x72\x20\x79\x6f"
                        "\x75\x20\x6f"
                        "\x6e\x6c\x79\x20\x6f\x6e\x65\x20\x74\x69\x70\x20\x66"
                        "\x6f\x72\x20"
                        "\x74\x68\x65\x20\x66\x75\x74\x75\x72\x65\x2c\x20\x73"
                        "\x75\x6e\x73"
                        "\x63\x72\x65\x65\x6e\x20\x77\x6f\x75\x6c\x64\x20\x62"
                        "\x65\x20\x69"
                        "\x74\x2e";
    char *expected = (char *)"\x6e\x2e\x35\x9a\x25\x68\xf9\x80\x41\xba\x07\x28"
                             "\xdd\x0d\x69\x81"
                             "\xe9\x7e\x7a\xec\x1d\x43\x60\xc2\x0a\x27\xaf\xcc"
                             "\xfd\x9f\xae\x0b"
                             "\xf9\x1b\x65\xc5\x52\x47\x33\xab\x8f\x59\x3d\xab"
                             "\xcd\x62\xb3\x57"
                             "\x16\x39\xd6\x24\xe6\x51\x52\xab\x8f\x53\x0c\x35"
                             "\x9f\x08\x61\xd8"
                             "\x07\xca\x0d\xbf\x50\x0d\x6a\x61\x56\xa3\x8e\x08"
                             "\x8a\x22\xb6\x5e"
                             "\x52\xbc\x51\x4d\x16\xcc\xf8\x06\x81\x8c\xe9\x1a"
                             "\xb7\x79\x37\x36"
                             "\x5a\xf9\x0b\xbf\x74\xa3\x5b\xe6\xb4\x0b\x8e\xed"
                             "\xf2\x78\x5e\x42"
                             "\x87\x4d";
    size_t len = strlen(src);
    char buffer[256];
    FIO_MEMCPY(buffer, src, len);
    fio_chacha20(buffer, len, key, nounce, 1);
    FIO_ASSERT(!memcmp(buffer, expected, len), "ChaCha20 encoding failed");
    fio_chacha20(buffer, len, key, nounce, 1);
    FIO_ASSERT(!memcmp(buffer, src, len), "ChaCha20 decoding failed");
  }
  { /* test Poly1305 independently */
    char *key = (char *)"\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe\x42"
                        "\xd5\x06\xa8"
                        "\x01\x03\x80\x8a\xfb\x0d\xb2\xfd\x4a\xbf\xf6\xaf\x41"
                        "\x49\xf5\x1b";
    char *msg = (char *)"Cryptographic Forum Research Group";
    char *expected = (char *)"\xa8\x06\x1d\xc1\x30\x51\x36\xc6\xc2\x2b\x8b\xaf"
                             "\x0c\x01\x27\xa9";
    char auth[16] = {0};
    char buf1[33] = {0};
    char buf2[33] = {0};
    fio_poly1305_auth(auth, key, msg, strlen(msg), NULL, 0);
    for (int i = 0; i < 16; ++i) {
      buf1[(i << 1)] = fio_i2c(((auth[i] >> 4) & 0xF));
      buf1[(i << 1) + 1] = fio_i2c(((auth[i]) & 0xF));
      buf2[(i << 1)] = fio_i2c(((expected[i] >> 4) & 0xF));
      buf2[(i << 1) + 1] = fio_i2c(((expected[i]) & 0xF));
    }
    FIO_ASSERT(!memcmp(auth, expected, 16),
               "Poly1305 example authentication failed:\n\t%s != %s",
               buf1,
               buf2);
    (void)expected;
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
