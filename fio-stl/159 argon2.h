/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ARGON2             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    ARGON2
                        Memory-Hard Password Hashing


Reference: RFC 9106 - Argon2 Memory-Hard Function for Password Hashing
           and Proof-of-Work Applications (version 1.3)

Implements Argon2d, Argon2i, and Argon2id using Blake2b as the underlying
hash function. Single-threaded (parallelism parameter affects memory layout
but lanes are processed sequentially).

When comparing Argon2 output hashes, use `fio_ct_is_eq` for constant-time
comparison to avoid timing side-channel attacks.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ARGON2) && !defined(H___FIO_ARGON2___H)
#define H___FIO_ARGON2___H
/* *****************************************************************************
Argon2 API
***************************************************************************** */

/** Argon2 type selector. */
typedef enum {
  FIO_ARGON2D = 0,
  FIO_ARGON2I = 1,
  FIO_ARGON2ID = 2,
} fio_argon2_type_e;

/** Argon2 named arguments. */
typedef struct {
  /** The password (message P). */
  fio_buf_info_s password;
  /** The salt (nonce S). */
  fio_buf_info_s salt;
  /** Optional secret key K. */
  fio_buf_info_s secret;
  /** Optional associated data X. */
  fio_buf_info_s ad;
  /** Time cost (number of passes t, minimum 1). */
  uint32_t t_cost;
  /** Memory cost in KiB (minimum 8*parallelism). */
  uint32_t m_cost;
  /** Degree of parallelism (number of lanes, 1 to 2^24-1). */
  uint32_t parallelism;
  /** Desired output (tag) length in bytes (minimum 4, default 32). */
  uint32_t outlen;
  /** Argon2 variant: FIO_ARGON2D, FIO_ARGON2I, or FIO_ARGON2ID. */
  fio_argon2_type_e type;
} fio_argon2_args_s;

/**
 * Computes Argon2 password hash (RFC 9106).
 *
 * Supports Argon2d, Argon2i, and Argon2id variants.
 * Output is written to the returned buffer (up to 64 bytes via fio_u512).
 * For outlen > 64, use fio_argon2_hash() instead.
 *
 * On error (invalid arguments, allocation failure) the result is all zeros.
 * When the output is used as a key (KDF), use fio_argon2_hash() and check its
 * return value.
 *
 * Use `fio_ct_is_eq` to compare output hashes in constant time.
 */
SFUNC fio_u512 fio_argon2(fio_argon2_args_s args);
#define fio_argon2(...) fio_argon2((fio_argon2_args_s){__VA_ARGS__})

/**
 * Computes Argon2 password hash into a caller-provided buffer.
 * Supports arbitrary output lengths (minimum 4 bytes; 0 selects 32). `out`
 * must hold `outlen` bytes.
 *
 * Returns 0 on success, -1 on error: NULL `out`, `outlen` 1-3, invalid `type`,
 * `parallelism` above 2^24-1, an input longer than 2^32-1 bytes, a memory
 * matrix exceeding the address space, or allocation failure (`out` untouched).
 */
SFUNC int fio_argon2_hash(void *out, fio_argon2_args_s args);
#define fio_argon2_hash(out, ...)                                              \
  fio_argon2_hash(out, (fio_argon2_args_s){__VA_ARGS__})

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* Argon2 version number (v1.3 = 0x13 = 19) */
#define FIO___ARGON2_VERSION 0x13

/* Block size: 1024 bytes = 128 uint64_t words */
#define FIO___ARGON2_BLOCK_SIZE  1024
#define FIO___ARGON2_BLOCK_WORDS 128
#define FIO___ARGON2_SYNC_POINTS 4

/* *****************************************************************************
GB function (Blake2b round function with multiplication hardening)
***************************************************************************** */

#define FIO___ARGON2_GB(a, b, c, d)                                            \
  do {                                                                         \
    (a) =                                                                      \
        (a) + (b) + 2 * (uint64_t)((uint32_t)(a)) * (uint64_t)((uint32_t)(b)); \
    (d) = fio_rrot64((d) ^ (a), 32);                                           \
    (c) =                                                                      \
        (c) + (d) + 2 * (uint64_t)((uint32_t)(c)) * (uint64_t)((uint32_t)(d)); \
    (b) = fio_rrot64((b) ^ (c), 24);                                           \
    (a) =                                                                      \
        (a) + (b) + 2 * (uint64_t)((uint32_t)(a)) * (uint64_t)((uint32_t)(b)); \
    (d) = fio_rrot64((d) ^ (a), 16);                                           \
    (c) =                                                                      \
        (c) + (d) + 2 * (uint64_t)((uint32_t)(c)) * (uint64_t)((uint32_t)(d)); \
    (b) = fio_rrot64((b) ^ (c), 63);                                           \
  } while (0)

