/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SHA3               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    SHA-3
                    SHA3-224, SHA3-256, SHA3-384, SHA3-512
                          SHAKE128, SHAKE256




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SHA3) && !defined(H___FIO_SHA3___H)
#define H___FIO_SHA3___H
/* *****************************************************************************
SHA-3 API
***************************************************************************** */

/** Keccak state (1600 bits = 200 bytes = 25 x 64-bit words) */
typedef struct {
  uint64_t state[25];
  uint8_t buf[200]; /* rate buffer (max rate = 1600 - 2*capacity) */
  size_t buflen;    /* bytes in buffer */
  size_t rate;      /* rate in bytes */
  size_t outlen;    /* output length in bytes (0 for SHAKE) */
  uint8_t delim;    /* domain separation byte */
} fio_sha3_s;

/* *****************************************************************************
SHA3 Fixed-Output Functions API
***************************************************************************** */

/** Initialize SHA3-224 (28-byte output). */
FIO_IFUNC fio_sha3_s fio_sha3_224_init(void);
/** Initialize SHA3-256 (32-byte output). */
FIO_IFUNC fio_sha3_s fio_sha3_256_init(void);
/** Initialize SHA3-384 (48-byte output). */
FIO_IFUNC fio_sha3_s fio_sha3_384_init(void);
/** Initialize SHA3-512 (64-byte output). */
FIO_IFUNC fio_sha3_s fio_sha3_512_init(void);

/** Feed data into SHA3 hash. */
SFUNC void fio_sha3_consume(fio_sha3_s *restrict h,
                            const void *restrict data,
                            size_t len);

/** Finalize SHA3 hash. Writes h->outlen bytes to out. */
SFUNC void fio_sha3_finalize(fio_sha3_s *restrict h, void *restrict out);

/** Simple SHA3-224 (28-byte output). */
SFUNC void fio_sha3_224(void *restrict out,
                        const void *restrict data,
                        size_t len);

/** Simple SHA3-256 (32-byte output). */
SFUNC void fio_sha3_256(void *restrict out,
                        const void *restrict data,
                        size_t len);

/** Simple SHA3-384 (48-byte output). */
SFUNC void fio_sha3_384(void *restrict out,
                        const void *restrict data,
                        size_t len);

/** Simple SHA3-512 (64-byte output). */
SFUNC void fio_sha3_512(void *restrict out,
                        const void *restrict data,
                        size_t len);

/* *****************************************************************************
SHAKE Extendable-Output Functions API
***************************************************************************** */

/** Initialize SHAKE128 (variable output length). */
FIO_IFUNC fio_sha3_s fio_shake128_init(void);
/** Initialize SHAKE256 (variable output length). */
FIO_IFUNC fio_sha3_s fio_shake256_init(void);

/** Feed data into SHAKE. */
#define fio_shake_consume fio_sha3_consume

/** Squeeze output from SHAKE. Can be called multiple times. */
SFUNC void fio_shake_squeeze(fio_sha3_s *restrict h,
                             void *restrict out,
                             size_t outlen);

/** Simple SHAKE128 with specified output length. */
SFUNC void fio_shake128(void *restrict out,
                        size_t outlen,
                        const void *restrict data,
                        size_t len);

/** Simple SHAKE256 with specified output length. */
SFUNC void fio_shake256(void *restrict out,
                        size_t outlen,
                        const void *restrict data,
                        size_t len);

/* *****************************************************************************
Implementation - inline functions
***************************************************************************** */

/** Initialize SHA3-224. */
FIO_IFUNC fio_sha3_s fio_sha3_224_init(void) {
  fio_sha3_s h = {0};
  h.rate = 144; /* (1600 - 2*224) / 8 = 144 */
  h.outlen = 28;
  h.delim = 0x06; /* SHA3 domain separator */
  return h;
}

/** Initialize SHA3-256. */
FIO_IFUNC fio_sha3_s fio_sha3_256_init(void) {
  fio_sha3_s h = {0};
  h.rate = 136; /* (1600 - 2*256) / 8 = 136 */
  h.outlen = 32;
  h.delim = 0x06;
  return h;
}

/** Initialize SHA3-384. */
FIO_IFUNC fio_sha3_s fio_sha3_384_init(void) {
  fio_sha3_s h = {0};
  h.rate = 104; /* (1600 - 2*384) / 8 = 104 */
  h.outlen = 48;
  h.delim = 0x06;
  return h;
}

/** Initialize SHA3-512. */
FIO_IFUNC fio_sha3_s fio_sha3_512_init(void) {
  fio_sha3_s h = {0};
  h.rate = 72; /* (1600 - 2*512) / 8 = 72 */
  h.outlen = 64;
  h.delim = 0x06;
  return h;
}

