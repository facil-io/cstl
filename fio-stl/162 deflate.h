/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_DEFLATE            /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              DEFLATE / INFLATE
                    RFC 1951 Compression & Decompression


Provides raw DEFLATE compression/decompression plus gzip wrappers.
Designed for WebSocket permessage-deflate (RFC 7692) and static file
compression.

Key design decisions:
- 64-bit branchless bit buffer for ~2x throughput over zlib
- Two-level packed Huffman tables: 11-bit litlen, 8-bit distance (~12KB)
- Fast inner loop handles >95% of data without state machine overhead
- Word-at-a-time match copies with offset-specific fast paths

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_DEFLATE) && !defined(H___FIO_DEFLATE___H)
#define H___FIO_DEFLATE___H

/* *****************************************************************************
DEFLATE / INFLATE API
***************************************************************************** */

/**
 * Decompresses raw DEFLATE data (no zlib/gzip headers).
 *
 * Returns:
 *  - On success: decompressed byte count (<= out_len).
 *  - On buffer too small: the REQUIRED buffer size (> out_len).
 *  - On corrupt/invalid data: 0.
 *  - When out==NULL or out_len==0: performs a full decode pass counting
 *    output bytes and returns the required size (0 if data is corrupt).
 *    This allows callers to query the decompressed size without allocating.
 */
SFUNC size_t fio_deflate_decompress(void *out,
                                    size_t out_len,
                                    const void *in,
                                    size_t in_len);

/** Conservative upper bound on decompressed size. */
FIO_IFUNC size_t fio_deflate_decompress_bound(size_t in_len);

/**
 * Compresses data using raw DEFLATE (no zlib/gzip headers).
 *
 * Returns compressed length on success, 0 on error.
 * level: 0=store, 1-3=fast, 4-6=normal, 7-9=best compression.
 */
SFUNC size_t fio_deflate_compress(void *out,
                                  size_t out_len,
                                  const void *in,
                                  size_t in_len,
                                  int level);

/** Upper bound on compressed output size. */
FIO_IFUNC size_t fio_deflate_compress_bound(size_t in_len);

/**
 * Compresses data with gzip wrapper (for HTTP Content-Encoding).
 *
 * Returns total output length on success, 0 on error.
 */
SFUNC size_t fio_gzip_compress(void *out,
                               size_t out_len,
                               const void *in,
                               size_t in_len,
                               int level);

/**
 * Decompresses gzip data.
 *
 * Returns:
 *  - On success: decompressed byte count (<= out_len).
 *  - On buffer too small: the REQUIRED buffer size (> out_len).
 *  - On corrupt/invalid data: 0.
 *  - When out==NULL or out_len==0: returns required size (0 if corrupt).
 */
SFUNC size_t fio_gzip_decompress(void *out,
                                 size_t out_len,
                                 const void *in,
                                 size_t in_len);

/* *****************************************************************************
Streaming API (for WebSocket permessage-deflate context takeover)
***************************************************************************** */

/** Opaque streaming deflate/inflate state. */
typedef struct fio_deflate_s fio_deflate_s;

/**
 * Creates a new streaming deflate/inflate state.
 *
 * `level`:       compression level 1-9 (ignored for decompression).
 * `is_compress`: non-zero for compression, 0 for decompression.
 *
 * Returns NULL on allocation failure.
 */
SFUNC fio_deflate_s *fio_deflate_new(int level, int is_compress);

/** Frees a streaming deflate/inflate state. */
SFUNC void fio_deflate_free(fio_deflate_s *s);

/** Resets a deflate streaming context (keeps allocated memory, clears state).
 */
SFUNC void fio_deflate_destroy(fio_deflate_s *s);

/**
 * Streaming compress/decompress.
 *
 * Processes `in_len` bytes from `in`, writing output to `out` (max `out_len`).
 * `flush`: 0=normal, 1=sync_flush (for WebSocket frame boundaries).
 *
 * Decompression returns:
 * - Decompressed byte count on success (<= out_len).
 * - Required buffer size when buffer too small (> out_len). Internal buffer is
 *   preserved for retry with a larger output buffer.
 * - 0 on corrupt/invalid data or allocation failure.
 */
SFUNC size_t fio_deflate_push(fio_deflate_s *s,
                              void *out,
                              size_t out_len,
                              const void *in,
                              size_t in_len,
                              int flush);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Constants and static tables
***************************************************************************** */

/* Huffman table dimensions */
#define FIO___DEFLATE_LITLEN_BITS  11
#define FIO___DEFLATE_LITLEN_SIZE  (1U << FIO___DEFLATE_LITLEN_BITS)
#define FIO___DEFLATE_DIST_BITS    8
#define FIO___DEFLATE_DIST_SIZE    (1U << FIO___DEFLATE_DIST_BITS)
#define FIO___DEFLATE_PRECODE_BITS 7
#define FIO___DEFLATE_PRECODE_SIZE (1U << FIO___DEFLATE_PRECODE_BITS)

/* Max subtable entries (worst case) */
#define FIO___DEFLATE_LITLEN_MAX (FIO___DEFLATE_LITLEN_SIZE + 512)
#define FIO___DEFLATE_DIST_MAX   (FIO___DEFLATE_DIST_SIZE + 256)

/* Window size */
#define FIO___DEFLATE_WINDOW_SIZE 32768
#define FIO___DEFLATE_WINDOW_MASK (FIO___DEFLATE_WINDOW_SIZE - 1)

/* Max match length */
#define FIO___DEFLATE_MAX_MATCH 258

/* Compression hash table */
#define FIO___DEFLATE_HASH_BITS  15
#define FIO___DEFLATE_HASH_SIZE  (1U << FIO___DEFLATE_HASH_BITS)
#define FIO___DEFLATE_HASH_SHIFT 5

/*
 * Packed Huffman table entry format (32-bit):
 *
 * For literals (bit 31 clear):
 *   bits [0..7]   = code length (bits to consume)
 *   bits [8..15]  = symbol value (literal byte)
 *
 * For length codes (bit 31 set, bit 30 clear):
 *   bits [0..7]   = code length (bits to consume from code only)
 *   bits [8..12]  = extra bits count for length
 *   bits [13..30] = length base value
 *
 * For subtable pointers (bits 31 and 30 set):
 *   bits [0..7]   = root table bits consumed
 *   bits [8..11]  = subtable bits
 *   bits [12..30] = subtable offset from table start
 */
#define FIO___DEFLATE_ENTRY_LIT(sym, len)                                      \
  ((uint32_t)(len) | ((uint32_t)(sym) << 8))
#define FIO___DEFLATE_ENTRY_LEN(base, extra, codelen)                          \
  (0x80000000U | (uint32_t)(codelen) | ((uint32_t)(extra) << 8) |              \
   ((uint32_t)(base) << 13))
#define FIO___DEFLATE_ENTRY_DIST(base, extra, codelen)                         \
  ((uint32_t)(codelen) | ((uint32_t)(extra) << 8) | ((uint32_t)(base) << 13))
#define FIO___DEFLATE_ENTRY_SUB(offset, root_bits, sub_bits)                   \
  (0xC0000000U | (uint32_t)(root_bits) | ((uint32_t)(sub_bits) << 8) |         \
   ((uint32_t)(offset) << 12))

#define FIO___DEFLATE_ENTRY_CODELEN(e)    ((e)&0xFFU)
#define FIO___DEFLATE_ENTRY_IS_LIT(e)     (!((e)&0x80000000U))
#define FIO___DEFLATE_ENTRY_LIT_SYM(e)    (((e) >> 8) & 0xFFU)
#define FIO___DEFLATE_ENTRY_IS_LEN(e)     (((e)&0xC0000000U) == 0x80000000U)
#define FIO___DEFLATE_ENTRY_LEN_BASE(e)   (((e) >> 13) & 0x3FFFFU)
#define FIO___DEFLATE_ENTRY_LEN_EXTRA(e)  (((e) >> 8) & 0x1FU)
#define FIO___DEFLATE_ENTRY_IS_SUB(e)     (((e)&0xC0000000U) == 0xC0000000U)
#define FIO___DEFLATE_ENTRY_SUB_OFF(e)    (((e) >> 12) & 0x3FFFFU)
#define FIO___DEFLATE_ENTRY_SUB_BITS(e)   (((e) >> 8) & 0xFU)
#define FIO___DEFLATE_ENTRY_DIST_BASE(e)  (((e) >> 13) & 0x3FFFFU)
#define FIO___DEFLATE_ENTRY_DIST_EXTRA(e) (((e) >> 8) & 0x1FU)

/* Length base values (symbols 257-285, 29 entries) */
static const uint16_t fio___deflate_len_base[29] = {
    3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

/* Length extra bits (symbols 257-285, 29 entries) */
static const uint8_t fio___deflate_len_extra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

/* Distance base values (30 entries) */
static const uint16_t fio___deflate_dist_base[30] = {
    1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
    33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

/* Distance extra bits (30 entries) */
static const uint8_t fio___deflate_dist_extra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

/* Length-to-symbol-index lookup: 256 entries for lengths 3-258.
   Index by (length - 3). Value is the symbol index (0-28).
   Symbol = value + 257. Use value to index len_base[] and len_extra[]. */
static const uint8_t fio___deflate_len_to_idx[256] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9,  10, 10, 11, 11, 12, 12, 12,
    12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16,
    16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 28};

/* Code length alphabet order (RFC 1951) */
static const uint8_t fio___deflate_codelen_order[19] =
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* *****************************************************************************
Bit buffer operations (64-bit branchless)
***************************************************************************** */

typedef struct {
  uint64_t bits;
  uint32_t count; /* number of valid bits in accumulator */
} fio___deflate_bitbuf_s;

/** Branchless refill: loads up to 8 bytes, advances input pointer. */
FIO_IFUNC void fio___deflate_bitbuf_refill(fio___deflate_bitbuf_s *b,
                                           const uint8_t **in) {
  /* Load 8 bytes (unaligned) and shift into position */
  b->bits |= fio_buf2u64_le(*in) << b->count;
  *in += (63 - b->count) >> 3;
  b->count |= 56;
}

/** Peek at the lowest n bits without consuming. */
FIO_IFUNC uint32_t fio___deflate_bitbuf_peek(fio___deflate_bitbuf_s *b,
                                             uint32_t n) {
  return (uint32_t)(b->bits) & ((1U << n) - 1);
}

/** Consume n bits from the buffer. */
FIO_IFUNC void fio___deflate_bitbuf_consume(fio___deflate_bitbuf_s *b,
                                            uint32_t n) {
  b->bits >>= n;
  b->count -= n;
}

/** Read and consume n bits. */
FIO_IFUNC uint32_t fio___deflate_bitbuf_read(fio___deflate_bitbuf_s *b,
                                             uint32_t n) {
  uint32_t val = fio___deflate_bitbuf_peek(b, n);
  fio___deflate_bitbuf_consume(b, n);
  return val;
}

/* *****************************************************************************
Bit writer for compression
***************************************************************************** */

typedef struct {
  uint64_t bits;
  uint32_t count;
  uint8_t *out;
  uint8_t *out_end;
} fio___deflate_bitwriter_s;

FIO_IFUNC void fio___deflate_bitwriter_init(fio___deflate_bitwriter_s *w,
                                            void *out,
                                            size_t out_len) {
  w->bits = 0;
  w->count = 0;
  w->out = (uint8_t *)out;
  w->out_end = (uint8_t *)out + out_len;
}

FIO_IFUNC void fio___deflate_bitwriter_flush_bits(
    fio___deflate_bitwriter_s *w) {
  if (w->count >= 8 && w->out + 8 <= w->out_end) {
    fio_u2buf64_le(w->out, w->bits);
    uint32_t bytes = w->count >> 3;
    w->out += bytes;
    w->bits >>= (bytes << 3);
    w->count &= 7;
  }
}

FIO_IFUNC void fio___deflate_bitwriter_put(fio___deflate_bitwriter_s *w,
                                           uint32_t val,
                                           uint32_t nbits) {
  w->bits |= (uint64_t)val << w->count;
  w->count += nbits;
  fio___deflate_bitwriter_flush_bits(w);
}

/** Write pre-reversed Huffman code (codes are reversed at build time). */
FIO_IFUNC void fio___deflate_bitwriter_put_huff(fio___deflate_bitwriter_s *w,
                                                uint32_t code,
                                                uint32_t nbits) {
  fio___deflate_bitwriter_put(w, code, nbits);
}

FIO_IFUNC void fio___deflate_bitwriter_align(fio___deflate_bitwriter_s *w) {
  if (w->count & 7) {
    uint32_t pad = 8 - (w->count & 7);
    fio___deflate_bitwriter_put(w, 0, pad);
  }
}

FIO_IFUNC size_t fio___deflate_bitwriter_finish(fio___deflate_bitwriter_s *w,
                                                void *out_start) {
  /* Flush remaining bits */
  while (w->count > 0 && w->out < w->out_end) {
    *w->out++ = (uint8_t)(w->bits & 0xFF);
    w->bits >>= 8;
    w->count = (w->count >= 8) ? (w->count - 8) : 0;
  }
  return (size_t)(w->out - (uint8_t *)out_start);
}

/* *****************************************************************************
Huffman table building (for decompression)
***************************************************************************** */

/**
 * Build a Huffman decode table from code lengths.
 *
 * Returns the total number of table entries used, or 0 on error.
 * `table` must have room for at least `(1 << root_bits) + max_subtable`
 * entries. `root_bits` is the first-level lookup width.
 *
 * table_type: 0 = simple (precode), 1 = litlen, 2 = distance
 */
FIO_SFUNC uint32_t fio___deflate_build_decode_table(uint32_t *table,
                                                    const uint8_t *lens,
                                                    uint32_t num_syms,
                                                    uint32_t root_bits,
                                                    int table_type) {
  uint32_t count[16] = {0};
  uint32_t offsets[16];
  uint16_t sorted[288]; /* max symbols */
  uint32_t max_len = 0;

  /* Count code lengths */
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (lens[i] > 15)
      return 0;
    count[lens[i]]++;
    if (lens[i] > max_len)
      max_len = lens[i];
  }

  /* Handle special cases */
  if (count[0] == num_syms) {
    /* All zero lengths - invalid for required tables */
    return 0;
  }

  /* Check for over-subscribed or incomplete code */
  {
    int32_t left = 1;
    for (uint32_t i = 1; i <= max_len; ++i) {
      left <<= 1;
      left -= (int32_t)count[i];
      if (left < 0)
        return 0; /* over-subscribed */
    }
  }

  /* Build offsets for sorting */
  offsets[0] = 0;
  offsets[1] = 0;
  for (uint32_t i = 1; i < 15; ++i)
    offsets[i + 1] = offsets[i] + count[i];

  /* Sort symbols by code length, then by symbol value */
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (lens[i])
      sorted[offsets[lens[i]]++] = (uint16_t)i;
  }

  /* Fill the root table and subtables */
  uint32_t table_pos = 1U << root_bits;
  uint32_t sym_idx = 0;

  /* Initialize root table to zero */
  FIO_MEMSET(table, 0, sizeof(uint32_t) * table_pos);

  /* Process each code length */
  uint32_t code = 0;
  for (uint32_t len = 1; len <= max_len; ++len) {
    for (uint32_t c = 0; c < count[len]; ++c) {
      uint32_t sym = sorted[sym_idx++];
      uint32_t entry;

      /* Build the table entry based on table type */
      if (table_type == 1) {
        /* Litlen table */
        if (sym < 256) {
          entry = FIO___DEFLATE_ENTRY_LIT(sym, len);
        } else if (sym == 256) {
          entry = FIO___DEFLATE_ENTRY_LEN(0, 0, len);
        } else if (sym >= 257 && sym <= 285) {
          uint32_t idx = sym - 257;
          entry = FIO___DEFLATE_ENTRY_LEN(fio___deflate_len_base[idx],
                                          fio___deflate_len_extra[idx],
                                          len);
        } else {
          entry = FIO___DEFLATE_ENTRY_LIT(sym, len);
        }
      } else if (table_type == 2 && sym < 30) {
        /* Distance table */
        entry = FIO___DEFLATE_ENTRY_DIST(fio___deflate_dist_base[sym],
                                         fio___deflate_dist_extra[sym],
                                         len);
      } else {
        /* Simple / precode table: just symbol + length */
        entry = FIO___DEFLATE_ENTRY_LIT(sym, len);
      }

      if (len <= root_bits) {
        /* Fits in root table - replicate across all matching bit patterns */
        uint32_t rev_code = 0;
        for (uint32_t b = 0; b < len; ++b)
          rev_code |= ((code >> (len - 1 - b)) & 1) << b;

        uint32_t step = 1U << len;
        for (uint32_t j = rev_code; j < (1U << root_bits); j += step)
          table[j] = entry;
      } else {
        /* Needs subtable */
        uint32_t rev_code = 0;
        for (uint32_t b = 0; b < len; ++b)
          rev_code |= ((code >> (len - 1 - b)) & 1) << b;

        /* Find or create subtable */
        uint32_t root_idx = rev_code & ((1U << root_bits) - 1);
        uint32_t sub_bits;
        uint32_t sub_off;

        if (!FIO___DEFLATE_ENTRY_IS_SUB(table[root_idx])) {
          /* Create new subtable */
          sub_bits = max_len - root_bits;
          if (sub_bits > 8)
            sub_bits = 8; /* cap subtable size */
          sub_off = table_pos;
          table[root_idx] =
              FIO___DEFLATE_ENTRY_SUB(sub_off, root_bits, sub_bits);
          /* Zero the subtable */
          FIO_MEMSET(table + sub_off, 0, sizeof(uint32_t) * (1U << sub_bits));
          table_pos += (1U << sub_bits);
        } else {
          sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(table[root_idx]);
          sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(table[root_idx]);
        }

        /* Fill subtable entry */
        uint32_t sub_code = (rev_code >> root_bits);
        uint32_t sub_len = len - root_bits;
        /* Adjust entry code length to be the subtable portion */
        entry = (entry & ~0xFFU) | sub_len;
        uint32_t sub_step = 1U << sub_len;
        for (uint32_t j = sub_code; j < (1U << sub_bits); j += sub_step)
          table[sub_off + j] = entry;
      }

      code++;
    }
    code <<= 1;
  }

  return table_pos;
}