/* *****************************************************************************
Permutation P: operates on 8 x 16-byte registers (16 uint64_t words)
S_i = (v_{2*i+1} || v_{2*i}), so v[0..15] is the 4x4 matrix.
`I(k)` maps the k-th word of the permutation input to an index in `r`, so rows
and columns are permuted in place (no gather / scatter copies).
***************************************************************************** */

#define FIO___ARGON2_P(r, I)                                                   \
  do {                                                                         \
    FIO___ARGON2_GB(r[I(0)], r[I(4)], r[I(8)], r[I(12)]);                      \
    FIO___ARGON2_GB(r[I(1)], r[I(5)], r[I(9)], r[I(13)]);                      \
    FIO___ARGON2_GB(r[I(2)], r[I(6)], r[I(10)], r[I(14)]);                     \
    FIO___ARGON2_GB(r[I(3)], r[I(7)], r[I(11)], r[I(15)]);                     \
    FIO___ARGON2_GB(r[I(0)], r[I(5)], r[I(10)], r[I(15)]);                     \
    FIO___ARGON2_GB(r[I(1)], r[I(6)], r[I(11)], r[I(12)]);                     \
    FIO___ARGON2_GB(r[I(2)], r[I(7)], r[I(8)], r[I(13)]);                      \
    FIO___ARGON2_GB(r[I(3)], r[I(4)], r[I(9)], r[I(14)]);                      \
  } while (0)
/* Row i: registers R_{8i} .. R_{8i+7} = words 16i .. 16i+15 */
#define FIO___ARGON2_ROW(k) (16 * i + (k))
/* Column i: registers R_{i}, R_{i+8}, ..., R_{i+56} (2 words each) */
#define FIO___ARGON2_COL(k) (2 * i + ((k) >> 1) * 16 + ((k)&1))

/* *****************************************************************************
Compression function G: two 1024-byte blocks -> one 1024-byte block
G(X, Y):
  R = X XOR Y
  Apply P row-wise to get Q
  Apply P column-wise to get Z
  Output = Z XOR R            (xor_out == 0)
  Output = Z XOR R XOR Output (xor_out != 0, passes after the first)

`r` is caller-owned scratch (128 words); the caller wipes it when done.
***************************************************************************** */

FIO_SFUNC void fio___argon2_compress(uint64_t *result,
                                     const uint64_t *x,
                                     const uint64_t *y,
                                     uint64_t *restrict r,
                                     int xor_out) {
  /* R = X XOR Y; result = R (or result ^= R) */
  if (xor_out) {
    for (size_t i = 0; i < FIO___ARGON2_BLOCK_WORDS; ++i) {
      r[i] = x[i] ^ y[i];
      result[i] ^= r[i];
    }
  } else {
    for (size_t i = 0; i < FIO___ARGON2_BLOCK_WORDS; ++i) {
      r[i] = x[i] ^ y[i];
      result[i] = r[i];
    }
  }
  for (size_t i = 0; i < 8; ++i)
    FIO___ARGON2_P(r, FIO___ARGON2_ROW);
  for (size_t i = 0; i < 8; ++i)
    FIO___ARGON2_P(r, FIO___ARGON2_COL);
  for (size_t i = 0; i < FIO___ARGON2_BLOCK_WORDS; ++i)
    result[i] ^= r[i];
}
#undef FIO___ARGON2_P
#undef FIO___ARGON2_ROW

/* *****************************************************************************
Variable-length hash function H' (Section 3.3)
Uses Blake2b as H. H^x(data) = Blake2b with output length x.
***************************************************************************** */

