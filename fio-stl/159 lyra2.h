/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_LYRA2              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    LYRA2
                        Memory-Hard Password Hashing


Reference: https://github.com/leocalm/Lyra (single-threaded, nPARALLEL==1)
Uses Blake2b as the underlying sponge permutation (SPONGE=0, RHO=1).

When comparing Lyra2 output hashes, use `fio_ct_is_eq` for constant-time
comparison to avoid timing side-channel attacks.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_LYRA2) && !defined(H___FIO_LYRA2___H)
#define H___FIO_LYRA2___H
/* *****************************************************************************
Lyra2 API
***************************************************************************** */

/** Lyra2 named arguments. */
typedef struct {
  /** The password to hash. */
  fio_buf_info_s password;
  /** The salt for the hash. */
  fio_buf_info_s salt;
  /** Time cost (number of rounds, minimum 1). */
  uint64_t t_cost;
  /** Memory cost (number of rows in the matrix, minimum 3). */
  uint64_t m_cost;
  /** Desired output length in bytes (default 32 if 0). */
  size_t outlen;
  /** Number of columns (default 256 if 0). */
  size_t n_cols;
} fio_lyra2_args_s;

/**
 * Computes Lyra2 password hash.
 *
 * Uses Blake2b as the underlying sponge/hash function.
 * Matches the reference C implementation (nPARALLEL==1, SPONGE==0, RHO==1).
 *
 * Output is written to the returned buffer (up to 512 bytes via fio_u512).
 * For outlen > 64, use fio_lyra2_hash() instead.
 *
 * Use `fio_ct_is_eq` to compare output hashes in constant time.
 */
SFUNC fio_u512 fio_lyra2(fio_lyra2_args_s args);
#define fio_lyra2(...) fio_lyra2((fio_lyra2_args_s){__VA_ARGS__})

/**
 * Computes Lyra2 password hash into a caller-provided buffer.
 * Supports arbitrary output lengths.
 * Returns 0 on success, -1 on error.
 */
SFUNC int fio_lyra2_hash(void *out, fio_lyra2_args_s args);
#define fio_lyra2_hash(out, ...)                                               \
  fio_lyra2_hash(out, (fio_lyra2_args_s){__VA_ARGS__})

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* Block length: 12 uint64_t = 96 bytes (rate portion of sponge) */
#define FIO___LYRA2_BLOCK_LEN_INT64 12
#define FIO___LYRA2_BLOCK_LEN_BYTES (FIO___LYRA2_BLOCK_LEN_INT64 * 8)

/* Blake2b-safe block: 8 uint64_t = 64 bytes (used for bootstrapping absorb) */
#define FIO___LYRA2_BLAKE2_SAFE_INT64 8
#define FIO___LYRA2_BLAKE2_SAFE_BYTES (FIO___LYRA2_BLAKE2_SAFE_INT64 * 8)

/* Row length in uint64_t words */
#define FIO___LYRA2_ROW_LEN_INT64(ncols)                                       \
  ((size_t)(ncols)*FIO___LYRA2_BLOCK_LEN_INT64)
#define FIO___LYRA2_ROW_LEN_BYTES(ncols) (FIO___LYRA2_ROW_LEN_INT64(ncols) * 8)

/* Row pointer by index (avoids indirection array) */
#define FIO___LYRA2_ROW(matrix, row, row_len)                                  \
  ((matrix) + (size_t)(row) * (size_t)(row_len))

/* *****************************************************************************
Blake2b permutation (full 12 rounds and reduced 1 round)
***************************************************************************** */

#define FIO___LYRA2_G(a, b, c, d)                                              \
  do {                                                                         \
    (a) += (b);                                                                \
    (d) = fio_rrot64((d) ^ (a), 32);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 24);                                           \
    (a) += (b);                                                                \
    (d) = fio_rrot64((d) ^ (a), 16);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 63);                                           \
  } while (0)

