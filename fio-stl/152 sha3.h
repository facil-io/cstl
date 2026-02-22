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

/* Keccak-f[1600] permutation — optimized scalar implementation.
 *
 * Fuses theta+rho+pi per-row, then applies chi with 5 temporaries per row.
 * Eliminates the B[25] temporary array. State values that would be clobbered
 * by earlier rows' chi are saved before processing.
 *
 * Note: ARM SHA3 NEON (EOR3/RAX1/BCAX) was attempted but regressed ~50% due
 * to GPR↔NEON transfer overhead. Pure scalar compiles to excellent ARM64 code
 * with clang -O3 (all state words in GPRs, fully unrolled). */
FIO_IFUNC void fio___keccak_f1600(uint64_t *state) {
  uint64_t C[5], t0, t1, t2, t3, t4;
  /* Save slots for state values clobbered by earlier rows' chi.
   * Row 0 writes [0..4]:  save s1(row2), s2(row4), s3(row1), s4(row3)
   * Row 1 writes [5..9]:  save s5(row3), s7(row2), s8(row4)
   * Row 2 writes [10..14]: save s11(row3), s14(row4)
   * Row 3 writes [15..19]: save s15(row4) */
  uint64_t s1, s2, s3, s4, s5, s7, s8, s11, s14, s15;

  for (size_t round = 0; round < 24; ++round) {
    /* Theta: compute column parities */
    C[0] = state[0] ^ state[5] ^ state[10] ^ state[15] ^ state[20];
    C[1] = state[1] ^ state[6] ^ state[11] ^ state[16] ^ state[21];
    C[2] = state[2] ^ state[7] ^ state[12] ^ state[17] ^ state[22];
    C[3] = state[3] ^ state[8] ^ state[13] ^ state[18] ^ state[23];
    C[4] = state[4] ^ state[9] ^ state[14] ^ state[19] ^ state[24];

    t0 = C[4] ^ fio_lrot64(C[1], 1); /* D[0] */
    t1 = C[0] ^ fio_lrot64(C[2], 1); /* D[1] */
    t2 = C[1] ^ fio_lrot64(C[3], 1); /* D[2] */
    t3 = C[2] ^ fio_lrot64(C[4], 1); /* D[3] */
    t4 = C[3] ^ fio_lrot64(C[0], 1); /* D[4] */

    /* Apply theta to state and save values needed by later rows */
    state[0] ^= t0;
    s1 = state[1] ^ t1;
    s2 = state[2] ^ t2;
    s3 = state[3] ^ t3;
    s4 = state[4] ^ t4;
    s5 = state[5] ^ t0;
    state[6] ^= t1;
    s7 = state[7] ^ t2;
    s8 = state[8] ^ t3;
    state[9] ^= t4;
    state[10] ^= t0;
    s11 = state[11] ^ t1;
    state[12] ^= t2;
    state[13] ^= t3;
    s14 = state[14] ^ t4;
    s15 = state[15] ^ t0;
    state[16] ^= t1;
    state[17] ^= t2;
    state[18] ^= t3;
    state[19] ^= t4;
    state[20] ^= t0;
    state[21] ^= t1;
    state[22] ^= t2;
    state[23] ^= t3;
    state[24] ^= t4;

    /* Row 0: rho+pi then chi+iota */
    t0 = state[0];
    t1 = fio_lrot64(state[6], 44);
    t2 = fio_lrot64(state[12], 43);
    t3 = fio_lrot64(state[18], 21);
    t4 = fio_lrot64(state[24], 14);
    state[0] = t0 ^ ((~t1) & t2) ^ fio___keccak_rc[round];
    state[1] = t1 ^ ((~t2) & t3);
    state[2] = t2 ^ ((~t3) & t4);
    state[3] = t3 ^ ((~t4) & t0);
    state[4] = t4 ^ ((~t0) & t1);

    /* Row 1: uses saved s3 (was state[3]) */
    t0 = fio_lrot64(s3, 28);
    t1 = fio_lrot64(state[9], 20);
    t2 = fio_lrot64(state[10], 3);
    t3 = fio_lrot64(state[16], 45);
    t4 = fio_lrot64(state[22], 61);
    state[5] = t0 ^ ((~t1) & t2);
    state[6] = t1 ^ ((~t2) & t3);
    state[7] = t2 ^ ((~t3) & t4);
    state[8] = t3 ^ ((~t4) & t0);
    state[9] = t4 ^ ((~t0) & t1);

    /* Row 2: uses saved s1 (was state[1]), s7 (was state[7]) */
    t0 = fio_lrot64(s1, 1);
    t1 = fio_lrot64(s7, 6);
    t2 = fio_lrot64(state[13], 25);
    t3 = fio_lrot64(state[19], 8);
    t4 = fio_lrot64(state[20], 18);
    state[10] = t0 ^ ((~t1) & t2);
    state[11] = t1 ^ ((~t2) & t3);
    state[12] = t2 ^ ((~t3) & t4);
    state[13] = t3 ^ ((~t4) & t0);
    state[14] = t4 ^ ((~t0) & t1);

    /* Row 3: uses saved s4, s5, s11 (were state[4], state[5], state[11]) */
    t0 = fio_lrot64(s4, 27);
    t1 = fio_lrot64(s5, 36);
    t2 = fio_lrot64(s11, 10);
    t3 = fio_lrot64(state[17], 15);
    t4 = fio_lrot64(state[23], 56);
    state[15] = t0 ^ ((~t1) & t2);
    state[16] = t1 ^ ((~t2) & t3);
    state[17] = t2 ^ ((~t3) & t4);
    state[18] = t3 ^ ((~t4) & t0);
    state[19] = t4 ^ ((~t0) & t1);

    /* Row 4: uses saved s2, s8, s14, s15 */
    t0 = fio_lrot64(s2, 62);
    t1 = fio_lrot64(s8, 55);
    t2 = fio_lrot64(s14, 39);
    t3 = fio_lrot64(s15, 41);
    t4 = fio_lrot64(state[21], 2);
    state[20] = t0 ^ ((~t1) & t2);
    state[21] = t1 ^ ((~t2) & t3);
    state[22] = t2 ^ ((~t3) & t4);
    state[23] = t3 ^ ((~t4) & t0);
    state[24] = t4 ^ ((~t0) & t1);
  }
}