FIO_SFUNC void fio___argon2_hash_prime(void *out,
                                       uint32_t outlen,
                                       const void *in,
                                       size_t inlen) {
  if (outlen <= 64) {
    /* H'^T(A) = H^T(LE32(T) || A) */
    fio_blake2b_s h = fio_blake2b_init(outlen, NULL, 0);
    uint8_t le_outlen[4];
    fio_u2buf32_le(le_outlen, outlen);
    fio_blake2b_consume(&h, le_outlen, 4);
    fio_blake2b_consume(&h, in, inlen);
    fio_u512 r = fio_blake2b_finalize(&h); /* finalize wipes `h` */
    FIO_MEMCPY(out, r.u8, outlen);
    fio_secure_zero(&r, sizeof(r));
  } else {
    /* r = ceil(T/32) - 2 (no `outlen + 31` wrap near UINT32_MAX) */
    uint32_t r = (outlen >> 5) + ((outlen & 31) != 0) - 2;
    uint8_t le_outlen[4];
    fio_u2buf32_le(le_outlen, outlen);

    /* V_1 = H^64(LE32(T) || A) */
    uint8_t v_prev[64];
    {
      fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
      fio_blake2b_consume(&h, le_outlen, 4);
      fio_blake2b_consume(&h, in, inlen);
      fio_u512 tmp = fio_blake2b_finalize(&h); /* finalize wipes `h` */
      FIO_MEMCPY(v_prev, tmp.u8, 64);
      fio_secure_zero(&tmp, sizeof(tmp));
    }
    /* Output W_1 (first 32 bytes of V_1) */
    FIO_MEMCPY(out, v_prev, 32);

    /* V_2 .. V_r: V_i = H^64(V_{i-1}), output W_i (first 32 bytes);
     * V_{r+1} = H^(T-32*r)(V_r), output in full (T-32*r is in 33..64). */
    for (uint32_t i = 2; i <= r + 1; ++i) {
      uint32_t len = (i <= r) ? 64 : (outlen - 32 * r);
      fio_blake2b_s h = fio_blake2b_init(len, NULL, 0);
      fio_blake2b_consume(&h, v_prev, 64);
      fio_u512 tmp = fio_blake2b_finalize(&h); /* finalize wipes `h` */
      FIO_MEMCPY((uint8_t *)out + (size_t)(i - 1) * 32,
                 tmp.u8,
                 (i <= r) ? 32 : len);
      FIO_MEMCPY(v_prev, tmp.u8, 64);
      fio_secure_zero(&tmp, sizeof(tmp));
    }
    fio_secure_zero(v_prev, sizeof(v_prev));
  }
}

/* *****************************************************************************
Indexing helpers (Section 3.4)
***************************************************************************** */

/**
 * Compute reference block index [l][z] from J_1, J_2.
 * Section 3.4.2.
 */
FIO_IFUNC void fio___argon2_index(uint32_t *ref_lane,
                                  uint32_t *ref_index,
                                  uint32_t j1,
                                  uint32_t j2,
                                  uint32_t cur_lane,
                                  uint32_t cur_pass,
                                  uint32_t cur_slice,
                                  uint32_t cur_index,
                                  uint32_t lanes,
                                  uint32_t seg_len,
                                  uint32_t q) {
  /* l = J_2 mod p */
  uint32_t l = j2 % lanes;
  /* For first pass, first slice: must reference current lane */
  if (cur_pass == 0 && cur_slice == 0)
    l = cur_lane;

  /* Compute reference area size |W| */
  uint32_t ref_area_size;
  if (l == cur_lane) {
    /* Same lane: all blocks in last 3 segments + current segment progress */
    if (cur_pass == 0) {
      /* First pass: only blocks computed so far in this lane */
      ref_area_size = cur_slice * seg_len + cur_index - 1;
    } else {
      ref_area_size = q - seg_len + cur_index - 1;
    }
  } else {
    /* Different lane: last 3 completed segments */
    if (cur_pass == 0) {
      ref_area_size = cur_slice * seg_len;
    } else {
      ref_area_size = q - seg_len;
    }
    /* If first block of segment, exclude last index */
    if (cur_index == 0)
      ref_area_size -= 1;
  }

  /* Map J_1 to position within reference area (Section 3.4.2) */
  uint64_t x = (uint64_t)j1 * (uint64_t)j1;
  x >>= 32;
  x = (uint64_t)ref_area_size * x;
  x >>= 32;
  uint32_t zz = ref_area_size - 1 - (uint32_t)x;

  /* Map zz to actual block index in lane l */
  uint32_t start;
  if (cur_pass == 0) {
    start = 0;
  } else {
    start = (cur_slice + 1) * seg_len;
    if (start >= q)
      start = 0;
  }

  *ref_lane = l;
  *ref_index = (start + zz) % q;
}