#define FIO___LYRA2_ROUND(v)                                                   \
  do {                                                                         \
    FIO___LYRA2_G((v)[0], (v)[4], (v)[8], (v)[12]);                            \
    FIO___LYRA2_G((v)[1], (v)[5], (v)[9], (v)[13]);                            \
    FIO___LYRA2_G((v)[2], (v)[6], (v)[10], (v)[14]);                           \
    FIO___LYRA2_G((v)[3], (v)[7], (v)[11], (v)[15]);                           \
    FIO___LYRA2_G((v)[0], (v)[5], (v)[10], (v)[15]);                           \
    FIO___LYRA2_G((v)[1], (v)[6], (v)[11], (v)[12]);                           \
    FIO___LYRA2_G((v)[2], (v)[7], (v)[8], (v)[13]);                            \
    FIO___LYRA2_G((v)[3], (v)[4], (v)[9], (v)[14]);                            \
  } while (0)

/** Full 12-round Blake2b permutation. */
FIO_IFUNC void fio___lyra2_sponge_full(uint64_t *v) {
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
  FIO___LYRA2_ROUND(v);
}

/** Reduced 1-round Blake2b permutation. */
FIO_IFUNC void fio___lyra2_sponge_reduced(uint64_t *v) { FIO___LYRA2_ROUND(v); }

/* *****************************************************************************
Sponge state initialization
***************************************************************************** */

FIO_IFUNC void fio___lyra2_init_state(uint64_t state[16]) {
  FIO_MEMSET(state, 0, 64); /* first 8 words = 0 */
  state[8] = 0x6a09e667f3bcc908ULL;
  state[9] = 0xbb67ae8584caa73bULL;
  state[10] = 0x3c6ef372fe94f82bULL;
  state[11] = 0xa54ff53a5f1d36f1ULL;
  state[12] = 0x510e527fade682d1ULL;
  state[13] = 0x9b05688c2b3e6c1fULL;
  state[14] = 0x1f83d9abfb41bd6bULL;
  state[15] = 0x5be0cd19137e2179ULL;
}

/* *****************************************************************************
Unrolled 12-word XOR/add/copy operations for sponge blocks
***************************************************************************** */

/** XOR 8 words of `in` into state (blake2b-safe absorb). */
FIO_IFUNC void fio___lyra2_xor8(uint64_t *state, const uint64_t *in) {
  state[0] ^= in[0];
  state[1] ^= in[1];
  state[2] ^= in[2];
  state[3] ^= in[3];
  state[4] ^= in[4];
  state[5] ^= in[5];
  state[6] ^= in[6];
  state[7] ^= in[7];
}

/** XOR 12 words of `in` into state. */
FIO_IFUNC void fio___lyra2_xor12(uint64_t *state, const uint64_t *in) {
  state[0] ^= in[0];
  state[1] ^= in[1];
  state[2] ^= in[2];
  state[3] ^= in[3];
  state[4] ^= in[4];
  state[5] ^= in[5];
  state[6] ^= in[6];
  state[7] ^= in[7];
  state[8] ^= in[8];
  state[9] ^= in[9];
  state[10] ^= in[10];
  state[11] ^= in[11];
}

/** Copy 12 words from state to dst. */
FIO_IFUNC void fio___lyra2_copy12(uint64_t *dst, const uint64_t *src) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];
  dst[4] = src[4];
  dst[5] = src[5];
  dst[6] = src[6];
  dst[7] = src[7];
  dst[8] = src[8];
  dst[9] = src[9];
  dst[10] = src[10];
  dst[11] = src[11];
}

/** XOR 12 words: dst[j] = a[j] ^ b[j]. */
FIO_IFUNC void fio___lyra2_xor12_into(uint64_t *dst,
                                      const uint64_t *a,
                                      const uint64_t *b) {
  dst[0] = a[0] ^ b[0];
  dst[1] = a[1] ^ b[1];
  dst[2] = a[2] ^ b[2];
  dst[3] = a[3] ^ b[3];
  dst[4] = a[4] ^ b[4];
  dst[5] = a[5] ^ b[5];
  dst[6] = a[6] ^ b[6];
  dst[7] = a[7] ^ b[7];
  dst[8] = a[8] ^ b[8];
  dst[9] = a[9] ^ b[9];
  dst[10] = a[10] ^ b[10];
  dst[11] = a[11] ^ b[11];
}

