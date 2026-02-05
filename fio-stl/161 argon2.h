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
  /** Degree of parallelism (number of lanes, minimum 1). */
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
 * Use `fio_ct_is_eq` to compare output hashes in constant time.
 */
SFUNC fio_u512 fio_argon2(fio_argon2_args_s args);
#define fio_argon2(...) fio_argon2((fio_argon2_args_s){__VA_ARGS__})

/**
 * Computes Argon2 password hash into a caller-provided buffer.
 * Supports arbitrary output lengths (minimum 4 bytes).
 * Returns 0 on success, -1 on error.
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
***************************************************************************** */

FIO_IFUNC void fio___argon2_permutation_p(uint64_t *v) {
  FIO___ARGON2_GB(v[0], v[4], v[8], v[12]);
  FIO___ARGON2_GB(v[1], v[5], v[9], v[13]);
  FIO___ARGON2_GB(v[2], v[6], v[10], v[14]);
  FIO___ARGON2_GB(v[3], v[7], v[11], v[15]);
  FIO___ARGON2_GB(v[0], v[5], v[10], v[15]);
  FIO___ARGON2_GB(v[1], v[6], v[11], v[12]);
  FIO___ARGON2_GB(v[2], v[7], v[8], v[13]);
  FIO___ARGON2_GB(v[3], v[4], v[9], v[14]);
}

/* *****************************************************************************
Compression function G: two 1024-byte blocks -> one 1024-byte block
G(X, Y):
  R = X XOR Y
  Apply P row-wise to get Q
  Apply P column-wise to get Z
  Output = Z XOR R
***************************************************************************** */

FIO_SFUNC void fio___argon2_compress(uint64_t *result,
                                     const uint64_t *x,
                                     const uint64_t *y) {
  uint64_t r[FIO___ARGON2_BLOCK_WORDS];
  uint64_t tmp[16];

  /* R = X XOR Y */
  for (size_t i = 0; i < FIO___ARGON2_BLOCK_WORDS; ++i)
    r[i] = x[i] ^ y[i];

  /* Copy R to result (will be XORed with Z at the end) */
  FIO_MEMCPY(result, r, FIO___ARGON2_BLOCK_SIZE);

  /* Apply P row-wise: 8 rows of 8 x 16-byte registers = 16 uint64_t each */
  for (size_t i = 0; i < 8; ++i) {
    /* Row i: registers R_{8i} .. R_{8i+7}, each 16 bytes = 2 uint64_t
       So row i spans r[16*i .. 16*i+15] */
    FIO_MEMCPY(tmp, r + 16 * i, 128);
    fio___argon2_permutation_p(tmp);
    FIO_MEMCPY(r + 16 * i, tmp, 128);
  }

  /* Apply P column-wise: 8 columns */
  for (size_t i = 0; i < 8; ++i) {
    /* Column i: registers R_{i}, R_{i+8}, R_{i+16}, ..., R_{i+56}
       Each register is 2 uint64_t at offset 2*reg_idx in the block */
    tmp[0] = r[2 * i];
    tmp[1] = r[2 * i + 1];
    tmp[2] = r[2 * (i + 8)];
    tmp[3] = r[2 * (i + 8) + 1];
    tmp[4] = r[2 * (i + 16)];
    tmp[5] = r[2 * (i + 16) + 1];
    tmp[6] = r[2 * (i + 24)];
    tmp[7] = r[2 * (i + 24) + 1];
    tmp[8] = r[2 * (i + 32)];
    tmp[9] = r[2 * (i + 32) + 1];
    tmp[10] = r[2 * (i + 40)];
    tmp[11] = r[2 * (i + 40) + 1];
    tmp[12] = r[2 * (i + 48)];
    tmp[13] = r[2 * (i + 48) + 1];
    tmp[14] = r[2 * (i + 56)];
    tmp[15] = r[2 * (i + 56) + 1];

    fio___argon2_permutation_p(tmp);

    r[2 * i] = tmp[0];
    r[2 * i + 1] = tmp[1];
    r[2 * (i + 8)] = tmp[2];
    r[2 * (i + 8) + 1] = tmp[3];
    r[2 * (i + 16)] = tmp[4];
    r[2 * (i + 16) + 1] = tmp[5];
    r[2 * (i + 24)] = tmp[6];
    r[2 * (i + 24) + 1] = tmp[7];
    r[2 * (i + 32)] = tmp[8];
    r[2 * (i + 32) + 1] = tmp[9];
    r[2 * (i + 40)] = tmp[10];
    r[2 * (i + 40) + 1] = tmp[11];
    r[2 * (i + 48)] = tmp[12];
    r[2 * (i + 48) + 1] = tmp[13];
    r[2 * (i + 56)] = tmp[14];
    r[2 * (i + 56) + 1] = tmp[15];
  }

  /* result = Z XOR R (Z is now in r, original R is in result) */
  for (size_t i = 0; i < FIO___ARGON2_BLOCK_WORDS; ++i)
    result[i] ^= r[i];
  fio_secure_zero(r, sizeof(r));
  fio_secure_zero(tmp, sizeof(tmp));
}

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
    fio_blake2b_finalize(&h, out);
    fio_secure_zero(&h, sizeof(h));
  } else {
    /* r = ceil(T/32) - 2 */
    uint32_t r = (outlen + 31) / 32 - 2;
    uint8_t le_outlen[4];
    fio_u2buf32_le(le_outlen, outlen);

    /* V_1 = H^64(LE32(T) || A) */
    uint8_t v_prev[64];
    {
      fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
      fio_blake2b_consume(&h, le_outlen, 4);
      fio_blake2b_consume(&h, in, inlen);
      fio_blake2b_finalize(&h, v_prev);
    }
    /* Output W_1 (first 32 bytes of V_1) */
    FIO_MEMCPY(out, v_prev, 32);

    /* V_2 .. V_r: each V_i = H^64(V_{i-1}) */
    for (uint32_t i = 2; i <= r; ++i) {
      uint8_t v_cur[64];
      fio_blake2b(v_cur, 64, v_prev, 64, NULL, 0);
      FIO_MEMCPY((uint8_t *)out + (i - 1) * 32, v_cur, 32);
      FIO_MEMCPY(v_prev, v_cur, 64);
    }

    /* V_{r+1} = H^(T-32*r)(V_r) */
    uint32_t last_len = outlen - 32 * r;
    uint8_t v_last[64];
    fio_blake2b(v_last, last_len, v_prev, 64, NULL, 0);
    FIO_MEMCPY((uint8_t *)out + r * 32, v_last, last_len);
    fio_secure_zero(v_prev, sizeof(v_prev));
    fio_secure_zero(v_last, sizeof(v_last));
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
                                          uint32_t seg_len) {
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
    fio___argon2_compress(tmp, zero_block, input_block);
    fio___argon2_compress(addresses +
                              (size_t)(i - 1) * FIO___ARGON2_BLOCK_WORDS,
                          zero_block,
                          tmp);
  }
}