/* *****************************************************************************
Generate Argon2i/Argon2id pseudo-random index block
***************************************************************************** */

FIO_SFUNC void fio___argon2_gen_addresses(uint64_t *addresses,
                                          uint64_t pass,
                                          uint64_t lane,
                                          uint64_t slice,
                                          uint64_t total_blocks,
                                          uint64_t total_passes,
                                          uint64_t type,
                                          uint32_t seg_len,
                                          uint64_t *scratch) {
  /* Number of 1024-byte address blocks needed:
     Each address block gives 128 uint64_t = 128 pairs of (J1,J2) as 32-bit
     Actually each uint64_t gives one (J1, J2) pair.
     We need seg_len pairs, so ceil(seg_len / 128) address blocks. */
  uint64_t zero_block[FIO___ARGON2_BLOCK_WORDS];
  uint64_t input_block[FIO___ARGON2_BLOCK_WORDS];
  uint64_t tmp[FIO___ARGON2_BLOCK_WORDS];

  FIO_MEMSET(zero_block, 0, FIO___ARGON2_BLOCK_SIZE);
  FIO_MEMSET(input_block, 0, FIO___ARGON2_BLOCK_SIZE);

  /* Z = LE64(r) || LE64(l) || LE64(sl) || LE64(m') || LE64(t) || LE64(y) */
  input_block[0] = pass;
  input_block[1] = lane;
  input_block[2] = slice;
  input_block[3] = total_blocks;
  input_block[4] = total_passes;
  input_block[5] = type;

  uint32_t num_addr_blocks =
      (seg_len + FIO___ARGON2_BLOCK_WORDS - 1) / FIO___ARGON2_BLOCK_WORDS;

  for (uint32_t i = 1; i <= num_addr_blocks; ++i) {
    input_block[6] = (uint64_t)i;
    /* G(ZERO, G(ZERO, input)) */
    fio___argon2_compress(tmp, zero_block, input_block, scratch, 0);
    fio___argon2_compress(addresses +
                              (size_t)(i - 1) * FIO___ARGON2_BLOCK_WORDS,
                          zero_block,
                          tmp,
                          scratch,
                          0);
  }
}

/* *****************************************************************************
Main Argon2 function
***************************************************************************** */