/** XOR wordwise-add of 3 blocks into state: state[j] ^= (a[j]+b[j]+c[j]). */
FIO_IFUNC void fio___lyra2_xor_add3(uint64_t *state,
                                    const uint64_t *a,
                                    const uint64_t *b,
                                    const uint64_t *c) {
  state[0] ^= (a[0] + b[0] + c[0]);
  state[1] ^= (a[1] + b[1] + c[1]);
  state[2] ^= (a[2] + b[2] + c[2]);
  state[3] ^= (a[3] + b[3] + c[3]);
  state[4] ^= (a[4] + b[4] + c[4]);
  state[5] ^= (a[5] + b[5] + c[5]);
  state[6] ^= (a[6] + b[6] + c[6]);
  state[7] ^= (a[7] + b[7] + c[7]);
  state[8] ^= (a[8] + b[8] + c[8]);
  state[9] ^= (a[9] + b[9] + c[9]);
  state[10] ^= (a[10] + b[10] + c[10]);
  state[11] ^= (a[11] + b[11] + c[11]);
}

/** XOR wordwise-add of 4 blocks into state. */
FIO_IFUNC void fio___lyra2_xor_add4(uint64_t *state,
                                    const uint64_t *a,
                                    const uint64_t *b,
                                    const uint64_t *c,
                                    const uint64_t *d) {
  state[0] ^= (a[0] + b[0] + c[0] + d[0]);
  state[1] ^= (a[1] + b[1] + c[1] + d[1]);
  state[2] ^= (a[2] + b[2] + c[2] + d[2]);
  state[3] ^= (a[3] + b[3] + c[3] + d[3]);
  state[4] ^= (a[4] + b[4] + c[4] + d[4]);
  state[5] ^= (a[5] + b[5] + c[5] + d[5]);
  state[6] ^= (a[6] + b[6] + c[6] + d[6]);
  state[7] ^= (a[7] + b[7] + c[7] + d[7]);
  state[8] ^= (a[8] + b[8] + c[8] + d[8]);
  state[9] ^= (a[9] + b[9] + c[9] + d[9]);
  state[10] ^= (a[10] + b[10] + c[10] + d[10]);
  state[11] ^= (a[11] + b[11] + c[11] + d[11]);
}

/** XOR state into dst: dst[j] ^= state[j]. */
FIO_IFUNC void fio___lyra2_xor12_self(uint64_t *dst, const uint64_t *state) {
  dst[0] ^= state[0];
  dst[1] ^= state[1];
  dst[2] ^= state[2];
  dst[3] ^= state[3];
  dst[4] ^= state[4];
  dst[5] ^= state[5];
  dst[6] ^= state[6];
  dst[7] ^= state[7];
  dst[8] ^= state[8];
  dst[9] ^= state[9];
  dst[10] ^= state[10];
  dst[11] ^= state[11];
}

/** XOR rotW(state) into dst: dst[j] ^= state[(j+2)%12] (no modulo). */
FIO_IFUNC void fio___lyra2_xor_rotw(uint64_t *dst, const uint64_t *state) {
  dst[0] ^= state[2];
  dst[1] ^= state[3];
  dst[2] ^= state[4];
  dst[3] ^= state[5];
  dst[4] ^= state[6];
  dst[5] ^= state[7];
  dst[6] ^= state[8];
  dst[7] ^= state[9];
  dst[8] ^= state[10];
  dst[9] ^= state[11];
  dst[10] ^= state[0];
  dst[11] ^= state[1];
}

/* *****************************************************************************
Absorb / Squeeze operations
***************************************************************************** */

/** Absorb a single 64-byte block (8 words) with full-round permutation. */
FIO_IFUNC void fio___lyra2_absorb_blake2_safe(uint64_t *state,
                                              const uint64_t *in) {
  fio___lyra2_xor8(state, in);
  fio___lyra2_sponge_full(state);
}

/** Absorb a single column (12 words) with full-round permutation. */
FIO_IFUNC void fio___lyra2_absorb_column(uint64_t *state, uint64_t *in) {
  fio___lyra2_xor12(state, in);
  fio___lyra2_sponge_full(state);
}

/** Squeeze output bytes from sponge state. */
FIO_SFUNC void fio___lyra2_squeeze(uint64_t *state,
                                   unsigned char *out,
                                   unsigned int len) {
  unsigned int full_blocks = len / FIO___LYRA2_BLOCK_LEN_BYTES;
  unsigned char *ptr = out;
  for (unsigned int i = 0; i < full_blocks; ++i) {
    FIO_MEMCPY(ptr, state, FIO___LYRA2_BLOCK_LEN_BYTES);
    fio___lyra2_sponge_full(state);
    ptr += FIO___LYRA2_BLOCK_LEN_BYTES;
  }
  unsigned int rem = len % FIO___LYRA2_BLOCK_LEN_BYTES;
  if (rem)
    FIO_MEMCPY(ptr, state, rem);
}