/* *****************************************************************************
Main Argon2 function
***************************************************************************** */

/** Computes Argon2 password hash into caller-provided buffer. */
SFUNC int fio_argon2_hash FIO_NOOP(void *out, fio_argon2_args_s args) {
  if (!out)
    return -1;

  /* Validate and clamp parameters */
  uint32_t outlen = args.outlen ? args.outlen : 32;
  if (outlen < 4)
    outlen = 4;
  uint32_t t_cost = args.t_cost < 1 ? 1 : args.t_cost;
  uint32_t parallelism = args.parallelism < 1 ? 1 : args.parallelism;
  uint32_t m_cost = args.m_cost;
  uint32_t type = (uint32_t)args.type;
  if (type > 2)
    type = 2;

  /* Minimum memory: 8 * parallelism KiB */
  uint32_t min_mem = 8 * parallelism;
  if (m_cost < min_mem)
    m_cost = min_mem;

  /* m' = 4 * p * floor(m / (4*p)) */
  uint32_t segment_length = m_cost / (4 * parallelism);
  if (segment_length < 1)
    segment_length = 1;
  uint32_t q = segment_length * FIO___ARGON2_SYNC_POINTS; /* columns per lane */
  uint32_t m_prime = q * parallelism;                     /* total blocks */

  /* ======================== Step 1: Compute H_0 ========================== */
  /* H_0 = H^64(LE32(p) || LE32(T) || LE32(m) || LE32(t) || LE32(v) ||
   *        LE32(y) || LE32(len(P)) || P || LE32(len(S)) || S ||
   *        LE32(len(K)) || K || LE32(len(X)) || X) */
  uint8_t h0[64];
  {
    fio_blake2b_s bctx = fio_blake2b_init(64, NULL, 0);
    uint8_t le32buf[4];

    fio_u2buf32_le(le32buf, parallelism);
    fio_blake2b_consume(&bctx, le32buf, 4);
    fio_u2buf32_le(le32buf, outlen);
    fio_blake2b_consume(&bctx, le32buf, 4);
    fio_u2buf32_le(le32buf, m_cost);
    fio_blake2b_consume(&bctx, le32buf, 4);
    fio_u2buf32_le(le32buf, t_cost);
    fio_blake2b_consume(&bctx, le32buf, 4);
    fio_u2buf32_le(le32buf, FIO___ARGON2_VERSION);
    fio_blake2b_consume(&bctx, le32buf, 4);
    fio_u2buf32_le(le32buf, type);
    fio_blake2b_consume(&bctx, le32buf, 4);

    /* Password */
    uint32_t pwd_len = args.password.buf ? (uint32_t)args.password.len : 0;
    fio_u2buf32_le(le32buf, pwd_len);
    fio_blake2b_consume(&bctx, le32buf, 4);
    if (pwd_len)
      fio_blake2b_consume(&bctx, args.password.buf, pwd_len);

    /* Salt */
    uint32_t salt_len = args.salt.buf ? (uint32_t)args.salt.len : 0;
    fio_u2buf32_le(le32buf, salt_len);
    fio_blake2b_consume(&bctx, le32buf, 4);
    if (salt_len)
      fio_blake2b_consume(&bctx, args.salt.buf, salt_len);

    /* Secret */
    uint32_t secret_len = args.secret.buf ? (uint32_t)args.secret.len : 0;
    fio_u2buf32_le(le32buf, secret_len);
    fio_blake2b_consume(&bctx, le32buf, 4);
    if (secret_len)
      fio_blake2b_consume(&bctx, args.secret.buf, secret_len);

    /* Associated data */
    uint32_t ad_len = args.ad.buf ? (uint32_t)args.ad.len : 0;
    fio_u2buf32_le(le32buf, ad_len);
    fio_blake2b_consume(&bctx, le32buf, 4);
    if (ad_len)
      fio_blake2b_consume(&bctx, args.ad.buf, ad_len);

    fio_blake2b_finalize(&bctx, h0);
    fio_secure_zero(&bctx, sizeof(bctx));
  }

  /* ======================== Step 2: Allocate memory ====================== */
  size_t total_blocks = (size_t)m_prime;
  size_t mem_bytes = total_blocks * FIO___ARGON2_BLOCK_SIZE;
  uint64_t *memory = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, mem_bytes, 0);
  if (!memory)
    return -1;
  FIO_MEMSET(memory, 0, mem_bytes);

  /* Helper macro: pointer to block B[lane][col] */