/** Initialize SHAKE128. */
FIO_IFUNC fio_sha3_s fio_shake128_init(void) {
  fio_sha3_s h = {0};
  h.rate = 168;   /* (1600 - 2*128) / 8 = 168 */
  h.outlen = 0;   /* variable output */
  h.delim = 0x1F; /* SHAKE domain separator */
  return h;
}

/** Initialize SHAKE256. */
FIO_IFUNC fio_sha3_s fio_shake256_init(void) {
  fio_sha3_s h = {0};
  h.rate = 136; /* (1600 - 2*256) / 8 = 136 */
  h.outlen = 0;
  h.delim = 0x1F;
  return h;
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Keccak-f[1600] Permutation
***************************************************************************** */

/* Keccak round constants */
static const uint64_t fio___keccak_rc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

/* Keccak rotation offsets */
static const uint8_t fio___keccak_rot[25] = {
    0,  1,  62, 28, 27, /* row 0 */
    36, 44, 6,  55, 20, /* row 1 */
    3,  10, 43, 25, 39, /* row 2 */
    41, 45, 15, 21, 8,  /* row 3 */
    18, 2,  61, 56, 14  /* row 4 */
};

/* Keccak-f[1600] permutation */
FIO_IFUNC void fio___keccak_f1600(uint64_t *state) {
  uint64_t C[5], D[5], B[25];

  for (size_t round = 0; round < 24; ++round) {
    /* Theta step */
    C[0] = state[0] ^ state[5] ^ state[10] ^ state[15] ^ state[20];
    C[1] = state[1] ^ state[6] ^ state[11] ^ state[16] ^ state[21];
    C[2] = state[2] ^ state[7] ^ state[12] ^ state[17] ^ state[22];
    C[3] = state[3] ^ state[8] ^ state[13] ^ state[18] ^ state[23];
    C[4] = state[4] ^ state[9] ^ state[14] ^ state[19] ^ state[24];

    D[0] = C[4] ^ fio_lrot64(C[1], 1);
    D[1] = C[0] ^ fio_lrot64(C[2], 1);
    D[2] = C[1] ^ fio_lrot64(C[3], 1);
    D[3] = C[2] ^ fio_lrot64(C[4], 1);
    D[4] = C[3] ^ fio_lrot64(C[0], 1);

    for (size_t i = 0; i < 25; i += 5) {
      state[i + 0] ^= D[0];
      state[i + 1] ^= D[1];
      state[i + 2] ^= D[2];
      state[i + 3] ^= D[3];
      state[i + 4] ^= D[4];
    }

    /* Rho and Pi steps combined */
    B[0] = state[0];
    B[1] = fio_lrot64(state[6], 44);
    B[2] = fio_lrot64(state[12], 43);
    B[3] = fio_lrot64(state[18], 21);
    B[4] = fio_lrot64(state[24], 14);
    B[5] = fio_lrot64(state[3], 28);
    B[6] = fio_lrot64(state[9], 20);
    B[7] = fio_lrot64(state[10], 3);
    B[8] = fio_lrot64(state[16], 45);
    B[9] = fio_lrot64(state[22], 61);
    B[10] = fio_lrot64(state[1], 1);
    B[11] = fio_lrot64(state[7], 6);
    B[12] = fio_lrot64(state[13], 25);
    B[13] = fio_lrot64(state[19], 8);
    B[14] = fio_lrot64(state[20], 18);
    B[15] = fio_lrot64(state[4], 27);
    B[16] = fio_lrot64(state[5], 36);
    B[17] = fio_lrot64(state[11], 10);
    B[18] = fio_lrot64(state[17], 15);
    B[19] = fio_lrot64(state[23], 56);
    B[20] = fio_lrot64(state[2], 62);
    B[21] = fio_lrot64(state[8], 55);
    B[22] = fio_lrot64(state[14], 39);
    B[23] = fio_lrot64(state[15], 41);
    B[24] = fio_lrot64(state[21], 2);

    /* Chi step */
    for (size_t i = 0; i < 25; i += 5) {
      state[i + 0] = B[i + 0] ^ ((~B[i + 1]) & B[i + 2]);
      state[i + 1] = B[i + 1] ^ ((~B[i + 2]) & B[i + 3]);
      state[i + 2] = B[i + 2] ^ ((~B[i + 3]) & B[i + 4]);
      state[i + 3] = B[i + 3] ^ ((~B[i + 4]) & B[i + 0]);
      state[i + 4] = B[i + 4] ^ ((~B[i + 0]) & B[i + 1]);
    }

    /* Iota step */
    state[0] ^= fio___keccak_rc[round];
  }
}

/* Absorb a block into the state */
FIO_IFUNC void fio___sha3_absorb(fio_sha3_s *restrict h) {
  /* XOR rate bytes into state (little-endian) */
  size_t rate_words = h->rate / 8;
  for (size_t i = 0; i < rate_words; ++i)
    h->state[i] ^= fio_buf2u64_le(h->buf + i * 8);
  fio___keccak_f1600(h->state);
}

/** Feed data into SHA3 hash. */
SFUNC void fio_sha3_consume(fio_sha3_s *restrict h,
                            const void *restrict data,
                            size_t len) {
  const uint8_t *p = (const uint8_t *)data;

  /* Fill buffer if partially filled */
  if (h->buflen > 0) {
    size_t fill = h->rate - h->buflen;
    if (len < fill) {
      FIO_MEMCPY(h->buf + h->buflen, p, len);
      h->buflen += len;
      return;
    }
    FIO_MEMCPY(h->buf + h->buflen, p, fill);
    fio___sha3_absorb(h);
    h->buflen = 0;
    p += fill;
    len -= fill;
  }

  /* Process full blocks */
  while (len >= h->rate) {
    FIO_MEMCPY(h->buf, p, h->rate);
    fio___sha3_absorb(h);
    p += h->rate;
    len -= h->rate;
  }

  /* Buffer remaining data */
  if (len > 0) {
    FIO_MEMCPY(h->buf, p, len);
    h->buflen = len;
  }
}

/** Finalize SHA3 hash (fixed output). */
SFUNC void fio_sha3_finalize(fio_sha3_s *restrict h, void *restrict out) {
  /* Pad: append domain separator, then 10*1 padding */
  FIO_MEMSET(h->buf + h->buflen, 0, h->rate - h->buflen);
  h->buf[h->buflen] = h->delim;
  h->buf[h->rate - 1] |= 0x80;

  /* Final absorb */
  fio___sha3_absorb(h);

  /* Squeeze output (little-endian) */
  uint8_t *o = (uint8_t *)out;
  for (size_t i = 0; i < h->outlen; ++i)
    o[i] = (uint8_t)(h->state[i / 8] >> (8 * (i % 8)));
}

/** Squeeze output from SHAKE (can be called multiple times). */
SFUNC void fio_shake_squeeze(fio_sha3_s *restrict h,
                             void *restrict out,
                             size_t outlen) {
  uint8_t *o = (uint8_t *)out;

  /* First call: finalize padding */
  if (h->outlen == 0) {
    FIO_MEMSET(h->buf + h->buflen, 0, h->rate - h->buflen);
    h->buf[h->buflen] = h->delim;
    h->buf[h->rate - 1] |= 0x80;
    fio___sha3_absorb(h);
    h->buflen = 0;
    h->outlen = 1; /* Mark as finalized */
  }

  /* Squeeze output */
  while (outlen > 0) {
    if (h->buflen == 0) {
      /* Output from state */
      size_t to_copy = (outlen < h->rate) ? outlen : h->rate;
      for (size_t i = 0; i < to_copy; ++i)
        o[i] = (uint8_t)(h->state[i / 8] >> (8 * (i % 8)));
      o += to_copy;
      outlen -= to_copy;
      if (outlen > 0) {
        fio___keccak_f1600(h->state);
      }
    } else {
      /* Should not happen in normal use */
      h->buflen = 0;
    }
  }
}

/** Simple SHA3-224. */
SFUNC void fio_sha3_224(void *restrict out,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_sha3_224_init();
  fio_sha3_consume(&h, data, len);
  fio_sha3_finalize(&h, out);
}

/** Simple SHA3-256. */
SFUNC void fio_sha3_256(void *restrict out,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_sha3_256_init();
  fio_sha3_consume(&h, data, len);
  fio_sha3_finalize(&h, out);
}

/** Simple SHA3-384. */
SFUNC void fio_sha3_384(void *restrict out,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_sha3_384_init();
  fio_sha3_consume(&h, data, len);
  fio_sha3_finalize(&h, out);
}

/** Simple SHA3-512. */
SFUNC void fio_sha3_512(void *restrict out,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_sha3_512_init();
  fio_sha3_consume(&h, data, len);
  fio_sha3_finalize(&h, out);
}

/** Simple SHAKE128. */
SFUNC void fio_shake128(void *restrict out,
                        size_t outlen,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_shake128_init();
  fio_sha3_consume(&h, data, len);
  fio_shake_squeeze(&h, out, outlen);
}

/** Simple SHAKE256. */
SFUNC void fio_shake256(void *restrict out,
                        size_t outlen,
                        const void *restrict data,
                        size_t len) {
  fio_sha3_s h = fio_shake256_init();
  fio_sha3_consume(&h, data, len);
  fio_shake_squeeze(&h, out, outlen);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA3 */
#undef FIO_SHA3
