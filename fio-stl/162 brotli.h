/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_BROTLI             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            BROTLI DECOMPRESSION
                    RFC 7932 Brotli Compressed Data Format


Provides Brotli decompression (decode only).

Key design decisions:
- 64-bit branchless bit buffer matching the deflate module pattern
- Two-level packed Huffman tables with 8-bit first level
- 122,784-byte static dictionary embedded for RFC 7932 compliance
- 121 transforms with prefix/suffix string table
- Context-dependent literal decoding with 4 context modes

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_BROTLI) && !defined(H___FIO_BROTLI___H)
#define H___FIO_BROTLI___H

/* *****************************************************************************
BROTLI API
***************************************************************************** */

/**
 * Decompresses Brotli-compressed data (RFC 7932).
 *
 * Returns:
 *  - On success: decompressed byte count (<= out_len).
 *  - On buffer too small: the REQUIRED buffer size (> out_len).
 *  - On corrupt/invalid data: 0.
 *  - When out==NULL or out_len==0: performs a header scan summing MLEN
 *    values and returns the required size (0 if data is corrupt).
 *    This allows callers to query the decompressed size without allocating.
 */
SFUNC size_t fio_brotli_decompress(void *out,
                                   size_t out_len,
                                   const void *in,
                                   size_t in_len);

/** Conservative upper bound on decompressed size for a Brotli stream. */
FIO_IFUNC size_t fio_brotli_decompress_bound(size_t in_len);

/**
 * Compresses data using Brotli (RFC 7932).
 *
 * Returns compressed length on success, 0 on error.
 * quality: 1-6 (1=fast greedy, 3-4=lazy matching, 5-6=hash chain + context
 * modeling + static dictionary). Caller provides output buffer sized via
 * fio_brotli_compress_bound().
 */
SFUNC size_t fio_brotli_compress(void *out,
                                 size_t out_len,
                                 const void *in,
                                 size_t in_len,
                                 int quality);

/** Upper bound on Brotli compressed output size. */
FIO_IFUNC size_t fio_brotli_compress_bound(size_t in_len);

/* *****************************************************************************
Inline implementations
***************************************************************************** */

/** Conservative decompression bound - Brotli can expand up to ~1032x. */
FIO_IFUNC size_t fio_brotli_decompress_bound(size_t in_len) {
  /* Brotli theoretical max is huge; use practical bound */
  if (in_len > ((size_t)1 << 30))
    return (size_t)1 << 32; /* 4GB cap */
  return in_len * 1032 + 1024;
}