/** Computes Argon2 password hash into caller-provided buffer. */
SFUNC int fio_argon2_hash FIO_NOOP(void *out, fio_argon2_args_s args) {
  /* Validate parameters - reject (never clamp) anything that would change the
   * output size or matrix shape, or exceed RFC 9106 / address-space limits. */
  uint32_t outlen = args.outlen ? args.outlen : 32;
  uint32_t t_cost = args.t_cost ? args.t_cost : 1;
  uint32_t parallelism = args.parallelism ? args.parallelism : 1;
  uint32_t m_cost = args.m_cost;
  uint32_t type = (uint32_t)args.type;
  fio_buf_info_s in[4] = {args.password, args.salt, args.secret, args.ad};
  if (!out || outlen < 4 || type > 2 || parallelism > 0xFFFFFFUL)
    return -1;
  for (size_t i = 0; i < 4; ++i) {
    if (!in[i].buf)
      in[i].len = 0; /* NULL input is treated as empty */
    if ((uint64_t)in[i].len > 0xFFFFFFFFULL)
      return -1;
  }

  /* Minimum memory: 8 * parallelism KiB (no overflow, p < 2^24) */
  if (m_cost < 8 * parallelism)
    m_cost = 8 * parallelism;

  /* m' = 4 * p * floor(m / (4*p)), segment_length >= 2 */
  uint32_t segment_length = m_cost / (4 * parallelism);
  uint32_t q = segment_length * FIO___ARGON2_SYNC_POINTS; /* columns per lane */
  uint32_t m_prime = q * parallelism; /* total blocks (<= m_cost) */
#if SIZE_MAX <= UINT32_MAX
  /* On 32-bit targets the block count may fit uint32_t while the byte size
   * does not fit size_t. On wider targets m_prime * 1024 cannot overflow. */
  if (m_prime > SIZE_MAX / FIO___ARGON2_BLOCK_SIZE)
    return -1;
#endif /* SIZE_MAX <= UINT32_MAX */

  /* ======================== Allocate memory ============================== */
  /* Every block is written before it is read (no zero-fill required). */
  size_t mem_bytes = (size_t)m_prime * FIO___ARGON2_BLOCK_SIZE;
  uint64_t *memory = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, mem_bytes, 0);
  if (!memory)
    return -1;
  /* Address buffer for Argon2i/Argon2id (data-independent segments) */
  uint64_t *addr_block = NULL;
  size_t addr_block_size = 0;
  if (type == FIO_ARGON2I || type == FIO_ARGON2ID) {
    size_t num_addr_blocks =
        ((size_t)segment_length + FIO___ARGON2_BLOCK_WORDS - 1) /
        FIO___ARGON2_BLOCK_WORDS;
    addr_block_size = num_addr_blocks * FIO___ARGON2_BLOCK_SIZE;
    addr_block = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, addr_block_size, 0);
    if (!addr_block) {
      FIO_MEM_FREE(memory, mem_bytes); /* nothing secret written yet */
      return -1;
    }
  }

  /* ======================== Step 1: Compute H_0 ========================== */
  /* H_0 = H^64(LE32(p) || LE32(T) || LE32(m) || LE32(t) || LE32(v) ||
   *        LE32(y) || LE32(len(P)) || P || LE32(len(S)) || S ||
   *        LE32(len(K)) || K || LE32(len(X)) || X) */
  /* h0_input = H_0 || LE32(column) || LE32(lane) (Steps 3-4) */
  uint8_t h0_input[72];
  {
    fio_blake2b_s bctx = fio_blake2b_init(64, NULL, 0);
    uint8_t le32buf[24];
    fio_u2buf32_le(le32buf, parallelism);
    fio_u2buf32_le(le32buf + 4, outlen);
    fio_u2buf32_le(le32buf + 8, m_cost);
    fio_u2buf32_le(le32buf + 12, t_cost);
    fio_u2buf32_le(le32buf + 16, FIO___ARGON2_VERSION);
    fio_u2buf32_le(le32buf + 20, type);
    fio_blake2b_consume(&bctx, le32buf, 24);
    for (size_t i = 0; i < 4; ++i) { /* P, S, K, X */
      fio_u2buf32_le(le32buf, (uint32_t)in[i].len);
      fio_blake2b_consume(&bctx, le32buf, 4);
      fio_blake2b_consume(&bctx, in[i].buf, in[i].len);
    }
    fio_u512 h0 = fio_blake2b_finalize(&bctx); /* finalize wipes `bctx` */
    FIO_MEMCPY(h0_input, h0.u8, 64);
    fio_secure_zero(&h0, sizeof(h0));
  }

  /* Helper macro: pointer to block B[lane][col] */
