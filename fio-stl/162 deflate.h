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
 * Returns decompressed length on success, 0 on error.
 * Caller provides output buffer sized via fio_deflate_decompress_bound().
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
 * Returns decompressed length on success, 0 on error.
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

/**
 * Streaming compress/decompress.
 *
 * Processes `in_len` bytes from `in`, writing output to `out` (max `out_len`).
 * `flush`: 0=normal, 1=sync_flush (for WebSocket frame boundaries).
 *
 * Returns bytes written to `out`, or 0 on error.
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

/* Code length alphabet order (RFC 1951) */
static const uint8_t fio___deflate_codelen_order[19] =
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* *****************************************************************************
CRC32 (for gzip)
***************************************************************************** */

static const uint32_t fio___deflate_crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};

FIO_SFUNC uint32_t fio___deflate_crc32(const void *data, size_t len) {
  const uint8_t *p = (const uint8_t *)data;
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0; i < len; ++i)
    crc = fio___deflate_crc32_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFU;
}

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

/** Write Huffman code (bit-reversed canonical code). */
FIO_IFUNC void fio___deflate_bitwriter_put_huff(fio___deflate_bitwriter_s *w,
                                                uint32_t code,
                                                uint32_t nbits) {
  /* Huffman codes are MSB-first but bitstream is LSB-first, so reverse */
  uint32_t rev = 0;
  for (uint32_t i = 0; i < nbits; ++i)
    rev |= ((code >> (nbits - 1 - i)) & 1) << i;
  fio___deflate_bitwriter_put(w, rev, nbits);
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
      if (out >= out_end)
        break;
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
    if (out + length > out_end)
      return -1;

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
  if (!out || !out_len || !in || !in_len)
    return 0;

  const uint8_t *inp = (const uint8_t *)in;
  const uint8_t *in_end = inp + in_len;
  uint8_t *outp = (uint8_t *)out;
  uint8_t *out_end = outp + out_len;
  uint8_t *out_start = outp;

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

      if (inp + len > in_end || outp + len > out_end)
        return 0;

      FIO_MEMCPY(outp, inp, len);
      inp += len;
      outp += len;
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

    /* Decode symbols using fast path when possible, slow path at boundaries */
    for (;;) {
      /* Fast path: need 8 bytes input margin and 258+8 bytes output margin */
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
      if (rc < 0)
        return 0; /* error */

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
        if (rc < 0)
          return 0;
        /* If slow path also returned 0, we need more data - error for
         * one-shot */
        return 0;
      }
    }
  }

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

      /* Over-subscribed: increase longest codes */
      /* Find a code at max_bits and try to shorten it */
      int fixed = 0;
      for (uint32_t i = max_bits; i > 1 && !fixed; --i) {
        if (count[i] > 0) {
          count[i]--;
          count[i - 1]++;
          /* Find a symbol with this length and shorten it */
          for (uint32_t s = 0; s < num_syms; ++s) {
            if (lens[s] == i) {
              lens[s]--;
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
 *  Returns codes in `codes` array (bit-reversed for LSB-first bitstream). */
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

  /* Assign codes */
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (lens[i]) {
      codes[i] = (uint16_t)next_code[lens[i]]++;
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

/** Hash function for 3-byte sequences */
FIO_IFUNC uint32_t fio___deflate_hash3(const uint8_t *p) {
  uint32_t h = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
  h = (h * 0x1E35A7BDU) >> (32 - FIO___DEFLATE_HASH_BITS);
  return h;
}

/** Find best match at current position.
 *  Returns match length (0 if no match found >= 3). */
FIO_SFUNC uint32_t fio___deflate_find_match(const uint8_t *src,
                                            uint32_t pos,
                                            uint32_t src_len,
                                            const uint16_t *head,
                                            const uint16_t *prev,
                                            uint32_t max_chain,
                                            uint32_t nice_length,
                                            uint32_t *match_dist) {
  uint32_t best_len = 2; /* minimum match is 3 */
  uint32_t best_dist = 0;
  uint32_t max_len = src_len - pos;
  if (max_len > FIO___DEFLATE_MAX_MATCH)
    max_len = FIO___DEFLATE_MAX_MATCH;
  if (max_len < 3)
    return 0;

  uint32_t hash = fio___deflate_hash3(src + pos);
  uint32_t chain_pos = head[hash];
  uint32_t chain_count = 0;

  while (chain_pos != 0 && chain_count < max_chain) {
    uint32_t dist = pos - chain_pos;
    if (dist == 0 || dist > FIO___DEFLATE_WINDOW_SIZE)
      break;

    /* Quick check: compare last byte of current best + first bytes */
    if (src[chain_pos + best_len] == src[pos + best_len] &&
        src[chain_pos] == src[pos] && src[chain_pos + 1] == src[pos + 1]) {
      /* Full comparison */
      uint32_t len = 2;
      while (len < max_len && src[chain_pos + len] == src[pos + len])
        ++len;

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

/** Find the length code index for a given length value. */
FIO_IFUNC uint32_t fio___deflate_len_to_sym(uint32_t length) {
  /* Binary search in length base table */
  uint32_t lo = 0, hi = 28;
  while (lo < hi) {
    uint32_t mid = (lo + hi + 1) >> 1;
    if (fio___deflate_len_base[mid] <= length)
      lo = mid;
    else
      hi = mid - 1;
  }
  return lo + 257;
}

/** Find the distance code index for a given distance value. */
FIO_IFUNC uint32_t fio___deflate_dist_to_sym(uint32_t distance) {
  uint32_t lo = 0, hi = 29;
  while (lo < hi) {
    uint32_t mid = (lo + hi + 1) >> 1;
    if (fio___deflate_dist_base[mid] <= distance)
      lo = mid;
    else
      hi = mid - 1;
  }
  return lo;
}

/** Upper bound on compressed output size. */
FIO_IFUNC size_t fio_deflate_compress_bound(size_t in_len) {
  /* Stored blocks: 5 bytes header per 65535 bytes + data */
  size_t num_blocks = (in_len + 65534) / 65535;
  return in_len + num_blocks * 5 + 16;
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

  /* Allocate hash tables on heap for large sizes */
  size_t alloc_size = sizeof(uint16_t) * FIO___DEFLATE_HASH_SIZE +
                      sizeof(uint16_t) * FIO___DEFLATE_WINDOW_SIZE;
  uint16_t *hash_mem = (uint16_t *)FIO_MEM_REALLOC(NULL, 0, alloc_size, 0);
  if (!hash_mem)
    return 0;

  uint16_t *head = hash_mem;
  uint16_t *prev = hash_mem + FIO___DEFLATE_HASH_SIZE;
  FIO_MEMSET(head, 0, sizeof(uint16_t) * FIO___DEFLATE_HASH_SIZE);
  FIO_MEMSET(prev, 0, sizeof(uint16_t) * FIO___DEFLATE_WINDOW_SIZE);

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

    if (pos + 2 < src_len) {
      match_len = fio___deflate_find_match(src,
                                           pos,
                                           src_len,
                                           head,
                                           prev,
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

      /* Update hash for skipped positions */
      uint32_t skip_end = pos - 1 + prev_match_len;
      if (skip_end > src_len)
        skip_end = src_len;
      for (uint32_t j = pos; j < skip_end && j + 2 < src_len; ++j) {
        uint32_t h = fio___deflate_hash3(src + j);
        prev[j & FIO___DEFLATE_WINDOW_MASK] = head[h];
        head[h] = (uint16_t)j;
      }
      pos = skip_end;
      prev_was_match = 0;
      continue;
    }

    /* Update hash chain */
    if (pos + 2 < src_len) {
      uint32_t h = fio___deflate_hash3(src + pos);
      prev[pos & FIO___DEFLATE_WINDOW_MASK] = head[h];
      head[h] = (uint16_t)pos;
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

      /* Update hash for match positions */
      for (uint32_t j = pos + 1; j < pos + match_len && j + 2 < src_len; ++j) {
        uint32_t h = fio___deflate_hash3(src + j);
        prev[j & FIO___DEFLATE_WINDOW_MASK] = head[h];
        head[h] = (uint16_t)j;
      }
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

  uint16_t ll_codes[286];
  uint16_t d_codes[30];
  fio___deflate_build_codes(ll_codes, ll_lens, 286);
  fio___deflate_build_codes(d_codes, d_lens, 30);

  /* Determine actual number of litlen and distance codes to emit */
  uint32_t num_ll = 286;
  while (num_ll > 257 && ll_lens[num_ll - 1] == 0)
    --num_ll;
  uint32_t num_d = 30;
  while (num_d > 1 && d_lens[num_d - 1] == 0)
    --num_d;

  /* Write compressed output */
  fio___deflate_bitwriter_s w;
  fio___deflate_bitwriter_init(&w, out, out_len);

  /* Block header: BFINAL=1, BTYPE=10 (dynamic) */
  fio___deflate_bitwriter_put(&w, 1, 1); /* BFINAL */
  fio___deflate_bitwriter_put(&w, 2, 2); /* BTYPE = dynamic */

  /* Write dynamic Huffman header */
  fio___deflate_write_dynamic_header(&w,
                                     ll_lens,
                                     num_ll,
                                     ll_codes,
                                     d_lens,
                                     num_d,
                                     d_codes);

  /* Write tokens */
  for (uint32_t i = 0; i < token_count; ++i) {
    if (tokens[i].dist == 0) {
      /* Literal */
      uint32_t sym = tokens[i].litlen;
      fio___deflate_bitwriter_put_huff(&w, ll_codes[sym], ll_lens[sym]);
    } else {
      /* Length/distance pair */
      uint32_t length = tokens[i].litlen;
      uint32_t distance = tokens[i].dist;

      /* Emit length code */
      uint32_t lsym = fio___deflate_len_to_sym(length);
      fio___deflate_bitwriter_put_huff(&w, ll_codes[lsym], ll_lens[lsym]);

      /* Emit length extra bits */
      uint32_t lidx = lsym - 257;
      if (fio___deflate_len_extra[lidx])
        fio___deflate_bitwriter_put(&w,
                                    length - fio___deflate_len_base[lidx],
                                    fio___deflate_len_extra[lidx]);

      /* Emit distance code */
      uint32_t dsym = fio___deflate_dist_to_sym(distance);
      fio___deflate_bitwriter_put_huff(&w, d_codes[dsym], d_lens[dsym]);

      /* Emit distance extra bits */
      if (fio___deflate_dist_extra[dsym])
        fio___deflate_bitwriter_put(&w,
                                    distance - fio___deflate_dist_base[dsym],
                                    fio___deflate_dist_extra[dsym]);
    }
  }

  /* End of block */
  fio___deflate_bitwriter_put_huff(&w, ll_codes[256], ll_lens[256]);

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
  uint32_t crc = fio___deflate_crc32(in, in_len);
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
  if (!out || !out_len || !in || in_len < 18)
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

  /* Decompress */
  size_t decompressed_len =
      fio_deflate_decompress(out, out_len, p + hdr_len, compressed_len);
  if (!decompressed_len && compressed_len > 5) /* allow empty */
    return 0;

  /* Verify CRC32 */
  uint32_t expected_crc = fio_buf2u32_le(trailer);
  uint32_t actual_crc = fio___deflate_crc32(out, decompressed_len);
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
***************************************************************************** */

struct fio_deflate_s {
  int is_compress;
  int level;
  /* Sliding window for decompression context takeover */
  uint32_t window_pos;
  uint8_t window[FIO___DEFLATE_WINDOW_SIZE];
  /* Compression state */
  uint16_t *hash_head;
  uint16_t *hash_prev;
  size_t hash_alloc;
};

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
  s->is_compress = is_compress;
  s->level = level;

  if (is_compress) {
    s->hash_alloc = sizeof(uint16_t) * FIO___DEFLATE_HASH_SIZE +
                    sizeof(uint16_t) * FIO___DEFLATE_WINDOW_SIZE;
    uint16_t *hash_mem = (uint16_t *)FIO_MEM_REALLOC(NULL, 0, s->hash_alloc, 0);
    if (!hash_mem) {
      FIO_MEM_FREE(s, alloc_size);
      return NULL;
    }
    FIO_MEMSET(hash_mem, 0, s->hash_alloc);
    s->hash_head = hash_mem;
    s->hash_prev = hash_mem + FIO___DEFLATE_HASH_SIZE;
  }

  return s;
}

void fio_deflate_free___(void); /* IDE Marker */
SFUNC void fio_deflate_free FIO_NOOP(fio_deflate_s *s) {
  if (!s)
    return;
  if (s->hash_head)
    FIO_MEM_FREE(s->hash_head, s->hash_alloc);
  FIO_MEM_FREE(s, sizeof(fio_deflate_s));
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
    /* Streaming compression: compress data, optionally add sync flush */
    size_t result = fio_deflate_compress(out, out_len, in, in_len, s->level);
    if (!result && in_len > 0)
      return 0;

    /* For WebSocket: if flush requested, the output already ends with
     * a complete block. The caller should strip the 0x00 0x00 0xFF 0xFF
     * sync marker if present. */
    (void)flush;
    return result;
  }

  /* Streaming decompression */
  if (!in || !in_len)
    return 0;

  /* For WebSocket permessage-deflate: append sync flush marker */
  size_t total_in = in_len;
  uint8_t *temp_in = NULL;
  const void *actual_in = in;

  if (flush) {
    /* Append 0x00 0x00 0xFF 0xFF */
    total_in = in_len + 4;
    temp_in = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total_in, 0);
    if (!temp_in)
      return 0;
    FIO_MEMCPY(temp_in, in, in_len);
    temp_in[in_len] = 0x00;
    temp_in[in_len + 1] = 0x00;
    temp_in[in_len + 2] = 0xFF;
    temp_in[in_len + 3] = 0xFF;
    actual_in = temp_in;
  }

  size_t result = fio_deflate_decompress(out, out_len, actual_in, total_in);

  /* Update sliding window for context takeover */
  if (result > 0) {
    const uint8_t *outp = (const uint8_t *)out;
    if (result >= FIO___DEFLATE_WINDOW_SIZE) {
      FIO_MEMCPY(s->window,
                 outp + result - FIO___DEFLATE_WINDOW_SIZE,
                 FIO___DEFLATE_WINDOW_SIZE);
      s->window_pos = FIO___DEFLATE_WINDOW_SIZE;
    } else {
      size_t copy_len = result;
      if (s->window_pos + copy_len > FIO___DEFLATE_WINDOW_SIZE) {
        /* Shift window */
        size_t keep = FIO___DEFLATE_WINDOW_SIZE - copy_len;
        FIO_MEMMOVE(s->window, s->window + s->window_pos - keep, keep);
        s->window_pos = (uint32_t)keep;
      }
      FIO_MEMCPY(s->window + s->window_pos, outp, copy_len);
      s->window_pos += (uint32_t)copy_len;
    }
  }

  if (temp_in)
    FIO_MEM_FREE(temp_in, total_in);

  return result;
}

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