/* *****************************************************************************
Row initialization: reducedSqueezeRow0
Fills M[0] from column C-1 down to 0 using reduced permutation.
***************************************************************************** */

FIO_SFUNC void fio___lyra2_squeeze_row0(uint64_t *state,
                                        uint64_t *row_out,
                                        size_t n_cols) {
  uint64_t *ptr = row_out + (n_cols - 1) * FIO___LYRA2_BLOCK_LEN_INT64;
  for (size_t i = 0; i < n_cols; ++i) {
    fio___lyra2_copy12(ptr, state);
    ptr -= FIO___LYRA2_BLOCK_LEN_INT64;
    fio___lyra2_sponge_reduced(state);
  }
}

/* *****************************************************************************
Row initialization: reducedDuplexRow1and2
For each col i (0..C-1): absorb rowIn[i], reduced permute,
write rowOut[C-1-i] = rowIn[i] XOR state.
***************************************************************************** */

FIO_SFUNC void fio___lyra2_duplex_row1and2(uint64_t *state,
                                           uint64_t *row_in,
                                           uint64_t *row_out,
                                           size_t n_cols) {
  uint64_t *ptr_in = row_in;
  uint64_t *ptr_out = row_out + (n_cols - 1) * FIO___LYRA2_BLOCK_LEN_INT64;
  for (size_t i = 0; i < n_cols; ++i) {
    fio___lyra2_xor12(state, ptr_in);
    fio___lyra2_sponge_reduced(state);
    fio___lyra2_xor12_into(ptr_out, ptr_in, state);
    ptr_in += FIO___LYRA2_BLOCK_LEN_INT64;
    ptr_out -= FIO___LYRA2_BLOCK_LEN_INT64;
  }
}

/* *****************************************************************************
Setup filling: reducedDuplexRowFilling
Absorbs rowInOut[col] + rowIn0[col] + rowIn1[col] (wordwise add),
reduced permute, writes rowOut[C-1-col] = rowIn0[col] XOR state,
updates rowInOut[col] ^= rotW(state) where rotW rotates by 2 words.
***************************************************************************** */

FIO_SFUNC void fio___lyra2_duplex_row_filling(uint64_t *state,
                                              uint64_t *row_inout,
                                              uint64_t *row_in0,
                                              uint64_t *row_in1,
                                              uint64_t *row_out,
                                              size_t n_cols) {
  uint64_t *ptr_inout = row_inout;
  uint64_t *ptr_in0 = row_in0;
  uint64_t *ptr_in1 = row_in1;
  uint64_t *ptr_out = row_out + (n_cols - 1) * FIO___LYRA2_BLOCK_LEN_INT64;

  for (size_t i = 0; i < n_cols; ++i) {
    fio___lyra2_xor_add3(state, ptr_inout, ptr_in0, ptr_in1);
    fio___lyra2_sponge_reduced(state);
    fio___lyra2_xor12_into(ptr_out, ptr_in0, state);
    fio___lyra2_xor_rotw(ptr_inout, state);
    ptr_inout += FIO___LYRA2_BLOCK_LEN_INT64;
    ptr_in0 += FIO___LYRA2_BLOCK_LEN_INT64;
    ptr_in1 += FIO___LYRA2_BLOCK_LEN_INT64;
    ptr_out -= FIO___LYRA2_BLOCK_LEN_INT64;
  }
}

/* *****************************************************************************
Wandering: reducedDuplexRowWandering
For each col i (0..C-1):
  col0 = state[4] % n_cols, col1 = state[6] % n_cols
  absorb rowInOut0[i] + rowInOut1[i] + rowIn0[col0] + rowIn1[col1]
  reduced permute
  rowInOut0[i] ^= state
  rowInOut1[i] ^= rotW(state)
***************************************************************************** */