/* *****************************************************************************
Pre-built fixed Huffman tables
***************************************************************************** */

/* Build fixed Huffman code lengths per RFC 1951 */
FIO_SFUNC void fio___deflate_fixed_litlen_lens(uint8_t lens[288]) {
  uint32_t i;
  for (i = 0; i <= 143; ++i)
    lens[i] = 8;
  for (; i <= 255; ++i)
    lens[i] = 9;
  for (; i <= 279; ++i)
    lens[i] = 7;
  for (; i <= 287; ++i)
    lens[i] = 8;
}

FIO_SFUNC void fio___deflate_fixed_dist_lens(uint8_t lens[32]) {
  for (uint32_t i = 0; i < 32; ++i)
    lens[i] = 5;
}

/* *****************************************************************************
Inflate (decompression) - fast inner loop
***************************************************************************** */

/**
 * Fast inflate inner loop. Processes data when sufficient input and output
 * space is available (at least 8 bytes input margin, 258+8 bytes output
 * margin).
 *
 * Returns 0 on success, -1 on error.
 * Updates all pointers through the state struct.
 */
FIO_SFUNC int fio___inflate_fast(const uint8_t *restrict *in_p,
                                 const uint8_t *in_safe_end,
                                 uint8_t *restrict *out_p,
                                 uint8_t *out_safe_end,
                                 uint8_t *out_start,
                                 fio___deflate_bitbuf_s *bb,
                                 const uint32_t *litlen_table,
                                 const uint32_t *dist_table) {
  const uint8_t *in = *in_p;
  uint8_t *out = *out_p;

  while (in < in_safe_end && out < out_safe_end) {
    /* Refill bit buffer */
    fio___deflate_bitbuf_refill(bb, &in);

    /* Decode literal/length */
    uint32_t bits = fio___deflate_bitbuf_peek(bb, FIO___DEFLATE_LITLEN_BITS);
    uint32_t entry = litlen_table[bits];

    /* Handle subtable */
    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = litlen_table[sub_off + bits];
    }

    if (FIO___DEFLATE_ENTRY_IS_LIT(entry)) {
      /* Literal byte - most common case */
      *out++ = (uint8_t)FIO___DEFLATE_ENTRY_LIT_SYM(entry);
      fio___deflate_bitbuf_consume(bb, FIO___DEFLATE_ENTRY_CODELEN(entry));
      continue;
    }

    /* Length/end-of-block code */
    uint32_t codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    fio___deflate_bitbuf_consume(bb, codelen);

    uint32_t length = FIO___DEFLATE_ENTRY_LEN_BASE(entry);
    uint32_t len_extra = FIO___DEFLATE_ENTRY_LEN_EXTRA(entry);

    /* End of block (base == 0 && extra == 0 means symbol 256) */
    if (length == 0 && len_extra == 0) {
      *in_p = in;
      *out_p = out;
      return 1; /* end of block signal */
    }

    /* Read length extra bits */
    if (len_extra)
      length += fio___deflate_bitbuf_read(bb, len_extra);

    /* Refill for distance */
    fio___deflate_bitbuf_refill(bb, &in);

    /* Decode distance */
    bits = fio___deflate_bitbuf_peek(bb, FIO___DEFLATE_DIST_BITS);
    entry = dist_table[bits];

    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = dist_table[sub_off + bits];
    }

    uint32_t dist_codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    fio___deflate_bitbuf_consume(bb, dist_codelen);

    uint32_t distance = FIO___DEFLATE_ENTRY_DIST_BASE(entry);
    uint32_t dist_extra = FIO___DEFLATE_ENTRY_DIST_EXTRA(entry);
    if (dist_extra)
      distance += fio___deflate_bitbuf_read(bb, dist_extra);

    /* Validate distance */
    if (distance == 0 || distance > (size_t)(out - out_start))
      return -1;

    /* Copy match - word-at-a-time with offset-specific fast paths */
    const uint8_t *src = out - distance;

    if (distance >= 8) {
      /* Non-overlapping or minimally overlapping: 8-byte copy loop */
      while (length >= 8) {
        FIO_MEMCPY(out, src, 8);
        out += 8;
        src += 8;
        length -= 8;
      }
      while (length > 0) {
        *out++ = *src++;
        --length;
      }
    } else if (distance == 1) {
      /* RLE: broadcast single byte */
      FIO_MEMSET(out, *src, length);
      out += length;
    } else {
      /* Small distance (2-7): byte-by-byte to handle overlap correctly */
      for (uint32_t i = 0; i < length; ++i)
        out[i] = src[i];
      out += length;
    }
  }

  *in_p = in;
  *out_p = out;
  return 0; /* need more data or output space */
}

/* *****************************************************************************
Inflate - slow path (byte-at-a-time for boundaries)
***************************************************************************** */

FIO_SFUNC int fio___inflate_slow(const uint8_t *restrict *in_p,
                                 const uint8_t *in_end,
                                 uint8_t *restrict *out_p,
                                 uint8_t *out_end,
                                 uint8_t *out_start,
                                 fio___deflate_bitbuf_s *bb,
                                 const uint32_t *litlen_table,
                                 const uint32_t *dist_table) {
  const uint8_t *in = *in_p;
  uint8_t *out = *out_p;

  for (;;) {
    /* Careful refill - byte at a time */
    while (bb->count < 15 && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < 1)
      break;

    /* Decode literal/length */
    uint32_t bits = fio___deflate_bitbuf_peek(
        bb,
        FIO___DEFLATE_LITLEN_BITS < bb->count ? FIO___DEFLATE_LITLEN_BITS
                                              : bb->count);
    uint32_t entry = litlen_table[bits & (FIO___DEFLATE_LITLEN_SIZE - 1)];

    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      if (bb->count < root_len)
        break;
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      /* Refill for subtable */
      while (bb->count < sub_bits && in < in_end) {
        bb->bits |= (uint64_t)(*in++) << bb->count;
        bb->count += 8;
      }
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = litlen_table[sub_off + bits];
    }

    uint32_t codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    if (bb->count < codelen)
      break;

    if (FIO___DEFLATE_ENTRY_IS_LIT(entry)) {
      if (out >= out_end) {
        /* Output buffer full — return -2 (overflow, not corrupt) */
        *in_p = in;
        *out_p = out;
        return -2;
      }
      *out++ = (uint8_t)FIO___DEFLATE_ENTRY_LIT_SYM(entry);
      fio___deflate_bitbuf_consume(bb, codelen);
      continue;
    }

    fio___deflate_bitbuf_consume(bb, codelen);

    uint32_t length = FIO___DEFLATE_ENTRY_LEN_BASE(entry);
    uint32_t len_extra = FIO___DEFLATE_ENTRY_LEN_EXTRA(entry);

    /* End of block */
    if (length == 0 && len_extra == 0) {
      *in_p = in;
      *out_p = out;
      return 1;
    }

    /* Read length extra bits */
    while (bb->count < len_extra && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < len_extra)
      break;
    if (len_extra)
      length += fio___deflate_bitbuf_read(bb, len_extra);

    /* Refill for distance */
    while (bb->count < 15 && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }

    /* Decode distance */
    bits = fio___deflate_bitbuf_peek(bb,
                                     FIO___DEFLATE_DIST_BITS < bb->count
                                         ? FIO___DEFLATE_DIST_BITS
                                         : bb->count);
    entry = dist_table[bits & (FIO___DEFLATE_DIST_SIZE - 1)];

    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      if (bb->count < root_len)
        break;
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      while (bb->count < sub_bits && in < in_end) {
        bb->bits |= (uint64_t)(*in++) << bb->count;
        bb->count += 8;
      }
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = dist_table[sub_off + bits];
    }

    uint32_t dist_codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    if (bb->count < dist_codelen)
      break;
    fio___deflate_bitbuf_consume(bb, dist_codelen);

    uint32_t distance = FIO___DEFLATE_ENTRY_DIST_BASE(entry);
    uint32_t dist_extra = FIO___DEFLATE_ENTRY_DIST_EXTRA(entry);

    while (bb->count < dist_extra && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < dist_extra)
      break;
    if (dist_extra)
      distance += fio___deflate_bitbuf_read(bb, dist_extra);

    if (distance == 0 || distance > (size_t)(out - out_start))
      return -1;
    if (out + length > out_end) {
      /* Output buffer overflow — return -2 (not corrupt) */
      *in_p = in;
      *out_p = out;
      return -2;
    }

    /* Byte-by-byte copy for slow path */
    const uint8_t *src = out - distance;
    for (uint32_t i = 0; i < length; ++i)
      out[i] = src[i];
    out += length;
  }

  *in_p = in;
  *out_p = out;
  return 0;
}

/* *****************************************************************************
Inflate - counting-only path (no output writes, just counts bytes)

Used when out==NULL (size query) or after buffer overflow (to determine
the total required size). Decodes the bitstream identically to the slow
path but only increments a virtual output position.

Returns:
  1  = end of block (out_pos updated with bytes decoded)
  -1 = corrupt data
  0  = need more input bits
***************************************************************************** */

FIO_SFUNC int fio___inflate_count(const uint8_t *restrict *in_p,
                                  const uint8_t *in_end,
                                  size_t *out_pos,
                                  size_t out_start_pos,
                                  fio___deflate_bitbuf_s *bb,
                                  const uint32_t *litlen_table,
                                  const uint32_t *dist_table) {
  const uint8_t *in = *in_p;
  size_t pos = *out_pos;

  for (;;) {
    /* Careful refill - byte at a time */
    while (bb->count < 15 && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < 1)
      break;

    /* Decode literal/length */
    uint32_t bits = fio___deflate_bitbuf_peek(
        bb,
        FIO___DEFLATE_LITLEN_BITS < bb->count ? FIO___DEFLATE_LITLEN_BITS
                                              : bb->count);
    uint32_t entry = litlen_table[bits & (FIO___DEFLATE_LITLEN_SIZE - 1)];

    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      if (bb->count < root_len)
        break;
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      while (bb->count < sub_bits && in < in_end) {
        bb->bits |= (uint64_t)(*in++) << bb->count;
        bb->count += 8;
      }
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = litlen_table[sub_off + bits];
    }

    uint32_t codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    if (bb->count < codelen)
      break;

    if (FIO___DEFLATE_ENTRY_IS_LIT(entry)) {
      pos++; /* count literal byte without writing */
      fio___deflate_bitbuf_consume(bb, codelen);
      continue;
    }

    fio___deflate_bitbuf_consume(bb, codelen);

    uint32_t length = FIO___DEFLATE_ENTRY_LEN_BASE(entry);
    uint32_t len_extra = FIO___DEFLATE_ENTRY_LEN_EXTRA(entry);

    /* End of block */
    if (length == 0 && len_extra == 0) {
      *in_p = in;
      *out_pos = pos;
      return 1;
    }

    /* Read length extra bits */
    while (bb->count < len_extra && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < len_extra)
      break;
    if (len_extra)
      length += fio___deflate_bitbuf_read(bb, len_extra);

    /* Refill for distance */
    while (bb->count < 15 && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }

    /* Decode distance */
    bits = fio___deflate_bitbuf_peek(bb,
                                     FIO___DEFLATE_DIST_BITS < bb->count
                                         ? FIO___DEFLATE_DIST_BITS
                                         : bb->count);
    entry = dist_table[bits & (FIO___DEFLATE_DIST_SIZE - 1)];

    if (FIO___DEFLATE_ENTRY_IS_SUB(entry)) {
      uint32_t root_len = FIO___DEFLATE_ENTRY_CODELEN(entry);
      if (bb->count < root_len)
        break;
      fio___deflate_bitbuf_consume(bb, root_len);
      uint32_t sub_off = FIO___DEFLATE_ENTRY_SUB_OFF(entry);
      uint32_t sub_bits = FIO___DEFLATE_ENTRY_SUB_BITS(entry);
      while (bb->count < sub_bits && in < in_end) {
        bb->bits |= (uint64_t)(*in++) << bb->count;
        bb->count += 8;
      }
      bits = fio___deflate_bitbuf_peek(bb, sub_bits);
      entry = dist_table[sub_off + bits];
    }

    uint32_t dist_codelen = FIO___DEFLATE_ENTRY_CODELEN(entry);
    if (bb->count < dist_codelen)
      break;
    fio___deflate_bitbuf_consume(bb, dist_codelen);

    uint32_t distance = FIO___DEFLATE_ENTRY_DIST_BASE(entry);
    uint32_t dist_extra = FIO___DEFLATE_ENTRY_DIST_EXTRA(entry);

    while (bb->count < dist_extra && in < in_end) {
      bb->bits |= (uint64_t)(*in++) << bb->count;
      bb->count += 8;
    }
    if (bb->count < dist_extra)
      break;
    if (dist_extra)
      distance += fio___deflate_bitbuf_read(bb, dist_extra);

    if (distance == 0 || distance > (pos - out_start_pos))
      return -1;

    /* Count match bytes without writing */
    pos += length;
  }

  *in_p = in;
  *out_pos = pos;
  return 0;
}