#define FIO___ARGON2_BLOCK(lane, col)                                          \
  (memory +                                                                    \
   ((size_t)(lane) * (size_t)q + (size_t)(col)) * FIO___ARGON2_BLOCK_WORDS)

  /* ======================== Steps 3-4: Init first two columns ============ */
  /* For each lane i: B[i][0] = H'^1024(H_0 || LE32(0) || LE32(i))
   *                  B[i][1] = H'^1024(H_0 || LE32(1) || LE32(i)) */
  for (uint32_t i = 0; i < parallelism; ++i) {
    for (uint32_t j = 0; j < 2; ++j) {
      uint64_t *blk = FIO___ARGON2_BLOCK(i, j);
      fio_u2buf32_le(h0_input + 64, j);
      fio_u2buf32_le(h0_input + 68, i);
      fio___argon2_hash_prime(blk, 1024, h0_input, 72);
#if __BIG_ENDIAN__ /* blocks are little-endian 64-bit words */
      for (size_t w = 0; w < FIO___ARGON2_BLOCK_WORDS; ++w)
        blk[w] = fio_ltole64(blk[w]);
#endif
    }
  }
  fio_secure_zero(h0_input, sizeof(h0_input));

  /* ======================== Step 5-6: Fill memory ======================== */
  /* scratch for the compression function, wiped once at the end */
  uint64_t scratch[FIO___ARGON2_BLOCK_WORDS];
  for (uint32_t pass = 0; pass < t_cost; ++pass) {
    for (uint32_t slice = 0; slice < FIO___ARGON2_SYNC_POINTS; ++slice) {
      /* data-independent indexing (Argon2i, first half-pass of Argon2id) */
      const int use_independent =
          (type == FIO_ARGON2I) ||
          (type == FIO_ARGON2ID && pass == 0 && slice < 2);
      const uint32_t start_index = (pass == 0 && slice == 0) ? 2 : 0;
      for (uint32_t lane = 0; lane < parallelism; ++lane) {
        if (use_independent)
          fio___argon2_gen_addresses(addr_block,
                                     (uint64_t)pass,
                                     (uint64_t)lane,
                                     (uint64_t)slice,
                                     (uint64_t)m_prime,
                                     (uint64_t)t_cost,
                                     (uint64_t)type,
                                     segment_length,
                                     scratch);

        for (uint32_t idx = start_index; idx < segment_length; ++idx) {
          uint32_t cur_col = slice * segment_length + idx;
          uint32_t prev_col = (cur_col == 0) ? (q - 1) : (cur_col - 1);
          uint64_t *cur_block = FIO___ARGON2_BLOCK(lane, cur_col);
          uint64_t *prev_block = FIO___ARGON2_BLOCK(lane, prev_col);

          /* J1, J2 from the address block or the previous block */
          uint64_t val = use_independent ? addr_block[idx] : prev_block[0];
          uint32_t ref_lane, ref_index;
          fio___argon2_index(&ref_lane,
                             &ref_index,
                             (uint32_t)val,
                             (uint32_t)(val >> 32),
                             lane,
                             pass,
                             slice,
                             idx,
                             parallelism,
                             segment_length,
                             q);

          /* First pass: B[i][j] = G(B[i][j-1], B[l][z])
           * Later passes: B[i][j] = G(B[i][j-1], B[l][z]) XOR B[i][j] */
          fio___argon2_compress(cur_block,
                                prev_block,
                                FIO___ARGON2_BLOCK(ref_lane, ref_index),
                                scratch,
                                pass != 0);
        }
      }
    }
  }
  fio_secure_zero(scratch, sizeof(scratch));

  /* ======================== Step 7-8: Finalize ============================ */
  /* C = B[0][q-1] XOR B[1][q-1] XOR ... XOR B[p-1][q-1] */
  uint64_t final_block[FIO___ARGON2_BLOCK_WORDS];
  FIO_MEMCPY(final_block,
             FIO___ARGON2_BLOCK(0, q - 1),
             FIO___ARGON2_BLOCK_SIZE);
  for (uint32_t i = 1; i < parallelism; ++i) {
    const uint64_t *last = FIO___ARGON2_BLOCK(i, q - 1);
    for (size_t w = 0; w < FIO___ARGON2_BLOCK_WORDS; ++w)
      final_block[w] ^= last[w];
  }
#if __BIG_ENDIAN__ /* hash C as little-endian bytes */
  for (size_t w = 0; w < FIO___ARGON2_BLOCK_WORDS; ++w)
    final_block[w] = fio_ltole64(final_block[w]);
#endif

  /* Tag = H'^T(C) */
  fio___argon2_hash_prime(out, outlen, final_block, FIO___ARGON2_BLOCK_SIZE);

  /* ======================== Cleanup ====================================== */
  fio_secure_zero(final_block, sizeof(final_block));
  if (addr_block) /* parameter-derived, not secret */
    FIO_MEM_FREE(addr_block, addr_block_size);
  fio_secure_zero(memory, mem_bytes);
  FIO_MEM_FREE(memory, mem_bytes);

#undef FIO___ARGON2_BLOCK
  return 0;
}

void fio_argon2___(void); /* IDE Marker */
/** Computes Argon2 password hash, returning result as fio_u512. */
SFUNC fio_u512 fio_argon2 FIO_NOOP(fio_argon2_args_s args) {
  fio_u512 result = {0}; /* stays all-zero on error */
  uint32_t outlen = args.outlen ? args.outlen : 32;
  if (outlen < 4)
    outlen = 4;
  if (outlen > 64)
    outlen = 64;
  args.outlen = outlen;
  (fio_argon2_hash)((void *)result.u8, args);
  return result;
}

#undef FIO___ARGON2_GB
#undef FIO___ARGON2_VERSION
#undef FIO___ARGON2_BLOCK_SIZE
#undef FIO___ARGON2_BLOCK_WORDS
#undef FIO___ARGON2_SYNC_POINTS

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ARGON2 */