/* Absorb: XOR rate bytes from buf into state, then permute.
 * On little-endian (ARM64, x86), state words are already in native order,
 * so we can XOR directly as uint64_t without byte-swapping. */
FIO_IFUNC void fio___sha3_absorb_buf(fio_sha3_s *restrict h) {
  const size_t rate_words = h->rate >> 3;
  for (size_t i = 0; i < rate_words; ++i)
    h->state[i] ^= fio_buf2u64_le(h->buf + (i << 3));
  fio___keccak_f1600(h->state);
}

/* Absorb directly from input pointer (avoids copy to buf for full blocks) */
FIO_IFUNC void fio___sha3_absorb_ptr(fio_sha3_s *restrict h,
                                     const uint8_t *restrict p) {
  const size_t rate_words = h->rate >> 3;
  for (size_t i = 0; i < rate_words; ++i)
    h->state[i] ^= fio_buf2u64_le(p + (i << 3));
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
    fio___sha3_absorb_buf(h);
    h->buflen = 0;
    p += fill;
    len -= fill;
  }

  /* Process full blocks directly from input (no copy to buf) */
  while (len >= h->rate) {
    fio___sha3_absorb_ptr(h, p);
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
  fio___sha3_absorb_buf(h);

  /* Squeeze output — copy full words then handle remainder.
   * State is in native (little-endian) order on LE platforms. */
  uint8_t *o = (uint8_t *)out;
  const size_t full_words = h->outlen >> 3;
  for (size_t i = 0; i < full_words; ++i)
    fio_u2buf64_le(o + (i << 3), h->state[i]);
  /* Handle remaining bytes (outlen not multiple of 8) */
  const size_t rem = h->outlen & 7;
  if (rem) {
    uint64_t last = fio_ltole64(h->state[full_words]);
    FIO_MEMCPY(o + (full_words << 3), &last, rem);
  }
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
    fio___sha3_absorb_buf(h);
    h->buflen = 0;
    h->outlen = 1; /* Mark as finalized */
  }

  /* Squeeze output — buflen tracks byte offset within current Keccak state.
   * Copy full words where possible, byte-by-byte only at boundaries. */
  while (outlen > 0) {
    size_t available = h->rate - h->buflen;
    size_t to_copy = (outlen < available) ? outlen : available;
    /* Copy squeeze output using word-aligned access where possible */
    size_t pos = h->buflen;
    size_t copied = 0;
    /* Handle leading partial word */
    if ((pos & 7) && copied < to_copy) {
      size_t word_rem = 8 - (pos & 7);
      if (word_rem > to_copy - copied)
        word_rem = to_copy - copied;
      uint64_t w = fio_ltole64(h->state[pos >> 3]);
      for (size_t j = 0; j < word_rem; ++j)
        o[copied + j] = (uint8_t)(w >> (8 * ((pos + j) & 7)));
      copied += word_rem;
      pos += word_rem;
    }
    /* Copy full words */
    while (copied + 8 <= to_copy) {
      fio_u2buf64_le(o + copied, h->state[pos >> 3]);
      copied += 8;
      pos += 8;
    }
    /* Handle trailing partial word */
    if (copied < to_copy) {
      uint64_t w = fio_ltole64(h->state[pos >> 3]);
      for (size_t j = 0; copied < to_copy; ++j, ++copied)
        o[copied] = (uint8_t)(w >> (8 * j));
    }
    o += to_copy;
    outlen -= to_copy;
    h->buflen += to_copy;
    if (h->buflen == h->rate) {
      fio___keccak_f1600(h->state);
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