/* *****************************************************************************
Inflate - main entry point
***************************************************************************** */

/** Upper bound on decompressed size (conservative: 1032x expansion). */
FIO_IFUNC size_t fio_deflate_decompress_bound(size_t in_len) {
  /* DEFLATE theoretical max is 1032:1 but practical is much less.
   * Use a generous bound. If input is very small, use a minimum. */
  size_t bound = in_len * 1032;
  if (bound < 4096)
    bound = 4096;
  if (bound / 1032 != in_len) /* overflow check */
    bound = (size_t)-1;
  return bound;
}

void fio_deflate_decompress___(void); /* IDE Marker */
SFUNC size_t fio_deflate_decompress FIO_NOOP(void *out,
                                             size_t out_len,
                                             const void *in,
                                             size_t in_len) {
  if (!in || !in_len)
    return 0;

  /* Counting mode: out==NULL or out_len==0 — decode without writing.
   * Normal mode: decode into buffer; on overflow switch to counting. */
  int counting = (!out || !out_len);

  const uint8_t *inp = (const uint8_t *)in;
  const uint8_t *in_end = inp + in_len;
  uint8_t *outp = counting ? NULL : (uint8_t *)out;
  uint8_t *out_end = counting ? NULL : (outp + out_len);
  uint8_t *out_start = outp;
  size_t out_pos = 0; /* virtual position for counting mode */

  fio___deflate_bitbuf_s bb = {0, 0};

  /* Huffman decode tables (on stack, ~12KB) */
  uint32_t litlen_table[FIO___DEFLATE_LITLEN_MAX];
  uint32_t dist_table[FIO___DEFLATE_DIST_MAX];

  uint32_t bfinal = 0;

  while (!bfinal) {
    /* Read block header: BFINAL (1 bit) + BTYPE (2 bits) */
    while (bb.count < 3 && inp < in_end) {
      bb.bits |= (uint64_t)(*inp++) << bb.count;
      bb.count += 8;
    }
    if (bb.count < 3)
      return 0; /* error: truncated */

    bfinal = fio___deflate_bitbuf_read(&bb, 1);
    uint32_t btype = fio___deflate_bitbuf_read(&bb, 2);

    if (btype == 0) {
      /* Stored block */
      /* Discard remaining bits in current byte */
      bb.bits >>= (bb.count & 7);
      bb.count &= ~7U;

      /* Need 4 bytes: LEN + NLEN */
      while (bb.count < 32 && inp < in_end) {
        bb.bits |= (uint64_t)(*inp++) << bb.count;
        bb.count += 8;
      }
      if (bb.count < 32)
        return 0;

      uint32_t len = fio___deflate_bitbuf_read(&bb, 16);
      uint32_t nlen = fio___deflate_bitbuf_read(&bb, 16);

      if ((len ^ nlen) != 0xFFFF)
        return 0; /* error: LEN/NLEN mismatch */

      /* Any remaining bits in the buffer need to be "unread" back to input */
      uint32_t rewind_bytes = bb.count >> 3;
      inp -= rewind_bytes;
      bb.bits = 0;
      bb.count = 0;

      if (inp + len > in_end)
        return 0; /* truncated input — corrupt */

      if (counting) {
        /* Just count bytes, skip input */
        out_pos += len;
        inp += len;
      } else if (outp + len > out_end) {
        /* Buffer overflow — switch to counting mode */
        out_pos = (size_t)(outp - out_start) + len;
        counting = 1;
        inp += len;
      } else {
        FIO_MEMCPY(outp, inp, len);
        inp += len;
        outp += len;
      }
      continue;
    }

    if (btype == 3)
      return 0; /* reserved block type */

    if (btype == 1) {
      /* Fixed Huffman codes */
      uint8_t ll_lens[288];
      uint8_t d_lens[32];
      fio___deflate_fixed_litlen_lens(ll_lens);
      fio___deflate_fixed_dist_lens(d_lens);

      if (!fio___deflate_build_decode_table(litlen_table,
                                            ll_lens,
                                            288,
                                            FIO___DEFLATE_LITLEN_BITS,
                                            1))
        return 0;
      if (!fio___deflate_build_decode_table(dist_table,
                                            d_lens,
                                            32,
                                            FIO___DEFLATE_DIST_BITS,
                                            2))
        return 0;
    } else {
      /* Dynamic Huffman codes (btype == 2) */
      /* Read header: HLIT, HDIST, HCLEN */
      while (bb.count < 14 && inp < in_end) {
        bb.bits |= (uint64_t)(*inp++) << bb.count;
        bb.count += 8;
      }
      if (bb.count < 14)
        return 0;

      uint32_t hlit = fio___deflate_bitbuf_read(&bb, 5) + 257;
      uint32_t hdist = fio___deflate_bitbuf_read(&bb, 5) + 1;
      uint32_t hclen = fio___deflate_bitbuf_read(&bb, 4) + 4;

      if (hlit > 286 || hdist > 30)
        return 0;

      /* Read code length code lengths */
      uint8_t cl_lens[19];
      FIO_MEMSET(cl_lens, 0, sizeof(cl_lens));

      for (uint32_t i = 0; i < hclen; ++i) {
        while (bb.count < 3 && inp < in_end) {
          bb.bits |= (uint64_t)(*inp++) << bb.count;
          bb.count += 8;
        }
        if (bb.count < 3)
          return 0;
        cl_lens[fio___deflate_codelen_order[i]] =
            (uint8_t)fio___deflate_bitbuf_read(&bb, 3);
      }

      /* Build precode table */
      uint32_t precode_table[FIO___DEFLATE_PRECODE_SIZE];
      if (!fio___deflate_build_decode_table(precode_table,
                                            cl_lens,
                                            19,
                                            FIO___DEFLATE_PRECODE_BITS,
                                            0))
        return 0;

      /* Decode literal/length + distance code lengths */
      uint8_t all_lens[286 + 30];
      FIO_MEMSET(all_lens, 0, sizeof(all_lens));
      uint32_t total_codes = hlit + hdist;
      uint32_t idx = 0;

      while (idx < total_codes) {
        while (bb.count < 7 + 7 && inp < in_end) {
          bb.bits |= (uint64_t)(*inp++) << bb.count;
          bb.count += 8;
        }

        uint32_t pbits = fio___deflate_bitbuf_peek(
            &bb,
            FIO___DEFLATE_PRECODE_BITS < bb.count ? FIO___DEFLATE_PRECODE_BITS
                                                  : bb.count);
        uint32_t pentry =
            precode_table[pbits & (FIO___DEFLATE_PRECODE_SIZE - 1)];
        uint32_t plen = FIO___DEFLATE_ENTRY_CODELEN(pentry);
        uint32_t psym = FIO___DEFLATE_ENTRY_LIT_SYM(pentry);

        if (bb.count < plen)
          return 0;
        fio___deflate_bitbuf_consume(&bb, plen);

        if (psym < 16) {
          all_lens[idx++] = (uint8_t)psym;
        } else if (psym == 16) {
          /* Copy previous 3-6 times */
          if (bb.count < 2)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 2) + 3;
          if (idx == 0 || idx + rep > total_codes)
            return 0;
          uint8_t prev = all_lens[idx - 1];
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = prev;
        } else if (psym == 17) {
          /* Repeat zero 3-10 times */
          if (bb.count < 3)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 3) + 3;
          if (idx + rep > total_codes)
            return 0;
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = 0;
        } else if (psym == 18) {
          /* Repeat zero 11-138 times */
          if (bb.count < 7)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 7) + 11;
          if (idx + rep > total_codes)
            return 0;
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = 0;
        } else {
          return 0;
        }
      }

      /* Build litlen and distance tables */
      if (!fio___deflate_build_decode_table(litlen_table,
                                            all_lens,
                                            hlit,
                                            FIO___DEFLATE_LITLEN_BITS,
                                            1))
        return 0;
      if (!fio___deflate_build_decode_table(dist_table,
                                            all_lens + hlit,
                                            hdist,
                                            FIO___DEFLATE_DIST_BITS,
                                            2))
        return 0;
    }

    /* Decode symbols */
    if (counting) {
      /* Counting mode: use counting-only path (no output writes) */
      for (;;) {
        int rc = fio___inflate_count(&inp,
                                     in_end,
                                     &out_pos,
                                     0,
                                     &bb,
                                     litlen_table,
                                     dist_table);
        if (rc == 1)
          break; /* end of block */
        if (rc < 0)
          return 0; /* corrupt */
        return 0;   /* need more data — error for one-shot */
      }
    } else {
      /* Normal mode: fast path when possible, slow path at boundaries */
      int overflow = 0;
      for (;;) {
        const uint8_t *in_safe = in_end - 8;
        uint8_t *out_safe = out_end - (FIO___DEFLATE_MAX_MATCH + 8);

        int rc;
        if (inp < in_safe && outp < out_safe) {
          rc = fio___inflate_fast(&inp,
                                  in_safe,
                                  &outp,
                                  out_safe,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
        } else {
          rc = fio___inflate_slow(&inp,
                                  in_end,
                                  &outp,
                                  out_end,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
        }

        if (rc == 1)
          break; /* end of block */
        if (rc == -1)
          return 0; /* corrupt data */
        if (rc == -2) {
          /* Buffer overflow — switch to counting mode for remainder */
          out_pos = (size_t)(outp - out_start);
          counting = 1;
          overflow = 1;
          break;
        }

        /* If fast path ran out of margin, try slow path for remaining */
        if (inp >= in_safe || outp >= out_safe) {
          rc = fio___inflate_slow(&inp,
                                  in_end,
                                  &outp,
                                  out_end,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
          if (rc == 1)
            break;
          if (rc == -1)
            return 0; /* corrupt */
          if (rc == -2) {
            out_pos = (size_t)(outp - out_start);
            counting = 1;
            overflow = 1;
            break;
          }
          /* slow path returned 0 — need more data, error for one-shot */
          return 0;
        }
      }
      /* If we switched to counting mid-block, finish this block in count mode
       */
      if (overflow) {
        for (;;) {
          int rc = fio___inflate_count(&inp,
                                       in_end,
                                       &out_pos,
                                       0,
                                       &bb,
                                       litlen_table,
                                       dist_table);
          if (rc == 1)
            break;
          if (rc < 0)
            return 0;
          return 0;
        }
      }
    }
  }

  if (counting)
    return out_pos;
  return (size_t)(outp - out_start);
}

/* *****************************************************************************
Deflate (compression) - Huffman tree building
***************************************************************************** */

/** Build canonical Huffman code lengths from frequency counts.
 *  Uses a simplified package-merge / length-limited approach.
 *  max_bits: maximum code length (15 for DEFLATE).
 *  Returns 0 on success. */
FIO_SFUNC int fio___deflate_build_code_lengths(uint8_t *lens,
                                               const uint32_t *freqs,
                                               uint32_t num_syms,
                                               uint32_t max_bits) {
  /* Simple approach: build a Huffman tree using a priority queue,
   * then limit depths to max_bits using the Kraft inequality. */

  /* Count non-zero symbols */
  uint32_t num_used = 0;
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (freqs[i])
      num_used++;
  }

  FIO_MEMSET(lens, 0, num_syms);

  if (num_used == 0)
    return 0;
  if (num_used == 1) {
    /* Single symbol: assign length 1 */
    for (uint32_t i = 0; i < num_syms; ++i) {
      if (freqs[i]) {
        lens[i] = 1;
        return 0;
      }
    }
  }

  /* Build Huffman tree using a simple O(n log n) approach.
   * We use a flat array as a min-heap. */
  typedef struct {
    uint32_t freq;
    int32_t sym; /* -1 for internal nodes */
    int32_t left, right;
  } fio___huff_node_s;

  fio___huff_node_s nodes[572]; /* max 286 leaves + 285 internal */
  uint32_t heap[572];
  uint32_t heap_size = 0;
  uint32_t node_count = 0;

/* Min-heap operations */
#define FIO___HUFF_PARENT(i) (((i)-1) >> 1)
#define FIO___HUFF_LEFT(i)   (((i) << 1) + 1)
#define FIO___HUFF_RIGHT(i)  (((i) << 1) + 2)

#define FIO___HUFF_SWAP(a, b)                                                  \
  do {                                                                         \
    uint32_t t_ = heap[a];                                                     \
    heap[a] = heap[b];                                                         \
    heap[b] = t_;                                                              \
  } while (0)

#define FIO___HUFF_LESS(a, b)                                                  \
  (nodes[heap[a]].freq < nodes[heap[b]].freq ||                                \
   (nodes[heap[a]].freq == nodes[heap[b]].freq && heap[a] < heap[b]))

  /* Sift up */
#define FIO___HUFF_SIFT_UP(pos)                                                \
  do {                                                                         \
    uint32_t i_ = (pos);                                                       \
    while (i_ > 0 && FIO___HUFF_LESS(i_, FIO___HUFF_PARENT(i_))) {             \
      FIO___HUFF_SWAP(i_, FIO___HUFF_PARENT(i_));                              \
      i_ = FIO___HUFF_PARENT(i_);                                              \
    }                                                                          \
  } while (0)

  /* Sift down */
#define FIO___HUFF_SIFT_DOWN(pos)                                              \
  do {                                                                         \
    uint32_t i_ = (pos);                                                       \
    for (;;) {                                                                 \
      uint32_t smallest_ = i_;                                                 \
      uint32_t l_ = FIO___HUFF_LEFT(i_);                                       \
      uint32_t r_ = FIO___HUFF_RIGHT(i_);                                      \
      if (l_ < heap_size && FIO___HUFF_LESS(l_, smallest_))                    \
        smallest_ = l_;                                                        \
      if (r_ < heap_size && FIO___HUFF_LESS(r_, smallest_))                    \
        smallest_ = r_;                                                        \
      if (smallest_ == i_)                                                     \
        break;                                                                 \
      FIO___HUFF_SWAP(i_, smallest_);                                          \
      i_ = smallest_;                                                          \
    }                                                                          \
  } while (0)

  /* Insert leaf nodes */
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (freqs[i]) {
      nodes[node_count].freq = freqs[i];
      nodes[node_count].sym = (int32_t)i;
      nodes[node_count].left = -1;
      nodes[node_count].right = -1;
      heap[heap_size] = node_count;
      heap_size++;
      FIO___HUFF_SIFT_UP(heap_size - 1);
      node_count++;
    }
  }

  /* Build tree by combining two smallest nodes */
  while (heap_size > 1) {
    /* Extract min */
    uint32_t a = heap[0];
    heap[0] = heap[--heap_size];
    FIO___HUFF_SIFT_DOWN(0);

    /* Extract second min */
    uint32_t b = heap[0];
    heap[0] = heap[--heap_size];
    FIO___HUFF_SIFT_DOWN(0);

    /* Create internal node */
    nodes[node_count].freq = nodes[a].freq + nodes[b].freq;
    nodes[node_count].sym = -1;
    nodes[node_count].left = (int32_t)a;
    nodes[node_count].right = (int32_t)b;

    heap[heap_size] = node_count;
    heap_size++;
    FIO___HUFF_SIFT_UP(heap_size - 1);
    node_count++;
  }

  /* Compute depths via iterative traversal */
  uint8_t depths[572];
  FIO_MEMSET(depths, 0, sizeof(depths));

  /* BFS/DFS to compute depths */
  {
    uint32_t stack[572];
    uint32_t sp = 0;
    uint32_t root = heap[0];
    stack[sp++] = root;
    depths[root] = 0;

    while (sp > 0) {
      uint32_t n = stack[--sp];
      if (nodes[n].sym >= 0) {
        /* Leaf node */
        lens[nodes[n].sym] = depths[n];
      } else {
        if (nodes[n].left >= 0) {
          depths[nodes[n].left] = depths[n] + 1;
          stack[sp++] = (uint32_t)nodes[n].left;
        }
        if (nodes[n].right >= 0) {
          depths[nodes[n].right] = depths[n] + 1;
          stack[sp++] = (uint32_t)nodes[n].right;
        }
      }
    }
  }

  /* Limit code lengths to max_bits */
  {
    uint32_t count[16] = {0};
    for (uint32_t i = 0; i < num_syms; ++i) {
      if (lens[i] > max_bits)
        lens[i] = (uint8_t)max_bits;
      if (lens[i])
        count[lens[i]]++;
    }

    /* Check Kraft inequality and fix if needed */
    for (;;) {
      int32_t kraft = 0;
      for (uint32_t i = 1; i <= max_bits; ++i)
        kraft += (int32_t)count[i] << (max_bits - i);

      if (kraft <= (1 << max_bits))
        break;

      /* Over-subscribed: lengthen shortest codes to reduce Kraft sum.
       * Moving a code from length i to i+1 reduces kraft by 2^(max-i-1). */
      int fixed = 0;
      for (uint32_t i = 1; i < max_bits && !fixed; ++i) {
        if (count[i] > 0) {
          count[i]--;
          count[i + 1]++;
          /* Find a symbol with this length and lengthen it */
          for (uint32_t s = 0; s < num_syms; ++s) {
            if (lens[s] == i) {
              lens[s]++;
              fixed = 1;
              break;
            }
          }
        }
      }
      if (!fixed)
        break;
    }
  }