FIO_SFUNC void fio___lyra2_duplex_row_wandering(uint64_t *state,
                                                uint64_t *row_inout0,
                                                uint64_t *row_inout1,
                                                uint64_t *row_in0,
                                                uint64_t *row_in1,
                                                size_t n_cols) {
  uint64_t *ptr0 = row_inout0;
  uint64_t *ptr1 = row_inout1;

  for (size_t i = 0; i < n_cols; ++i) {
    uint64_t *p_in0 =
        row_in0 + (state[4] % n_cols) * FIO___LYRA2_BLOCK_LEN_INT64;
    uint64_t *p_in1 =
        row_in1 + (state[6] % n_cols) * FIO___LYRA2_BLOCK_LEN_INT64;

    fio___lyra2_xor_add4(state, ptr0, ptr1, p_in0, p_in1);
    fio___lyra2_sponge_reduced(state);
    fio___lyra2_xor12_self(ptr0, state);
    fio___lyra2_xor_rotw(ptr1, state);

    ptr0 += FIO___LYRA2_BLOCK_LEN_INT64;
    ptr1 += FIO___LYRA2_BLOCK_LEN_INT64;
  }
}

/* *****************************************************************************
Main Lyra2 function
***************************************************************************** */

/** Computes Lyra2 password hash into caller-provided buffer. */
SFUNC int fio_lyra2_hash FIO_NOOP(void *K, fio_lyra2_args_s args) {
  if (!K)
    return -1;

  /* Validate and clamp parameters */
  unsigned int kLen = (unsigned int)(args.outlen ? args.outlen : 32);
  unsigned int pwdlen =
      (unsigned int)(args.password.buf ? args.password.len : 0);
  unsigned int saltlen = (unsigned int)(args.salt.buf ? args.salt.len : 0);
  unsigned int timeCost = (unsigned int)(args.t_cost < 1 ? 1 : args.t_cost);
  unsigned int nRows = (unsigned int)(args.m_cost < 3 ? 3 : args.m_cost);
  unsigned int nCols = (unsigned int)(args.n_cols ? args.n_cols : 256);
  const void *pwd = args.password.buf;
  const void *salt = args.salt.buf;

  size_t row_len_int64 = FIO___LYRA2_ROW_LEN_INT64(nCols);
  size_t row_len_bytes = FIO___LYRA2_ROW_LEN_BYTES(nCols);

  /* Overflow check */
  if (nRows > (SIZE_MAX / row_len_bytes))
    return -1;

  /* Single allocation for entire matrix (no separate row pointer array) */
  size_t matrix_bytes = (size_t)nRows * row_len_bytes;
  uint64_t *wholeMatrix = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, matrix_bytes, 0);
  if (!wholeMatrix)
    return -1;

  /* ======================== Padding (pwd || salt || params) =============== */
  /* params = kLen || pwdlen || saltlen || timeCost || nRows || nCols
   * each as unsigned int (4 bytes) = 24 bytes total */
  uint64_t nBlocksInput =
      ((uint64_t)saltlen + (uint64_t)pwdlen + 6 * sizeof(unsigned int)) /
          FIO___LYRA2_BLAKE2_SAFE_BYTES +
      1;

  /* Write into beginning of wholeMatrix */
  unsigned char *ptrByte = (unsigned char *)wholeMatrix;
  FIO_MEMSET(ptrByte, 0, (size_t)nBlocksInput * FIO___LYRA2_BLAKE2_SAFE_BYTES);

  if (pwdlen)
    FIO_MEMCPY(ptrByte, pwd, pwdlen);
  ptrByte += pwdlen;
  if (saltlen)
    FIO_MEMCPY(ptrByte, salt, saltlen);
  ptrByte += saltlen;

  /* Params as unsigned int (4 bytes each, native byte order) */
  FIO_MEMCPY(ptrByte, &kLen, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);
  FIO_MEMCPY(ptrByte, &pwdlen, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);
  FIO_MEMCPY(ptrByte, &saltlen, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);
  FIO_MEMCPY(ptrByte, &timeCost, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);
  FIO_MEMCPY(ptrByte, &nRows, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);
  FIO_MEMCPY(ptrByte, &nCols, sizeof(unsigned int));
  ptrByte += sizeof(unsigned int);

  /* 10*1 padding */
  *ptrByte = 0x80;
  ptrByte = (unsigned char *)wholeMatrix;
  ptrByte += (size_t)nBlocksInput * FIO___LYRA2_BLAKE2_SAFE_BYTES - 1;
  *ptrByte ^= 0x01;

  /* ======================== Initialize Sponge State ====================== */
  uint64_t state[16];
  fio___lyra2_init_state(state);

  /* ======================== Absorb Input ================================= */
  {
    uint64_t *ptrWord = wholeMatrix;
    for (uint64_t i = 0; i < nBlocksInput; ++i) {
      fio___lyra2_absorb_blake2_safe(state, ptrWord);
      ptrWord += FIO___LYRA2_BLAKE2_SAFE_INT64;
    }
  }

  /* ======================== Setup Phase ================================== */
  /* M[0] */
  fio___lyra2_squeeze_row0(state,
                           FIO___LYRA2_ROW(wholeMatrix, 0, row_len_int64),
                           nCols);
  /* M[1] */
  fio___lyra2_duplex_row1and2(state,
                              FIO___LYRA2_ROW(wholeMatrix, 0, row_len_int64),
                              FIO___LYRA2_ROW(wholeMatrix, 1, row_len_int64),
                              nCols);
  /* M[2] */
  fio___lyra2_duplex_row1and2(state,
                              FIO___LYRA2_ROW(wholeMatrix, 1, row_len_int64),
                              FIO___LYRA2_ROW(wholeMatrix, 2, row_len_int64),
                              nCols);

  /* Filling loop */
  {
    int64_t gap = 1;
    uint64_t step = 1;
    uint64_t window = 2;
    uint64_t sqrtVal = 2;
    uint64_t row0 = 3;
    uint64_t prev0 = 2;
    uint64_t row1 = 1;
    uint64_t prev1 = 0;

    for (; row0 < nRows; ++row0) {
      fio___lyra2_duplex_row_filling(
          state,
          FIO___LYRA2_ROW(wholeMatrix, row1, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, prev0, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, prev1, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, row0, row_len_int64),
          nCols);
      prev0 = row0;
      prev1 = row1;
      row1 = (row1 + step) & (window - 1);
      if (row1 == 0) {
        window *= 2;
        step = sqrtVal + (uint64_t)gap;
        gap = -gap;
        if (gap == -1)
          sqrtVal *= 2;
      }
    }

    /* ======================== Wandering Phase ============================= */
    for (uint64_t i = 0; i < (uint64_t)timeCost * (uint64_t)nRows; ++i) {
      row0 = state[0] % nRows;
      row1 = state[2] % nRows;
      fio___lyra2_duplex_row_wandering(
          state,
          FIO___LYRA2_ROW(wholeMatrix, row0, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, row1, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, prev0, row_len_int64),
          FIO___LYRA2_ROW(wholeMatrix, prev1, row_len_int64),
          nCols);
      prev0 = row0;
      prev1 = row1;
    }

    /* ======================== Wrap-up Phase =============================== */
    fio___lyra2_absorb_column(
        state,
        FIO___LYRA2_ROW(wholeMatrix, row0, row_len_int64));
    fio___lyra2_squeeze(state, (unsigned char *)K, kLen);
  }

  /* ======================== Cleanup ====================================== */
  fio_secure_zero(wholeMatrix, matrix_bytes);
  FIO_MEM_FREE(wholeMatrix, matrix_bytes);
  fio_secure_zero(state, sizeof(state));

  return 0;
}

void fio_lyra2___(void); /* IDE Marker */
/** Computes Lyra2 password hash, returning result as fio_u512. */
SFUNC fio_u512 fio_lyra2 FIO_NOOP(fio_lyra2_args_s args) {
  fio_u512 result = {0};
  size_t outlen = args.outlen ? args.outlen : 32;
  if (outlen > 64)
    outlen = 64;
  args.outlen = outlen;
  (fio_lyra2_hash)((void *)result.u8, args);
  return result;
}

#undef FIO___LYRA2_G
#undef FIO___LYRA2_ROUND
#undef FIO___LYRA2_BLOCK_LEN_INT64
#undef FIO___LYRA2_BLOCK_LEN_BYTES
#undef FIO___LYRA2_BLAKE2_SAFE_INT64
#undef FIO___LYRA2_BLAKE2_SAFE_BYTES
#undef FIO___LYRA2_ROW_LEN_INT64
#undef FIO___LYRA2_ROW_LEN_BYTES
#undef FIO___LYRA2_ROW

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_LYRA2 */