#define FIO___ARGON2_BLOCK(lane, col)                                          \
  (memory +                                                                    \
   ((size_t)(lane) * (size_t)q + (size_t)(col)) * FIO___ARGON2_BLOCK_WORDS)

  /* ======================== Steps 3-4: Init first two columns ============ */
  {
    /* For each lane i: B[i][0] = H'^1024(H_0 || LE32(0) || LE32(i))
     *                  B[i][1] = H'^1024(H_0 || LE32(1) || LE32(i)) */
    uint8_t h0_input[72]; /* 64 + 4 + 4 */
    FIO_MEMCPY(h0_input, h0, 64);

    for (uint32_t i = 0; i < parallelism; ++i) {
      /* B[i][0] */
      fio_u2buf32_le(h0_input + 64, 0);
      fio_u2buf32_le(h0_input + 68, i);
      fio___argon2_hash_prime(FIO___ARGON2_BLOCK(i, 0), 1024, h0_input, 72);

      /* B[i][1] */
      fio_u2buf32_le(h0_input + 64, 1);
      fio_u2buf32_le(h0_input + 68, i);
      fio___argon2_hash_prime(FIO___ARGON2_BLOCK(i, 1), 1024, h0_input, 72);
    }
    fio_secure_zero(h0_input, sizeof(h0_input));
  }

  /* ======================== Step 5-6: Fill memory ======================== */
  /* Allocate address buffer for Argon2i/Argon2id */
  uint64_t *addr_block = NULL;
  size_t addr_block_size = 0;
  if (type == FIO_ARGON2I || type == FIO_ARGON2ID) {
    uint32_t num_addr_blocks = (segment_length + FIO___ARGON2_BLOCK_WORDS - 1) /
                               FIO___ARGON2_BLOCK_WORDS;
    addr_block_size = (size_t)num_addr_blocks * FIO___ARGON2_BLOCK_SIZE;
    addr_block = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, addr_block_size, 0);
    if (!addr_block) {
      fio_secure_zero(memory, mem_bytes);
      FIO_MEM_FREE(memory, mem_bytes);
      return -1;
    }
  }

  for (uint32_t pass = 0; pass < t_cost; ++pass) {
    for (uint32_t slice = 0; slice < FIO___ARGON2_SYNC_POINTS; ++slice) {
      for (uint32_t lane = 0; lane < parallelism; ++lane) {
        /* Determine if we need data-independent indexing for this segment */
        int use_independent = 0;
        if (type == FIO_ARGON2I)
          use_independent = 1;
        else if (type == FIO_ARGON2ID && pass == 0 && slice < 2)
          use_independent = 1;

        /* Generate address block for data-independent indexing */
        if (use_independent && addr_block) {
          fio___argon2_gen_addresses(addr_block,
                                     (uint64_t)pass,
                                     (uint64_t)lane,
                                     (uint64_t)slice,
                                     (uint64_t)m_prime,
                                     (uint64_t)t_cost,
                                     (uint64_t)type,
                                     segment_length);
        }

        uint32_t start_index = 0;
        if (pass == 0 && slice == 0)
          start_index = 2; /* First two blocks already computed */

        /* tmp_block hoisted outside inner loop to avoid repeated stack alloc */
        uint64_t tmp_block[FIO___ARGON2_BLOCK_WORDS];
        for (uint32_t idx = start_index; idx < segment_length; ++idx) {
          uint32_t cur_col = slice * segment_length + idx;
          uint32_t prev_col = (cur_col == 0) ? (q - 1) : (cur_col - 1);

          uint32_t j1, j2;
          if (use_independent && addr_block) {
            /* Data-independent: get J1, J2 from address block */
            uint64_t val = addr_block[idx];
            j1 = (uint32_t)(val & 0xFFFFFFFFULL);
            j2 = (uint32_t)(val >> 32);
          } else {
            /* Data-dependent: J1, J2 from previous block */
            j1 = (uint32_t)(FIO___ARGON2_BLOCK(lane, prev_col)[0] &
                            0xFFFFFFFFULL);
            j2 = (uint32_t)((FIO___ARGON2_BLOCK(lane, prev_col)[0] >> 32) &
                            0xFFFFFFFFULL);
          }

          uint32_t ref_lane, ref_index;
          fio___argon2_index(&ref_lane,
                             &ref_index,
                             j1,
                             j2,
                             lane,
                             pass,
                             slice,
                             idx,
                             parallelism,
                             segment_length,
                             q);

          uint64_t *cur_block = FIO___ARGON2_BLOCK(lane, cur_col);
          uint64_t *prev_block = FIO___ARGON2_BLOCK(lane, prev_col);
          uint64_t *ref_block = FIO___ARGON2_BLOCK(ref_lane, ref_index);

          if (pass == 0) {
            /* First pass: B[i][j] = G(B[i][j-1], B[l][z]) */
            fio___argon2_compress(cur_block, prev_block, ref_block);
          } else {
            /* Subsequent passes: B[i][j] = G(B[i][j-1], B[l][z]) XOR B[i][j]
             */
            fio___argon2_compress(tmp_block, prev_block, ref_block);
            for (size_t w = 0; w < FIO___ARGON2_BLOCK_WORDS; ++w)
              cur_block[w] ^= tmp_block[w];
          }
        }
        fio_secure_zero(tmp_block, sizeof(tmp_block));
      }
    }
  }

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

  /* Tag = H'^T(C) */
  fio___argon2_hash_prime(out, outlen, final_block, FIO___ARGON2_BLOCK_SIZE);

  /* ======================== Cleanup ====================================== */
  fio_secure_zero(final_block, sizeof(final_block));
  if (addr_block) {
    fio_secure_zero(addr_block, addr_block_size);
    FIO_MEM_FREE(addr_block, addr_block_size);
  }
  fio_secure_zero(memory, mem_bytes);
  FIO_MEM_FREE(memory, mem_bytes);
  fio_secure_zero(h0, sizeof(h0));

#undef FIO___ARGON2_BLOCK
  return 0;
}

void fio_argon2___(void); /* IDE Marker */
/** Computes Argon2 password hash, returning result as fio_u512. */
SFUNC fio_u512 fio_argon2 FIO_NOOP(fio_argon2_args_s args) {
  fio_u512 result = {0};
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