#undef FIO___HUFF_PARENT
#undef FIO___HUFF_LEFT
#undef FIO___HUFF_RIGHT
#undef FIO___HUFF_SWAP
#undef FIO___HUFF_LESS
#undef FIO___HUFF_SIFT_UP
#undef FIO___HUFF_SIFT_DOWN

  return 0;
}

/** Build canonical Huffman codes from code lengths.
 *  Codes are pre-reversed (LSB-first) for direct bitstream emission. */
FIO_SFUNC void fio___deflate_build_codes(uint16_t *codes,
                                         const uint8_t *lens,
                                         uint32_t num_syms) {
  uint32_t count[16] = {0};
  uint32_t next_code[16] = {0};

  for (uint32_t i = 0; i < num_syms; ++i)
    count[lens[i]]++;

  /* Compute starting codes for each length */
  uint32_t code = 0;
  for (uint32_t bits = 1; bits <= 15; ++bits) {
    code = (code + count[bits - 1]) << 1;
    next_code[bits] = code;
  }

  /* Assign codes and bit-reverse for LSB-first bitstream */
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (lens[i]) {
      uint32_t c = next_code[lens[i]]++;
      uint32_t nbits = lens[i];
      uint32_t rev = 0;
      for (uint32_t b = 0; b < nbits; ++b)
        rev |= ((c >> b) & 1) << (nbits - 1 - b);
      codes[i] = (uint16_t)rev;
    } else {
      codes[i] = 0;
    }
  }
}

/* *****************************************************************************
Deflate (compression) - main implementation
***************************************************************************** */

/** Compression level parameters */
typedef struct {
  uint32_t good_length;
  uint32_t max_lazy;
  uint32_t nice_length;
  uint32_t max_chain;
  int lazy;
} fio___deflate_level_params_s;

static const fio___deflate_level_params_s fio___deflate_levels[10] = {
    {0, 0, 0, 0, 0},         /* level 0: store */
    {4, 4, 8, 4, 0},         /* level 1: fast greedy */
    {5, 8, 16, 8, 0},        /* level 2: greedy */
    {6, 16, 32, 32, 0},      /* level 3: greedy */
    {4, 4, 16, 16, 1},       /* level 4: lazy */
    {8, 16, 32, 32, 1},      /* level 5: lazy */
    {8, 16, 128, 128, 1},    /* level 6: default lazy */
    {8, 32, 128, 256, 1},    /* level 7: lazy */
    {32, 128, 258, 1024, 1}, /* level 8: lazy */
    {32, 258, 258, 4096, 1}, /* level 9: max lazy */
};

/** Hash function for 4-byte sequences (single unaligned load). */
FIO_IFUNC uint32_t fio___deflate_hash4(const uint8_t *p) {
  return (fio_buf2u32_le(p) * 0x1E35A7BDU) >> (32 - FIO___DEFLATE_HASH_BITS);
}

/** Extend a match using 8-byte word-at-a-time comparison.
 *  `len` is the number of already-verified matching bytes.
 *  Returns the total match length (up to max_len). */
FIO_IFUNC uint32_t fio___deflate_extend_match(const uint8_t *src,
                                              uint32_t pos,
                                              uint32_t candidate,
                                              uint32_t len,
                                              uint32_t max_len) {
  while (len + 8 <= max_len) {
    uint64_t a = fio_buf2u64_le(src + pos + len);
    uint64_t b = fio_buf2u64_le(src + candidate + len);
    uint64_t diff = a ^ b;
    if (diff) {
      len += (uint32_t)(fio_lsb_index_unsafe(diff) >> 3);
      return len < max_len ? len : max_len;
    }
    len += 8;
  }
  while (len < max_len && src[pos + len] == src[candidate + len])
    ++len;
  return len;
}

/** Find best match at current position.
 *  Returns match length (0 if no match found >= 3).
 *  Uses generation counter: head_gen[h] must equal gen for a valid entry. */
FIO_SFUNC uint32_t fio___deflate_find_match(const uint8_t *src,
                                            uint32_t pos,
                                            uint32_t src_len,
                                            const uint32_t *head,
                                            const uint32_t *prev,
                                            const uint32_t *head_gen,
                                            uint32_t gen,
                                            uint32_t max_chain,
                                            uint32_t nice_length,
                                            uint32_t *match_dist) {
  uint32_t best_len = 2; /* minimum match is 3 */
  uint32_t best_dist = 0;
  uint32_t max_len = src_len - pos;
  if (max_len > FIO___DEFLATE_MAX_MATCH)
    max_len = FIO___DEFLATE_MAX_MATCH;
  if (max_len < 4)
    return 0;

  uint32_t prefix = fio_buf2u32_le(src + pos);
  uint32_t hash = fio___deflate_hash4(src + pos);
  /* Generation check: stale head entry means no chain to walk */
  if (head_gen[hash] != gen)
    return 0;
  uint32_t chain_pos = head[hash];
  uint32_t chain_count = 0;

  while (chain_pos != 0 && chain_count < max_chain) {
    uint32_t dist = pos - chain_pos;
    if (dist == 0 || dist > FIO___DEFLATE_WINDOW_SIZE)
      break;

    /* Quick check: 4-byte prefix + last byte of current best */
    if (fio_buf2u32_le(src + chain_pos) == prefix &&
        src[chain_pos + best_len] == src[pos + best_len]) {
      /* Full comparison (first 4 bytes already verified) */
      uint32_t len =
          fio___deflate_extend_match(src, pos, chain_pos, 4, max_len);

      if (len > best_len) {
        best_len = len;
        best_dist = dist;
        if (len >= nice_length)
          break;
      }
    }

    chain_pos = prev[chain_pos & FIO___DEFLATE_WINDOW_MASK];
    chain_count++;
  }

  if (best_len >= 3) {
    *match_dist = best_dist;
    return best_len;
  }
  return 0;
}

/** Write a dynamic Huffman block header. */
FIO_SFUNC void fio___deflate_write_dynamic_header(fio___deflate_bitwriter_s *w,
                                                  const uint8_t *ll_lens,
                                                  uint32_t num_ll,
                                                  const uint16_t *ll_codes,
                                                  const uint8_t *d_lens,
                                                  uint32_t num_d,
                                                  const uint16_t *d_codes) {
  /* Encode code lengths with RLE */
  uint8_t all_lens[286 + 30];
  uint32_t total = num_ll + num_d;
  FIO_MEMCPY(all_lens, ll_lens, num_ll);
  FIO_MEMCPY(all_lens + num_ll, d_lens, num_d);

  /* RLE encode the code lengths */
  uint8_t rle_syms[286 + 30 + 32];  /* symbols */
  uint8_t rle_extra[286 + 30 + 32]; /* extra bits values */
  uint32_t rle_count = 0;

  for (uint32_t i = 0; i < total;) {
    if (all_lens[i] == 0) {
      /* Count consecutive zeros */
      uint32_t run = 1;
      while (i + run < total && all_lens[i + run] == 0)
        ++run;

      while (run >= 11) {
        uint32_t r = run > 138 ? 138 : run;
        rle_syms[rle_count] = 18;
        rle_extra[rle_count] = (uint8_t)(r - 11);
        rle_count++;
        run -= r;
        i += r;
      }
      while (run >= 3) {
        uint32_t r = run > 10 ? 10 : run;
        rle_syms[rle_count] = 17;
        rle_extra[rle_count] = (uint8_t)(r - 3);
        rle_count++;
        run -= r;
        i += r;
      }
      while (run > 0) {
        rle_syms[rle_count] = 0;
        rle_extra[rle_count] = 0;
        rle_count++;
        run--;
        i++;
      }
    } else {
      uint8_t val = all_lens[i];
      rle_syms[rle_count] = val;
      rle_extra[rle_count] = 0;
      rle_count++;
      i++;

      /* Count repeats of same value */
      uint32_t run = 0;
      while (i + run < total && all_lens[i + run] == val)
        ++run;

      while (run >= 3) {
        uint32_t r = run > 6 ? 6 : run;
        rle_syms[rle_count] = 16;
        rle_extra[rle_count] = (uint8_t)(r - 3);
        rle_count++;
        run -= r;
        i += r;
      }
      while (run > 0) {
        rle_syms[rle_count] = val;
        rle_extra[rle_count] = 0;
        rle_count++;
        run--;
        i++;
      }
    }
  }

  /* Build code length Huffman tree */
  uint32_t cl_freqs[19] = {0};
  for (uint32_t i = 0; i < rle_count; ++i)
    cl_freqs[rle_syms[i]]++;

  uint8_t cl_lens[19];
  fio___deflate_build_code_lengths(cl_lens, cl_freqs, 19, 7);

  uint16_t cl_codes[19];
  fio___deflate_build_codes(cl_codes, cl_lens, 19);

  /* Determine HCLEN */
  uint32_t hclen = 19;
  while (hclen > 4 && cl_lens[fio___deflate_codelen_order[hclen - 1]] == 0)
    --hclen;

  /* Write header */
  fio___deflate_bitwriter_put(w, num_ll - 257, 5);
  fio___deflate_bitwriter_put(w, num_d - 1, 5);
  fio___deflate_bitwriter_put(w, hclen - 4, 4);

  /* Write code length code lengths in permuted order */
  for (uint32_t i = 0; i < hclen; ++i)
    fio___deflate_bitwriter_put(w, cl_lens[fio___deflate_codelen_order[i]], 3);

  /* Write RLE-encoded code lengths */
  for (uint32_t i = 0; i < rle_count; ++i) {
    uint8_t sym = rle_syms[i];
    fio___deflate_bitwriter_put_huff(w, cl_codes[sym], cl_lens[sym]);
    if (sym == 16)
      fio___deflate_bitwriter_put(w, rle_extra[i], 2);
    else if (sym == 17)
      fio___deflate_bitwriter_put(w, rle_extra[i], 3);
    else if (sym == 18)
      fio___deflate_bitwriter_put(w, rle_extra[i], 7);
  }

  (void)ll_codes;
  (void)d_codes;
}

/** Find the length symbol for a given length value (O(1) table lookup). */
FIO_IFUNC uint32_t fio___deflate_len_to_sym(uint32_t length) {
  return (uint32_t)fio___deflate_len_to_idx[length - 3] + 257;
}

/** Find the distance symbol for a given distance value (branchless log2). */
FIO_IFUNC uint32_t fio___deflate_dist_to_sym(uint32_t distance) {
  if (distance <= 2)
    return distance - 1;
  uint32_t d = distance - 1;
  uint32_t msb = (uint32_t)fio_bits_msb_index(d);
  return (msb << 1) + ((d >> (msb - 1)) & 1);
}

/** Upper bound on compressed output size. */
FIO_IFUNC size_t fio_deflate_compress_bound(size_t in_len) {
  /* Worst case: stored blocks (5 bytes header per 65535 bytes + data).
   * Add extra margin for dynamic Huffman header overhead (~320 bytes max)
   * and bitwriter flush margin (8 bytes). */
  size_t num_blocks = (in_len + 65534) / 65535;
  return in_len + num_blocks * 5 + 336;
}