/** Compression bound (generous, for future encoder). */
FIO_IFUNC size_t fio_brotli_compress_bound(size_t in_len) {
  return in_len + (in_len >> 2) + 1024;
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Constants and configuration
***************************************************************************** */

/* Huffman table dimensions - 8-bit first level for all code types */
#define FIO___BROTLI_HUFF_BITS 8
#define FIO___BROTLI_HUFF_SIZE (1U << FIO___BROTLI_HUFF_BITS)
#define FIO___BROTLI_HUFF_MASK (FIO___BROTLI_HUFF_SIZE - 1)

/* Maximum code lengths per RFC 7932 */
#define FIO___BROTLI_MAX_CODE_LEN 15
#define FIO___BROTLI_MAX_CL_LEN   5

/* Alphabet sizes */
#define FIO___BROTLI_MAX_LITERAL 256
#define FIO___BROTLI_MAX_IACLEN  704 /* insert-and-copy length */
#define FIO___BROTLI_MAX_DIST                                                  \
  544 /* max distance alphabet (NPOSTFIX=3,NDIRECT=120) */
#define FIO___BROTLI_MAX_BLOCK_TYPE 258 /* 256 types + 2 special */

/* Maximum window size: 2^24 = 16MB */
#define FIO___BROTLI_MAX_WBITS  24
#define FIO___BROTLI_MAX_WINDOW (1U << FIO___BROTLI_MAX_WBITS)

/* Context modes */
#define FIO___BROTLI_CONTEXT_LSB6   0
#define FIO___BROTLI_CONTEXT_MSB6   1
#define FIO___BROTLI_CONTEXT_UTF8   2
#define FIO___BROTLI_CONTEXT_SIGNED 3

/* Number of literal contexts per block type */
#define FIO___BROTLI_LITERAL_CONTEXTS 64
/* Number of distance contexts per block type */
#define FIO___BROTLI_DISTANCE_CONTEXTS 4

/* Transform types */
#define FIO___BROTLI_TRANSFORM_IDENTITY        0
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_1     1
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_2     2
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_3     3
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_4     4
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_5     5
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_6     6
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_7     7
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_8     8
#define FIO___BROTLI_TRANSFORM_OMIT_LAST_9     9
#define FIO___BROTLI_TRANSFORM_UPPERCASE_FIRST 10
#define FIO___BROTLI_TRANSFORM_UPPERCASE_ALL   11
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_1    12
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_2    13
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_3    14
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_4    15
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_5    16
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_6    17
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_7    18
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_8    19
#define FIO___BROTLI_TRANSFORM_OMIT_FIRST_9    20

/* Number of transforms */
#define FIO___BROTLI_NUM_TRANSFORMS 121

/* Dictionary word counts per length (lengths 4-24) */
/* ndbits[len] gives log2(number of words of that length) */

/*
 * Packed Huffman table entry format (32-bit):
 *
 * For direct symbols (bit 31 clear):
 *   bits [0..7]   = code length (bits to consume)
 *   bits [8..23]  = symbol value
 *
 * For subtable pointers (bit 31 set):
 *   bits [0..7]   = root table bits consumed
 *   bits [8..11]  = subtable bits
 *   bits [12..30] = subtable offset from table start
 */
#define FIO___BROTLI_ENTRY_SYM(sym, len)                                       \
  ((uint32_t)(len) | ((uint32_t)(sym) << 8))
#define FIO___BROTLI_ENTRY_SUB(offset, root_bits, sub_bits)                    \
  (0x80000000U | (uint32_t)(root_bits) | ((uint32_t)(sub_bits) << 8) |         \
   ((uint32_t)(offset) << 12))

#define FIO___BROTLI_ENTRY_CODELEN(e)  ((e)&0xFFU)
#define FIO___BROTLI_ENTRY_IS_SYM(e)   (!((e)&0x80000000U))
#define FIO___BROTLI_ENTRY_SYMBOL(e)   (((e) >> 8) & 0xFFFFU)
#define FIO___BROTLI_ENTRY_IS_SUB(e)   ((e)&0x80000000U)
#define FIO___BROTLI_ENTRY_SUB_OFF(e)  (((e) >> 12) & 0x7FFFFU)
#define FIO___BROTLI_ENTRY_SUB_BITS(e) (((e) >> 8) & 0xFU)

/* *****************************************************************************
Bit reader - 64-bit buffer, LSB-first
***************************************************************************** */

typedef struct {
  uint64_t bits;           /* bit buffer */
  unsigned avail;          /* bits available in buffer */
  const uint8_t *src;      /* current read position */
  const uint8_t *end;      /* end of input */
  const uint8_t *safe_end; /* end - 8: safe for 8-byte unaligned load */
} fio___brotli_bits_s;

FIO_SFUNC void fio___brotli_bits_init(fio___brotli_bits_s *b,
                                      const void *data,
                                      size_t len) {
  b->bits = 0;
  b->avail = 0;
  b->src = (const uint8_t *)data;
  b->end = b->src + len;
  b->safe_end = (len >= 8) ? (b->end - 8) : b->src - 1;
}

FIO_IFUNC void fio___brotli_bits_fill(fio___brotli_bits_s *b) {
  if (b->src <= b->safe_end) {
    /* Branchless bulk refill: single 8-byte unaligned load */
    b->bits |= fio_buf2u64_le(b->src) << b->avail;
    unsigned consumed = (63 - b->avail) >> 3;
    b->src += consumed;
    b->avail |= 56; /* equivalent to: avail += consumed << 3 */
  } else {
    /* Byte-at-a-time near end of input */
    while (b->avail <= 56 && b->src < b->end) {
      b->bits |= (uint64_t)(*(b->src++)) << b->avail;
      b->avail += 8;
    }
  }
}

FIO_IFUNC uint32_t fio___brotli_bits_peek(fio___brotli_bits_s *b, unsigned n) {
  return (uint32_t)(b->bits & ((1ULL << n) - 1));
}

FIO_IFUNC void fio___brotli_bits_drop(fio___brotli_bits_s *b, unsigned n) {
  b->bits >>= n;
  b->avail -= n;
}

FIO_IFUNC uint32_t fio___brotli_bits_get(fio___brotli_bits_s *b, unsigned n) {
  uint32_t v;
  if (!n)
    return 0;
  fio___brotli_bits_fill(b);
  if (n > b->avail)
    return 0; /* input exhausted */
  v = fio___brotli_bits_peek(b, n);
  fio___brotli_bits_drop(b, n);
  return v;
}

/* Read a single bit */
FIO_IFUNC uint32_t fio___brotli_bits_get1(fio___brotli_bits_s *b) {
  fio___brotli_bits_fill(b);
  if (b->avail == 0)
    return 0; /* input exhausted */
  uint32_t v = (uint32_t)(b->bits & 1);
  b->bits >>= 1;
  b->avail -= 1;
  return v;
}

/* *****************************************************************************
Huffman table builder - builds two-level lookup tables
***************************************************************************** */

/**
 * Reverse the bottom `n` bits of `code`.
 * Used to convert canonical Huffman codes (MSB-first) to LSB-first lookup keys.
 */
FIO_IFUNC uint32_t fio___brotli_bit_reverse(uint32_t code, unsigned n) {
  uint32_t r = 0;
  for (unsigned i = 0; i < n; i++) {
    r = (r << 1) | (code & 1);
    code >>= 1;
  }
  return r;
}

/**
 * Build a two-level Huffman lookup table from code lengths.
 *
 * table:     output table (must have enough space)
 * root_bits: bits for first-level table (typically 8)
 * lengths:   code length for each symbol (0 = unused)
 * num_syms:  number of symbols
 *
 * Returns total table size (entries used), or 0 on error.
 */
FIO_SFUNC uint32_t fio___brotli_build_table(uint32_t *table,
                                            unsigned root_bits,
                                            const uint8_t *lengths,
                                            unsigned num_syms) {
  uint16_t count[FIO___BROTLI_MAX_CODE_LEN + 1] = {0};
  uint16_t offsets[FIO___BROTLI_MAX_CODE_LEN + 1];
  uint16_t sorted[FIO___BROTLI_MAX_IACLEN]; /* max alphabet */
  unsigned max_len = 0;
  unsigned root_size = 1U << root_bits;
  uint32_t total = root_size;

  /* Count code lengths */
  for (unsigned i = 0; i < num_syms; i++) {
    if (lengths[i]) {
      if (lengths[i] > FIO___BROTLI_MAX_CODE_LEN)
        return 0; /* invalid: code length exceeds RFC 7932 maximum */
      count[lengths[i]]++;
      if (lengths[i] > max_len)
        max_len = lengths[i];
    }
  }

  /* Handle special cases */
  if (max_len == 0) {
    /* All zero - fill with symbol 0 */
    for (unsigned i = 0; i < root_size; i++)
      table[i] = FIO___BROTLI_ENTRY_SYM(0, 0);
    return root_size;
  }

  /* Build offsets for sorting */
  offsets[1] = 0;
  for (unsigned i = 2; i <= max_len; i++)
    offsets[i] = offsets[i - 1] + count[i - 1];

  /* Sort symbols by code length, then by symbol value */
  for (unsigned i = 0; i < num_syms; i++) {
    if (lengths[i])
      sorted[offsets[lengths[i]]++] = (uint16_t)i;
  }

  /* Rebuild offsets */
  offsets[1] = 0;
  for (unsigned i = 2; i <= max_len; i++)
    offsets[i] = offsets[i - 1] + count[i - 1];

  /* Assign canonical codes and fill first-level table */
  {
    uint32_t code = 0;
    unsigned sym_idx = 0;

    for (unsigned len = 1; len <= max_len && len <= root_bits; len++) {
      for (unsigned c = 0; c < count[len]; c++) {
        uint16_t sym = sorted[sym_idx++];
        uint32_t entry = FIO___BROTLI_ENTRY_SYM(sym, len);
        /* Fill all slots that match this code (stride = 1 << len) */
        uint32_t slot = fio___brotli_bit_reverse(code, len);
        uint32_t stride = 1U << len;
        for (uint32_t s = slot; s < root_size; s += stride)
          table[s] = entry;
        code++;
      }
      code <<= 1;
    }

    /* Build second-level subtables for codes longer than root_bits */
    if (max_len > root_bits) {
      /* Reset code to where we left off */
      code = 0;
      sym_idx = 0;
      for (unsigned len = 1; len <= root_bits; len++) {
        code = (code + count[len]) << 1;
        sym_idx += count[len];
      }

      int32_t prev_root_slot = -1;
      uint32_t sub_offset = root_size;
      unsigned sub_bits = 0;

      for (unsigned len = root_bits + 1; len <= max_len; len++) {
        for (unsigned c = 0; c < count[len]; c++) {
          uint16_t sym = sorted[sym_idx++];
          /* Root slot is determined by lower root_bits of reversed code */
          uint32_t rev_code = fio___brotli_bit_reverse(code, len);
          uint32_t root_slot = rev_code & (root_size - 1);

          if ((int32_t)root_slot != prev_root_slot) {
            /* New subtable needed */
            /* Determine subtable size: find max code length sharing this root
             */
            unsigned max_sub_len = 0;
            uint32_t test_code = code;
            for (unsigned tl = len; tl <= max_len; tl++) {
              unsigned tc = (tl == len) ? (count[tl] - c) : count[tl];
              for (unsigned ti = 0; ti < tc; ti++) {
                uint32_t trev = fio___brotli_bit_reverse(test_code, tl);
                if ((trev & (root_size - 1)) == root_slot) {
                  if (tl > max_sub_len)
                    max_sub_len = tl;
                }
                test_code++;
              }
              test_code <<= 1;
            }
            sub_bits = max_sub_len - root_bits;
            if (sub_bits > 15)
              sub_bits = 15; /* safety clamp */

            /* Write root entry pointing to subtable */
            table[root_slot] =
                FIO___BROTLI_ENTRY_SUB(sub_offset, root_bits, sub_bits);
            prev_root_slot = (int32_t)root_slot;

            /* Clear subtable */
            uint32_t sub_size = 1U << sub_bits;
            FIO_MEMSET(table + sub_offset, 0, sub_size * sizeof(uint32_t));
            total = sub_offset + sub_size;
            sub_offset = total;
          }

          /* Fill subtable entry */
          uint32_t sub_code = rev_code >> root_bits;
          uint32_t sub_size = 1U << sub_bits;
          uint32_t entry = FIO___BROTLI_ENTRY_SYM(sym, len);
          uint32_t sub_stride = 1U << (len - root_bits);
          uint32_t base_off = FIO___BROTLI_ENTRY_SUB_OFF(table[root_slot]);
          for (uint32_t s = sub_code; s < sub_size; s += sub_stride)
            table[base_off + s] = entry;

          code++;
        }
        code <<= 1;
      }
    }
  }

  return total;
}

/**
 * Decode one symbol from a Huffman table.
 */
FIO_IFUNC uint32_t fio___brotli_huff_decode(fio___brotli_bits_s *b,
                                            const uint32_t *table,
                                            unsigned root_bits) {
  fio___brotli_bits_fill(b);
  uint32_t idx = fio___brotli_bits_peek(b, root_bits);
  uint32_t entry = table[idx];

  if (FIO___BROTLI_ENTRY_IS_SYM(entry)) {
    fio___brotli_bits_drop(b, FIO___BROTLI_ENTRY_CODELEN(entry));
    return FIO___BROTLI_ENTRY_SYMBOL(entry);
  }

  /* Subtable lookup */
  fio___brotli_bits_drop(b, root_bits);
  fio___brotli_bits_fill(b);
  unsigned sub_bits = FIO___BROTLI_ENTRY_SUB_BITS(entry);
  uint32_t sub_off = FIO___BROTLI_ENTRY_SUB_OFF(entry);
  uint32_t sub_idx = fio___brotli_bits_peek(b, sub_bits);
  entry = table[sub_off + sub_idx];
  fio___brotli_bits_drop(b, FIO___BROTLI_ENTRY_CODELEN(entry) - root_bits);
  return FIO___BROTLI_ENTRY_SYMBOL(entry);
}

/* *****************************************************************************
Prefix code reading (RFC 7932 Section 3.4-3.5)
***************************************************************************** */

/**
 * Read a prefix code definition and build the Huffman table.
 * Returns table size used, or 0 on error.
 */
FIO_SFUNC uint32_t fio___brotli_read_prefix_code(fio___brotli_bits_s *b,
                                                 uint32_t *table,
                                                 unsigned root_bits,
                                                 unsigned alphabet_size) {
  uint8_t lengths[FIO___BROTLI_MAX_IACLEN];
  FIO_MEMSET(lengths, 0, alphabet_size);

  uint32_t hskip = fio___brotli_bits_get(b, 2);

  if (hskip == 1) {
    /* Simple prefix code */
    uint32_t nsym = fio___brotli_bits_get(b, 2) + 1;
    unsigned sym_bits = 0;
    {
      unsigned tmp = alphabet_size - 1;
      while (tmp) {
        sym_bits++;
        tmp >>= 1;
      }
    }
    if (sym_bits == 0)
      sym_bits = 1;

    uint16_t symbols[4];
    for (uint32_t i = 0; i < nsym; i++) {
      symbols[i] = (uint16_t)fio___brotli_bits_get(b, sym_bits);
      if (symbols[i] >= alphabet_size)
        return 0; /* invalid */
    }

    /* Build table directly for simple codes */
    uint32_t root_size = 1U << root_bits;
    switch (nsym) {
    case 1:
      for (uint32_t i = 0; i < root_size; i++)
        table[i] = FIO___BROTLI_ENTRY_SYM(symbols[0], 0);
      return root_size;
    case 2:
      lengths[symbols[0]] = 1;
      lengths[symbols[1]] = 1;
      return fio___brotli_build_table(table, root_bits, lengths, alphabet_size);
    case 3:
      lengths[symbols[0]] = 1;
      lengths[symbols[1]] = 2;
      lengths[symbols[2]] = 2;
      return fio___brotli_build_table(table, root_bits, lengths, alphabet_size);
    case 4: {
      uint32_t tree_select = fio___brotli_bits_get(b, 1);
      if (tree_select) {
        lengths[symbols[0]] = 1;
        lengths[symbols[1]] = 2;
        lengths[symbols[2]] = 3;
        lengths[symbols[3]] = 3;
      } else {
        lengths[symbols[0]] = 2;
        lengths[symbols[1]] = 2;
        lengths[symbols[2]] = 2;
        lengths[symbols[3]] = 2;
      }
      return fio___brotli_build_table(table, root_bits, lengths, alphabet_size);
    }
    }
    return 0;
  }

  /* Complex prefix code */
  /* Read code-length code lengths */
  uint8_t cl_lengths[18];
  FIO_MEMSET(cl_lengths, 0, 18);

  /* Static prefix code for code-length code lengths (RFC 7932 Section 3.5).
   * Indexed by 4-bit peek into the LSB-first bitstream.
   * Matches google/brotli reference: kCodeLengthPrefixLength/Value tables. */
  static const uint8_t cl_vlc_len[16] =
      {2, 2, 2, 3, 2, 2, 2, 4, 2, 2, 2, 3, 2, 2, 2, 4};
  static const uint8_t cl_vlc_val[16] =
      {0, 4, 3, 2, 0, 4, 3, 1, 0, 4, 3, 2, 0, 4, 3, 5};

  uint32_t cl_space = 32;
  for (unsigned i = hskip; i < 18; i++) {
    fio___brotli_bits_fill(b);
    uint32_t ix = fio___brotli_bits_peek(b, 4) & 0xF;
    uint32_t cl_val = cl_vlc_val[ix];
    fio___brotli_bits_drop(b, cl_vlc_len[ix]);
    cl_lengths[fio___brotli_cl_order[i]] = (uint8_t)cl_val;
    if (cl_val) {
      cl_space = cl_space - (32U >> cl_val);
      if (cl_space - 1U >= 32U) /* space is 0 or wrapped */
        break;
    }
  }

  /* Build code-length Huffman table */
  uint32_t cl_table[1 << FIO___BROTLI_MAX_CL_LEN]; /* 32 entries */
  if (!fio___brotli_build_table(cl_table,
                                FIO___BROTLI_MAX_CL_LEN,
                                cl_lengths,
                                18))
    return 0;

  /* Read symbol code lengths using the code-length Huffman table.
   * Uses cumulative repeat mechanism per RFC 7932 / reference decoder:
   * consecutive repeat symbols of the same type accumulate rather than
   * being independent. See ProcessRepeatedCodeLength in google/brotli. */
  unsigned sym_count = 0;
  uint8_t prev_len = 8;         /* initial previous code length */
  uint32_t space = 32768;       /* 1 << 15 */
  uint32_t repeat = 0;          /* cumulative repeat count */
  uint32_t repeat_code_len = 0; /* code length being repeated (0=zero) */

  while (sym_count < alphabet_size && space > 0) {
    uint32_t cl_sym =
        fio___brotli_huff_decode(b, cl_table, FIO___BROTLI_MAX_CL_LEN);

    if (cl_sym < 16) {
      /* Literal code length - resets repeat state */
      repeat = 0;
      lengths[sym_count++] = (uint8_t)cl_sym;
      if (cl_sym) {
        prev_len = (uint8_t)cl_sym;
        space -= (32768U >> cl_sym);
      }
    } else {
      /* Repeat symbol (16 = repeat previous, 17 = repeat zero).
       * Cumulative: consecutive same-type repeats accumulate. */
      uint32_t new_len;
      uint32_t extra_bits;
      if (cl_sym == 16) {
        new_len = prev_len;
        extra_bits = 2;
      } else {
        new_len = 0;
        extra_bits = 3;
      }
      /* Reset accumulator if repeat type changed */
      if (repeat_code_len != new_len) {
        repeat = 0;
        repeat_code_len = new_len;
      }
      uint32_t old_repeat = repeat;
      if (repeat > 0) {
        repeat -= 2;
        repeat <<= extra_bits;
      }
      repeat += fio___brotli_bits_get(b, extra_bits) + 3;
      uint32_t repeat_delta = repeat - old_repeat;
      /* Apply repeat_delta symbols */
      for (uint32_t r = 0; r < repeat_delta && sym_count < alphabet_size; r++) {
        lengths[sym_count++] = (uint8_t)new_len;
        if (new_len)
          space -= (32768U >> new_len);
      }
    }
  }

  /* Fill remaining with zeros */
  while (sym_count < alphabet_size)
    lengths[sym_count++] = 0;

  return fio___brotli_build_table(table, root_bits, lengths, alphabet_size);
}

/* *****************************************************************************
Block type and count decoding
***************************************************************************** */

typedef struct {
  uint32_t *type_table;  /* Huffman table for block type codes */
  uint32_t *count_table; /* Huffman table for block count codes */
  uint32_t num_types;    /* number of block types */
  uint32_t type;         /* current block type */
  uint32_t prev_type;    /* previous block type */
  uint32_t count;        /* remaining count for current block */
} fio___brotli_block_s;

/**
 * Read block type and count Huffman codes for one category.
 */
FIO_SFUNC int fio___brotli_read_block_switch(fio___brotli_bits_s *b,
                                             fio___brotli_block_s *blk,
                                             uint32_t *type_table_buf,
                                             uint32_t *count_table_buf) {
  blk->type_table = type_table_buf;
  blk->count_table = count_table_buf;

  if (blk->num_types < 2) {
    blk->type = 0;
    blk->prev_type = 1; /* per spec */
    return 0;           /* no switching needed */
  }

  /* Read block type Huffman code (alphabet = num_types + 2) */
  uint32_t type_alpha = blk->num_types + 2;
  if (!fio___brotli_read_prefix_code(b,
                                     type_table_buf,
                                     FIO___BROTLI_HUFF_BITS,
                                     type_alpha))
    return -1;

  /* Read block count Huffman code (alphabet = 26) */
  if (!fio___brotli_read_prefix_code(b,
                                     count_table_buf,
                                     FIO___BROTLI_HUFF_BITS,
                                     26))
    return -1;

  /* Read initial block count */
  uint32_t count_sym =
      fio___brotli_huff_decode(b, count_table_buf, FIO___BROTLI_HUFF_BITS);
  if (count_sym >= 26)
    return -1;
  blk->count =
      fio___brotli_block_count_base[count_sym] +
      fio___brotli_bits_get(b, fio___brotli_block_count_extra[count_sym]);

  blk->type = 0;
  blk->prev_type = 1;
  return 0;
}

/**
 * Decode a block switch command: updates type and reads new count.
 */
FIO_SFUNC int fio___brotli_decode_block_switch(fio___brotli_bits_s *b,
                                               fio___brotli_block_s *blk) {
  uint32_t type_code =
      fio___brotli_huff_decode(b, blk->type_table, FIO___BROTLI_HUFF_BITS);
  uint32_t new_type;
  if (type_code == 0) {
    new_type = blk->prev_type;
  } else if (type_code == 1) {
    new_type = (blk->type + 1) % blk->num_types;
  } else {
    new_type = type_code - 2;
  }
  if (new_type >= blk->num_types)
    return -1;
  blk->prev_type = blk->type;
  blk->type = new_type;

  /* Read new block count */
  uint32_t count_sym =
      fio___brotli_huff_decode(b, blk->count_table, FIO___BROTLI_HUFF_BITS);
  if (count_sym >= 26)
    return -1;
  blk->count =
      fio___brotli_block_count_base[count_sym] +
      fio___brotli_bits_get(b, fio___brotli_block_count_extra[count_sym]);
  return 0;
}

/* *****************************************************************************
Context map decoding (RFC 7932 Section 7.3)
***************************************************************************** */

/**
 * Read a context map of map_size entries, each in [0, num_trees).
 * Applies optional inverse move-to-front transform.
 */
FIO_SFUNC int fio___brotli_read_context_map(fio___brotli_bits_s *b,
                                            uint8_t *map,
                                            uint32_t map_size,
                                            uint32_t num_trees) {
  if (num_trees < 2) {
    FIO_MEMSET(map, 0, map_size);
    return 0;
  }

  /* Read RLEMAX */
  uint32_t rlemax = 0;
  if (fio___brotli_bits_get1(b)) {
    rlemax = fio___brotli_bits_get(b, 4) + 1;
  }

  /* Read Huffman code for context map values.
   * Alphabet size = num_trees + rlemax */
  uint32_t alpha = num_trees + rlemax;
  /* Allocate table on stack - max alphabet is 256 + 16 = 272 */
  uint32_t cm_table[FIO___BROTLI_HUFF_SIZE + 512];
  if (!fio___brotli_read_prefix_code(b,
                                     cm_table,
                                     FIO___BROTLI_HUFF_BITS,
                                     alpha))
    return -1;

  /* Decode context map entries */
  uint32_t i = 0;
  while (i < map_size) {
    uint32_t sym =
        fio___brotli_huff_decode(b, cm_table, FIO___BROTLI_HUFF_BITS);
    if (sym == 0) {
      map[i++] = 0;
    } else if (sym <= rlemax) {
      /* RLE zero: repeat 0 for (1 << sym) + read_bits(sym) times */
      uint32_t rep = (1U << sym) + fio___brotli_bits_get(b, sym);
      for (uint32_t r = 0; r < rep && i < map_size; r++)
        map[i++] = 0;
    } else {
      uint32_t val = sym - rlemax;
      if (val >= num_trees)
        return -1; /* invalid context map value */
      map[i++] = (uint8_t)val;
    }
  }

  /* Read IMTF flag and apply inverse move-to-front if set */
  if (fio___brotli_bits_get1(b)) {
    uint8_t mtf[256];
    for (unsigned m = 0; m < 256; m++)
      mtf[m] = (uint8_t)m;
    for (i = 0; i < map_size; i++) {
      uint8_t idx = map[i];
      uint8_t val = mtf[idx];
      map[i] = val;
      /* Move val to front */
      for (uint8_t j = idx; j > 0; j--)
        mtf[j] = mtf[j - 1];
      mtf[0] = val;
    }
  }

  /* Validate all context map entries are in [0, num_trees) */
  for (i = 0; i < map_size; i++) {
    if (map[i] >= num_trees)
      return -1;
  }

  return 0;
}

/* *****************************************************************************
Dictionary word transform (RFC 7932 Appendix B)
***************************************************************************** */

FIO_SFUNC int fio___brotli_uppercase(uint8_t *p) {
  if (p[0] < 0xC0) {
    if (p[0] >= 'a' && p[0] <= 'z')
      p[0] ^= 32;
    return 1;
  }
  if (p[0] < 0xE0) {
    p[1] ^= 32;
    return 2;
  }
  p[2] ^= 5;
  return 3;
}

/**
 * Apply transform to a dictionary word, writing result to dst.
 * Returns number of bytes written.
 */
FIO_SFUNC int fio___brotli_transform_word(uint8_t *dst,
                                          const uint8_t *word,
                                          int word_len,
                                          int transform_idx) {
  const uint8_t *t = fio___brotli_transforms + transform_idx * 3;
  uint8_t prefix_id = t[0];
  uint8_t type = t[1];
  uint8_t suffix_id = t[2];

  /* Write prefix */
  const uint8_t *prefix =
      fio___brotli_prefix_suffix + fio___brotli_prefix_suffix_map[prefix_id];
  int prefix_len = *prefix++;
  int written = 0;
  FIO_MEMCPY(dst, prefix, (size_t)prefix_len);
  written += prefix_len;

  /* Apply transform to word */
  int omit_first = 0, omit_last = 0;
  if (type <= FIO___BROTLI_TRANSFORM_OMIT_LAST_9) {
    omit_last = type;
  } else if (type >= FIO___BROTLI_TRANSFORM_OMIT_FIRST_1 &&
             type <= FIO___BROTLI_TRANSFORM_OMIT_FIRST_9) {
    omit_first = type - FIO___BROTLI_TRANSFORM_OMIT_FIRST_1 + 1;
  }

  int copy_len = word_len;
  const uint8_t *copy_src = word;
  if (omit_first > 0 && omit_first < copy_len) {
    copy_src += omit_first;
    copy_len -= omit_first;
  } else if (omit_first >= copy_len) {
    copy_len = 0;
  }
  if (omit_last > 0 && omit_last < copy_len) {
    copy_len -= omit_last;
  } else if (omit_last >= copy_len &&
             type <= FIO___BROTLI_TRANSFORM_OMIT_LAST_9) {
    copy_len = 0;
  }

  FIO_MEMCPY(dst + written, copy_src, (size_t)copy_len);

  /* Apply uppercase transforms */
  if (type == FIO___BROTLI_TRANSFORM_UPPERCASE_FIRST) {
    fio___brotli_uppercase(dst + written);
  } else if (type == FIO___BROTLI_TRANSFORM_UPPERCASE_ALL) {
    int pos = 0;
    while (pos < copy_len) {
      pos += fio___brotli_uppercase(dst + written + pos);
    }
  }

  written += copy_len;

  /* Write suffix */
  const uint8_t *suffix =
      fio___brotli_prefix_suffix + fio___brotli_prefix_suffix_map[suffix_id];
  int suffix_len = *suffix++;
  FIO_MEMCPY(dst + written, suffix, (size_t)suffix_len);
  written += suffix_len;

  return written;
}

/* *****************************************************************************
Main Brotli decompressor
***************************************************************************** */

/**
 * Read VarLenUint8 from the bitstream.
 * Returns 0-255.
 */
FIO_SFUNC uint32_t fio___brotli_read_varlen_uint8(fio___brotli_bits_s *b) {
  if (!fio___brotli_bits_get1(b))
    return 0;
  uint32_t n = fio___brotli_bits_get(b, 3);
  if (n == 0)
    return 1;
  return fio___brotli_bits_get(b, n) + (1U << n);
}

/**
 * Decode insert-and-copy command symbol into insert_code, copy_code,
 * and dist_code_zero flag.
 *
 * RFC 7932 Section 5: 704 command codes organized in 11 blocks of 64.
 */
FIO_SFUNC void fio___brotli_decode_iac(uint32_t cmd,
                                       uint32_t *insert_code,
                                       uint32_t *copy_code,
                                       int *dist_code_zero) {
  /* RFC 7932 Section 5, Table:
   *
   *          Insert
   *          length        Copy length code
   *          code       0..7       8..15     16..23
   *                 +----------+----------+
   *           0..7  |   0..63  |  64..127 | <-- distance symbol 0
   *                 +----------+----------+----------+
   *           0..7  | 128..191 | 192..255 | 384..447 |
   *                 +----------+----------+----------+
   *           8..15 | 256..319 | 320..383 | 512..575 |
   *                 +----------+----------+----------+
   *          16..23 | 448..511 | 576..639 | 640..703 |
   *                 +----------+----------+----------+
   *
   * Within each 64-code block:
   *   insert_offset = (cmd_in_block >> 3) & 7
   *   copy_offset   = cmd_in_block & 7
   */
  /* clang-format off */
  static const uint8_t ib[11] = {0,  0,  0,  0,  8,  8,  0, 16,  8, 16, 16};
  static const uint8_t cb[11] = {0,  8,  0,  8,  0,  8, 16,  0, 16,  8, 16};
  static const uint8_t dz[11] = {1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  /* clang-format on */

  uint32_t block = cmd >> 6;
  if (block > 10)
    block = 10;
  uint32_t within = cmd & 63;

  *insert_code = ib[block] + ((within >> 3) & 7);
  *copy_code = cb[block] + (within & 7);
  *dist_code_zero = dz[block];

  if (*insert_code > 23)
    *insert_code = 23;
  if (*copy_code > 23)
    *copy_code = 23;
}

/**
 * Scan remaining meta-block headers in a Brotli stream, summing MLEN values.
 * Used after buffer overflow to determine total required output size.
 * The bitstream must be positioned at the start of a meta-block header.
 * Returns the sum of remaining MLEN values, or 0 on corrupt data.
 */
FIO_SFUNC size_t fio___brotli_scan_remaining_mlen(fio___brotli_bits_s *bits) {
  size_t total = 0;
  int is_last = 0;
  while (!is_last) {
    fio___brotli_bits_fill(bits);
    is_last = (int)fio___brotli_bits_get1(bits);

    /* ISLASTEMPTY */
    if (is_last && fio___brotli_bits_get1(bits))
      break;

    /* Meta-block length (MLEN) */
    uint32_t mnibbles = fio___brotli_bits_get(bits, 2);
    if (mnibbles == 3) {
      /* Metadata or empty meta-block */
      if (!is_last) {
        if (fio___brotli_bits_get1(bits))
          return 0; /* reserved bit */
        uint32_t mskipbytes = fio___brotli_bits_get(bits, 2);
        if (mskipbytes > 0) {
          uint32_t mskiplen = fio___brotli_bits_get(bits, mskipbytes * 8) + 1;
          if (bits->avail & 7)
            fio___brotli_bits_drop(bits, bits->avail & 7);
          bits->src += mskiplen;
          if (bits->src > bits->end)
            return 0;
          bits->bits = 0;
          bits->avail = 0;
        } else {
          if (bits->avail & 7)
            fio___brotli_bits_drop(bits, bits->avail & 7);
        }
      }
      continue;
    }

    uint32_t mlen_nibbles = mnibbles + 4;
    uint32_t mlen = fio___brotli_bits_get(bits, mlen_nibbles * 4) + 1;
    total += mlen;

    /* Check for uncompressed meta-block */
    if (!is_last && fio___brotli_bits_get1(bits)) {
      /* Uncompressed: skip MLEN raw bytes */
      if (bits->avail & 7)
        fio___brotli_bits_drop(bits, bits->avail & 7);
      bits->src -= (bits->avail / 8);
      bits->bits = 0;
      bits->avail = 0;
      if (bits->src + mlen > bits->end)
        return 0;
      bits->src += mlen;
      continue;
    }

    /* Compressed meta-block: if this is the last one, we're done.
     * If not last, we cannot skip the compressed body without full decode,
     * so return 0 to indicate we can't determine the size. */
    if (!is_last)
      return 0;
  }
  return total;
}

SFUNC size_t fio_brotli_decompress(void *out,
                                   size_t out_len,
                                   const void *in,
                                   size_t in_len) {
  if (!in || !in_len)
    return 0;

  /* Size query mode: out==NULL or out_len==0 — scan headers for MLEN sum */
  if (!out || !out_len) {
    fio___brotli_bits_s bits;
    fio___brotli_bits_init(&bits, in, in_len);

    /* Parse and skip WBITS */
    fio___brotli_bits_fill(&bits);
    if (!fio___brotli_bits_get1(&bits)) {
      /* wbits = 16, nothing more to read */
    } else {
      uint32_t n = fio___brotli_bits_get(&bits, 3);
      if (n == 0) {
        n = fio___brotli_bits_get(&bits, 3);
        if (n == 1)
          return 0; /* large window / invalid */
      }
    }

    return fio___brotli_scan_remaining_mlen(&bits);
  }

  fio___brotli_bits_s bits;
  fio___brotli_bits_init(&bits, in, in_len);

  uint8_t *dst = (uint8_t *)out;
  size_t pos = 0;

  /* Read window size (WBITS) - RFC 7932 Section 9.1 */
  uint32_t wbits;
  fio___brotli_bits_fill(&bits);
  if (!fio___brotli_bits_get1(&bits)) {
    wbits = 16;
  } else {
    uint32_t n = fio___brotli_bits_get(&bits, 3);
    if (n != 0) {
      wbits = n + 17; /* n=1->18, n=2->19, ..., n=7->24 */
    } else {
      n = fio___brotli_bits_get(&bits, 3);
      if (n == 1)
        return 0; /* large window / invalid - not supported */
      if (n != 0) {
        wbits = n + 8; /* n=2->10, n=3->11, ..., n=7->15 */
      } else {
        wbits = 17;
      }
    }
  }

  uint32_t window_size = (1U << wbits) - 16U;

  /* Distance ring buffer - RFC 7932 Section 4 */
  int32_t dist_rb[4] = {16, 15, 11, 4};
  uint32_t dist_rb_idx = 0;

  /* Process meta-blocks */
  int is_last = 0;
  while (!is_last) {
    fio___brotli_bits_fill(&bits);

    is_last = (int)fio___brotli_bits_get1(&bits);

    /* ISLASTEMPTY */
    if (is_last && fio___brotli_bits_get1(&bits))
      break;

    /* Meta-block length (MLEN) */
    uint32_t mnibbles = fio___brotli_bits_get(&bits, 2);
    if (mnibbles == 3) {
      /* Metadata or empty meta-block */
      if (!is_last) {
        if (fio___brotli_bits_get1(&bits))
          return 0; /* reserved bit */
        uint32_t mskipbytes = fio___brotli_bits_get(&bits, 2);
        if (mskipbytes > 0) {
          uint32_t mskiplen = fio___brotli_bits_get(&bits, mskipbytes * 8) + 1;
          if (bits.avail & 7)
            fio___brotli_bits_drop(&bits, bits.avail & 7);
          bits.src += mskiplen;
          if (bits.src > bits.end)
            return 0;
          bits.bits = 0;
          bits.avail = 0;
        } else {
          if (bits.avail & 7)
            fio___brotli_bits_drop(&bits, bits.avail & 7);
        }
      }
      continue;
    }

    uint32_t mlen_nibbles = mnibbles + 4;
    uint32_t mlen = fio___brotli_bits_get(&bits, mlen_nibbles * 4) + 1;

    /* Check for uncompressed meta-block */
    if (!is_last && fio___brotli_bits_get1(&bits)) {
      /* Align to byte boundary and push back unused bits */
      if (bits.avail & 7)
        fio___brotli_bits_drop(&bits, bits.avail & 7);
      bits.src -= (bits.avail / 8);
      bits.bits = 0;
      bits.avail = 0;
      if (bits.src + mlen > bits.end)
        return 0;
      if (pos + mlen > out_len) {
        /* Buffer overflow on uncompressed meta-block.
         * Add remaining bytes and scan subsequent headers. */
        size_t needed = pos + mlen;
        bits.src += mlen;
        size_t rest = fio___brotli_scan_remaining_mlen(&bits);
        return needed + rest;
      }
      FIO_MEMCPY(dst + pos, bits.src, mlen);
      bits.src += mlen;
      pos += mlen;
      continue;
    }

    /* === Compressed meta-block === */

    /* Read NBLTYPES for each category (L, I, D) */
    uint32_t nbltypes[3];
    for (int cat = 0; cat < 3; cat++) {
      if (!fio___brotli_bits_get1(&bits)) {
        nbltypes[cat] = 1;
      } else {
        nbltypes[cat] = fio___brotli_read_varlen_uint8(&bits) + 2;
        if (nbltypes[cat] > 256)
          return 0; /* RFC 7932: max 256 block types */
      }
    }

    /* Read block switch info */
    fio___brotli_block_s blk_lit = {0}, blk_iac = {0}, blk_dist = {0};
    blk_lit.num_types = nbltypes[0];
    blk_iac.num_types = nbltypes[1];
    blk_dist.num_types = nbltypes[2];

    uint32_t blk_tables[6][FIO___BROTLI_HUFF_SIZE + 512];
    if (fio___brotli_read_block_switch(&bits,
                                       &blk_lit,
                                       blk_tables[0],
                                       blk_tables[1]))
      return 0;
    if (fio___brotli_read_block_switch(&bits,
                                       &blk_iac,
                                       blk_tables[2],
                                       blk_tables[3]))
      return 0;
    if (fio___brotli_read_block_switch(&bits,
                                       &blk_dist,
                                       blk_tables[4],
                                       blk_tables[5]))
      return 0;

    /* NPOSTFIX and NDIRECT */
    uint32_t npostfix = fio___brotli_bits_get(&bits, 2);
    uint32_t ndirect = fio___brotli_bits_get(&bits, 4) << npostfix;

    /* Context modes for each literal block type */
    uint8_t context_modes[256];
    for (uint32_t i = 0; i < nbltypes[0]; i++)
      context_modes[i] = (uint8_t)fio___brotli_bits_get(&bits, 2);

    /* Read NTREESL and literal context map */
    uint32_t ntreesl = fio___brotli_read_varlen_uint8(&bits) + 1;
    if (ntreesl > 256)
      return 0; /* RFC 7932: max 256 literal Huffman trees */
    uint32_t lit_map_size = nbltypes[0] * FIO___BROTLI_LITERAL_CONTEXTS;
    uint8_t lit_cmap_stack[256 * 64];
    uint8_t *lit_cmap =
        (lit_map_size <= sizeof(lit_cmap_stack))
            ? lit_cmap_stack
            : (uint8_t *)FIO_MEM_REALLOC(NULL, 0, lit_map_size, 0);
    if (!lit_cmap)
      return 0;
    if (fio___brotli_read_context_map(&bits, lit_cmap, lit_map_size, ntreesl))
      goto err_free_lit;

    /* Read NTREESD and distance context map */
    uint32_t ntreesd = fio___brotli_read_varlen_uint8(&bits) + 1;
    if (ntreesd > 256)
      goto err_free_lit; /* RFC 7932: max 256 distance Huffman trees */
    uint32_t dist_map_size = nbltypes[2] * FIO___BROTLI_DISTANCE_CONTEXTS;
    uint8_t dist_cmap_stack[256 * 4];
    uint8_t *dist_cmap =
        (dist_map_size <= sizeof(dist_cmap_stack))
            ? dist_cmap_stack
            : (uint8_t *)FIO_MEM_REALLOC(NULL, 0, dist_map_size, 0);
    if (!dist_cmap)
      goto err_free_lit;
    if (fio___brotli_read_context_map(&bits, dist_cmap, dist_map_size, ntreesd))
      goto err_free_dist;

    /* Distance alphabet size */
    uint32_t dist_alpha = 16 + ndirect + (48U << npostfix);

    /* Allocate all Huffman tables in one block */
    uint32_t per_lit = FIO___BROTLI_HUFF_SIZE + 512;
    uint32_t per_iac = FIO___BROTLI_HUFF_SIZE + 2048;
    uint32_t per_dist = FIO___BROTLI_HUFF_SIZE + 1024;
    uint32_t total_table_size =
        per_lit * ntreesl + per_iac * nbltypes[1] + per_dist * ntreesd;

    uint32_t *all_tables = (uint32_t *)
        FIO_MEM_REALLOC(NULL, 0, total_table_size * sizeof(uint32_t), 0);
    if (!all_tables)
      goto err_free_dist;

    uint32_t *lit_tables = all_tables;
    uint32_t *iac_tables = all_tables + per_lit * ntreesl;
    uint32_t *dist_tables = iac_tables + per_iac * nbltypes[1];

    uint32_t lit_offsets[256], iac_offsets[256], dist_offsets[256];

    /* Read literal Huffman trees */
    for (uint32_t t = 0; t < ntreesl; t++) {
      lit_offsets[t] = t * per_lit;
      if (!fio___brotli_read_prefix_code(&bits,
                                         lit_tables + lit_offsets[t],
                                         FIO___BROTLI_HUFF_BITS,
                                         256))
        goto err_free_all;
    }

    /* Read insert-and-copy Huffman trees */
    for (uint32_t t = 0; t < nbltypes[1]; t++) {
      iac_offsets[t] = t * per_iac;
      if (!fio___brotli_read_prefix_code(&bits,
                                         iac_tables + iac_offsets[t],
                                         FIO___BROTLI_HUFF_BITS,
                                         704))
        goto err_free_all;
    }

    /* Read distance Huffman trees */
    for (uint32_t t = 0; t < ntreesd; t++) {
      dist_offsets[t] = t * per_dist;
      if (!fio___brotli_read_prefix_code(&bits,
                                         dist_tables + dist_offsets[t],
                                         FIO___BROTLI_HUFF_BITS,
                                         dist_alpha))
        goto err_free_all;
    }

    /* === Process commands === */
    uint32_t meta_remaining = mlen;
    while (meta_remaining > 0) {
      /* IAC block switch */
      if (blk_iac.count == 0 && blk_iac.num_types >= 2) {
        if (fio___brotli_decode_block_switch(&bits, &blk_iac)) {
          goto err_free_all;
        }
      }
      blk_iac.count--;

      /* Decode insert-and-copy command */
      if (blk_iac.type >= nbltypes[1]) {
        goto err_free_all;
      }
      uint32_t iac_sym =
          fio___brotli_huff_decode(&bits,
                                   iac_tables + iac_offsets[blk_iac.type],
                                   FIO___BROTLI_HUFF_BITS);

      uint32_t insert_code, copy_code;
      int dist_code_zero;
      fio___brotli_decode_iac(iac_sym,
                              &insert_code,
                              &copy_code,
                              &dist_code_zero);

      uint32_t insert_len =
          fio___brotli_insert_base[insert_code] +
          fio___brotli_bits_get(&bits, fio___brotli_insert_extra[insert_code]);
      uint32_t copy_len =
          fio___brotli_copy_base[copy_code] +
          fio___brotli_bits_get(&bits, fio___brotli_copy_extra[copy_code]);

      /* Insert literals */
      for (uint32_t li = 0; li < insert_len; li++) {
        if (pos >= out_len) {
          /* Buffer overflow — return required size.
           * Remaining in this meta-block: meta_remaining
           * Remaining literals in this insert: insert_len - li
           * But we can't continue decoding (context needs output).
           * Use meta_remaining as the count for this meta-block. */
          size_t needed = pos + meta_remaining;
          FIO_MEM_FREE(all_tables, total_table_size * sizeof(uint32_t));
          if (lit_cmap != lit_cmap_stack)
            FIO_MEM_FREE(lit_cmap, lit_map_size);
          if (dist_cmap != dist_cmap_stack)
            FIO_MEM_FREE(dist_cmap, dist_map_size);
          if (!is_last) {
            size_t rest = fio___brotli_scan_remaining_mlen(&bits);
            needed += rest;
          }
          return needed;
        }

        /* Literal block switch */
        if (blk_lit.count == 0 && blk_lit.num_types >= 2) {
          if (fio___brotli_decode_block_switch(&bits, &blk_lit)) {
            goto err_free_all;
          }
        }
        blk_lit.count--;

        /* Context-dependent literal decoding */
        if (blk_lit.type >= nbltypes[0]) {
          goto err_free_all;
        }
        uint8_t p1 = pos > 0 ? dst[pos - 1] : 0;
        uint8_t p2 = pos > 1 ? dst[pos - 2] : 0;
        uint8_t mode = context_modes[blk_lit.type];
        uint32_t ctx =
            (uint32_t)fio___brotli_context_lut[mode * 512 + p1] |
            (uint32_t)fio___brotli_context_lut[mode * 512 + 256 + p2];

        uint32_t lit_tree_idx = lit_cmap[blk_lit.type * 64 + ctx];
        if (lit_tree_idx >= ntreesl) {
          goto err_free_all;
        }
        uint32_t literal =
            fio___brotli_huff_decode(&bits,
                                     lit_tables + lit_offsets[lit_tree_idx],
                                     FIO___BROTLI_HUFF_BITS);

        dst[pos++] = (uint8_t)literal;
        if (meta_remaining == 0) {
          goto err_free_all; /* more literals than meta-block length */
        }
        meta_remaining--;
      }

      if (meta_remaining == 0)
        break;

      /* Decode distance */
      uint32_t distance;
      if (dist_code_zero) {
        distance = (uint32_t)dist_rb[(dist_rb_idx - 1) & 3];
      } else {
        /* Distance block switch */
        if (blk_dist.count == 0 && blk_dist.num_types >= 2) {
          if (fio___brotli_decode_block_switch(&bits, &blk_dist)) {
            goto err_free_all;
          }
        }
        blk_dist.count--;

        /* Distance context: 0 for copy_len=2, 1 for 3, 2 for 4, 3 for 5+ */
        uint32_t dist_ctx =
            (copy_len < 2) ? 0 : ((copy_len > 4) ? 3 : (copy_len - 2));

        if (blk_dist.type >= nbltypes[2]) {
          goto err_free_all;
        }
        uint32_t dist_tree_idx = dist_cmap[blk_dist.type * 4 + dist_ctx];
        if (dist_tree_idx >= ntreesd) {
          goto err_free_all;
        }
        uint32_t dist_sym =
            fio___brotli_huff_decode(&bits,
                                     dist_tables + dist_offsets[dist_tree_idx],
                                     FIO___BROTLI_HUFF_BITS);

        if (dist_sym < 16) {
          /* Ring buffer distance (RFC 7932 Section 4, Table 2) */
          static const int8_t doff[16] =
              {0, 0, 0, 0, -1, 1, -2, 2, -3, 3, -1, 1, -2, 2, -3, 3};
          static const uint8_t drb[16] =
              {1, 2, 3, 4, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
          int32_t bd = dist_rb[(dist_rb_idx - drb[dist_sym]) & 3];
          distance = (uint32_t)(bd + doff[dist_sym]);
          if (distance == 0) {
            goto err_free_all;
          }
        } else if (dist_sym < 16 + ndirect) {
          distance = dist_sym - 15;
        } else {
          /* Complex distance code with extra bits */
          uint32_t d = dist_sym - 16 - ndirect;
          uint32_t postfix = d & ((1U << npostfix) - 1);
          d >>= npostfix;
          uint32_t hcode = d >> 1;
          uint32_t lcode = d & 1;
          uint32_t nbits = 1 + hcode;
          uint32_t offset = ((2 + lcode) << nbits) - 4;
          uint32_t extra = fio___brotli_bits_get(&bits, nbits);
          distance = ((offset + extra) << npostfix) + postfix + ndirect + 1;
        }

        /* Update ring buffer (dist_sym 0 = last distance, not pushed;
         * static dictionary references also not pushed - RFC 7932 Sec 4) */
        if (dist_sym != 0 && distance <= (uint32_t)pos) {
          dist_rb[dist_rb_idx & 3] = (int32_t)distance;
          dist_rb_idx++;
        }
      }

      /* Copy from backward reference or static dictionary */
      if (distance <= (uint32_t)pos) {
        /* Normal backward reference */
        if (copy_len > meta_remaining) {
          goto err_free_all;
        }
        for (uint32_t ci = 0; ci < copy_len; ci++) {
          if (pos >= out_len) {
            /* Buffer overflow during back-reference copy */
            size_t needed = pos + meta_remaining;
            FIO_MEM_FREE(all_tables, total_table_size * sizeof(uint32_t));
            if (lit_cmap != lit_cmap_stack)
              FIO_MEM_FREE(lit_cmap, lit_map_size);
            if (dist_cmap != dist_cmap_stack)
              FIO_MEM_FREE(dist_cmap, dist_map_size);
            if (!is_last) {
              size_t rest = fio___brotli_scan_remaining_mlen(&bits);
              needed += rest;
            }
            return needed;
          }
          dst[pos] = dst[pos - distance];
          pos++;
          meta_remaining--;
        }
      } else {
        /* Static dictionary reference (RFC 7932 Section 8).
         * The word length is determined by copy_len.
         * The word index and transform are derived from the distance. */
        uint32_t max_dist =
            (uint32_t)((size_t)pos < (size_t)window_size ? pos : window_size);
        uint32_t dict_off = distance - max_dist - 1;
        uint32_t wlen = copy_len;
        if (wlen < 4 || wlen > 24 || fio___brotli_ndbits[wlen] == 0) {
          goto err_free_all;
        }
        uint32_t nwords = 1U << fio___brotli_ndbits[wlen];
        uint32_t word_id = dict_off % nwords;
        uint32_t transform_id = dict_off / nwords;
        if (transform_id >= FIO___BROTLI_NUM_TRANSFORMS) {
          goto err_free_all;
        }
        const uint8_t *word = fio___brotli_dict +
                              fio___brotli_dict_offsets[wlen] + word_id * wlen;
        uint8_t transformed[256];
        int tlen = fio___brotli_transform_word(transformed,
                                               word,
                                               (int)wlen,
                                               (int)transform_id);
        for (int ti = 0; ti < tlen; ti++) {
          if (pos >= out_len) {
            /* Buffer overflow during dict copy */
            size_t needed = pos + meta_remaining;
            FIO_MEM_FREE(all_tables, total_table_size * sizeof(uint32_t));
            if (lit_cmap != lit_cmap_stack)
              FIO_MEM_FREE(lit_cmap, lit_map_size);
            if (dist_cmap != dist_cmap_stack)
              FIO_MEM_FREE(dist_cmap, dist_map_size);
            if (!is_last) {
              size_t rest = fio___brotli_scan_remaining_mlen(&bits);
              needed += rest;
            }
            return needed;
          }
          dst[pos++] = transformed[ti];
          meta_remaining--;
        }
      }
    }

    /* Cleanup for this meta-block */
    FIO_MEM_FREE(all_tables, total_table_size * sizeof(uint32_t));
    if (lit_cmap != lit_cmap_stack)
      FIO_MEM_FREE(lit_cmap, lit_map_size);
    if (dist_cmap != dist_cmap_stack)
      FIO_MEM_FREE(dist_cmap, dist_map_size);
    continue;

  err_free_all:
    FIO_MEM_FREE(all_tables, total_table_size * sizeof(uint32_t));
  err_free_dist:
    if (dist_cmap != dist_cmap_stack)
      FIO_MEM_FREE(dist_cmap, dist_map_size);
  err_free_lit:
    if (lit_cmap != lit_cmap_stack)
      FIO_MEM_FREE(lit_cmap, lit_map_size);
    return 0;
  }

  return pos;
}

/* *****************************************************************************




                            BROTLI COMPRESSION
                    RFC 7932 Brotli Compressed Data Format


Quality 1-6 compressor: LZ77 with hash table + canonical Huffman encoding.
q1-4: direct-mapped hash, single literal tree.
q5-6: hash chain + distance cache + score-based lazy matching + context
modeling (2 literal trees, UTF-8/LSB6 mode detection) + static dictionary
search (122K dictionary, identity + uppercase_first transforms).
Single meta-block output. No block splitting, NPOSTFIX=0, NDIRECT=0.

***************************************************************************** */

/* *****************************************************************************
Bit writer for compression
***************************************************************************** */

typedef struct {
  uint64_t bits;
  uint32_t count;
  uint8_t *out;
  uint8_t *out_end;
} fio___brotli_bw_s;

FIO_IFUNC void fio___brotli_bw_init(fio___brotli_bw_s *w,
                                    void *out,
                                    size_t out_len) {
  w->bits = 0;
  w->count = 0;
  w->out = (uint8_t *)out;
  w->out_end = (uint8_t *)out + out_len;
}

FIO_IFUNC void fio___brotli_bw_flush(fio___brotli_bw_s *w) {
  if (w->count >= 8 && w->out + 8 <= w->out_end) {
    fio_u2buf64_le(w->out, w->bits);
    uint32_t bytes = w->count >> 3;
    w->out += bytes;
    w->bits >>= (bytes << 3);
    w->count &= 7;
  }
}

/** Write nbits LSB-first from val. */
FIO_IFUNC void fio___brotli_bw_put(fio___brotli_bw_s *w,
                                   uint64_t val,
                                   uint32_t nbits) {
  w->bits |= val << w->count;
  w->count += nbits;
  fio___brotli_bw_flush(w);
}

/** Write pre-reversed Huffman code (codes are reversed at build time). */
FIO_IFUNC void fio___brotli_bw_put_huff(fio___brotli_bw_s *w,
                                        uint16_t code,
                                        uint8_t nbits) {
  fio___brotli_bw_put(w, code, nbits);
}

/** Flush remaining bits and return total bytes written. */
FIO_IFUNC size_t fio___brotli_bw_finish(fio___brotli_bw_s *w, void *out_start) {
  while (w->count > 0 && w->out < w->out_end) {
    *w->out++ = (uint8_t)(w->bits & 0xFF);
    w->bits >>= 8;
    w->count = (w->count >= 8) ? (w->count - 8) : 0;
  }
  return (size_t)(w->out - (uint8_t *)out_start);
}

/* *****************************************************************************
Huffman tree building for compression (depth-limited)
***************************************************************************** */

/**
 * Build canonical Huffman code lengths from frequency counts.
 * max_bits: maximum code length (15 for data, 5 for code-length codes).
 * Returns 0 on success.
 */
FIO_SFUNC int fio___brotli_build_code_lengths(uint8_t *lens,
                                              const uint32_t *freqs,
                                              uint32_t num_syms,
                                              uint32_t max_bits) {
  uint32_t num_used = 0;
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (freqs[i])
      num_used++;
  }

  FIO_MEMSET(lens, 0, num_syms);

  if (num_used == 0)
    return 0;
  if (num_used == 1) {
    for (uint32_t i = 0; i < num_syms; ++i) {
      if (freqs[i]) {
        lens[i] = 1;
        return 0;
      }
    }
  }

  /* Build Huffman tree using min-heap. */
  typedef struct {
    uint32_t freq;
    int32_t sym;
    int32_t left, right;
  } fio___brhuff_node_s;

  /* Max nodes: 704 leaves + 703 internal for IAC alphabet */
  fio___brhuff_node_s nodes[1410];
  uint32_t heap[1410];
  uint32_t heap_size = 0;
  uint32_t node_count = 0;

#define FIO___BRHUFF_PARENT(i) (((i)-1) >> 1)
#define FIO___BRHUFF_LEFT(i)   (((i) << 1) + 1)
#define FIO___BRHUFF_RIGHT(i)  (((i) << 1) + 2)
#define FIO___BRHUFF_SWAP(a, b)                                                \
  do {                                                                         \
    uint32_t t_ = heap[a];                                                     \
    heap[a] = heap[b];                                                         \
    heap[b] = t_;                                                              \
  } while (0)
#define FIO___BRHUFF_LESS(a, b)                                                \
  (nodes[heap[a]].freq < nodes[heap[b]].freq ||                                \
   (nodes[heap[a]].freq == nodes[heap[b]].freq && heap[a] < heap[b]))
#define FIO___BRHUFF_SIFT_UP(pos)                                              \
  do {                                                                         \
    uint32_t i_ = (pos);                                                       \
    while (i_ > 0 && FIO___BRHUFF_LESS(i_, FIO___BRHUFF_PARENT(i_))) {         \
      FIO___BRHUFF_SWAP(i_, FIO___BRHUFF_PARENT(i_));                          \
      i_ = FIO___BRHUFF_PARENT(i_);                                            \
    }                                                                          \
  } while (0)
#define FIO___BRHUFF_SIFT_DOWN(pos)                                            \
  do {                                                                         \
    uint32_t i_ = (pos);                                                       \
    for (;;) {                                                                 \
      uint32_t sm_ = i_;                                                       \
      uint32_t l_ = FIO___BRHUFF_LEFT(i_);                                     \
      uint32_t r_ = FIO___BRHUFF_RIGHT(i_);                                    \
      if (l_ < heap_size && FIO___BRHUFF_LESS(l_, sm_))                        \
        sm_ = l_;                                                              \
      if (r_ < heap_size && FIO___BRHUFF_LESS(r_, sm_))                        \
        sm_ = r_;                                                              \
      if (sm_ == i_)                                                           \
        break;                                                                 \
      FIO___BRHUFF_SWAP(i_, sm_);                                              \
      i_ = sm_;                                                                \
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
      FIO___BRHUFF_SIFT_UP(heap_size - 1);
      node_count++;
    }
  }

  /* Build tree by combining two smallest nodes */
  while (heap_size > 1) {
    uint32_t a = heap[0];
    heap[0] = heap[--heap_size];
    FIO___BRHUFF_SIFT_DOWN(0);
    uint32_t b = heap[0];
    heap[0] = heap[--heap_size];
    FIO___BRHUFF_SIFT_DOWN(0);

    nodes[node_count].freq = nodes[a].freq + nodes[b].freq;
    nodes[node_count].sym = -1;
    nodes[node_count].left = (int32_t)a;
    nodes[node_count].right = (int32_t)b;
    heap[heap_size] = node_count;
    heap_size++;
    FIO___BRHUFF_SIFT_UP(heap_size - 1);
    node_count++;
  }

  /* Compute depths via iterative DFS */
  {
    uint8_t depths[1410];
    FIO_MEMSET(depths, 0, sizeof(depths));
    uint32_t stack[1410];
    uint32_t sp = 0;
    uint32_t root = heap[0];
    stack[sp++] = root;
    depths[root] = 0;

    while (sp > 0) {
      uint32_t n = stack[--sp];
      if (nodes[n].sym >= 0) {
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

  /* Limit code lengths to max_bits using Kraft inequality.
   * After clamping, the code may be over-complete (kraft > target) or
   * under-complete (kraft < target). Both must be fixed:
   *   - Over-complete: shorten codes (move symbols to shorter lengths)
   *   - Under-complete: lengthen codes (move symbols to longer lengths)
   * The decoder's Kraft sum must reach exactly 0 for proper termination. */
  {
    uint32_t cnt[16] = {0};
    for (uint32_t i = 0; i < num_syms; ++i) {
      if (lens[i] > max_bits)
        lens[i] = (uint8_t)max_bits;
      if (lens[i])
        cnt[lens[i]]++;
    }

    int32_t target = (int32_t)(1U << max_bits);

    /* Fix over-complete: lengthen codes until kraft <= target.
     * Over-complete means too many short codes; move symbols from shorter
     * to longer lengths to reduce the Kraft sum. */
    for (;;) {
      int32_t kraft = 0;
      for (uint32_t i = 1; i <= max_bits; ++i)
        kraft += (int32_t)cnt[i] << (max_bits - i);
      if (kraft <= target)
        break;
      int fixed = 0;
      for (uint32_t i = 1; i < max_bits && !fixed; ++i) {
        if (cnt[i] > 0) {
          cnt[i]--;
          cnt[i + 1]++;
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

    /* Fix under-complete: shorten codes until kraft >= target.
     * Under-complete means too many long codes; move symbols from longer
     * to shorter lengths to increase the Kraft sum. */
    for (;;) {
      int32_t kraft = 0;
      for (uint32_t i = 1; i <= max_bits; ++i)
        kraft += (int32_t)cnt[i] << (max_bits - i);
      if (kraft >= target)
        break;
      int fixed = 0;
      for (uint32_t i = max_bits; i > 1 && !fixed; --i) {
        if (cnt[i] > 0) {
          cnt[i]--;
          cnt[i - 1]++;
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

#undef FIO___BRHUFF_PARENT
#undef FIO___BRHUFF_LEFT
#undef FIO___BRHUFF_RIGHT
#undef FIO___BRHUFF_SWAP
#undef FIO___BRHUFF_LESS
#undef FIO___BRHUFF_SIFT_UP
#undef FIO___BRHUFF_SIFT_DOWN

  return 0;
}

/**
 * Build canonical Huffman codes from code lengths.
 * Codes are pre-reversed (LSB-first) for direct bitstream emission.
 */
FIO_SFUNC void fio___brotli_build_codes(uint16_t *codes,
                                        const uint8_t *lens,
                                        uint32_t num_syms) {
  uint32_t count[16] = {0};
  uint32_t next_code[16] = {0};

  for (uint32_t i = 0; i < num_syms; ++i)
    count[lens[i]]++;
  count[0] = 0; /* symbols with length 0 don't participate in code assignment */

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
Prefix code encoding (RFC 7932 Section 3.4-3.5)
***************************************************************************** */

/**
 * Write a prefix code definition to the bitstream.
 * Handles simple (1-4 symbols) and complex (5+ symbols) cases.
 */
FIO_SFUNC void fio___brotli_write_prefix_code(fio___brotli_bw_s *w,
                                              const uint8_t *lens,
                                              const uint16_t *codes,
                                              uint32_t num_syms,
                                              uint32_t alphabet_size) {

  /* Count non-zero symbols and collect up to 4 */
  uint32_t used_count = 0;
  uint32_t used_syms[4] = {0};
  for (uint32_t i = 0; i < num_syms; ++i) {
    if (lens[i]) {
      if (used_count < 4)
        used_syms[used_count] = i;
      used_count++;
    }
  }

  /* Calculate ALPHABET_BITS = ceil(log2(alphabet_size)) */
  uint32_t alphabet_bits = 0;
  {
    uint32_t tmp = alphabet_size - 1;
    while (tmp) {
      alphabet_bits++;
      tmp >>= 1;
    }
  }
  if (alphabet_bits == 0)
    alphabet_bits = 1;

  /* === Simple prefix codes (1-4 symbols) === */
  if (used_count <= 4 && used_count >= 1) {
    /* HSKIP = 1 (simple code marker) */
    fio___brotli_bw_put(w, 1, 2);
    /* NSYM - 1 */
    fio___brotli_bw_put(w, used_count - 1, 2);

    /* Sort symbols by depth for canonical ordering */
    /* Simple bubble sort for at most 4 elements */
    for (uint32_t i = 0; i < used_count; ++i) {
      for (uint32_t j = i + 1; j < used_count; ++j) {
        if (lens[used_syms[j]] < lens[used_syms[i]] ||
            (lens[used_syms[j]] == lens[used_syms[i]] &&
             used_syms[j] < used_syms[i])) {
          uint32_t tmp = used_syms[i];
          used_syms[i] = used_syms[j];
          used_syms[j] = tmp;
        }
      }
    }

    /* Write symbol values */
    for (uint32_t i = 0; i < used_count; ++i)
      fio___brotli_bw_put(w, used_syms[i], alphabet_bits);

    /* Tree-select bit for NSYM=4 */
    if (used_count == 4) {
      /* tree_select=1 means depths {1,2,3,3}; tree_select=0 means {2,2,2,2} */
      uint32_t tree_select = (lens[used_syms[0]] == 1) ? 1 : 0;
      fio___brotli_bw_put(w, tree_select, 1);
    }
    return;
  }

  /* === Complex prefix code (5+ symbols) === */

  /* Step 1: Build RLE sequence of code lengths */
  uint8_t rle_syms[1024];
  uint8_t rle_extra[1024];
  uint32_t rle_count = 0;

  /* Trim trailing zeros */
  uint32_t trimmed_len = num_syms;
  while (trimmed_len > 0 && lens[trimmed_len - 1] == 0)
    --trimmed_len;

  uint8_t prev_nonzero = 8; /* BROTLI_INITIAL_REPEATED_CODE_LENGTH */

  for (uint32_t i = 0; i < trimmed_len;) {
    uint8_t val = lens[i];
    uint32_t run = 1;
    while (i + run < trimmed_len && lens[i + run] == val)
      ++run;

    if (val == 0) {
      /* Encode zero runs using symbol 17 (repeat zero, 3 extra bits).
       * IMPORTANT: never emit consecutive symbol 17s — the decompressor
       * uses cumulative repeat semantics that would miscount. Emit at
       * most one symbol 17 per run; use literal 0s for the remainder. */
      if (run >= 3) {
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
      /* Emit the value itself first if different from previous */
      if (val != prev_nonzero) {
        rle_syms[rle_count] = val;
        rle_extra[rle_count] = 0;
        rle_count++;
        prev_nonzero = val;
        run--;
        i++;
      }
      /* Encode remaining repeats using symbol 16 (repeat prev, 2 extra bits).
       * IMPORTANT: never emit consecutive symbol 16s — the decompressor
       * uses cumulative repeat semantics. Emit at most one symbol 16 per
       * group; break with a literal value between groups. */
      if (run >= 3) {
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

  /* Step 2: Build histogram of code-length symbols */
  uint32_t cl_freqs[18];
  FIO_MEMSET(cl_freqs, 0, sizeof(cl_freqs));
  for (uint32_t i = 0; i < rle_count; ++i)
    cl_freqs[rle_syms[i]]++;

  /* Step 3: Build Huffman tree for code-length alphabet (max depth 5) */
  uint8_t cl_lens[18];
  fio___brotli_build_code_lengths(cl_lens, cl_freqs, 18, 5);

  uint16_t cl_codes[18];
  fio___brotli_build_codes(cl_codes, cl_lens, 18);

  /* Step 4: Determine how many code-length-code-lengths to write.
   * Written in the order: {1,2,3,4,0,5,17,6,16,7,8,9,10,11,12,13,14,15}
   * Trim trailing zeros in this permuted order. */
  uint32_t num_cl = 18;
  while (num_cl > 4 && cl_lens[fio___brotli_cl_order[num_cl - 1]] == 0)
    --num_cl;

  /* Determine HSKIP: how many leading code-length-code-lengths are zero.
   * HSKIP=0: write all from index 0
   * HSKIP=2: skip indices 0,1 (symbols 1,2 have cl_len=0)
   * HSKIP=3: skip indices 0,1,2 (symbols 1,2,3 have cl_len=0) */
  uint32_t hskip = 0;
  if (cl_lens[fio___brotli_cl_order[0]] == 0 &&
      cl_lens[fio___brotli_cl_order[1]] == 0) {
    hskip = 2;
    if (cl_lens[fio___brotli_cl_order[2]] == 0)
      hskip = 3;
  }

  /* Write HSKIP (2 bits) */
  fio___brotli_bw_put(w, hskip, 2);

  /* Step 5: Write code-length-code-lengths using fixed VLC.
   * Fixed VLC (bit-reversed for LSB-first writer):
   *   value 0: 0x00 (2 bits)
   *   value 1: 0x07 (4 bits)
   *   value 2: 0x03 (3 bits)
   *   value 3: 0x02 (2 bits)
   *   value 4: 0x01 (2 bits)
   *   value 5: 0x0F (4 bits) */
  static const uint8_t cl_vlc_code[6] = {0x00, 0x07, 0x03, 0x02, 0x01, 0x0F};
  static const uint8_t cl_vlc_bits[6] = {2, 4, 3, 2, 2, 4};

  /* Write code-length-code-lengths, tracking Kraft space to match the
   * decompressor's early termination (RFC 7932 / google/brotli reference).
   * The decompressor stops when cl_space reaches 0 or wraps. If the Kraft
   * sum is NOT complete at num_cl, we must continue writing zeros up to 18
   * so the decompressor reads the right number of entries. */
  {
    uint32_t cl_space = 32;
    uint32_t cl_end = num_cl;
    /* Check if Kraft sum is complete within num_cl entries */
    for (uint32_t i = hskip; i < num_cl; ++i) {
      uint8_t v = cl_lens[fio___brotli_cl_order[i]];
      if (v) {
        cl_space -= (32U >> v);
        if (cl_space == 0 || cl_space > 32U) {
          cl_end = i + 1; /* Kraft complete here — stop writing */
          break;
        }
      }
    }
    if (cl_space > 0 && cl_space <= 32U) {
      /* Kraft sum not complete — must write up to 18 entries so the
       * decompressor reads the right amount. Extend with zeros. */
      cl_end = 18;
    }
    for (uint32_t i = hskip; i < cl_end; ++i) {
      uint8_t v = (i < num_cl) ? cl_lens[fio___brotli_cl_order[i]] : 0;
      fio___brotli_bw_put(w, cl_vlc_code[v], cl_vlc_bits[v]);
    }
  }

  /* Step 6: Write RLE-encoded code lengths using the CL Huffman code */
  for (uint32_t i = 0; i < rle_count; ++i) {
    uint8_t sym = rle_syms[i];
    fio___brotli_bw_put_huff(w, cl_codes[sym], cl_lens[sym]);
    if (sym == 16)
      fio___brotli_bw_put(w, rle_extra[i], 2);
    else if (sym == 17)
      fio___brotli_bw_put(w, rle_extra[i], 3);
  }

  (void)codes; /* codes used implicitly via lens for canonical reconstruction */
}

/* *****************************************************************************
Insert-and-copy command encoding (RFC 7932 Section 5)
***************************************************************************** */

/**
 * Encode (insert_len, copy_len) into an IAC command code + extra bits.
 * Returns the command symbol (0-703).
 * Sets *insert_extra_bits, *insert_extra_val, *copy_extra_bits,
 * *copy_extra_val.
 */
FIO_SFUNC uint32_t fio___brotli_encode_iac(uint32_t insert_len,
                                           uint32_t copy_len,
                                           int *use_dist_zero,
                                           uint32_t *insert_extra_bits,
                                           uint32_t *insert_extra_val,
                                           uint32_t *copy_extra_bits,
                                           uint32_t *copy_extra_val) {
  /* Find insert_code: largest i where insert_base[i] <= insert_len */
  uint32_t ic = 0;
  for (uint32_t i = 1; i < 24; ++i) {
    if (fio___brotli_insert_base[i] <= insert_len)
      ic = i;
    else
      break;
  }
  *insert_extra_bits = fio___brotli_insert_extra[ic];
  *insert_extra_val = insert_len - fio___brotli_insert_base[ic];

  /* Find copy_code: largest i where copy_base[i] <= copy_len */
  uint32_t cc = 0;
  for (uint32_t i = 1; i < 24; ++i) {
    if (fio___brotli_copy_base[i] <= copy_len)
      cc = i;
    else
      break;
  }
  *copy_extra_bits = fio___brotli_copy_extra[cc];
  *copy_extra_val = copy_len - fio___brotli_copy_base[cc];

  /* Map (insert_code, copy_code) to command symbol using the IAC table.
   * RFC 7932 Section 5 table:
   *   insert_code 0..7:
   *     copy_code 0..7:   block 0 (dist_zero) or block 2 (normal)
   *     copy_code 8..15:  block 1 (dist_zero) or block 3 (normal)
   *     copy_code 16..23: block 6 (normal only)
   *   insert_code 8..15:
   *     copy_code 0..7:   block 4
   *     copy_code 8..15:  block 5
   *     copy_code 16..23: block 8
   *   insert_code 16..23:
   *     copy_code 0..7:   block 7
   *     copy_code 8..15:  block 9
   *     copy_code 16..23: block 10
   */
  /* clang-format off */
  static const uint8_t block_map[3][3][2] = {
    /* insert 0-7 */  {{0, 2}, {1, 3}, {6, 6}},
    /* insert 8-15 */ {{4, 4}, {5, 5}, {8, 8}},
    /* insert 16-23 */{{7, 7}, {9, 9}, {10, 10}},
  };
  /* clang-format on */

  uint32_t ir = ic >> 3; /* 0, 1, or 2 */
  uint32_t cr = cc >> 3; /* 0, 1, or 2 */
  if (ir > 2)
    ir = 2;
  if (cr > 2)
    cr = 2;

  /* dist_zero blocks are only available for insert 0-7, copy 0-15.
   * If dist_zero was requested but unavailable, clear the flag so the
   * caller knows a distance code must be written. */
  int dz_available = (ir == 0 && cr <= 1);
  int dz_effective = (*use_dist_zero && dz_available);
  *use_dist_zero = dz_effective;

  uint32_t block = block_map[ir][cr][dz_effective ? 0 : 1];
  uint32_t within = ((ic & 7) << 3) | (cc & 7);

  return block * 64 + within;
}

/* *****************************************************************************
Distance encoding (RFC 7932 Section 4, NPOSTFIX=0, NDIRECT=0)
***************************************************************************** */

/**
 * Encode a distance value into a distance symbol + extra bits.
 * With NPOSTFIX=0, NDIRECT=0, distance alphabet size = 64.
 * dist_rb: distance ring buffer [4], dist_rb_idx: next write index.
 * Returns distance symbol. Sets *extra_bits, *extra_val.
 */
FIO_SFUNC uint32_t fio___brotli_encode_distance(uint32_t distance,
                                                const int32_t *dist_rb,
                                                uint32_t dist_rb_idx,
                                                uint32_t *extra_bits,
                                                uint32_t *extra_val) {
  *extra_bits = 0;
  *extra_val = 0;

  /* Check ring buffer: dist_sym 0 = last distance */
  if (distance == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3])
    return 0;
  if (distance == (uint32_t)dist_rb[(dist_rb_idx - 2) & 3])
    return 1;
  if (distance == (uint32_t)dist_rb[(dist_rb_idx - 3) & 3])
    return 2;
  if (distance == (uint32_t)dist_rb[(dist_rb_idx - 4) & 3])
    return 3;

  /* General distance encoding (NPOSTFIX=0, NDIRECT=0):
   * For distance d >= 1:
   *   dextra = d - 1
   *   if dextra < 2: ndistbits = 0, hcode = dextra, dist_sym = 16 + dextra
   *   else: ndistbits = floor(log2(dextra)) - 1
   *         hcode = 2*(ndistbits+1) + ((dextra >> ndistbits) & 1)
   *         dist_sym = 16 + hcode - 4 ... no, let me derive properly.
   *
   * RFC 7932 Section 4:
   *   dist_sym 16+: d = NDIRECT + 1 + (offset + extra) << NPOSTFIX + postfix
   *   With NPOSTFIX=0, NDIRECT=0:
   *     d = 1 + offset + extra
   *     where offset = ((2 + lcode) << ndistbits) - 4
   *     dist_sym = 16 + 2*ndistbits + lcode (for ndistbits >= 1)
   *     dist_sym = 16 + lcode (for ndistbits = 0, offset = -2 or -1)
   *
   * Actually: dist_sym = 16 + 2*(ndistbits+1) + lcode - 4 ... let me just
   * compute from the formula in the decompressor:
   *   d_raw = dist_sym - 16
   *   hcode = d_raw >> 1
   *   lcode = d_raw & 1
   *   nbits = 1 + hcode
   *   offset = ((2 + lcode) << nbits) - 4
   *   distance = offset + extra + 1
   *
   * So to encode: given distance d:
   *   We need: offset + extra = d - 1
   *   offset = ((2 + lcode) << nbits) - 4
   *   We need to find nbits, lcode such that offset <= d-1 < offset +
   * (1<<nbits)
   */
  uint32_t dval = distance - 1; /* 0-based */

  if (dval < 4) {
    /* dist_sym 16..19: nbits=1, covers distances 1-4
     * d_raw=0: hcode=0, lcode=0, nbits=1, offset=2*2-4=0, extra 0..1 -> d=1,2
     * d_raw=1: hcode=0, lcode=1, nbits=1, offset=3*2-4=2, extra 0..1 -> d=3,4
     */
    if (dval < 2) {
      *extra_bits = 1;
      *extra_val = dval;
      return 16; /* d_raw = 0 */
    } else {
      *extra_bits = 1;
      *extra_val = dval - 2;
      return 17; /* d_raw = 1 */
    }
  }

  /* For dval >= 4, find ndistbits (called hcode in decompressor):
   * We need the largest hcode such that ((2 + 0) << (1+hcode)) - 4 <= dval
   * i.e., (2 << nbits) - 4 <= dval, where nbits = 1 + hcode
   * i.e., nbits <= floor(log2((dval + 4) / 2))
   */
  uint32_t nbits = 1;
  while ((2U << (nbits + 1)) - 4 <= dval)
    ++nbits;

  /* Try lcode = 0 first */
  uint32_t offset0 = (2U << nbits) - 4;
  if (dval >= offset0 && dval < offset0 + (1U << nbits)) {
    *extra_bits = nbits;
    *extra_val = dval - offset0;
    uint32_t hcode = nbits - 1;
    return 16 + 2 * hcode; /* d_raw = 2*hcode + 0 */
  }

  /* lcode = 1 */
  uint32_t offset1 = (3U << nbits) - 4;
  if (dval >= offset1 && dval < offset1 + (1U << nbits)) {
    *extra_bits = nbits;
    *extra_val = dval - offset1;
    uint32_t hcode = nbits - 1;
    return 16 + 2 * hcode + 1; /* d_raw = 2*hcode + 1 */
  }

  /* Shouldn't reach here for valid distances, but try next nbits */
  nbits++;
  offset0 = (2U << nbits) - 4;
  *extra_bits = nbits;
  *extra_val = dval - offset0;
  return 16 + 2 * (nbits - 1);
}

/* *****************************************************************************
WBITS encoding (RFC 7932 Section 9.1)
***************************************************************************** */

FIO_SFUNC void fio___brotli_write_wbits(fio___brotli_bw_s *w, uint32_t wbits) {
  /* RFC 7932 Section 9.1 — must match decompressor's parsing:
   *   bit0=0 → WBITS=16
   *   bit0=1, read 3 bits as n:
   *     n=1..7 → WBITS = n+17 (18..24)
   *     n=0 → read 3 more bits as m:
   *       m=0 → WBITS=17
   *       m=1 → invalid (reserved)
   *       m=2..7 → WBITS = m+8 (10..15) */
  if (wbits == 16) {
    fio___brotli_bw_put(w, 0, 1);
  } else if (wbits >= 18 && wbits <= 24) {
    fio___brotli_bw_put(w, 1, 1);
    fio___brotli_bw_put(w, wbits - 17, 3); /* n = wbits-17 (1..7) */
  } else if (wbits == 17) {
    fio___brotli_bw_put(w, 1, 1); /* bit0 = 1 */
    fio___brotli_bw_put(w, 0, 3); /* n = 0 */
    fio___brotli_bw_put(w, 0, 3); /* m = 0 → WBITS=17 */
  } else if (wbits >= 10 && wbits <= 15) {
    fio___brotli_bw_put(w, 1, 1);         /* bit0 = 1 */
    fio___brotli_bw_put(w, 0, 3);         /* n = 0 */
    fio___brotli_bw_put(w, wbits - 8, 3); /* m = wbits-8 (2..7) */
  }
}

/* *****************************************************************************
Meta-block length encoding (RFC 7932 Section 9.2)
***************************************************************************** */

FIO_SFUNC void fio___brotli_write_mlen(fio___brotli_bw_s *w, uint32_t mlen) {
  /* MLEN is encoded as MNIBBLES nibbles (RFC 7932 Section 9.2).
   * MNIBBLES stored as 2 bits: 00→4, 01→5, 10→6, 11→special(empty/metadata).
   * Minimum is 4 nibbles (16 bits). For small MLEN, upper nibbles are zero. */
  uint32_t mlen_m1 = mlen - 1;

  if (mlen_m1 < (1U << 16)) {
    fio___brotli_bw_put(w, 0, 2); /* MNIBBLES = 4 */
    fio___brotli_bw_put(w, mlen_m1, 16);
  } else if (mlen_m1 < (1U << 20)) {
    fio___brotli_bw_put(w, 1, 2); /* MNIBBLES = 5 */
    fio___brotli_bw_put(w, mlen_m1, 20);
  } else {
    fio___brotli_bw_put(w, 2, 2); /* MNIBBLES = 6 */
    fio___brotli_bw_put(w, mlen_m1, 24);
  }
}

/* *****************************************************************************
Hash table for LZ77 match finding
***************************************************************************** */

#define FIO___BROTLI_HASH_BITS 15
#define FIO___BROTLI_HASH_SIZE (1U << FIO___BROTLI_HASH_BITS)

FIO_IFUNC uint32_t fio___brotli_hash4(const uint8_t *p) {
  uint32_t h = fio_buf2u32_le(p);
  return (h * 0x1E35A7BDU) >> (32 - FIO___BROTLI_HASH_BITS);
}

/* *****************************************************************************
Context modeling for q5+ literal coding (RFC 7932 Section 7.1-7.3)
***************************************************************************** */

/**
 * Detect whether input is likely UTF-8 text.
 * Returns FIO___BROTLI_CONTEXT_UTF8 (2) for text, FIO___BROTLI_CONTEXT_LSB6 (0)
 * otherwise.
 */
FIO_SFUNC uint32_t fio___brotli_detect_context_mode(const uint8_t *src,
                                                    uint32_t src_len) {
  uint32_t sample = src_len < 256 ? src_len : 256;
  uint32_t ascii = 0, cont = 0, lead2 = 0, lead3 = 0, lead4 = 0;
  for (uint32_t i = 0; i < sample; ++i) {
    uint8_t b = src[i];
    ascii += (b < 0x80);
    cont += (b >= 0x80 && b <= 0xBF);
    lead2 += (b >= 0xC0 && b <= 0xDF);
    lead3 += (b >= 0xE0 && b <= 0xEF);
    lead4 += (b >= 0xF0 && b <= 0xF7);
  }
  /* UTF-8 heuristic: mostly ASCII, continuation bytes match lead bytes */
  uint32_t expected_cont = lead2 + lead3 * 2 + lead4 * 3;
  if (ascii > sample * 3 / 5 && cont > 0 && expected_cont > 0 &&
      cont >= expected_cont * 3 / 4 && cont <= expected_cont * 5 / 4)
    return FIO___BROTLI_CONTEXT_UTF8;
  /* Also detect pure ASCII text (no high bytes at all) */
  if (ascii > sample * 4 / 5)
    return FIO___BROTLI_CONTEXT_UTF8;
  return FIO___BROTLI_CONTEXT_LSB6;
}

/**
 * Static 2-context UTF-8 context map.
 * Context IDs 48-63 (after letters) → tree 1, everything else → tree 0.
 * This separates "after letter" literals from "after space/punct/digit".
 */
static const uint8_t fio___brotli_cmap_utf8_2[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  0-15 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16-31 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 32-47 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 48-63 */
};

/**
 * Static 2-context LSB6 context map.
 * Even context IDs → tree 0, odd → tree 1.
 */
static const uint8_t fio___brotli_cmap_lsb6_2[64] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, /*  0-15 */
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, /* 16-31 */
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, /* 32-47 */
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, /* 48-63 */
};

/**
 * Write VarLenUint8 value to bitstream (RFC 7932 Section 9.2).
 * Encodes value 0-255. Must match decompressor's read_varlen_uint8.
 */
FIO_IFUNC void fio___brotli_write_varlen_uint8(fio___brotli_bw_s *w,
                                               uint32_t value) {
  if (value == 0) {
    fio___brotli_bw_put(w, 0, 1); /* bit 0 → value = 0 */
    return;
  }
  fio___brotli_bw_put(w, 1, 1); /* bit 1 → more bits follow */
  if (value == 1) {
    fio___brotli_bw_put(w, 0, 3); /* n = 0 → value = 1 */
    return;
  }
  /* Find n such that (1 << n) <= value < (1 << (n+1)) */
  uint32_t n = 1;
  while ((1U << (n + 1)) <= value)
    ++n;
  fio___brotli_bw_put(w, n, 3);                 /* n bits */
  fio___brotli_bw_put(w, value - (1U << n), n); /* extra bits */
}

/**
 * Write a context map to the bitstream (RFC 7932 Section 7.3).
 * map: array of map_size entries, each in [0, num_trees).
 * Must produce output that fio___brotli_read_context_map can decode.
 */
FIO_SFUNC void fio___brotli_write_context_map(fio___brotli_bw_s *w,
                                              const uint8_t *map,
                                              uint32_t map_size,
                                              uint32_t num_trees) {
  /* RLEMAX = 0 (no RLE encoding, map is only 64 entries) */
  fio___brotli_bw_put(w, 0, 1); /* RLEMAX flag = 0 */

  /* Build Huffman code for context map values (alphabet = num_trees) */
  uint32_t cm_freqs[256];
  FIO_MEMSET(cm_freqs, 0, num_trees * sizeof(uint32_t));
  for (uint32_t i = 0; i < map_size; ++i)
    cm_freqs[map[i]]++;

  uint8_t cm_lens[256];
  uint16_t cm_codes[256];
  fio___brotli_build_code_lengths(cm_lens, cm_freqs, num_trees, 15);
  fio___brotli_build_codes(cm_codes, cm_lens, num_trees);

  /* Write the prefix code for context map symbols */
  fio___brotli_write_prefix_code(w, cm_lens, cm_codes, num_trees, num_trees);

  /* Fix single-symbol code lengths (same as main compressor does) */
  {
    uint32_t n = 0;
    for (uint32_t i = 0; i < num_trees; ++i)
      n += (cm_lens[i] != 0);
    if (n == 1)
      for (uint32_t i = 0; i < num_trees; ++i)
        cm_lens[i] = 0;
  }

  /* Write context map entries */
  for (uint32_t i = 0; i < map_size; ++i) {
    uint8_t val = map[i];
    fio___brotli_bw_put_huff(w, cm_codes[val], cm_lens[val]);
  }

  /* IMTF = 0 (no inverse move-to-front) */
  fio___brotli_bw_put(w, 0, 1);
}

/* *****************************************************************************
Score-based match evaluation for q5+ lazy matching
***************************************************************************** */

/** Score a match: higher is better. cached_idx < 0 means not from cache.
 *
 * Scoring model: each byte of match length saves ~8 bits (one literal Huffman
 * code). The distance costs some bits to encode. Cached distances cost ~4 bits
 * (the ring buffer index symbol), non-cached distances cost roughly
 * 2*log2(distance) bits (symbol + extra bits).
 *
 * We use units of "score points" where 100 points ≈ 1 byte of benefit. */
FIO_IFUNC int32_t fio___brotli_match_score(uint32_t len,
                                           uint32_t distance,
                                           int cached_idx) {
  int32_t score = (int32_t)len * 100; /* benefit: bytes saved */
  if (cached_idx >= 0) {
    /* Cached distance: costs ~4 bits for ring buffer symbol.
     * Higher cache indices cost slightly more (less likely to be short code).
     */
    score -= 50 + cached_idx * 12;
  } else {
    /* Non-cached distance: approximate total bit cost.
     * Distance symbol + extra bits ≈ 2 * floor(log2(distance)) bits. */
    uint32_t d = distance;
    int bits = 0;
    while (d > 1) {
      d >>= 1;
      bits++;
    }
    score -= bits * 25;
  }
  return score;
}

/* *****************************************************************************
Hash chain for q5+ LZ77 match finding (32K buckets, depth-16 ring buffer)
***************************************************************************** */

#define FIO___BROTLI_HC_BITS    15
#define FIO___BROTLI_HC_BUCKETS (1U << FIO___BROTLI_HC_BITS)
#define FIO___BROTLI_HC_DEPTH   16

typedef struct {
  uint32_t gen;                                 /* current generation */
  uint32_t bucket_gen[FIO___BROTLI_HC_BUCKETS]; /* per-bucket generation */
  uint16_t num[FIO___BROTLI_HC_BUCKETS];
  uint32_t buckets[FIO___BROTLI_HC_BUCKETS * FIO___BROTLI_HC_DEPTH];
} fio___brotli_hc_s;

/** Reset hash chain via generation counter (no memset of 2MB+ buckets). */
FIO_IFUNC void fio___brotli_hc_reset(fio___brotli_hc_s *hc) {
  hc->gen++;
  /* On gen wrap-around (extremely unlikely: 4 billion calls), zero bucket_gen
   */
  if (hc->gen == 0) {
    FIO_MEMSET(hc->bucket_gen, 0, sizeof(hc->bucket_gen));
    hc->gen = 1;
  }
}

FIO_IFUNC uint32_t fio___brotli_hc_hash4(const uint8_t *p) {
  uint32_t h = fio_buf2u32_le(p);
  /* Use different multiplier from q1-4 hash to reduce correlation */
  return (h * 0x1E35A7BDU) >> (32 - FIO___BROTLI_HC_BITS);
}

FIO_IFUNC void fio___brotli_hc_insert(fio___brotli_hc_s *hc,
                                      uint32_t h,
                                      uint32_t pos) {
  /* If bucket is from a previous generation, reset its count */
  if (hc->bucket_gen[h] != hc->gen) {
    hc->num[h] = 0;
    hc->bucket_gen[h] = hc->gen;
  }
  uint32_t idx = hc->num[h] % FIO___BROTLI_HC_DEPTH;
  hc->buckets[h * FIO___BROTLI_HC_DEPTH + idx] = pos;
  hc->num[h]++;
}

/** Extend a match using 8-byte word-at-a-time comparison.
 *  `len` is the number of already-verified matching bytes.
 *  Returns the total match length (up to max_len). */
FIO_IFUNC uint32_t fio___brotli_extend_match(const uint8_t *src,
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

/** Find best match in hash chain. Returns match length (0 if none >= 4).
 * Uses score-based selection: prefers closer matches when length difference
 * doesn't justify the extra distance cost. This prevents the hash chain from
 * picking longer matches at much farther distances that diversify the distance
 * Huffman tree and hurt overall compression.
 * max_depth limits chain walk depth (0 = use full FIO___BROTLI_HC_DEPTH). */
FIO_SFUNC uint32_t fio___brotli_hc_find(const fio___brotli_hc_s *hc,
                                        const uint8_t *src,
                                        uint32_t src_len,
                                        uint32_t pos,
                                        uint32_t window_size,
                                        uint32_t *best_dist,
                                        uint32_t max_depth) {
  uint32_t h = fio___brotli_hc_hash4(src + pos);
  /* Generation check: if bucket is stale, treat as empty */
  if (hc->bucket_gen[h] != hc->gen)
    return 0;
  uint32_t count = hc->num[h];
  if (!count)
    return 0;
  uint32_t max_len = src_len - pos;
  if (max_len > (1U << 24) - 1)
    max_len = (1U << 24) - 1;
  uint32_t best_len = 3; /* require >= 4 */
  int32_t best_score = 0;
  uint32_t prefix = fio_buf2u32_le(src + pos);
  /* Walk chain from newest to oldest */
  uint32_t depth =
      count < FIO___BROTLI_HC_DEPTH ? count : FIO___BROTLI_HC_DEPTH;
  if (max_depth && depth > max_depth)
    depth = max_depth;
  uint32_t start = (count - 1) % FIO___BROTLI_HC_DEPTH;
  for (uint32_t i = 0; i < depth; ++i) {
    uint32_t slot = (start + FIO___BROTLI_HC_DEPTH - i) % FIO___BROTLI_HC_DEPTH;
    uint32_t candidate = hc->buckets[h * FIO___BROTLI_HC_DEPTH + slot];
    uint32_t dist = pos - candidate;
    if (dist == 0 || dist > window_size)
      continue;
    /* Quick 4-byte prefix rejection */
    if (fio_buf2u32_le(src + candidate) != prefix)
      continue;
    /* Extend match */
    uint32_t len = fio___brotli_extend_match(src, pos, candidate, 4, max_len);
    if (len >= 4) {
      int32_t sc = fio___brotli_match_score(len, dist, -1);
      if (sc > best_score) {
        best_len = len;
        best_score = sc;
        *best_dist = dist;
        if (len == max_len)
          break; /* can't do better */
      }
    }
  }
  return best_len >= 4 ? best_len : 0;
}

/* *****************************************************************************
Static dictionary hash table for q5+ (RFC 7932 Appendix A)
Hashes first 4 bytes of each dictionary word. 2 entries per bucket.
Built once on first use (lazy init with static storage).
***************************************************************************** */

#define FIO___BROTLI_DICT_HASH_BITS 14
#define FIO___BROTLI_DICT_HASH_SIZE (1U << FIO___BROTLI_DICT_HASH_BITS)

typedef struct {
  uint8_t len; /* word length (4-24), 0 = empty slot */
  uint8_t pad;
  uint16_t word_id; /* word index within length group */
} fio___brotli_dict_entry_s;

/* 2 entries per bucket to reduce collisions */
static fio___brotli_dict_entry_s
    fio___brotli_dict_ht[FIO___BROTLI_DICT_HASH_SIZE * 2];
static int fio___brotli_dict_ht_ready = 0;

FIO_IFUNC uint32_t fio___brotli_dict_hash4(const uint8_t *p) {
  uint32_t h = fio_buf2u32_le(p);
  return (h * 0x1E35A7BDU) >> (32 - FIO___BROTLI_DICT_HASH_BITS);
}

/** Build the static dictionary hash table (called once). */
FIO_SFUNC void fio___brotli_dict_ht_build(void) {
  if (fio___brotli_dict_ht_ready)
    return;
  FIO_MEMSET(fio___brotli_dict_ht, 0, sizeof(fio___brotli_dict_ht));
  for (uint32_t wlen = 4; wlen <= 24; ++wlen) {
    if (!fio___brotli_ndbits[wlen])
      continue;
    uint32_t nwords = 1U << fio___brotli_ndbits[wlen];
    uint32_t offset = fio___brotli_dict_offsets[wlen];
    for (uint32_t wid = 0; wid < nwords; ++wid) {
      const uint8_t *word = fio___brotli_dict + offset + wid * wlen;
      uint32_t h = fio___brotli_dict_hash4(word);
      uint32_t base = h * 2;
      /* Insert into first empty slot, or overwrite second (LRU-ish) */
      if (fio___brotli_dict_ht[base].len == 0) {
        fio___brotli_dict_ht[base].len = (uint8_t)wlen;
        fio___brotli_dict_ht[base].word_id = (uint16_t)wid;
      } else if (fio___brotli_dict_ht[base + 1].len == 0) {
        fio___brotli_dict_ht[base + 1].len = (uint8_t)wlen;
        fio___brotli_dict_ht[base + 1].word_id = (uint16_t)wid;
      } else {
        /* Both slots full — overwrite second slot */
        fio___brotli_dict_ht[base + 1].len = (uint8_t)wlen;
        fio___brotli_dict_ht[base + 1].word_id = (uint16_t)wid;
      }
    }
  }
  fio___brotli_dict_ht_ready = 1;
}

/**
 * Try to find a static dictionary match at position `pos`.
 * Returns the effective match length (after transform), or 0 if none found.
 * Sets *out_copy_len (word length for IAC), *out_distance (dict distance),
 * *out_score.
 *
 * Only tries identity (transform 0) and uppercase_first with no prefix/suffix
 * (transform 9). These are the only transforms that produce output of exactly
 * `word_len` bytes, matching the copy_len in the IAC command.
 */
FIO_SFUNC uint32_t fio___brotli_dict_find(const uint8_t *src,
                                          uint32_t src_len,
                                          uint32_t pos,
                                          uint32_t window_size,
                                          uint32_t *out_copy_len,
                                          uint32_t *out_distance,
                                          int32_t *out_score) {
  if (pos + 4 > src_len)
    return 0;
  uint32_t max_backward = (pos < window_size) ? pos : window_size;
  uint32_t h = fio___brotli_dict_hash4(src + pos);
  uint32_t base = h * 2;
  uint32_t best_len = 0;
  int32_t best_score = 0;

  for (uint32_t slot = 0; slot < 2; ++slot) {
    fio___brotli_dict_entry_s e = fio___brotli_dict_ht[base + slot];
    if (e.len == 0)
      continue;
    uint32_t wlen = e.len;
    uint32_t wid = e.word_id;
    if (pos + wlen > src_len)
      continue;
    uint32_t nwords = 1U << fio___brotli_ndbits[wlen];
    const uint8_t *word =
        fio___brotli_dict + fio___brotli_dict_offsets[wlen] + wid * wlen;

    /* Quick 4-byte prefix rejection */
    if (fio_buf2u32_le(src + pos) != fio_buf2u32_le(word)) {
      /* Try uppercase_first: first byte uppercase in input, lowercase in dict
       */
      if (!(src[pos] >= 'A' && src[pos] <= 'Z' && word[0] == (src[pos] | 0x20)))
        continue;
      /* Check remaining 3 bytes of prefix */
      if (src[pos + 1] != word[1] || src[pos + 2] != word[2] ||
          src[pos + 3] != word[3])
        continue;
      /* Full match check for uppercase_first (transform index 9) */
      int match = 1;
      for (uint32_t k = 4; k < wlen; ++k) {
        if (src[pos + k] != word[k]) {
          match = 0;
          break;
        }
      }
      if (!match)
        continue;
      /* Transform 9: uppercase_first, no prefix, no suffix */
      uint32_t dist = max_backward + 1 + wid + 9 * nwords;
      uint32_t d = dist;
      int bits = 0;
      while (d > 1) {
        d >>= 1;
        bits++;
      }
      int32_t sc = (int32_t)wlen * 100 - bits * 25;
      if (sc > best_score && sc > 0) {
        best_len = wlen;
        best_score = sc;
        *out_copy_len = wlen;
        *out_distance = dist;
        *out_score = sc;
        (void)0;
      }
      continue;
    }

    /* Full match check for identity transform (transform 0) */
    if (wlen > 4 && FIO_MEMCMP(src + pos + 4, word + 4, wlen - 4) != 0)
      continue;

    uint32_t dist = max_backward + 1 + wid; /* transform 0: + 0 * nwords */
    uint32_t d = dist;
    int bits = 0;
    while (d > 1) {
      d >>= 1;
      bits++;
    }
    int32_t sc = (int32_t)wlen * 100 - bits * 25;
    if (sc > best_score && sc > 0) {
      best_len = wlen;
      best_score = sc;
      *out_copy_len = wlen;
      *out_distance = dist;
      *out_score = sc;
      (void)0;
    }
  }

  return best_len;
}

/* *****************************************************************************
Block splitting for q6+ literal coding (RFC 7932 Section 6)
***************************************************************************** */

/** Block split command: (block_type, block_length) pair. */
typedef struct {
  uint32_t type;
  uint32_t length;
} fio___brotli_blksplit_s;

/** Encode a block count value: find the symbol and extra bits. */
FIO_IFUNC void fio___brotli_encode_block_count(uint32_t count,
                                               uint32_t *sym,
                                               uint32_t *extra_bits,
                                               uint32_t *extra_val) {
  /* Linear scan — only 26 symbols, called rarely (per block switch). */
  for (uint32_t s = 25; s > 0; --s) {
    if (count >= fio___brotli_block_count_base[s]) {
      *sym = s;
      *extra_bits = fio___brotli_block_count_extra[s];
      *extra_val = count - fio___brotli_block_count_base[s];
      return;
    }
  }
  *sym = 0;
  *extra_bits = fio___brotli_block_count_extra[0];
  *extra_val = count - fio___brotli_block_count_base[0];
}

/** Shannon entropy cost of a byte histogram (in bits). */
FIO_SFUNC double fio___brotli_entropy_cost(const uint32_t *freq,
                                           uint32_t total) {
  if (total == 0)
    return 0.0;
  double cost = 0.0;
  for (uint32_t i = 0; i < 256; ++i) {
    if (freq[i] > 0) {
      /* -freq[i] * log2(freq[i]/total) = freq[i] * log2(total/freq[i]) */
      double log2_inv_p = 0.0;
      /* Fast integer log2 approximation + fractional correction */
      uint32_t ratio = total / freq[i];
      uint32_t r = ratio;
      while (r > 1) {
        r >>= 1;
        log2_inv_p += 1.0;
      }
      /* Fractional correction: linear interpolation */
      double frac = (double)ratio / (double)(1U << (uint32_t)log2_inv_p) - 1.0;
      log2_inv_p += frac * 0.6; /* approximate log2(1+x) ≈ 0.6x for x in
                                    [0,1] */
      cost += (double)freq[i] * log2_inv_p;
    }
  }
  return cost;
}

/**
 * Greedy literal block splitting for q6.
 *
 * Processes the literal stream (extracted from tokens) and produces
 * block split commands. Each block gets its own literal Huffman tree.
 *
 * Algorithm: Process literals in chunks of ~512. At each boundary, decide:
 * - Continue current block type (entropy similar to current histogram)
 * - Switch to previous block type (entropy closer to second-to-last)
 * - Start new block type (entropy sufficiently different, saves > 28 bits)
 *
 * Returns number of block splits. splits[] is filled with (type, length) pairs.
 * Max 256 block types, max max_splits entries.
 */
FIO_SFUNC uint32_t fio___brotli_split_literals(const uint8_t *src,
                                               const void *tokens_ptr,
                                               uint32_t token_count,
                                               fio___brotli_blksplit_s *splits,
                                               uint32_t max_splits) {
  /* Re-declare token struct to avoid forward declaration issues */
  typedef struct {
    uint32_t insert_len;
    uint32_t copy_len;
    uint32_t distance;
    int use_dist_zero;
    int is_dict_ref;
  } tok_s;
  const tok_s *tokens = (const tok_s *)tokens_ptr;

  /* Count total literals */
  uint32_t total_lits = 0;
  for (uint32_t t = 0; t < token_count; ++t)
    total_lits += tokens[t].insert_len;

  /* Need enough literals to justify splitting overhead.
   * Block splitting adds block type/count Huffman codes + context map overhead.
   * On small data (<8KB), this overhead exceeds any entropy benefit. */
  if (total_lits < 8192 || max_splits < 2) {
    splits[0].type = 0;
    splits[0].length = total_lits;
    return 1;
  }

/* Per-block-type histograms (max 256 types, but we limit to ~32 for
 * practical purposes to keep stack usage reasonable) */
#define FIO___BROTLI_MAX_BLKTYPES 32
  uint32_t histograms[FIO___BROTLI_MAX_BLKTYPES][256];
  uint32_t hist_totals[FIO___BROTLI_MAX_BLKTYPES];
  FIO_MEMSET(histograms, 0, sizeof(histograms));
  FIO_MEMSET(hist_totals, 0, sizeof(hist_totals));

  /* Chunk size for block boundary decisions */
  uint32_t chunk_size = 512;
  /* Switch cost in bits — from reference encoder */
  double switch_cost = 28.1;

  uint32_t num_types = 1;
  uint32_t cur_type = 0;
  uint32_t prev_type = 1; /* per RFC 7932 spec, initial prev_type = 1 */
  uint32_t split_count = 0;
  uint32_t cur_block_len = 0;

  /* Current chunk histogram */
  uint32_t chunk_freq[256];
  FIO_MEMSET(chunk_freq, 0, sizeof(chunk_freq));
  uint32_t chunk_count = 0;

  /* Walk through all literals */
  uint32_t lit_pos = 0;
  for (uint32_t t = 0; t < token_count; ++t) {
    for (uint32_t i = 0; i < tokens[t].insert_len; ++i) {
      uint32_t p = lit_pos + i;
      uint8_t byte = src[p];

      chunk_freq[byte]++;
      chunk_count++;
      cur_block_len++;

      /* At chunk boundary, decide whether to split */
      if (chunk_count >= chunk_size) {
        /* Add chunk to current type's histogram */
        for (uint32_t b = 0; b < 256; ++b)
          histograms[cur_type][b] += chunk_freq[b];
        hist_totals[cur_type] += chunk_count;

        /* If we have enough data in current type, evaluate split */
        if (cur_block_len > chunk_size && hist_totals[cur_type] > chunk_size) {
          /* Cost of encoding this chunk with current type's histogram */
          double cost_current =
              fio___brotli_entropy_cost(chunk_freq, chunk_count);

          /* Try previous type if it has data */
          double cost_prev = 1e30;
          if (prev_type < num_types && hist_totals[prev_type] > 0) {
            /* Estimate cost using prev type's histogram as predictor */
            double prev_total_cost =
                fio___brotli_entropy_cost(histograms[prev_type],
                                          hist_totals[prev_type]);
            double prev_per_byte =
                (hist_totals[prev_type] > 0)
                    ? prev_total_cost / (double)hist_totals[prev_type]
                    : 8.0;
            cost_prev = prev_per_byte * (double)chunk_count;
          }

          /* Cost of starting a new type (just the chunk's own entropy) */
          double cost_new = cost_current; /* self-entropy is optimal */

          /* Savings from switching to prev vs staying */
          double save_prev = cost_current - cost_prev - switch_cost;
          /* Savings from new type vs staying (new type gets its own tree) */
          double save_new = cost_current - cost_new - switch_cost -
                            40.0; /* extra overhead for new Huffman tree */

          if (save_prev > 0 && save_prev >= save_new &&
              split_count + 1 < max_splits) {
            /* Switch to previous type */
            /* Finalize current block */
            uint32_t final_len = cur_block_len - chunk_count;
            if (final_len > 0) {
              splits[split_count].type = cur_type;
              splits[split_count].length = final_len;
              split_count++;
            }
            /* Remove chunk from current type's histogram (we're switching) */
            for (uint32_t b = 0; b < 256; ++b)
              histograms[cur_type][b] -= chunk_freq[b];
            hist_totals[cur_type] -= chunk_count;
            /* Add chunk to prev type */
            for (uint32_t b = 0; b < 256; ++b)
              histograms[prev_type][b] += chunk_freq[b];
            hist_totals[prev_type] += chunk_count;

            uint32_t old_type = cur_type;
            cur_type = prev_type;
            prev_type = old_type;
            cur_block_len = chunk_count;
          } else if (save_new > switch_cost &&
                     num_types < FIO___BROTLI_MAX_BLKTYPES &&
                     split_count + 1 < max_splits) {
            /* Start new block type */
            uint32_t final_len = cur_block_len - chunk_count;
            if (final_len > 0) {
              splits[split_count].type = cur_type;
              splits[split_count].length = final_len;
              split_count++;
            }
            /* Remove chunk from current type's histogram */
            for (uint32_t b = 0; b < 256; ++b)
              histograms[cur_type][b] -= chunk_freq[b];
            hist_totals[cur_type] -= chunk_count;
            /* New type gets the chunk */
            prev_type = cur_type;
            cur_type = num_types;
            for (uint32_t b = 0; b < 256; ++b)
              histograms[cur_type][b] = chunk_freq[b];
            hist_totals[cur_type] = chunk_count;
            num_types++;
            cur_block_len = chunk_count;
          }
        }

        /* Reset chunk */
        FIO_MEMSET(chunk_freq, 0, sizeof(chunk_freq));
        chunk_count = 0;
      }
    }
    lit_pos += tokens[t].insert_len;
    if (tokens[t].distance > 0)
      lit_pos += tokens[t].copy_len;
  }

  /* Flush remaining chunk to current type */
  if (chunk_count > 0) {
    for (uint32_t b = 0; b < 256; ++b)
      histograms[cur_type][b] += chunk_freq[b];
    hist_totals[cur_type] += chunk_count;
  }

  /* Finalize last block */
  if (cur_block_len > 0 && split_count < max_splits) {
    splits[split_count].type = cur_type;
    splits[split_count].length = cur_block_len;
    split_count++;
  }

  /* If only 1 type was used, simplify */
  if (num_types == 1 || split_count <= 1) {
    splits[0].type = 0;
    splits[0].length = total_lits;
    return 1;
  }

  /* Compact: remap types to be contiguous 0..N-1 */
  uint8_t type_used[FIO___BROTLI_MAX_BLKTYPES];
  uint8_t type_remap[FIO___BROTLI_MAX_BLKTYPES];
  FIO_MEMSET(type_used, 0, sizeof(type_used));
  for (uint32_t s = 0; s < split_count; ++s)
    type_used[splits[s].type] = 1;
  uint32_t remap_count = 0;
  for (uint32_t t = 0; t < num_types; ++t) {
    if (type_used[t])
      type_remap[t] = (uint8_t)remap_count++;
  }
  for (uint32_t s = 0; s < split_count; ++s)
    splits[s].type = type_remap[splits[s].type];

  /* Merge adjacent blocks of the same type */
  uint32_t w = 0;
  for (uint32_t s = 0; s < split_count; ++s) {
    if (w > 0 && splits[w - 1].type == splits[s].type) {
      splits[w - 1].length += splits[s].length;
    } else {
      splits[w++] = splits[s];
    }
  }
  split_count = w;

  if (split_count <= 1) {
    splits[0].type = 0;
    splits[0].length = total_lits;
    return 1;
  }

  return split_count;
#undef FIO___BROTLI_MAX_BLKTYPES
}

/**
 * Write block type Huffman code for the encoder.
 * Alphabet = num_types + 2 (0=repeat_prev, 1=next, 2..N+1=explicit).
 * Encodes a block type code given current/prev type and the new type.
 */
FIO_IFUNC uint32_t fio___brotli_encode_block_type(uint32_t new_type,
                                                  uint32_t cur_type,
                                                  uint32_t prev_type,
                                                  uint32_t num_types) {
  if (new_type == prev_type)
    return 0; /* repeat previous */
  if (new_type == (cur_type + 1) % num_types)
    return 1;          /* next type */
  return new_type + 2; /* explicit */
}

/* *****************************************************************************
Main compressor
***************************************************************************** */

void fio_brotli_compress___(void); /* IDE Marker */
SFUNC size_t fio_brotli_compress FIO_NOOP(void *out,
                                          size_t out_len,
                                          const void *in,
                                          size_t in_len,
                                          int quality) {
  if (!out || !out_len)
    return 0;

  /* Empty input: emit valid empty Brotli stream */
  if (!in || !in_len) {
    if (out_len < 1)
      return 0;
    /* WBITS=16 (bit0=0), ISLAST=1 (bit1=1), ISLASTEMPTY=1 (bit2=1),
     * padding zeros -> byte = 0b00000110 = 0x06 */
    ((uint8_t *)out)[0] = 0x06;
    return 1;
  }

  if (quality < 1)
    quality = 1;
  if (quality > 6)
    quality = 6;

  const uint8_t *src = (const uint8_t *)in;
  uint32_t src_len = (uint32_t)(in_len > 0x01000000U ? 0x01000000U : in_len);

  /* Choose WBITS: smallest power-of-2 window >= input size */
  uint32_t wbits = 10;
  while (wbits < FIO___BROTLI_MAX_WBITS && (1U << wbits) < src_len + 16)
    ++wbits;

  /* Allocate hash table (q1-4, or q5+ on small data) or hash chain (q5+).
   * On small data (<8KB), the hash chain's multi-entry buckets provide no
   * benefit (most buckets have 0-1 entries at ~12% load), while the distance
   * cache and score-based lazy matching add overhead that hurts compression.
   * Use q4's simpler direct-mapped hash for match finding in this case. */
  uint32_t *hash_table = NULL;
  size_t hash_alloc = 0; /* total allocation size for hash_table + hash_gen */
  size_t hc_alloc = 0;
  fio___brotli_hc_s *hash_chain = NULL;
  int q5_small_data = (quality >= 5 && src_len < 8192);
  /* Note: q6 block splitting also disabled for small data (threshold 8192
   * in fio___brotli_split_literals), so q5_small_data covers both q5+q6. */

  /* q1-4 hash_gen[] parallel array for generation-based lazy clearing.
   * Avoids zeroing the full 128KB hash_table on each call. */
  uint32_t *hash_gen = NULL;
  uint32_t hash_generation = 1; /* generation counter for q1-4 path */

  if (quality >= 5 && !q5_small_data) {
    hc_alloc = sizeof(fio___brotli_hc_s);
    hash_chain = (fio___brotli_hc_s *)FIO_MEM_REALLOC(NULL, 0, hc_alloc, 0);
    if (!hash_chain)
      return 0;
    /* Only zero bucket_gen[] + gen (not the 2MB+ buckets array).
     * fio___brotli_hc_insert checks bucket_gen[h] != gen before trusting. */
    FIO_MEMSET(hash_chain->bucket_gen, 0, sizeof(hash_chain->bucket_gen));
    hash_chain->gen = 1;
  } else {
    hash_alloc = sizeof(uint32_t) * FIO___BROTLI_HASH_SIZE + /* hash_table */
                 sizeof(uint32_t) * FIO___BROTLI_HASH_SIZE;  /* hash_gen */
    hash_table = (uint32_t *)FIO_MEM_REALLOC(NULL, 0, hash_alloc, 0);
    if (!hash_table)
      return 0;
    hash_gen = hash_table + FIO___BROTLI_HASH_SIZE;
    /* Only zero hash_gen[] (not hash_table[] — stale entries detected by
     * generation mismatch). */
    FIO_MEMSET(hash_gen, 0, sizeof(uint32_t) * FIO___BROTLI_HASH_SIZE);
  }
  if (quality >= 5) {
    /* Build static dictionary hash table (lazy init, built once) */
    fio___brotli_dict_ht_build();
  }

  /* LZ77 tokens: (insert_len, copy_len, distance) */
  typedef struct {
    uint32_t insert_len;
    uint32_t copy_len;
    uint32_t distance;
    int use_dist_zero; /* 1 if distance == last distance */
    int is_dict_ref;   /* 1 if static dictionary reference (no ring buf push) */
  } fio___brotli_token_s;

  size_t max_tokens = (size_t)src_len + 1;
  size_t token_alloc = sizeof(fio___brotli_token_s) * max_tokens;
  fio___brotli_token_s *tokens =
      (fio___brotli_token_s *)FIO_MEM_REALLOC(NULL, 0, token_alloc, 0);
  if (!tokens) {
    if (hash_chain)
      FIO_MEM_FREE(hash_chain, hc_alloc);
    if (hash_table)
      FIO_MEM_FREE(hash_table, hash_alloc);
    return 0;
  }
  uint32_t token_count = 0;

  /* Distance ring buffer for encoding — must match decompressor init.
   * dist_rb[(idx-1)&3] = most recent distance = 4 per RFC 7932 Section 4. */
  int32_t dist_rb[4] = {16,
                        15,
                        11,
                        4}; /* [idx-4]=16,[idx-3]=15,[idx-2]=11,[idx-1]=4 */
  uint32_t dist_rb_idx = 0;

  /* === Pass 1: LZ77 match finding === */
  uint32_t pos = 0;
  uint32_t insert_start = 0; /* start of current literal run */
  uint32_t prev_match_len = 0;
  uint32_t prev_match_dist = 0;
  uint32_t prev_match_pos = 0;
  int prev_was_match = 0;
  int32_t prev_match_score = 0;
  int prev_match_is_dict = 0;
  uint32_t window_size = (1U << wbits) - 16;

  if (quality >= 5 && !q5_small_data) {
    /* ================================================================
     * q5+ path: hash chain + distance cache + score-based lazy matching
     * (only for data >= 8KB where hash chain depth provides benefit)
     *
     * Incompressibility detection: after ~1024 positions, if the match
     * rate is below ~5%, the data is effectively random/incompressible.
     * In that case, fall back to a simpler mode: depth-1 hash chain
     * lookup only (no distance cache, no dict, no score-based lazy).
     * This avoids walking 16 chain entries + 4 cache entries + dict
     * lookup per position for zero benefit.
     * ================================================================ */
    uint32_t q5_probe_count = 0; /* positions checked during probe phase */
    uint32_t q5_match_count = 0; /* matches found during probe phase */
    int q5_fast_mode = 0;        /* 1 = incompressible, use fast path */
    while (pos < src_len) {
      uint32_t match_len = 0;
      uint32_t match_dist = 0;
      int32_t match_score = 0;
      int match_is_dict = 0;

      if (pos + 3 < src_len) {
        uint32_t max_len = src_len - pos;
        if (max_len > (1U << 24) - 1)
          max_len = (1U << 24) - 1;

        if (q5_fast_mode) {
          /* --- Fast mode: depth-1 hash chain only (no cache/dict/lazy) --- */
          uint32_t hc_dist = 0;
          uint32_t hc_len = fio___brotli_hc_find(hash_chain,
                                                 src,
                                                 src_len,
                                                 pos,
                                                 window_size,
                                                 &hc_dist,
                                                 1);
          if (hc_len >= 4) {
            match_len = hc_len;
            match_dist = hc_dist;
          }
        } else {
          /* --- Distance cache check --- */
          for (int ci = 0; ci < 4; ++ci) {
            int32_t d = dist_rb[(dist_rb_idx - 1 - ci) & 3];
            if (d <= 0 || pos < (uint32_t)d)
              continue;
            /* Extend match from cache candidate */
            uint32_t cand = pos - (uint32_t)d;
            uint32_t clen =
                fio___brotli_extend_match(src, pos, cand, 0, max_len);
            if (clen >= 4) {
              int32_t sc = fio___brotli_match_score(clen, (uint32_t)d, ci);
              if (sc > match_score) {
                match_len = clen;
                match_dist = (uint32_t)d;
                match_score = sc;
                match_is_dict = 0;
              }
            }
          }

          /* --- Hash chain lookup --- */
          uint32_t hc_dist = 0;
          uint32_t hc_len = fio___brotli_hc_find(hash_chain,
                                                 src,
                                                 src_len,
                                                 pos,
                                                 window_size,
                                                 &hc_dist,
                                                 0);
          if (hc_len >= 4) {
            int32_t sc = fio___brotli_match_score(hc_len, hc_dist, -1);
            if (sc > match_score) {
              match_len = hc_len;
              match_dist = hc_dist;
              match_score = sc;
              match_is_dict = 0;
            }
          }

          /* --- Static dictionary lookup (when LZ77 match is weak) --- */
          if (match_len < 6) {
            uint32_t dict_copy_len = 0, dict_dist = 0;
            int32_t dict_score = 0;
            uint32_t dict_len = fio___brotli_dict_find(src,
                                                       src_len,
                                                       pos,
                                                       window_size,
                                                       &dict_copy_len,
                                                       &dict_dist,
                                                       &dict_score);
            if (dict_len > 0 && dict_score > match_score) {
              match_len = dict_copy_len; /* word length = copy_len for IAC */
              match_dist = dict_dist;
              match_score = dict_score;
              match_is_dict = 1;
            }
          }
        }
      }

      /* Insert current position into hash chain */
      fio___brotli_hc_insert(hash_chain, fio___brotli_hc_hash4(src + pos), pos);

      /* --- Incompressibility probe: track match rate in first 1024 pos --- */
      if (!q5_fast_mode) {
        q5_probe_count++;
        q5_match_count += (match_len >= 4);
        if (q5_probe_count >= 1024) {
          /* < 5% match rate => incompressible, switch to fast mode.
           * On compressible data (HTML/JSON), match rate is typically 30-60%.
           * On random data, match rate is ~0.4% (hash collisions only). */
          q5_fast_mode = (q5_match_count < 51);
        }
      }

      /* --- Lazy matching (score-based for normal mode, simple for fast) --- */
      if (q5_fast_mode) {
        /* Fast mode: simple lazy matching (same as q3-q4 path).
         * No score-based comparison — just check if current match is
         * meaningfully longer than the deferred one. */
        if (prev_was_match) {
          if (match_len > prev_match_len + 1) {
            /* Better match here: emit literal for previous position */
            prev_was_match = 0;
          } else {
            /* Use previous match (may be a dict ref from normal mode at the
             * transition point — preserve is_dict_ref for correctness). */
            uint32_t il = prev_match_pos - insert_start;
            int use_dz =
                (!prev_match_is_dict &&
                 prev_match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
            tokens[token_count].insert_len = il;
            tokens[token_count].copy_len = prev_match_len;
            tokens[token_count].distance = prev_match_dist;
            tokens[token_count].use_dist_zero = use_dz;
            tokens[token_count].is_dict_ref = prev_match_is_dict;
            token_count++;

            if (!use_dz && !prev_match_is_dict) {
              dist_rb[dist_rb_idx & 3] = (int32_t)prev_match_dist;
              dist_rb_idx++;
            }

            /* Insert skipped positions into hash chain */
            uint32_t skip_end = prev_match_pos + prev_match_len;
            if (skip_end > src_len)
              skip_end = src_len;
            for (uint32_t j = prev_match_pos + 1;
                 j < skip_end && j + 3 < src_len;
                 ++j)
              fio___brotli_hc_insert(hash_chain,
                                     fio___brotli_hc_hash4(src + j),
                                     j);

            insert_start = skip_end;
            if (pos < skip_end) {
              pos = skip_end;
              prev_was_match = 0;
              continue;
            }
            prev_was_match = 0;
            /* Fall through to evaluate current match */
          }
        }

        if (match_len >= 4) {
          /* Defer: check next position for better match */
          prev_match_len = match_len;
          prev_match_dist = match_dist;
          prev_match_pos = pos;
          prev_match_is_dict = 0;
          prev_was_match = 1;
          pos++;
          continue;
        }
        pos++;
      } else {
        /* Normal mode: score-based lazy matching */
        if (prev_was_match) {
          /* Require meaningful improvement to override previous match.
           * Deferring costs 1 literal byte (~8 bits). With 100 points per byte,
           * the threshold should be ~80 points (slightly less than a full byte
           * since the literal may compress well). */
          if (match_score > prev_match_score + 80) {
            /* Better match here: drop previous, emit literal for prev pos */
            prev_was_match = 0;
          } else {
            /* Use previous match */
            uint32_t il = prev_match_pos - insert_start;
            int use_dz =
                (!prev_match_is_dict &&
                 prev_match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
            tokens[token_count].insert_len = il;
            tokens[token_count].copy_len = prev_match_len;
            tokens[token_count].distance = prev_match_dist;
            tokens[token_count].use_dist_zero = use_dz;
            tokens[token_count].is_dict_ref = prev_match_is_dict;
            token_count++;

            /* Dict refs don't update ring buffer (RFC 7932 Section 4) */
            if (!use_dz && !prev_match_is_dict) {
              dist_rb[dist_rb_idx & 3] = (int32_t)prev_match_dist;
              dist_rb_idx++;
            }

            /* Insert skipped positions into hash chain */
            uint32_t skip_end = prev_match_pos + prev_match_len;
            if (skip_end > src_len)
              skip_end = src_len;
            for (uint32_t j = prev_match_pos + 1;
                 j < skip_end && j + 3 < src_len;
                 ++j)
              fio___brotli_hc_insert(hash_chain,
                                     fio___brotli_hc_hash4(src + j),
                                     j);

            insert_start = skip_end;
            if (pos < skip_end) {
              pos = skip_end;
              prev_was_match = 0;
              continue;
            }
            prev_was_match = 0;
            /* Fall through to evaluate current match */
          }
        }

        if (match_len >= 4) {
          /* Defer: check next position for better match */
          prev_match_len = match_len;
          prev_match_dist = match_dist;
          prev_match_pos = pos;
          prev_match_score = match_score;
          prev_match_is_dict = match_is_dict;
          prev_was_match = 1;
          pos++;
          continue;
        }
        pos++;
      }
    }
  } else {
    /* ================================================================
     * q1-4 path: direct-mapped hash table + simple lazy matching.
     * Also used by q5+ on small data (<8KB) where hash chain provides
     * no benefit — most buckets have 0-1 entries at ~12% load factor,
     * and the distance cache / score-based lazy matching add overhead
     * that hurts compression ratio. No dict lookup for small data
     * either — dict distances expand the Huffman tree too much.
     * ================================================================ */
    while (pos < src_len) {
      uint32_t match_len = 0;
      uint32_t match_dist = 0;

      if (pos + 3 < src_len) {
        uint32_t h = fio___brotli_hash4(src + pos);
        uint32_t candidate = hash_table[h];
        hash_table[h] = pos;

        if (hash_gen[h] == hash_generation && pos - candidate <= window_size &&
            pos - candidate > 0) {
          /* Check if first 4 bytes match */
          if (fio_buf2u32_le(src + pos) == fio_buf2u32_le(src + candidate)) {
            uint32_t max_len = src_len - pos;
            if (max_len > (1U << 24) - 1)
              max_len = (1U << 24) - 1;
            match_len =
                fio___brotli_extend_match(src, pos, candidate, 4, max_len);
            match_dist = pos - candidate;
          }
        }
        hash_gen[h] = hash_generation;
      }

      /* Lazy matching for quality >= 3 */
      if (quality >= 3 && prev_was_match && pos + 3 < src_len) {
        if (match_len > prev_match_len + 1) {
          /* Better match here: emit literal for previous position */
          prev_was_match = 0;
          /* The hash was already updated for prev position, continue */
        } else {
          /* Use previous match */
          uint32_t il = prev_match_pos - insert_start;
          int use_dz =
              (prev_match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
          tokens[token_count].insert_len = il;
          tokens[token_count].copy_len = prev_match_len;
          tokens[token_count].distance = prev_match_dist;
          tokens[token_count].use_dist_zero = use_dz;
          tokens[token_count].is_dict_ref = 0;
          token_count++;

          /* Update distance ring buffer */
          if (!use_dz) {
            dist_rb[dist_rb_idx & 3] = (int32_t)prev_match_dist;
            dist_rb_idx++;
          }

          /* Update hash for skipped positions */
          uint32_t skip_end = prev_match_pos + prev_match_len;
          if (skip_end > src_len)
            skip_end = src_len;
          for (uint32_t j = prev_match_pos + 1; j < skip_end && j + 3 < src_len;
               ++j) {
            uint32_t hj = fio___brotli_hash4(src + j);
            hash_table[hj] = j;
            hash_gen[hj] = hash_generation;
          }

          insert_start = skip_end;
          pos = skip_end;
          prev_was_match = 0;
          continue;
        }
      }

      if (prev_was_match) {
        /* Emit previous match (no better match found) */
        uint32_t il = prev_match_pos - insert_start;
        int use_dz =
            (prev_match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
        tokens[token_count].insert_len = il;
        tokens[token_count].copy_len = prev_match_len;
        tokens[token_count].distance = prev_match_dist;
        tokens[token_count].use_dist_zero = use_dz;
        tokens[token_count].is_dict_ref = 0;
        token_count++;

        if (!use_dz) {
          dist_rb[dist_rb_idx & 3] = (int32_t)prev_match_dist;
          dist_rb_idx++;
        }

        uint32_t skip_end = prev_match_pos + prev_match_len;
        if (skip_end > src_len)
          skip_end = src_len;
        for (uint32_t j = prev_match_pos + 1; j < skip_end && j + 3 < src_len;
             ++j) {
          uint32_t hj = fio___brotli_hash4(src + j);
          hash_table[hj] = j;
          hash_gen[hj] = hash_generation;
        }

        insert_start = skip_end;
        /* If current pos is within the match we just emitted, skip ahead */
        if (pos < skip_end) {
          pos = skip_end;
          prev_was_match = 0;
          continue;
        }
        prev_was_match = 0;
      }

      if (match_len >= 4) {
        if (quality >= 3) {
          /* Defer: check next position for better match */
          prev_match_len = match_len;
          prev_match_dist = match_dist;
          prev_match_pos = pos;
          prev_was_match = 1;
          pos++;
          continue;
        }

        /* Greedy: emit match immediately */
        uint32_t il = pos - insert_start;
        int use_dz = (match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
        tokens[token_count].insert_len = il;
        tokens[token_count].copy_len = match_len;
        tokens[token_count].distance = match_dist;
        tokens[token_count].use_dist_zero = use_dz;
        tokens[token_count].is_dict_ref = 0;
        token_count++;

        if (!use_dz) {
          dist_rb[dist_rb_idx & 3] = (int32_t)match_dist;
          dist_rb_idx++;
        }

        /* Update hash for match positions */
        for (uint32_t j = pos + 1; j < pos + match_len && j + 3 < src_len;
             ++j) {
          uint32_t hj = fio___brotli_hash4(src + j);
          hash_table[hj] = j;
          hash_gen[hj] = hash_generation;
        }

        insert_start = pos + match_len;
        pos += match_len;
      } else {
        pos++;
      }
    }
  } /* end q1-4 vs q5+ */

  /* Flush any pending lazy match */
  if (prev_was_match) {
    uint32_t il = prev_match_pos - insert_start;
    int use_dz = (!prev_match_is_dict &&
                  prev_match_dist == (uint32_t)dist_rb[(dist_rb_idx - 1) & 3]);
    tokens[token_count].insert_len = il;
    tokens[token_count].copy_len = prev_match_len;
    tokens[token_count].distance = prev_match_dist;
    tokens[token_count].use_dist_zero = use_dz;
    tokens[token_count].is_dict_ref = prev_match_is_dict;
    token_count++;
    insert_start = prev_match_pos + prev_match_len;
  }

  /* Final trailing literals: emit as insert-only command */
  if (insert_start < src_len) {
    tokens[token_count].insert_len = src_len - insert_start;
    tokens[token_count].copy_len = 4; /* minimum copy_len for IAC encoding */
    tokens[token_count].distance = 0; /* signals: no actual copy */
    tokens[token_count].use_dist_zero = 0;
    tokens[token_count].is_dict_ref = 0;
    token_count++;
  }

  /* === Dictionary reference cost/benefit analysis ===
   * Dictionary references use large distances that require high-numbered
   * distance symbols, expanding the distance Huffman tree. If the total
   * savings from dictionary matches don't exceed the tree overhead,
   * convert all dict refs back to literals for better compression.
   *
   * True costs (often underestimated):
   * 1. Distance Huffman tree expansion: dict distances use high symbols
   *    (>= max_backward + 1), requiring the distance prefix code to describe
   *    many more symbols. Each additional distance symbol costs ~8-15 bits
   *    in the prefix code description.
   * 2. IAC command fragmentation: each dict ref is a separate token,
   *    increasing the IAC alphabet usage and Huffman tree size.
   * 3. Per-ref distance encoding: symbol (~8-12 bits) + extra bits (~8-11).
   *
   * True savings:
   * - copy_len bytes not emitted as literals. But on repetitive data,
   *   literals compress well (~5-6 bits each), so savings are modest.
   *
   * Conservative approach: require substantial net savings to justify
   * the overhead. The tree expansion cost scales with the number of
   * distinct distance symbols used by dict refs. */
  {
    uint32_t dict_count = 0;
    int32_t dict_net_bits = 0; /* positive = savings, negative = cost */
    for (uint32_t t = 0; t < token_count; ++t) {
      if (tokens[t].is_dict_ref)
        dict_count++;
    }
    if (dict_count > 0) {
      /* Each dict ref adds a distinct high-numbered distance symbol.
       * The distance prefix code must describe all these symbols.
       * Cost per distinct symbol in prefix code: ~10-15 bits.
       * Plus the per-ref encoding cost. */
      for (uint32_t t = 0; t < token_count; ++t) {
        if (!tokens[t].is_dict_ref)
          continue;
        /* Savings: copy_len bytes not emitted as literals.
         * Use 6 bits/literal (conservative — repetitive data compresses
         * literals well). */
        int32_t saved = (int32_t)tokens[t].copy_len * 6;
        /* Cost: distance symbol + extra bits.
         * Distance symbol Huffman code: ~10 bits (rare symbols get long codes).
         * Distance extra bits: log2(distance). */
        uint32_t d = tokens[t].distance;
        int32_t dist_bits = 0;
        while (d > 1) {
          d >>= 1;
          dist_bits++;
        }
        /* Total per-ref cost: Huffman code (~10) + extra bits + IAC overhead
         * (~5 bits for the extra token vs merging into adjacent literal run) */
        int32_t cost = 10 + dist_bits + 5;
        dict_net_bits += saved - cost;
      }
      /* Tree overhead: distance prefix code must describe symbols up to
       * the highest dict distance symbol. This costs ~10 bits per distinct
       * symbol in the expanded range, plus ~40 bits base overhead. */
      dict_net_bits -= (int32_t)(40 + dict_count * 10);
    }

    if (dict_count > 0 && dict_net_bits <= 0) {
      /* Dictionary references cost more than they save — disable them.
       * Convert each dict ref to literals by merging its copy_len bytes
       * into the next token's insert_len. */
      uint32_t tw = 0;
      for (uint32_t t = 0; t < token_count; ++t) {
        if (tokens[t].is_dict_ref) {
          /* This token's insert_len literals + copy_len dict bytes
           * all become literals for the next token. */
          uint32_t extra_lits = tokens[t].insert_len + tokens[t].copy_len;
          if (t + 1 < token_count) {
            tokens[t + 1].insert_len += extra_lits;
          } else {
            /* Last token: emit as trailing literals */
            tokens[tw].insert_len = extra_lits;
            tokens[tw].copy_len = 4;
            tokens[tw].distance = 0;
            tokens[tw].use_dist_zero = 0;
            tokens[tw].is_dict_ref = 0;
            tw++;
          }
          continue;
        }
        if (tw != t)
          tokens[tw] = tokens[t];
        tw++;
      }
      token_count = tw;
    }
  }

  /* === Verify token consistency === */

  /* === Block splitting for q6 === */
  fio___brotli_blksplit_s lit_splits[512]; /* max block split commands */
  uint32_t lit_split_count = 1;
  uint32_t nbltypesl = 1;

  if (quality >= 6) {
    lit_split_count =
        fio___brotli_split_literals(src, tokens, token_count, lit_splits, 512);
    /* Count distinct block types */
    nbltypesl = 0;
    for (uint32_t s = 0; s < lit_split_count; ++s) {
      if (lit_splits[s].type >= nbltypesl)
        nbltypesl = lit_splits[s].type + 1;
    }
  }
  if (nbltypesl < 1)
    nbltypesl = 1;

  /* === Pass 2: Build frequency histograms === */

  /* Context modeling for q5 (not q6 — q6 uses block splitting instead).
   * Only enable when there are enough literals to amortize the overhead
   * of encoding the context map + extra Huffman tree (~40-60 bytes). */
  uint32_t cmode = FIO___BROTLI_CONTEXT_LSB6;
  uint32_t ntreesl = 1;
  const uint8_t *cmap = NULL;
  int use_context_modeling = 0;

  if (quality == 5 && src_len >= 32) {
    uint32_t total_lits = 0;
    for (uint32_t t = 0; t < token_count; ++t)
      total_lits += tokens[t].insert_len;
    /* Context modeling adds ~40-60 bytes overhead (context map + extra Huffman
     * tree). Need enough literals to amortize: at 0.5 bits/literal savings,
     * need ~640-960 literals just to break even. Use 8192 as threshold to
     * ensure clear benefit. On small data (<8KB), the overhead dominates. */
    if (total_lits >= 8192) {
      cmode = fio___brotli_detect_context_mode(src, src_len);
      ntreesl = 2;
      cmap = (cmode == FIO___BROTLI_CONTEXT_UTF8) ? fio___brotli_cmap_utf8_2
                                                  : fio___brotli_cmap_lsb6_2;
      use_context_modeling = 1;
    }
  }

  /* For q6 with block splitting: NTREESL = NBLTYPESL (one tree per block type,
   * no context modeling). Each block type gets its own literal Huffman tree. */
  if (nbltypesl > 1) {
    ntreesl = nbltypesl;
    cmap = NULL;
    use_context_modeling = 0;
    cmode = FIO___BROTLI_CONTEXT_LSB6;
  }

/* Literal frequency histograms: up to 32 trees (max block types) */
#define FIO___BROTLI_MAX_LIT_TREES 32
  uint32_t lit_freqs[FIO___BROTLI_MAX_LIT_TREES][256];
  uint32_t iac_freqs[FIO___BROTLI_MAX_IACLEN];
  uint32_t dist_freqs[64]; /* NPOSTFIX=0, NDIRECT=0 -> 64 symbols */
  FIO_MEMSET(lit_freqs, 0, sizeof(lit_freqs));
  FIO_MEMSET(iac_freqs, 0, sizeof(iac_freqs));
  FIO_MEMSET(dist_freqs, 0, sizeof(dist_freqs));

  /* Reset distance ring buffer for encoding pass (must match decompressor) */
  dist_rb[0] = 16;
  dist_rb[1] = 15;
  dist_rb[2] = 11;
  dist_rb[3] = 4;
  dist_rb_idx = 0;

  uint32_t lit_pos = 0; /* tracks position in source for literal counting */

  /* For block splitting: track which split we're in and remaining count */
  uint32_t bs_idx = 0;       /* current split index */
  uint32_t bs_remaining = 0; /* literals remaining in current block */
  uint32_t bs_cur_type = 0;  /* current block type */
  if (nbltypesl > 1) {
    bs_remaining = lit_splits[0].length;
    bs_cur_type = lit_splits[0].type;
  }

  for (uint32_t t = 0; t < token_count; ++t) {
    /* Count literal frequencies per tree */
    for (uint32_t i = 0; i < tokens[t].insert_len; ++i) {
      uint32_t p = lit_pos + i;
      if (nbltypesl > 1) {
        /* Block splitting: tree index = current block type */
        /* Advance block split if needed */
        while (bs_remaining == 0 && bs_idx + 1 < lit_split_count) {
          bs_idx++;
          bs_remaining = lit_splits[bs_idx].length;
          bs_cur_type = lit_splits[bs_idx].type;
        }
        lit_freqs[bs_cur_type][src[p]]++;
        if (bs_remaining > 0)
          bs_remaining--;
      } else if (use_context_modeling) {
        uint8_t p1 = p > 0 ? src[p - 1] : 0;
        uint8_t p2 = p > 1 ? src[p - 2] : 0;
        uint32_t ctx =
            (uint32_t)fio___brotli_context_lut[cmode * 512 + p1] |
            (uint32_t)fio___brotli_context_lut[cmode * 512 + 256 + p2];
        uint32_t tree_idx = cmap[ctx];
        lit_freqs[tree_idx][src[p]]++;
      } else {
        lit_freqs[0][src[p]]++;
      }
    }
    lit_pos += tokens[t].insert_len;

    /* Encode IAC command to get the symbol */
    uint32_t ieb, iev, ceb, cev;
    uint32_t copy_len = tokens[t].copy_len;
    int use_dz = tokens[t].use_dist_zero;

    /* For trailing literals with no actual copy, use dist_zero path */
    if (tokens[t].distance == 0 && t == token_count - 1) {
      use_dz = 1;
    }

    uint32_t iac_sym = fio___brotli_encode_iac(tokens[t].insert_len,
                                               copy_len,
                                               &use_dz,
                                               &ieb,
                                               &iev,
                                               &ceb,
                                               &cev);
    iac_freqs[iac_sym]++;

    /* Count distance frequencies */
    if (!use_dz && tokens[t].distance > 0) {
      uint32_t deb, dev;
      uint32_t dist_sym = fio___brotli_encode_distance(tokens[t].distance,
                                                       dist_rb,
                                                       dist_rb_idx,
                                                       &deb,
                                                       &dev);
      dist_freqs[dist_sym]++;

      /* Update ring buffer — RFC 7932 Section 4: push for dist_sym != 0.
       * Dictionary references are NOT pushed (RFC 7932 Section 4). */
      if (dist_sym != 0 && !tokens[t].is_dict_ref) {
        dist_rb[dist_rb_idx & 3] = (int32_t)tokens[t].distance;
        dist_rb_idx++;
      }
    }

    if (tokens[t].distance > 0)
      lit_pos += copy_len;
  }

  /* Ensure at least one symbol in each literal tree */
  for (uint32_t tr = 0; tr < ntreesl; ++tr) {
    int has_any = 0;
    for (uint32_t i = 0; i < 256; ++i) {
      if (lit_freqs[tr][i]) {
        has_any = 1;
        break;
      }
    }
    if (!has_any)
      lit_freqs[tr][0] = 1;
  }

  /* Ensure at least one distance symbol */
  {
    int has_dist = 0;
    for (uint32_t i = 0; i < 64; ++i) {
      if (dist_freqs[i]) {
        has_dist = 1;
        break;
      }
    }
    if (!has_dist)
      dist_freqs[0] = 1;
  }

  /* === Build Huffman codes === */
  uint8_t lit_lens[FIO___BROTLI_MAX_LIT_TREES][256];
  uint16_t lit_codes[FIO___BROTLI_MAX_LIT_TREES][256];
  for (uint32_t tr = 0; tr < ntreesl; ++tr) {
    fio___brotli_build_code_lengths(lit_lens[tr], lit_freqs[tr], 256, 15);
    fio___brotli_build_codes(lit_codes[tr], lit_lens[tr], 256);
  }

  uint8_t iac_lens[FIO___BROTLI_MAX_IACLEN];
  uint16_t iac_codes[FIO___BROTLI_MAX_IACLEN];
  fio___brotli_build_code_lengths(iac_lens,
                                  iac_freqs,
                                  FIO___BROTLI_MAX_IACLEN,
                                  15);
  fio___brotli_build_codes(iac_codes, iac_lens, FIO___BROTLI_MAX_IACLEN);

  uint8_t dist_lens[64];
  uint16_t dist_codes[64];
  fio___brotli_build_code_lengths(dist_lens, dist_freqs, 64, 15);
  fio___brotli_build_codes(dist_codes, dist_lens, 64);

  /* === Build block type/count Huffman codes (for block splitting) === */
  uint32_t bt_alpha = nbltypesl + 2; /* block type alphabet size */
  uint32_t bt_freqs[258];            /* max 256 + 2 */
  uint8_t bt_lens[258];
  uint16_t bt_codes[258];
  uint32_t bc_freqs[26]; /* block count alphabet */
  uint8_t bc_lens[26];
  uint16_t bc_codes[26];

  if (nbltypesl > 1) {
    FIO_MEMSET(bt_freqs, 0, bt_alpha * sizeof(uint32_t));
    FIO_MEMSET(bc_freqs, 0, sizeof(bc_freqs));

    /* Count block type code frequencies from splits (skip first block) */
    uint32_t bt_cur = lit_splits[0].type;
    uint32_t bt_prev = 1; /* per RFC 7932 spec */
    /* First block count */
    uint32_t bc_sym, bc_eb, bc_ev;
    fio___brotli_encode_block_count(lit_splits[0].length,
                                    &bc_sym,
                                    &bc_eb,
                                    &bc_ev);
    bc_freqs[bc_sym]++;

    for (uint32_t s = 1; s < lit_split_count; ++s) {
      uint32_t type_code = fio___brotli_encode_block_type(lit_splits[s].type,
                                                          bt_cur,
                                                          bt_prev,
                                                          nbltypesl);
      bt_freqs[type_code]++;
      bt_prev = bt_cur;
      bt_cur = lit_splits[s].type;

      fio___brotli_encode_block_count(lit_splits[s].length,
                                      &bc_sym,
                                      &bc_eb,
                                      &bc_ev);
      bc_freqs[bc_sym]++;
    }

    /* Ensure at least one symbol in each */
    {
      int has = 0;
      for (uint32_t i = 0; i < bt_alpha; ++i)
        has += (bt_freqs[i] > 0);
      if (!has)
        bt_freqs[0] = 1;
    }
    {
      int has = 0;
      for (uint32_t i = 0; i < 26; ++i)
        has += (bc_freqs[i] > 0);
      if (!has)
        bc_freqs[0] = 1;
    }

    fio___brotli_build_code_lengths(bt_lens, bt_freqs, bt_alpha, 15);
    fio___brotli_build_codes(bt_codes, bt_lens, bt_alpha);
    fio___brotli_build_code_lengths(bc_lens, bc_freqs, 26, 15);
    fio___brotli_build_codes(bc_codes, bc_lens, 26);
  }

  /* === Pass 3: Write bitstream === */
  fio___brotli_bw_s w;
  fio___brotli_bw_init(&w, out, out_len);

  /* Stream header: WBITS */
  fio___brotli_write_wbits(&w, wbits);

  /* Meta-block header (RFC 7932 Section 9.2) */
  fio___brotli_bw_put(&w, 1, 1);        /* ISLAST = 1 */
  fio___brotli_bw_put(&w, 0, 1);        /* ISLASTEMPTY = 0 (not empty) */
  fio___brotli_write_mlen(&w, src_len); /* MLEN */
  /* ISUNCOMPRESSED bit is only present when ISLAST=0, so omitted here. */

  /* Block type counts: NBLTYPESL, NBLTYPESI=1, NBLTYPESD=1 */
  if (nbltypesl > 1) {
    /* NBLTYPESL encoded as VarLenUint8(nbltypesl - 1) with leading 1-bit */
    fio___brotli_write_varlen_uint8(&w, nbltypesl - 1);

    /* Write block type prefix code (alphabet = nbltypesl + 2) */
    fio___brotli_write_prefix_code(&w, bt_lens, bt_codes, bt_alpha, bt_alpha);

    /* Write block count prefix code (alphabet = 26) */
    fio___brotli_write_prefix_code(&w, bc_lens, bc_codes, 26, 26);

    /* Fix single-symbol code lengths for block type/count codes */
    {
      uint32_t n = 0;
      for (uint32_t i = 0; i < bt_alpha; ++i)
        n += (bt_lens[i] != 0);
      if (n == 1)
        for (uint32_t i = 0; i < bt_alpha; ++i)
          bt_lens[i] = 0;
    }
    {
      uint32_t n = 0;
      for (uint32_t i = 0; i < 26; ++i)
        n += (bc_lens[i] != 0);
      if (n == 1)
        for (uint32_t i = 0; i < 26; ++i)
          bc_lens[i] = 0;
    }

    /* Write first block count */
    {
      uint32_t bc_sym, bc_eb, bc_ev;
      fio___brotli_encode_block_count(lit_splits[0].length,
                                      &bc_sym,
                                      &bc_eb,
                                      &bc_ev);
      fio___brotli_bw_put_huff(&w, bc_codes[bc_sym], bc_lens[bc_sym]);
      if (bc_eb)
        fio___brotli_bw_put(&w, bc_ev, bc_eb);
    }
  } else {
    fio___brotli_bw_put(&w, 0, 1); /* NBLTYPESL = 1 */
  }
  fio___brotli_bw_put(&w, 0, 1); /* NBLTYPESI = 1 */
  fio___brotli_bw_put(&w, 0, 1); /* NBLTYPESD = 1 */

  /* NPOSTFIX and NDIRECT */
  fio___brotli_bw_put(&w, 0, 2); /* NPOSTFIX = 0 */
  fio___brotli_bw_put(&w, 0, 4); /* NDIRECT = 0 (NDIRECT >> NPOSTFIX = 0) */

  /* Context modes for each literal block type */
  for (uint32_t bt = 0; bt < nbltypesl; ++bt)
    fio___brotli_bw_put(&w, cmode, 2); /* CMODE[bt] */

  /* NTREESL — VarLenUint8 encoding of (ntreesl - 1) */
  fio___brotli_write_varlen_uint8(&w, ntreesl - 1);

  /* Context map for literals */
  if (ntreesl >= 2) {
    if (use_context_modeling) {
      /* q5: 2 trees with context map */
      fio___brotli_write_context_map(&w, cmap, 64 * nbltypesl, ntreesl);
    } else if (nbltypesl > 1) {
      /* q6 block splitting: identity context map.
       * map_size = nbltypesl * 64. Entry [bt*64 + ctx] = bt.
       * This maps all 64 contexts of block type bt to tree bt. */
      uint8_t id_cmap[32 * 64]; /* max 32 block types * 64 contexts */
      for (uint32_t bt = 0; bt < nbltypesl; ++bt)
        FIO_MEMSET(id_cmap + bt * 64, (int)bt, 64);
      fio___brotli_write_context_map(&w, id_cmap, nbltypesl * 64, ntreesl);
    }
  }

  /* NTREESD = 1 (trivial context map for distances) */
  fio___brotli_bw_put(&w, 0, 1); /* VarLenUint8 = 0 -> NTREESD = 1 */

  /* Write prefix code tables */
  /* Literal prefix codes (ntreesl trees, 256 symbols each) */
  for (uint32_t tr = 0; tr < ntreesl; ++tr)
    fio___brotli_write_prefix_code(&w, lit_lens[tr], lit_codes[tr], 256, 256);

  /* IAC prefix code (704 symbols) */
  fio___brotli_write_prefix_code(&w,
                                 iac_lens,
                                 iac_codes,
                                 FIO___BROTLI_MAX_IACLEN,
                                 FIO___BROTLI_MAX_IACLEN);

  /* Distance prefix code (64 symbols) */
  fio___brotli_write_prefix_code(&w, dist_lens, dist_codes, 64, 64);

  /* Fix single-symbol code lengths: the decompressor's NSYM=1 simple prefix
   * code uses code length 0 (0 bits per symbol), but build_code_lengths
   * assigns length 1. Adjust to match what the decompressor reconstructs. */
  {
    uint32_t n;
    for (uint32_t tr = 0; tr < ntreesl; ++tr) {
      n = 0;
      for (uint32_t i = 0; i < 256; ++i)
        n += (lit_lens[tr][i] != 0);
      if (n == 1)
        for (uint32_t i = 0; i < 256; ++i)
          lit_lens[tr][i] = 0;
    }
    n = 0;
    for (uint32_t i = 0; i < FIO___BROTLI_MAX_IACLEN; ++i)
      n += (iac_lens[i] != 0);
    if (n == 1)
      for (uint32_t i = 0; i < FIO___BROTLI_MAX_IACLEN; ++i)
        iac_lens[i] = 0;
    n = 0;
    for (uint32_t i = 0; i < 64; ++i)
      n += (dist_lens[i] != 0);
    if (n == 1)
      for (uint32_t i = 0; i < 64; ++i)
        dist_lens[i] = 0;
  }

  /* === Write compressed data === */
  /* Reset distance ring buffer for final encoding pass (must match
   * decompressor) */
  dist_rb[0] = 16;
  dist_rb[1] = 15;
  dist_rb[2] = 11;
  dist_rb[3] = 4;
  dist_rb_idx = 0;
  lit_pos = 0;

  /* Block splitting state for literal emission */
  bs_idx = 0;
  bs_remaining = (nbltypesl > 1) ? lit_splits[0].length : 0;
  bs_cur_type = (nbltypesl > 1) ? lit_splits[0].type : 0;
  uint32_t bs_prev_type = 1; /* per RFC 7932 spec */

  for (uint32_t t = 0; t < token_count; ++t) {
    uint32_t ieb, iev, ceb, cev;
    uint32_t copy_len = tokens[t].copy_len;
    int use_dz = tokens[t].use_dist_zero;

    if (tokens[t].distance == 0 && t == token_count - 1)
      use_dz = 1;

    uint32_t iac_sym = fio___brotli_encode_iac(tokens[t].insert_len,
                                               copy_len,
                                               &use_dz,
                                               &ieb,
                                               &iev,
                                               &ceb,
                                               &cev);

    /* Write IAC command */
    fio___brotli_bw_put_huff(&w, iac_codes[iac_sym], iac_lens[iac_sym]);

    /* Write insert extra bits */
    if (ieb)
      fio___brotli_bw_put(&w, iev, ieb);

    /* Write copy extra bits */
    if (ceb)
      fio___brotli_bw_put(&w, cev, ceb);

    /* Write literal bytes */
    for (uint32_t i = 0; i < tokens[t].insert_len; ++i) {
      uint32_t p = lit_pos + i;
      uint8_t lit = src[p];

      if (nbltypesl > 1) {
        /* Check for block switch */
        while (bs_remaining == 0 && bs_idx + 1 < lit_split_count) {
          bs_idx++;
          uint32_t new_type = lit_splits[bs_idx].type;
          /* Write block switch command */
          uint32_t type_code = fio___brotli_encode_block_type(new_type,
                                                              bs_cur_type,
                                                              bs_prev_type,
                                                              nbltypesl);
          fio___brotli_bw_put_huff(&w, bt_codes[type_code], bt_lens[type_code]);
          /* Write new block count */
          uint32_t bc_sym, bc_eb, bc_ev;
          fio___brotli_encode_block_count(lit_splits[bs_idx].length,
                                          &bc_sym,
                                          &bc_eb,
                                          &bc_ev);
          fio___brotli_bw_put_huff(&w, bc_codes[bc_sym], bc_lens[bc_sym]);
          if (bc_eb)
            fio___brotli_bw_put(&w, bc_ev, bc_eb);

          bs_prev_type = bs_cur_type;
          bs_cur_type = new_type;
          bs_remaining = lit_splits[bs_idx].length;
        }
        /* Use block type's tree */
        uint32_t tree_idx = bs_cur_type;
        fio___brotli_bw_put_huff(&w,
                                 lit_codes[tree_idx][lit],
                                 lit_lens[tree_idx][lit]);
        if (bs_remaining > 0)
          bs_remaining--;
      } else if (use_context_modeling) {
        uint8_t p1 = p > 0 ? src[p - 1] : 0;
        uint8_t p2 = p > 1 ? src[p - 2] : 0;
        uint32_t ctx =
            (uint32_t)fio___brotli_context_lut[cmode * 512 + p1] |
            (uint32_t)fio___brotli_context_lut[cmode * 512 + 256 + p2];
        uint32_t tree_idx = cmap[ctx];
        fio___brotli_bw_put_huff(&w,
                                 lit_codes[tree_idx][lit],
                                 lit_lens[tree_idx][lit]);
      } else {
        fio___brotli_bw_put_huff(&w, lit_codes[0][lit], lit_lens[0][lit]);
      }
    }
    lit_pos += tokens[t].insert_len;

    /* Write distance */
    if (!use_dz && tokens[t].distance > 0) {
      uint32_t deb, dev;
      uint32_t dist_sym = fio___brotli_encode_distance(tokens[t].distance,
                                                       dist_rb,
                                                       dist_rb_idx,
                                                       &deb,
                                                       &dev);

      fio___brotli_bw_put_huff(&w, dist_codes[dist_sym], dist_lens[dist_sym]);
      if (deb)
        fio___brotli_bw_put(&w, dev, deb);

      /* Update ring buffer — RFC 7932 Section 4: push for dist_sym != 0.
       * Dictionary references are NOT pushed (RFC 7932 Section 4). */
      if (dist_sym != 0 && !tokens[t].is_dict_ref) {
        dist_rb[dist_rb_idx & 3] = (int32_t)tokens[t].distance;
        dist_rb_idx++;
      }
    }

    if (tokens[t].distance > 0)
      lit_pos += copy_len;
  }
#undef FIO___BROTLI_MAX_LIT_TREES

  size_t result = fio___brotli_bw_finish(&w, out);

  FIO_MEM_FREE(tokens, token_alloc);
  if (hash_chain)
    FIO_MEM_FREE(hash_chain, hc_alloc);
  if (hash_table)
    FIO_MEM_FREE(hash_table, hash_alloc);

  return result;
}

#undef FIO___BROTLI_HASH_BITS
#undef FIO___BROTLI_HASH_SIZE
#undef FIO___BROTLI_HC_BITS
#undef FIO___BROTLI_HC_BUCKETS
#undef FIO___BROTLI_HC_DEPTH
#undef FIO___BROTLI_DICT_HASH_BITS
#undef FIO___BROTLI_DICT_HASH_SIZE

/* *****************************************************************************
Cleanup
*****************************************************************************
*/

#undef FIO___BROTLI_HUFF_BITS
#undef FIO___BROTLI_HUFF_SIZE
#undef FIO___BROTLI_HUFF_MASK
#undef FIO___BROTLI_MAX_CODE_LEN
#undef FIO___BROTLI_MAX_CL_LEN
#undef FIO___BROTLI_MAX_LITERAL
#undef FIO___BROTLI_MAX_IACLEN
#undef FIO___BROTLI_MAX_DIST
#undef FIO___BROTLI_MAX_BLOCK_TYPE
#undef FIO___BROTLI_MAX_WBITS
#undef FIO___BROTLI_MAX_WINDOW
#undef FIO___BROTLI_CONTEXT_LSB6
#undef FIO___BROTLI_CONTEXT_MSB6
#undef FIO___BROTLI_CONTEXT_UTF8
#undef FIO___BROTLI_CONTEXT_SIGNED
#undef FIO___BROTLI_LITERAL_CONTEXTS
#undef FIO___BROTLI_DISTANCE_CONTEXTS
#undef FIO___BROTLI_TRANSFORM_IDENTITY
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_1
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_2
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_3
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_4
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_5
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_6
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_7
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_8
#undef FIO___BROTLI_TRANSFORM_OMIT_LAST_9
#undef FIO___BROTLI_TRANSFORM_UPPERCASE_FIRST
#undef FIO___BROTLI_TRANSFORM_UPPERCASE_ALL
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_1
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_2
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_3
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_4
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_5
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_6
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_7
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_8
#undef FIO___BROTLI_TRANSFORM_OMIT_FIRST_9
#undef FIO___BROTLI_NUM_TRANSFORMS
#undef FIO___BROTLI_ENTRY_SYM
#undef FIO___BROTLI_ENTRY_SUB
#undef FIO___BROTLI_ENTRY_CODELEN
#undef FIO___BROTLI_ENTRY_IS_SYM
#undef FIO___BROTLI_ENTRY_SYMBOL
#undef FIO___BROTLI_ENTRY_IS_SUB
#undef FIO___BROTLI_ENTRY_SUB_OFF
#undef FIO___BROTLI_ENTRY_SUB_BITS

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_BROTLI
#endif /* FIO_BROTLI */