void fio_deflate_compress___(void); /* IDE Marker */
SFUNC size_t fio_deflate_compress
FIO_NOOP(void *out, size_t out_len, const void *in, size_t in_len, int level) {
  if (!out || !out_len)
    return 0;
  if (!in || !in_len) {
    /* Empty input: emit empty stored block */
    if (out_len < 5)
      return 0;
    uint8_t *o = (uint8_t *)out;
    o[0] = 0x01; /* BFINAL=1, BTYPE=00 (stored) */
    o[1] = 0x00; /* LEN low */
    o[2] = 0x00; /* LEN high */
    o[3] = 0xFF; /* NLEN low */
    o[4] = 0xFF; /* NLEN high */
    return 5;
  }

  if (level < 0)
    level = 0;
  if (level > 9)
    level = 9;

  const uint8_t *src = (const uint8_t *)in;
  uint32_t src_len = (uint32_t)(in_len > 0xFFFFFFFFU ? 0xFFFFFFFFU : in_len);

  /* Level 0: stored blocks */
  if (level == 0) {
    fio___deflate_bitwriter_s w;
    fio___deflate_bitwriter_init(&w, out, out_len);

    uint32_t pos = 0;
    while (pos < src_len) {
      uint32_t block_len = src_len - pos;
      if (block_len > 65535)
        block_len = 65535;
      uint32_t is_final = (pos + block_len >= src_len) ? 1 : 0;

      /* BFINAL + BTYPE=00 */
      fio___deflate_bitwriter_put(&w, is_final, 1);
      fio___deflate_bitwriter_put(&w, 0, 2);
      fio___deflate_bitwriter_align(&w);

      /* LEN and NLEN */
      fio___deflate_bitwriter_put(&w, block_len & 0xFFFF, 16);
      fio___deflate_bitwriter_put(&w, (~block_len) & 0xFFFF, 16);

      /* Raw data */
      if (w.out + block_len > w.out_end)
        return 0;
      FIO_MEMCPY(w.out, src + pos, block_len);
      w.out += block_len;

      pos += block_len;
    }
    return fio___deflate_bitwriter_finish(&w, out);
  }

  /* Levels 1-9: LZ77 + Huffman */
  const fio___deflate_level_params_s *params = &fio___deflate_levels[level];

  /* Allocate hash tables on heap.
   * Uses generation counter to avoid zeroing head[] (128KB) and prev[] (128KB)
   * on each call. head_gen[h] tracks which generation wrote head[h]; stale
   * entries are treated as empty. prev[] chain links are cut at insertion time
   * when the head entry is stale, so prev[] never needs zeroing either. */
  size_t alloc_size = sizeof(uint32_t) * FIO___DEFLATE_HASH_SIZE * 2 +
                      sizeof(uint32_t) * FIO___DEFLATE_WINDOW_SIZE;
  uint32_t *hash_mem = (uint32_t *)FIO_MEM_REALLOC(NULL, 0, alloc_size, 0);
  if (!hash_mem)
    return 0;

  uint32_t *head = hash_mem;
  uint32_t *head_gen = hash_mem + FIO___DEFLATE_HASH_SIZE;
  uint32_t *prev = hash_mem + FIO___DEFLATE_HASH_SIZE * 2;
  /* No memset needed — generation counter makes stale entries harmless.
   * Initialize head_gen to 0; gen starts at 1 so all entries are stale. */
  FIO_MEMSET(head_gen, 0, sizeof(uint32_t) * FIO___DEFLATE_HASH_SIZE);
  uint32_t gen = 1;

  /* Symbol frequency counts */
  uint32_t ll_freqs[286];
  uint32_t d_freqs[30];
  FIO_MEMSET(ll_freqs, 0, sizeof(ll_freqs));
  FIO_MEMSET(d_freqs, 0, sizeof(d_freqs));

  /* LZ77 output tokens */
  typedef struct {
    uint16_t litlen; /* literal byte or length (if dist > 0) */
    uint16_t dist;   /* 0 for literal, >0 for match */
  } fio___deflate_token_s;

  /* Allocate token buffer */
  size_t max_tokens = src_len + 1;
  size_t token_alloc = sizeof(fio___deflate_token_s) * max_tokens;
  fio___deflate_token_s *tokens =
      (fio___deflate_token_s *)FIO_MEM_REALLOC(NULL, 0, token_alloc, 0);
  if (!tokens) {
    FIO_MEM_FREE(hash_mem, alloc_size);
    return 0;
  }
  uint32_t token_count = 0;

  /* LZ77 pass: find matches and build token stream */
  uint32_t pos = 0;
  uint32_t prev_match_len = 0;
  uint32_t prev_match_dist = 0;
  int prev_was_match = 0;

  while (pos < src_len) {
    uint32_t match_len = 0;
    uint32_t match_dist = 0;

    if (pos + 3 < src_len) {
      match_len = fio___deflate_find_match(src,
                                           pos,
                                           src_len,
                                           head,
                                           prev,
                                           head_gen,
                                           gen,
                                           params->max_chain,
                                           params->nice_length,
                                           &match_dist);
    }

    /* Lazy matching */
    if (params->lazy && prev_was_match && match_len > prev_match_len) {
      /* Better match at current position - emit literal for previous */
      tokens[token_count].litlen = src[pos - 1];
      tokens[token_count].dist = 0;
      token_count++;
      ll_freqs[src[pos - 1]]++;
      prev_was_match = 0;
    } else if (prev_was_match) {
      /* Use previous match */
      tokens[token_count].litlen = (uint16_t)prev_match_len;
      tokens[token_count].dist = (uint16_t)prev_match_dist;
      token_count++;

      uint32_t sym = fio___deflate_len_to_sym(prev_match_len);
      ll_freqs[sym]++;
      uint32_t dsym = fio___deflate_dist_to_sym(prev_match_dist);
      d_freqs[dsym]++;

      /* Update hash for skipped positions (level-dependent density) */
      uint32_t skip_end = pos - 1 + prev_match_len;
      if (skip_end > src_len)
        skip_end = src_len;
      if (level >= 4) {
        /* Level 4+: full insertion (needed for good lazy matching) */
        for (uint32_t j = pos; j < skip_end && j + 3 < src_len; ++j) {
          uint32_t h = fio___deflate_hash4(src + j);
          prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? head[h] : 0;
          head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      } else if (level >= 2) {
        /* Level 2-3: sparse insertion every 4th position */
        for (uint32_t j = pos + 3; j < skip_end && j + 3 < src_len; j += 4) {
          uint32_t h = fio___deflate_hash4(src + j);
          prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? head[h] : 0;
          head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      }
      /* Level 1: skip all mid-match insertions (zlib-style) */
      pos = skip_end;
      prev_was_match = 0;
      continue;
    }

    /* Update hash chain */
    if (pos + 3 < src_len) {
      uint32_t h = fio___deflate_hash4(src + pos);
      prev[pos & FIO___DEFLATE_WINDOW_MASK] =
          (head_gen[h] == gen) ? head[h] : 0;
      head[h] = (uint32_t)pos;
      head_gen[h] = gen;
    }

    if (match_len >= 3) {
      if (params->lazy && match_len < params->max_lazy) {
        /* Defer: check next position for better match */
        prev_match_len = match_len;
        prev_match_dist = match_dist;
        prev_was_match = 1;
        pos++;
        continue;
      }

      /* Emit match immediately */
      tokens[token_count].litlen = (uint16_t)match_len;
      tokens[token_count].dist = (uint16_t)match_dist;
      token_count++;

      uint32_t sym = fio___deflate_len_to_sym(match_len);
      ll_freqs[sym]++;
      uint32_t dsym = fio___deflate_dist_to_sym(match_dist);
      d_freqs[dsym]++;

      /* Update hash for match positions (level-dependent density) */
      if (level >= 4) {
        /* Level 4+: full insertion (needed for good lazy matching) */
        for (uint32_t j = pos + 1; j < pos + match_len && j + 3 < src_len;
             ++j) {
          uint32_t h = fio___deflate_hash4(src + j);
          prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? head[h] : 0;
          head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      } else if (level >= 2) {
        /* Level 2-3: sparse insertion every 4th position */
        for (uint32_t j = pos + 4; j < pos + match_len && j + 3 < src_len;
             j += 4) {
          uint32_t h = fio___deflate_hash4(src + j);
          prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? head[h] : 0;
          head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      }
      /* Level 1: skip all mid-match insertions (zlib-style) */
      pos += match_len;
    } else {
      /* Emit literal */
      tokens[token_count].litlen = src[pos];
      tokens[token_count].dist = 0;
      token_count++;
      ll_freqs[src[pos]]++;
      pos++;
    }
  }

  /* Flush any pending lazy match */
  if (prev_was_match) {
    tokens[token_count].litlen = (uint16_t)prev_match_len;
    tokens[token_count].dist = (uint16_t)prev_match_dist;
    token_count++;
    uint32_t sym = fio___deflate_len_to_sym(prev_match_len);
    ll_freqs[sym]++;
    uint32_t dsym = fio___deflate_dist_to_sym(prev_match_dist);
    d_freqs[dsym]++;
  }

  /* Add end-of-block symbol */
  ll_freqs[256]++;

  /* Build Huffman trees */
  uint8_t ll_lens[286];
  uint8_t d_lens[30];
  fio___deflate_build_code_lengths(ll_lens, ll_freqs, 286, 15);
  fio___deflate_build_code_lengths(d_lens, d_freqs, 30, 15);

  /* Ensure at least one distance code exists */
  {
    int has_dist = 0;
    for (uint32_t i = 0; i < 30; ++i) {
      if (d_lens[i]) {
        has_dist = 1;
        break;
      }
    }
    if (!has_dist)
      d_lens[0] = 1;
  }

  /* Ensure end-of-block has a code */
  if (ll_lens[256] == 0)
    ll_lens[256] = 1;

  /* Determine actual number of litlen and distance codes to emit */
  uint32_t num_ll = 286;
  while (num_ll > 257 && ll_lens[num_ll - 1] == 0)
    --num_ll;
  uint32_t num_d = 30;
  while (num_d > 1 && d_lens[num_d - 1] == 0)
    --num_d;

  uint16_t ll_codes[286];
  uint16_t d_codes[30];
  fio___deflate_build_codes(ll_codes, ll_lens, 286);
  fio___deflate_build_codes(d_codes, d_lens, 30);

  /* ---- Cost comparison: fixed vs dynamic Huffman ---- */

  /* Fixed Huffman code lengths per RFC 1951 §3.2.6.
   * Must use 288 entries for correct canonical code generation. */
  uint8_t fixed_ll_lens[288];
  uint8_t fixed_d_lens[30];
  fio___deflate_fixed_litlen_lens(fixed_ll_lens);
  for (uint32_t i = 0; i < 30; ++i)
    fixed_d_lens[i] = 5;

  /* Cost comparison: only Huffman code bits differ between fixed and dynamic.
   * Extra bits for length/distance are identical and cancel out. */
  uint64_t fixed_cost = 3; /* BFINAL(1) + BTYPE(2), no code-length header */
  uint64_t dyn_cost = 3;   /* BFINAL(1) + BTYPE(2) */

  /* Litlen symbol costs (code bits only) */
  for (uint32_t i = 0; i < 286; ++i) {
    if (ll_freqs[i]) {
      fixed_cost += (uint64_t)ll_freqs[i] * fixed_ll_lens[i];
      dyn_cost += (uint64_t)ll_freqs[i] * ll_lens[i];
    }
  }

  /* Distance symbol costs (code bits only, extra bits cancel out) */
  for (uint32_t i = 0; i < 30; ++i) {
    if (d_freqs[i]) {
      fixed_cost += (uint64_t)d_freqs[i] * fixed_d_lens[i];
      dyn_cost += (uint64_t)d_freqs[i] * d_lens[i];
    }
  }

  /* Dynamic header overhead: compute the RLE-encoded header cost.
   * Replicates the header encoding logic to count bits without writing. */
  {
    uint8_t all_lens_tmp[286 + 30];
    uint32_t total = num_ll + num_d;
    FIO_MEMCPY(all_lens_tmp, ll_lens, num_ll);
    FIO_MEMCPY(all_lens_tmp + num_ll, d_lens, num_d);

    /* RLE encode to count code-length symbols */
    uint32_t cl_freqs_tmp[19];
    FIO_MEMSET(cl_freqs_tmp, 0, sizeof(cl_freqs_tmp));

    for (uint32_t i = 0; i < total;) {
      if (all_lens_tmp[i] == 0) {
        uint32_t run = 1;
        while (i + run < total && all_lens_tmp[i + run] == 0)
          ++run;
        while (run >= 11) {
          uint32_t r = run > 138 ? 138 : run;
          cl_freqs_tmp[18]++;
          run -= r;
          i += r;
        }
        while (run >= 3) {
          uint32_t r = run > 10 ? 10 : run;
          cl_freqs_tmp[17]++;
          run -= r;
          i += r;
        }
        while (run > 0) {
          cl_freqs_tmp[0]++;
          run--;
          i++;
        }
      } else {
        uint8_t val = all_lens_tmp[i];
        cl_freqs_tmp[val]++;
        i++;
        uint32_t run = 0;
        while (i + run < total && all_lens_tmp[i + run] == val)
          ++run;
        while (run >= 3) {
          uint32_t r = run > 6 ? 6 : run;
          cl_freqs_tmp[16]++;
          run -= r;
          i += r;
        }
        while (run > 0) {
          cl_freqs_tmp[val]++;
          run--;
          i++;
        }
      }
    }

    /* Build code-length Huffman to get their code lengths */
    uint8_t cl_lens_tmp[19];
    fio___deflate_build_code_lengths(cl_lens_tmp, cl_freqs_tmp, 19, 7);

    /* HCLEN: how many code-length code lengths to transmit */
    uint32_t hclen_tmp = 19;
    while (hclen_tmp > 4 &&
           cl_lens_tmp[fio___deflate_codelen_order[hclen_tmp - 1]] == 0)
      --hclen_tmp;

    /* Header bits: HLIT(5) + HDIST(5) + HCLEN(4) + code-length code lengths */
    dyn_cost += 5 + 5 + 4;
    dyn_cost +=
        (uint64_t)hclen_tmp * 3; /* 3 bits per code-length code length */

    /* RLE-encoded code lengths: each symbol's Huffman code + extra bits */
    for (uint32_t i = 0; i < 19; ++i) {
      if (cl_freqs_tmp[i])
        dyn_cost += (uint64_t)cl_freqs_tmp[i] * cl_lens_tmp[i];
    }
    /* Extra bits for repeat codes: 16→2 bits, 17→3 bits, 18→7 bits */
    dyn_cost += (uint64_t)cl_freqs_tmp[16] * 2;
    dyn_cost += (uint64_t)cl_freqs_tmp[17] * 3;
    dyn_cost += (uint64_t)cl_freqs_tmp[18] * 7;
  }

  /* Choose fixed Huffman if cheaper or equal (avoids header overhead) */
  int use_fixed = (fixed_cost <= dyn_cost);

  /* Build fixed codes if needed (must use all 288 for correct canonicals) */
  uint16_t fixed_ll_codes[288];
  uint16_t fixed_d_codes[30];
  if (use_fixed) {
    fio___deflate_build_codes(fixed_ll_codes, fixed_ll_lens, 288);
    fio___deflate_build_codes(fixed_d_codes, fixed_d_lens, 30);
  }

  /* Select which code tables to use for encoding */
  const uint8_t *enc_ll_lens = use_fixed ? fixed_ll_lens : ll_lens;
  const uint16_t *enc_ll_codes = use_fixed ? fixed_ll_codes : ll_codes;
  const uint8_t *enc_d_lens = use_fixed ? fixed_d_lens : d_lens;
  const uint16_t *enc_d_codes = use_fixed ? fixed_d_codes : d_codes;

  /* Write compressed output */
  fio___deflate_bitwriter_s w;
  fio___deflate_bitwriter_init(&w, out, out_len);

  /* Block header */
  fio___deflate_bitwriter_put(&w, 1, 1); /* BFINAL */
  if (use_fixed) {
    fio___deflate_bitwriter_put(&w, 1, 2); /* BTYPE = 01 (fixed) */
  } else {
    fio___deflate_bitwriter_put(&w, 2, 2); /* BTYPE = 10 (dynamic) */
    /* Write dynamic Huffman header */
    fio___deflate_write_dynamic_header(&w,
                                       ll_lens,
                                       num_ll,
                                       ll_codes,
                                       d_lens,
                                       num_d,
                                       d_codes);
  }

  /* Write tokens */
  for (uint32_t i = 0; i < token_count; ++i) {
    if (tokens[i].dist == 0) {
      /* Literal */
      uint32_t sym = tokens[i].litlen;
      fio___deflate_bitwriter_put_huff(&w, enc_ll_codes[sym], enc_ll_lens[sym]);
    } else {
      /* Length/distance pair */
      uint32_t length = tokens[i].litlen;
      uint32_t distance = tokens[i].dist;

      /* Emit length code */
      uint32_t lsym = fio___deflate_len_to_sym(length);
      fio___deflate_bitwriter_put_huff(&w,
                                       enc_ll_codes[lsym],
                                       enc_ll_lens[lsym]);

      /* Emit length extra bits */
      uint32_t lidx = lsym - 257;
      if (fio___deflate_len_extra[lidx])
        fio___deflate_bitwriter_put(&w,
                                    length - fio___deflate_len_base[lidx],
                                    fio___deflate_len_extra[lidx]);

      /* Emit distance code */
      uint32_t dsym = fio___deflate_dist_to_sym(distance);
      fio___deflate_bitwriter_put_huff(&w, enc_d_codes[dsym], enc_d_lens[dsym]);

      /* Emit distance extra bits */
      if (fio___deflate_dist_extra[dsym])
        fio___deflate_bitwriter_put(&w,
                                    distance - fio___deflate_dist_base[dsym],
                                    fio___deflate_dist_extra[dsym]);
    }
  }

  /* End of block */
  fio___deflate_bitwriter_put_huff(&w, enc_ll_codes[256], enc_ll_lens[256]);

  size_t result = fio___deflate_bitwriter_finish(&w, out);

  FIO_MEM_FREE(tokens, token_alloc);
  FIO_MEM_FREE(hash_mem, alloc_size);

  return result;
}

/* *****************************************************************************
Gzip wrappers
***************************************************************************** */

void fio_gzip_compress___(void); /* IDE Marker */
SFUNC size_t fio_gzip_compress
FIO_NOOP(void *out, size_t out_len, const void *in, size_t in_len, int level) {
  if (!out || out_len < 18) /* minimum: 10 header + 0 data + 8 trailer */
    return 0;

  uint8_t *o = (uint8_t *)out;

  /* Gzip header (10 bytes) */
  o[0] = 0x1F; /* magic */
  o[1] = 0x8B; /* magic */
  o[2] = 0x08; /* method: deflate */
  o[3] = 0x00; /* flags: none */
  o[4] = 0x00; /* mtime */
  o[5] = 0x00;
  o[6] = 0x00;
  o[7] = 0x00;
  o[8] = (level >= 7) ? 0x02 : ((level <= 1) ? 0x04 : 0x00); /* xfl */
  o[9] = 0xFF;                                               /* OS: unknown */

  /* Compress the data */
  size_t compressed_len =
      fio_deflate_compress(o + 10, out_len - 18, in, in_len, level);
  if (!compressed_len && in_len > 0)
    return 0;

  /* CRC32 + ISIZE trailer (8 bytes) */
  uint32_t crc = fio_crc32(in, in_len, 0);
  uint32_t isize = (uint32_t)(in_len & 0xFFFFFFFFU);

  uint8_t *trailer = o + 10 + compressed_len;
  fio_u2buf32_le(trailer, crc);
  fio_u2buf32_le(trailer + 4, isize);

  return 10 + compressed_len + 8;
}

void fio_gzip_decompress___(void); /* IDE Marker */
SFUNC size_t fio_gzip_decompress FIO_NOOP(void *out,
                                          size_t out_len,
                                          const void *in,
                                          size_t in_len) {
  if (!in || in_len < 18)
    return 0;

  const uint8_t *p = (const uint8_t *)in;

  /* Validate gzip header */
  if (p[0] != 0x1F || p[1] != 0x8B)
    return 0; /* bad magic */
  if (p[2] != 0x08)
    return 0; /* not deflate method */

  uint8_t flags = p[3];
  size_t hdr_len = 10;

  /* Skip optional header fields */
  if (flags & 0x04) {
    /* FEXTRA */
    if (hdr_len + 2 > in_len)
      return 0;
    uint16_t xlen = fio_buf2u16_le(p + hdr_len);
    hdr_len += 2 + xlen;
  }
  if (flags & 0x08) {
    /* FNAME: skip null-terminated string */
    while (hdr_len < in_len && p[hdr_len] != 0)
      hdr_len++;
    hdr_len++; /* skip null terminator */
  }
  if (flags & 0x10) {
    /* FCOMMENT: skip null-terminated string */
    while (hdr_len < in_len && p[hdr_len] != 0)
      hdr_len++;
    hdr_len++;
  }
  if (flags & 0x02) {
    /* FHCRC: 2-byte header CRC */
    hdr_len += 2;
  }

  if (hdr_len + 8 > in_len)
    return 0;

  /* Compressed data is between header and 8-byte trailer */
  size_t compressed_len = in_len - hdr_len - 8;
  const uint8_t *trailer = p + in_len - 8;

  /* Decompress (supports counting mode when out==NULL) */
  size_t decompressed_len =
      fio_deflate_decompress(out, out_len, p + hdr_len, compressed_len);
  if (!decompressed_len && compressed_len > 5) /* allow empty */
    return 0;

  /* If result > out_len, this is a size query or buffer-too-small.
   * We can still verify ISIZE from the trailer but not CRC32
   * (since we don't have the actual decompressed data). */
  if (!out || !out_len || decompressed_len > out_len) {
    /* Verify ISIZE only (CRC32 requires actual data) */
    uint32_t expected_isize = fio_buf2u32_le(trailer + 4);
    if ((uint32_t)(decompressed_len & 0xFFFFFFFFU) != expected_isize)
      return 0;
    return decompressed_len;
  }

  /* Verify CRC32 */
  uint32_t expected_crc = fio_buf2u32_le(trailer);
  uint32_t actual_crc = fio_crc32(out, decompressed_len, 0);
  if (actual_crc != expected_crc)
    return 0;

  /* Verify ISIZE */
  uint32_t expected_isize = fio_buf2u32_le(trailer + 4);
  if ((uint32_t)(decompressed_len & 0xFFFFFFFFU) != expected_isize)
    return 0;

  return decompressed_len;
}

/* *****************************************************************************
Streaming API (for WebSocket permessage-deflate)

True incremental streaming with context takeover:
- Compressor: maintains sliding window + hash chain across push calls.
  On flush, emits non-final DEFLATE block(s) + sync flush marker.
  LZ77 matches can reference data from previous push calls.
- Decompressor: buffers compressed input. On flush, prepends the sliding
  window to the output buffer so back-references from later messages
  resolve correctly against earlier message data.
***************************************************************************** */

/** Maximum buffered input before auto-flush (compressor). */
#define FIO___DEFLATE_STREAM_BUF_MAX FIO___DEFLATE_WINDOW_SIZE

struct fio_deflate_s {
  /* Compression hash chain (persistent across calls) */
  uint32_t *hash_head;
  uint32_t *hash_prev;
  uint32_t *hash_head_gen; /* generation stamp per head[] entry */
  size_t hash_alloc;
  uint32_t hash_gen; /* current generation (incremented each compress call) */
  /* Input buffer (accumulates data between flush calls) */
  uint8_t *buf;
  size_t buf_len;
  size_t buf_cap;
  /* Sliding window for context takeover (both compress and decompress) */
  uint32_t window_pos;
  /* level and state */
  uint8_t is_compress;
  uint8_t level;
  uint8_t window[FIO___DEFLATE_WINDOW_SIZE];
};

/** Ensure the input buffer has room for `need` more bytes. */
FIO_SFUNC int fio___deflate_stream_buf_grow(fio_deflate_s *s, size_t need) {
  size_t required = s->buf_len + need;
  if (required <= s->buf_cap)
    return 0;
  size_t new_cap = s->buf_cap ? s->buf_cap : 4096;
  while (new_cap < required)
    new_cap <<= 1;
  uint8_t *nb =
      (uint8_t *)FIO_MEM_REALLOC(s->buf, s->buf_cap, new_cap, s->buf_len);
  if (!nb)
    return -1;
  s->buf = nb;
  s->buf_cap = new_cap;
  return 0;
}

/* *****************************************************************************
Streaming compression internals

Builds a combined buffer [window | new_data] and runs LZ77 on it.
The window prefix provides match context from previous messages.
Only new_data bytes produce output tokens; window bytes are skipped
(but their hash entries are inserted so matches can reference them).
***************************************************************************** */

FIO_SFUNC size_t fio___deflate_stream_compress(fio_deflate_s *s,
                                               void *out,
                                               size_t out_len,
                                               const uint8_t *new_data,
                                               size_t new_len) {
  if (!new_len && !out_len)
    return 0;

  const fio___deflate_level_params_s *params = &fio___deflate_levels[s->level];

  /* Build combined buffer: [window_prefix | new_data] */
  uint32_t prefix_len = s->window_pos;
  uint32_t total_len = prefix_len + (uint32_t)new_len;

  uint8_t *combined = NULL;
  size_t combined_alloc = 0;
  if (total_len) {
    combined_alloc = total_len;
    combined = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, combined_alloc, 0);
    if (!combined)
      return 0;
    if (prefix_len)
      FIO_MEMCPY(combined, s->window, prefix_len);
    if (new_len)
      FIO_MEMCPY(combined + prefix_len, new_data, new_len);
  }

  /* Advance generation counter instead of zeroing 256KB of hash tables.
   * Stale entries (gen mismatch) are treated as empty. */
  s->hash_gen++;
  uint32_t gen = s->hash_gen;
  uint32_t *head_gen = s->hash_head_gen;

  /* Insert hash entries for the window prefix (no tokens emitted).
   * This allows new_data matches to reference window content. */
  for (uint32_t i = 0; i + 3 < prefix_len; ++i) {
    uint32_t h = fio___deflate_hash4(combined + i);
    s->hash_prev[i & FIO___DEFLATE_WINDOW_MASK] =
        (head_gen[h] == gen) ? s->hash_head[h] : 0;
    s->hash_head[h] = (uint32_t)i;
    head_gen[h] = gen;
  }

  /* LZ77 pass: find matches starting from prefix_len */
  typedef struct {
    uint16_t litlen;
    uint16_t dist;
  } fio___deflate_token_s;

  size_t max_tokens = new_len + 1;
  size_t token_alloc =
      sizeof(fio___deflate_token_s) * (max_tokens ? max_tokens : 1);
  fio___deflate_token_s *tokens =
      (fio___deflate_token_s *)FIO_MEM_REALLOC(NULL, 0, token_alloc, 0);
  if (!tokens) {
    if (combined)
      FIO_MEM_FREE(combined, combined_alloc);
    return 0;
  }
  uint32_t token_count = 0;

  uint32_t ll_freqs[286];
  uint32_t d_freqs[30];
  FIO_MEMSET(ll_freqs, 0, sizeof(ll_freqs));
  FIO_MEMSET(d_freqs, 0, sizeof(d_freqs));

  uint32_t pos = prefix_len;
  uint32_t prev_match_len = 0;
  uint32_t prev_match_dist = 0;
  int prev_was_match = 0;

  while (pos < total_len) {
    uint32_t match_len = 0;
    uint32_t match_dist = 0;

    if (pos + 3 < total_len) {
      match_len = fio___deflate_find_match(combined,
                                           pos,
                                           total_len,
                                           s->hash_head,
                                           s->hash_prev,
                                           head_gen,
                                           gen,
                                           params->max_chain,
                                           params->nice_length,
                                           &match_dist);
    }

    /* Lazy matching */
    if (params->lazy && prev_was_match && match_len > prev_match_len) {
      tokens[token_count].litlen = combined[pos - 1];
      tokens[token_count].dist = 0;
      token_count++;
      ll_freqs[combined[pos - 1]]++;
      prev_was_match = 0;
    } else if (prev_was_match) {
      tokens[token_count].litlen = (uint16_t)prev_match_len;
      tokens[token_count].dist = (uint16_t)prev_match_dist;
      token_count++;
      uint32_t sym = fio___deflate_len_to_sym(prev_match_len);
      ll_freqs[sym]++;
      uint32_t dsym = fio___deflate_dist_to_sym(prev_match_dist);
      d_freqs[dsym]++;

      uint32_t skip_end = pos - 1 + prev_match_len;
      if (skip_end > total_len)
        skip_end = total_len;
      if (s->level >= 4) {
        /* Level 4+: full insertion (needed for good lazy matching) */
        for (uint32_t j = pos; j < skip_end && j + 3 < total_len; ++j) {
          uint32_t h = fio___deflate_hash4(combined + j);
          s->hash_prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? s->hash_head[h] : 0;
          s->hash_head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      } else if (s->level >= 2) {
        /* Level 2-3: sparse insertion every 4th position */
        for (uint32_t j = pos + 3; j < skip_end && j + 3 < total_len; j += 4) {
          uint32_t h = fio___deflate_hash4(combined + j);
          s->hash_prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? s->hash_head[h] : 0;
          s->hash_head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      }
      /* Level 1: skip all mid-match insertions (zlib-style) */
      pos = skip_end;
      prev_was_match = 0;
      continue;
    }

    /* Update hash chain */
    if (pos + 3 < total_len) {
      uint32_t h = fio___deflate_hash4(combined + pos);
      s->hash_prev[pos & FIO___DEFLATE_WINDOW_MASK] =
          (head_gen[h] == gen) ? s->hash_head[h] : 0;
      s->hash_head[h] = (uint32_t)pos;
      head_gen[h] = gen;
    }

    if (match_len >= 3) {
      if (params->lazy && match_len < params->max_lazy) {
        prev_match_len = match_len;
        prev_match_dist = match_dist;
        prev_was_match = 1;
        pos++;
        continue;
      }

      tokens[token_count].litlen = (uint16_t)match_len;
      tokens[token_count].dist = (uint16_t)match_dist;
      token_count++;
      uint32_t sym = fio___deflate_len_to_sym(match_len);
      ll_freqs[sym]++;
      uint32_t dsym = fio___deflate_dist_to_sym(match_dist);
      d_freqs[dsym]++;

      if (s->level >= 4) {
        /* Level 4+: full insertion (needed for good lazy matching) */
        for (uint32_t j = pos + 1; j < pos + match_len && j + 3 < total_len;
             ++j) {
          uint32_t h = fio___deflate_hash4(combined + j);
          s->hash_prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? s->hash_head[h] : 0;
          s->hash_head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      } else if (s->level >= 2) {
        /* Level 2-3: sparse insertion every 4th position */
        for (uint32_t j = pos + 4; j < pos + match_len && j + 3 < total_len;
             j += 4) {
          uint32_t h = fio___deflate_hash4(combined + j);
          s->hash_prev[j & FIO___DEFLATE_WINDOW_MASK] =
              (head_gen[h] == gen) ? s->hash_head[h] : 0;
          s->hash_head[h] = (uint32_t)j;
          head_gen[h] = gen;
        }
      }
      /* Level 1: skip all mid-match insertions (zlib-style) */
      pos += match_len;
    } else {
      tokens[token_count].litlen = combined[pos];
      tokens[token_count].dist = 0;
      token_count++;
      ll_freqs[combined[pos]]++;
      pos++;
    }
  }

  /* Flush pending lazy match */
  if (prev_was_match) {
    tokens[token_count].litlen = (uint16_t)prev_match_len;
    tokens[token_count].dist = (uint16_t)prev_match_dist;
    token_count++;
    uint32_t sym = fio___deflate_len_to_sym(prev_match_len);
    ll_freqs[sym]++;
    uint32_t dsym = fio___deflate_dist_to_sym(prev_match_dist);
    d_freqs[dsym]++;
  }

  /* Add end-of-block symbol */
  ll_freqs[256]++;

  /* Build Huffman trees */
  uint8_t ll_lens[286];
  uint8_t d_lens[30];
  fio___deflate_build_code_lengths(ll_lens, ll_freqs, 286, 15);
  fio___deflate_build_code_lengths(d_lens, d_freqs, 30, 15);

  /* Ensure at least one distance code */
  {
    int has_dist = 0;
    for (uint32_t i = 0; i < 30; ++i) {
      if (d_lens[i]) {
        has_dist = 1;
        break;
      }
    }
    if (!has_dist)
      d_lens[0] = 1;
  }
  if (ll_lens[256] == 0)
    ll_lens[256] = 1;

  uint32_t num_ll = 286;
  while (num_ll > 257 && ll_lens[num_ll - 1] == 0)
    --num_ll;
  uint32_t num_d = 30;
  while (num_d > 1 && d_lens[num_d - 1] == 0)
    --num_d;

  uint16_t ll_codes[286];
  uint16_t d_codes[30];
  fio___deflate_build_codes(ll_codes, ll_lens, 286);
  fio___deflate_build_codes(d_codes, d_lens, 30);

  /* ---- Cost comparison: fixed vs dynamic Huffman ---- */
  uint8_t fixed_ll_lens[288];
  uint8_t fixed_d_lens[30];
  fio___deflate_fixed_litlen_lens(fixed_ll_lens);
  for (uint32_t i = 0; i < 30; ++i)
    fixed_d_lens[i] = 5;

  uint64_t fixed_cost = 3;
  uint64_t dyn_cost = 3;

  for (uint32_t i = 0; i < 286; ++i) {
    if (ll_freqs[i]) {
      fixed_cost += (uint64_t)ll_freqs[i] * fixed_ll_lens[i];
      dyn_cost += (uint64_t)ll_freqs[i] * ll_lens[i];
    }
  }
  for (uint32_t i = 0; i < 30; ++i) {
    if (d_freqs[i]) {
      fixed_cost += (uint64_t)d_freqs[i] * fixed_d_lens[i];
      dyn_cost += (uint64_t)d_freqs[i] * d_lens[i];
    }
  }

  /* Dynamic header overhead */
  {
    uint8_t all_lens_tmp[286 + 30];
    uint32_t total = num_ll + num_d;
    FIO_MEMCPY(all_lens_tmp, ll_lens, num_ll);
    FIO_MEMCPY(all_lens_tmp + num_ll, d_lens, num_d);

    uint32_t cl_freqs_tmp[19];
    FIO_MEMSET(cl_freqs_tmp, 0, sizeof(cl_freqs_tmp));

    for (uint32_t i = 0; i < total;) {
      if (all_lens_tmp[i] == 0) {
        uint32_t run = 1;
        while (i + run < total && all_lens_tmp[i + run] == 0)
          ++run;
        while (run >= 11) {
          uint32_t r = run > 138 ? 138 : run;
          cl_freqs_tmp[18]++;
          run -= r;
          i += r;
        }
        while (run >= 3) {
          uint32_t r = run > 10 ? 10 : run;
          cl_freqs_tmp[17]++;
          run -= r;
          i += r;
        }
        while (run > 0) {
          cl_freqs_tmp[0]++;
          run--;
          i++;
        }
      } else {
        uint8_t val = all_lens_tmp[i];
        cl_freqs_tmp[val]++;
        i++;
        uint32_t run = 0;
        while (i + run < total && all_lens_tmp[i + run] == val)
          ++run;
        while (run >= 3) {
          uint32_t r = run > 6 ? 6 : run;
          cl_freqs_tmp[16]++;
          run -= r;
          i += r;
        }
        while (run > 0) {
          cl_freqs_tmp[val]++;
          run--;
          i++;
        }
      }
    }

    uint8_t cl_lens_tmp[19];
    fio___deflate_build_code_lengths(cl_lens_tmp, cl_freqs_tmp, 19, 7);

    uint32_t hclen_tmp = 19;
    while (hclen_tmp > 4 &&
           cl_lens_tmp[fio___deflate_codelen_order[hclen_tmp - 1]] == 0)
      --hclen_tmp;

    dyn_cost += 5 + 5 + 4;
    dyn_cost += (uint64_t)hclen_tmp * 3;

    for (uint32_t i = 0; i < 19; ++i) {
      if (cl_freqs_tmp[i])
        dyn_cost += (uint64_t)cl_freqs_tmp[i] * cl_lens_tmp[i];
    }
    dyn_cost += (uint64_t)cl_freqs_tmp[16] * 2;
    dyn_cost += (uint64_t)cl_freqs_tmp[17] * 3;
    dyn_cost += (uint64_t)cl_freqs_tmp[18] * 7;
  }

  int use_fixed = (fixed_cost <= dyn_cost);

  uint16_t fixed_ll_codes[288];
  uint16_t fixed_d_codes[30];
  if (use_fixed) {
    fio___deflate_build_codes(fixed_ll_codes, fixed_ll_lens, 288);
    fio___deflate_build_codes(fixed_d_codes, fixed_d_lens, 30);
  }

  const uint8_t *enc_ll_lens = use_fixed ? fixed_ll_lens : ll_lens;
  const uint16_t *enc_ll_codes = use_fixed ? fixed_ll_codes : ll_codes;
  const uint8_t *enc_d_lens = use_fixed ? fixed_d_lens : d_lens;
  const uint16_t *enc_d_codes = use_fixed ? fixed_d_codes : d_codes;

  /* Write compressed output */
  fio___deflate_bitwriter_s w;
  fio___deflate_bitwriter_init(&w, out, out_len);

  /* BFINAL=0 (non-final block — streaming continues) */
  fio___deflate_bitwriter_put(&w, 0, 1);
  if (use_fixed) {
    fio___deflate_bitwriter_put(&w, 1, 2); /* BTYPE=01 */
  } else {
    fio___deflate_bitwriter_put(&w, 2, 2); /* BTYPE=10 */
    fio___deflate_write_dynamic_header(&w,
                                       ll_lens,
                                       num_ll,
                                       ll_codes,
                                       d_lens,
                                       num_d,
                                       d_codes);
  }

  /* Write tokens */
  for (uint32_t i = 0; i < token_count; ++i) {
    if (tokens[i].dist == 0) {
      uint32_t sym = tokens[i].litlen;
      fio___deflate_bitwriter_put_huff(&w, enc_ll_codes[sym], enc_ll_lens[sym]);
    } else {
      uint32_t length = tokens[i].litlen;
      uint32_t distance = tokens[i].dist;
      uint32_t lsym = fio___deflate_len_to_sym(length);
      fio___deflate_bitwriter_put_huff(&w,
                                       enc_ll_codes[lsym],
                                       enc_ll_lens[lsym]);
      uint32_t lidx = lsym - 257;
      if (fio___deflate_len_extra[lidx])
        fio___deflate_bitwriter_put(&w,
                                    length - fio___deflate_len_base[lidx],
                                    fio___deflate_len_extra[lidx]);
      uint32_t dsym = fio___deflate_dist_to_sym(distance);
      fio___deflate_bitwriter_put_huff(&w, enc_d_codes[dsym], enc_d_lens[dsym]);
      if (fio___deflate_dist_extra[dsym])
        fio___deflate_bitwriter_put(&w,
                                    distance - fio___deflate_dist_base[dsym],
                                    fio___deflate_dist_extra[dsym]);
    }
  }

  /* End of block */
  fio___deflate_bitwriter_put_huff(&w, enc_ll_codes[256], enc_ll_lens[256]);

  /* Sync flush: byte-align then emit empty stored block (0x00 0x00 0xFF 0xFF)
   * Format: BFINAL=0, BTYPE=00 (stored), pad to byte, LEN=0, NLEN=0xFFFF */
  fio___deflate_bitwriter_align(&w);
  /* Empty stored block: BFINAL=0, BTYPE=00 = 0x00 byte, LEN=0, NLEN=0xFFFF */
  if (w.out + 5 <= w.out_end) {
    w.out[0] = 0x00; /* BFINAL=0, BTYPE=00, padding=000000 */
    w.out[1] = 0x00; /* LEN low */
    w.out[2] = 0x00; /* LEN high */
    w.out[3] = 0xFF; /* NLEN low */
    w.out[4] = 0xFF; /* NLEN high */
    w.out += 5;
  }

  size_t result = (size_t)(w.out - (uint8_t *)out);

  /* Update sliding window with the new data */
  if (new_len >= FIO___DEFLATE_WINDOW_SIZE) {
    FIO_MEMCPY(s->window,
               new_data + new_len - FIO___DEFLATE_WINDOW_SIZE,
               FIO___DEFLATE_WINDOW_SIZE);
    s->window_pos = FIO___DEFLATE_WINDOW_SIZE;
  } else if (new_len > 0) {
    if (s->window_pos + (uint32_t)new_len > FIO___DEFLATE_WINDOW_SIZE) {
      /* Shift: keep last (WINDOW_SIZE - new_len) bytes of window */
      uint32_t keep = FIO___DEFLATE_WINDOW_SIZE - (uint32_t)new_len;
      FIO_MEMMOVE(s->window, s->window + s->window_pos - keep, keep);
      s->window_pos = keep;
    }
    FIO_MEMCPY(s->window + s->window_pos, new_data, new_len);
    s->window_pos += (uint32_t)new_len;
  }

  FIO_MEM_FREE(tokens, token_alloc);
  if (combined)
    FIO_MEM_FREE(combined, combined_alloc);

  return result;
}

/* *****************************************************************************
Streaming decompression internals

Prepends the sliding window to the output buffer so that back-references
from the compressed stream resolve against data from previous messages.
Only the NEW decompressed bytes are returned to the caller.

Strategy: each flush produces exactly one compressed block (BFINAL=0) followed
by a sync flush marker. We copy the compressed data, set BFINAL=1 on the first
block, and use a modified inflate that starts writing after a pre-populated
window prefix (so back-references into previous messages resolve correctly).
***************************************************************************** */

/**
 * Inflate with pre-populated output prefix.
 *
 * `out[0..prefix_len-1]` must be pre-filled with window context.
 * Decompressed output is written starting at `out[prefix_len]`.
 * Back-references can reach into the prefix (prior message data).
 *
 * Returns:
 *  - On success: total bytes in `out` (prefix + new), <= out_len.
 *  - On buffer too small: the REQUIRED total size (> out_len).
 *  - On corrupt/invalid data: 0.
 */
FIO_SFUNC size_t fio___deflate_decompress_prefixed(void *out,
                                                   size_t out_len,
                                                   size_t prefix_len,
                                                   const void *in,
                                                   size_t in_len) {
  if (!out || !out_len || !in || !in_len)
    return 0;
  if (prefix_len >= out_len)
    return 0;

  const uint8_t *inp = (const uint8_t *)in;
  const uint8_t *in_end = inp + in_len;
  uint8_t *out_start = (uint8_t *)out;    /* includes window prefix */
  uint8_t *outp = out_start + prefix_len; /* write after prefix */
  uint8_t *out_end = out_start + out_len;

  int counting = 0;
  size_t out_pos = 0; /* virtual position for counting mode */

  fio___deflate_bitbuf_s bb = {0, 0};

  uint32_t litlen_table[FIO___DEFLATE_LITLEN_MAX];
  uint32_t dist_table[FIO___DEFLATE_DIST_MAX];

  uint32_t bfinal = 0;

  while (!bfinal) {
    while (bb.count < 3 && inp < in_end) {
      bb.bits |= (uint64_t)(*inp++) << bb.count;
      bb.count += 8;
    }
    if (bb.count < 3)
      return 0;

    bfinal = fio___deflate_bitbuf_read(&bb, 1);
    uint32_t btype = fio___deflate_bitbuf_read(&bb, 2);

    if (btype == 0) {
      bb.bits >>= (bb.count & 7);
      bb.count &= ~7U;

      while (bb.count < 32 && inp < in_end) {
        bb.bits |= (uint64_t)(*inp++) << bb.count;
        bb.count += 8;
      }
      if (bb.count < 32)
        return 0;

      uint32_t len = fio___deflate_bitbuf_read(&bb, 16);
      uint32_t nlen = fio___deflate_bitbuf_read(&bb, 16);

      if ((len ^ nlen) != 0xFFFF)
        return 0;

      uint32_t rewind_bytes = bb.count >> 3;
      inp -= rewind_bytes;
      bb.bits = 0;
      bb.count = 0;

      if (inp + len > in_end)
        return 0; /* truncated input */

      if (counting) {
        out_pos += len;
        inp += len;
      } else if (outp + len > out_end) {
        out_pos = (size_t)(outp - out_start) + len;
        counting = 1;
        inp += len;
      } else {
        FIO_MEMCPY(outp, inp, len);
        inp += len;
        outp += len;
      }
      continue;
    }

    if (btype == 3)
      return 0;

    if (btype == 1) {
      uint8_t ll_lens[288];
      uint8_t d_lens[32];
      fio___deflate_fixed_litlen_lens(ll_lens);
      fio___deflate_fixed_dist_lens(d_lens);

      if (!fio___deflate_build_decode_table(litlen_table,
                                            ll_lens,
                                            288,
                                            FIO___DEFLATE_LITLEN_BITS,
                                            1))
        return 0;
      if (!fio___deflate_build_decode_table(dist_table,
                                            d_lens,
                                            32,
                                            FIO___DEFLATE_DIST_BITS,
                                            2))
        return 0;
    } else {
      while (bb.count < 14 && inp < in_end) {
        bb.bits |= (uint64_t)(*inp++) << bb.count;
        bb.count += 8;
      }
      if (bb.count < 14)
        return 0;

      uint32_t hlit = fio___deflate_bitbuf_read(&bb, 5) + 257;
      uint32_t hdist = fio___deflate_bitbuf_read(&bb, 5) + 1;
      uint32_t hclen = fio___deflate_bitbuf_read(&bb, 4) + 4;

      if (hlit > 286 || hdist > 30)
        return 0;

      uint8_t cl_lens[19];
      FIO_MEMSET(cl_lens, 0, sizeof(cl_lens));

      for (uint32_t i = 0; i < hclen; ++i) {
        while (bb.count < 3 && inp < in_end) {
          bb.bits |= (uint64_t)(*inp++) << bb.count;
          bb.count += 8;
        }
        if (bb.count < 3)
          return 0;
        cl_lens[fio___deflate_codelen_order[i]] =
            (uint8_t)fio___deflate_bitbuf_read(&bb, 3);
      }

      uint32_t precode_table[FIO___DEFLATE_PRECODE_SIZE];
      if (!fio___deflate_build_decode_table(precode_table,
                                            cl_lens,
                                            19,
                                            FIO___DEFLATE_PRECODE_BITS,
                                            0))
        return 0;

      uint8_t all_lens[286 + 30];
      FIO_MEMSET(all_lens, 0, sizeof(all_lens));
      uint32_t total_codes = hlit + hdist;
      uint32_t idx = 0;

      while (idx < total_codes) {
        while (bb.count < 7 + 7 && inp < in_end) {
          bb.bits |= (uint64_t)(*inp++) << bb.count;
          bb.count += 8;
        }

        uint32_t pbits = fio___deflate_bitbuf_peek(
            &bb,
            FIO___DEFLATE_PRECODE_BITS < bb.count ? FIO___DEFLATE_PRECODE_BITS
                                                  : bb.count);
        uint32_t pentry =
            precode_table[pbits & (FIO___DEFLATE_PRECODE_SIZE - 1)];
        uint32_t plen = FIO___DEFLATE_ENTRY_CODELEN(pentry);
        uint32_t psym = FIO___DEFLATE_ENTRY_LIT_SYM(pentry);

        if (bb.count < plen)
          return 0;
        fio___deflate_bitbuf_consume(&bb, plen);

        if (psym < 16) {
          all_lens[idx++] = (uint8_t)psym;
        } else if (psym == 16) {
          if (bb.count < 2)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 2) + 3;
          if (idx == 0 || idx + rep > total_codes)
            return 0;
          uint8_t prev_val = all_lens[idx - 1];
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = prev_val;
        } else if (psym == 17) {
          if (bb.count < 3)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 3) + 3;
          if (idx + rep > total_codes)
            return 0;
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = 0;
        } else if (psym == 18) {
          if (bb.count < 7)
            return 0;
          uint32_t rep = fio___deflate_bitbuf_read(&bb, 7) + 11;
          if (idx + rep > total_codes)
            return 0;
          for (uint32_t r = 0; r < rep; ++r)
            all_lens[idx++] = 0;
        } else {
          return 0;
        }
      }

      if (!fio___deflate_build_decode_table(litlen_table,
                                            all_lens,
                                            hlit,
                                            FIO___DEFLATE_LITLEN_BITS,
                                            1))
        return 0;
      if (!fio___deflate_build_decode_table(dist_table,
                                            all_lens + hlit,
                                            hdist,
                                            FIO___DEFLATE_DIST_BITS,
                                            2))
        return 0;
    }

    /* Decode symbols — out_start includes the prefix for distance validation */
    if (counting) {
      /* Counting mode */
      for (;;) {
        int rc = fio___inflate_count(&inp,
                                     in_end,
                                     &out_pos,
                                     0,
                                     &bb,
                                     litlen_table,
                                     dist_table);
        if (rc == 1)
          break;
        if (rc < 0)
          return 0;
        return 0;
      }
    } else {
      int overflow = 0;
      for (;;) {
        const uint8_t *in_safe = in_end - 8;
        uint8_t *out_safe = out_end - (FIO___DEFLATE_MAX_MATCH + 8);

        int rc;
        if (inp < in_safe && outp < out_safe) {
          rc = fio___inflate_fast(&inp,
                                  in_safe,
                                  &outp,
                                  out_safe,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
        } else {
          rc = fio___inflate_slow(&inp,
                                  in_end,
                                  &outp,
                                  out_end,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
        }

        if (rc == 1)
          break;
        if (rc == -1)
          return 0;
        if (rc == -2) {
          out_pos = (size_t)(outp - out_start);
          counting = 1;
          overflow = 1;
          break;
        }

        if (inp >= in_safe || outp >= out_safe) {
          rc = fio___inflate_slow(&inp,
                                  in_end,
                                  &outp,
                                  out_end,
                                  out_start,
                                  &bb,
                                  litlen_table,
                                  dist_table);
          if (rc == 1)
            break;
          if (rc == -1)
            return 0;
          if (rc == -2) {
            out_pos = (size_t)(outp - out_start);
            counting = 1;
            overflow = 1;
            break;
          }
          return 0;
        }
      }
      if (overflow) {
        for (;;) {
          int rc = fio___inflate_count(&inp,
                                       in_end,
                                       &out_pos,
                                       0,
                                       &bb,
                                       litlen_table,
                                       dist_table);
          if (rc == 1)
            break;
          if (rc < 0)
            return 0;
          return 0;
        }
      }
    }
  }

  if (counting)
    return out_pos;
  return (size_t)(outp - out_start);
}

FIO_SFUNC size_t fio___deflate_stream_decompress(fio_deflate_s *s,
                                                 void *out,
                                                 size_t out_len,
                                                 const uint8_t *comp_data,
                                                 size_t comp_len) {
  if (!comp_len)
    return 0;

  /* Allocate temp buffer: [window_prefix | space_for_new_output] */
  uint32_t prefix_len = s->window_pos;
  size_t temp_len = prefix_len + out_len;
  /* Allocate temp output + modified input copy in one allocation */
  size_t alloc_len = temp_len + comp_len;
  uint8_t *alloc = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, alloc_len, 0);
  if (!alloc)
    return 0;

  uint8_t *temp = alloc;
  uint8_t *mod_in = alloc + temp_len;

  /* Copy window prefix into temp output buffer */
  if (prefix_len)
    FIO_MEMCPY(temp, s->window, prefix_len);

  /* Copy compressed data and set BFINAL=1 on the first block so the
   * one-shot decompressor stops after processing it. */
  FIO_MEMCPY(mod_in, comp_data, comp_len);
  mod_in[0] |= 0x01; /* set BFINAL bit (bit 0 of first byte) */

  /* Decompress with prefix: output starts after the window, but
   * back-references can reach into the window prefix. */
  size_t total_out = fio___deflate_decompress_prefixed(temp,
                                                       temp_len,
                                                       prefix_len,
                                                       mod_in,
                                                       comp_len);

  size_t new_bytes = 0;
  if (total_out > prefix_len)
    new_bytes = total_out - prefix_len;

  if (new_bytes > out_len) {
    /* Buffer too small — return required size, don't update window */
    FIO_MEM_FREE(alloc, alloc_len);
    return new_bytes;
  }

  if (new_bytes)
    FIO_MEMCPY(out, temp + prefix_len, new_bytes);

  /* Update sliding window with the last 32KB of ALL decompressed output */
  if (total_out > 0) {
    if (total_out >= FIO___DEFLATE_WINDOW_SIZE) {
      FIO_MEMCPY(s->window,
                 temp + total_out - FIO___DEFLATE_WINDOW_SIZE,
                 FIO___DEFLATE_WINDOW_SIZE);
      s->window_pos = FIO___DEFLATE_WINDOW_SIZE;
    } else {
      FIO_MEMCPY(s->window, temp, total_out);
      s->window_pos = (uint32_t)total_out;
    }
  }

  FIO_MEM_FREE(alloc, alloc_len);
  return new_bytes;
}

/* *****************************************************************************
Streaming API - public functions
***************************************************************************** */

void fio_deflate_new___(void); /* IDE Marker */
SFUNC fio_deflate_s *fio_deflate_new FIO_NOOP(int level, int is_compress) {
  if (level < 1)
    level = 1;
  if (level > 9)
    level = 9;

  size_t alloc_size = sizeof(fio_deflate_s);
  fio_deflate_s *s = (fio_deflate_s *)FIO_MEM_REALLOC(NULL, 0, alloc_size, 0);
  if (!s)
    return NULL;

  FIO_MEMSET(s, 0, alloc_size);
  s->is_compress = (uint8_t)(!!is_compress);
  s->level = (uint8_t)level;

  if (is_compress) {
    /* Allocate head[] + head_gen[] + prev[] in one block.
     * head_gen[] is zeroed; head[] and prev[] are left uninitialized
     * (generation counter makes stale entries harmless). */
    s->hash_alloc = sizeof(uint32_t) * FIO___DEFLATE_HASH_SIZE * 2 +
                    sizeof(uint32_t) * FIO___DEFLATE_WINDOW_SIZE;
    uint32_t *hash_mem = (uint32_t *)FIO_MEM_REALLOC(NULL, 0, s->hash_alloc, 0);
    if (!hash_mem) {
      FIO_MEM_FREE(s, alloc_size);
      return NULL;
    }
    s->hash_head = hash_mem;
    s->hash_head_gen = hash_mem + FIO___DEFLATE_HASH_SIZE;
    s->hash_prev = hash_mem + FIO___DEFLATE_HASH_SIZE * 2;
    FIO_MEMSET(s->hash_head_gen, 0, sizeof(uint32_t) * FIO___DEFLATE_HASH_SIZE);
    s->hash_gen = 0; /* first compress call will increment to 1 */
  }

  return s;
}

void fio_deflate_free___(void); /* IDE Marker */
SFUNC void fio_deflate_free FIO_NOOP(fio_deflate_s *s) {
  if (!s)
    return;
  if (s->hash_head)
    FIO_MEM_FREE(s->hash_head, s->hash_alloc);
  if (s->buf)
    FIO_MEM_FREE(s->buf, s->buf_cap);
  FIO_MEM_FREE(s, sizeof(fio_deflate_s));
}

void fio_deflate_destroy___(void); /* IDE Marker */
SFUNC void fio_deflate_destroy FIO_NOOP(fio_deflate_s *s) {
  if (!s)
    return;
  /* Reset sliding window */
  s->window_pos = 0;
  FIO_MEMSET(s->window, 0, FIO___DEFLATE_WINDOW_SIZE);
  /* Reset hash chain state via generation counter (compressor only).
   * Zeroing head_gen[] (128KB) invalidates all entries without touching
   * head[] (128KB) or prev[] (128KB). */
  if (s->hash_head_gen) {
    FIO_MEMSET(s->hash_head_gen, 0, sizeof(uint32_t) * FIO___DEFLATE_HASH_SIZE);
    s->hash_gen = 0;
  }
  /* Reset input buffer (keep allocation) */
  s->buf_len = 0;
}

void fio_deflate_push___(void); /* IDE Marker */
SFUNC size_t fio_deflate_push FIO_NOOP(fio_deflate_s *s,
                                       void *out,
                                       size_t out_len,
                                       const void *in,
                                       size_t in_len,
                                       int flush) {
  if (!s || !out || !out_len)
    return 0;

  if (s->is_compress) {
    /* Accumulate input */
    if (in && in_len) {
      if (fio___deflate_stream_buf_grow(s, in_len))
        return 0;
      FIO_MEMCPY(s->buf + s->buf_len, in, in_len);
      s->buf_len += in_len;
    }

    /* Compress on flush or when buffer is full */
    if (!flush && s->buf_len < FIO___DEFLATE_STREAM_BUF_MAX)
      return 0; /* buffered, no output yet */

    size_t result =
        fio___deflate_stream_compress(s, out, out_len, s->buf, s->buf_len);
    s->buf_len = 0; /* buffer consumed */
    return result;
  }

  /* Streaming decompression: accumulate compressed input */
  if (in && in_len) {
    if (fio___deflate_stream_buf_grow(s, in_len))
      return 0;
    FIO_MEMCPY(s->buf + s->buf_len, in, in_len);
    s->buf_len += in_len;
  }

  if (!flush)
    return 0; /* buffered, no output yet */

  /* On flush: append sync flush marker and decompress */
  if (fio___deflate_stream_buf_grow(s, 4))
    return 0;
  s->buf[s->buf_len] = 0x00;
  s->buf[s->buf_len + 1] = 0x00;
  s->buf[s->buf_len + 2] = 0xFF;
  s->buf[s->buf_len + 3] = 0xFF;
  size_t comp_len = s->buf_len + 4;

  size_t result =
      fio___deflate_stream_decompress(s, out, out_len, s->buf, comp_len);

  if (result <= out_len)
    s->buf_len = 0; /* buffer consumed on success or corrupt (0) */
  /* On overflow (result > out_len): keep buffer for retry with larger out */
  return result;
}

#undef FIO___DEFLATE_STREAM_BUF_MAX

/* *****************************************************************************
Cleanup
***************************************************************************** */

#undef FIO___DEFLATE_LITLEN_BITS
#undef FIO___DEFLATE_LITLEN_SIZE
#undef FIO___DEFLATE_DIST_BITS
#undef FIO___DEFLATE_DIST_SIZE
#undef FIO___DEFLATE_PRECODE_BITS
#undef FIO___DEFLATE_PRECODE_SIZE
#undef FIO___DEFLATE_LITLEN_MAX
#undef FIO___DEFLATE_DIST_MAX
#undef FIO___DEFLATE_WINDOW_SIZE
#undef FIO___DEFLATE_WINDOW_MASK
#undef FIO___DEFLATE_MAX_MATCH
#undef FIO___DEFLATE_HASH_BITS
#undef FIO___DEFLATE_HASH_SIZE
#undef FIO___DEFLATE_HASH_SHIFT

#undef FIO___DEFLATE_ENTRY_LIT
#undef FIO___DEFLATE_ENTRY_LEN
#undef FIO___DEFLATE_ENTRY_DIST
#undef FIO___DEFLATE_ENTRY_SUB
#undef FIO___DEFLATE_ENTRY_CODELEN
#undef FIO___DEFLATE_ENTRY_IS_LIT
#undef FIO___DEFLATE_ENTRY_LIT_SYM
#undef FIO___DEFLATE_ENTRY_IS_LEN
#undef FIO___DEFLATE_ENTRY_LEN_BASE
#undef FIO___DEFLATE_ENTRY_LEN_EXTRA
#undef FIO___DEFLATE_ENTRY_IS_SUB
#undef FIO___DEFLATE_ENTRY_SUB_OFF
#undef FIO___DEFLATE_ENTRY_SUB_BITS
#undef FIO___DEFLATE_ENTRY_DIST_BASE
#undef FIO___DEFLATE_ENTRY_DIST_EXTRA

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_DEFLATE */
