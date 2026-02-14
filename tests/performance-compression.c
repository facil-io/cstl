/* *****************************************************************************
Compression Performance Benchmarks
Compares facil.io DEFLATE and Brotli against system zlib (when available).
Measures throughput (MB/s), ops/sec, and compression ratio.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_RAND
#define FIO_DEFLATE
#define FIO_BROTLI
#include FIO_INCLUDE_FILE

#ifdef HAVE_OPENSSL
#include <openssl/opensslv.h>
#endif

/* System zlib detection */
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

/* *****************************************************************************
Benchmarking Macros
***************************************************************************** */

/* Benchmark compression: reports MB/s, ops/s, and ratio */
#define FIO_BENCH_COMPRESS_RATIO(name_str,                                     \
                                 target_time_ms,                               \
                                 data_size,                                    \
                                 compressed_size,                              \
                                 code_block)                                   \
  do {                                                                         \
    clock_t bench_start = clock();                                             \
    uint64_t bench_iterations = 0;                                             \
    for (;                                                                     \
         (clock() - bench_start) < ((target_time_ms)*CLOCKS_PER_SEC / 1000) || \
         bench_iterations < 50;                                                \
         ++bench_iterations) {                                                 \
      code_block;                                                              \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    clock_t bench_end = clock();                                               \
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;       \
    double ops_sec = bench_iterations / (elapsed > 0.0 ? elapsed : 0.0001);    \
    double mb_sec = (ops_sec * (double)(data_size)) / (1024.0 * 1024.0);       \
    double ratio = (data_size) > 0 ? 100.0 * (double)(compressed_size) /       \
                                         (double)(data_size)                   \
                                   : 0.0;                                      \
    fprintf(stderr,                                                            \
            "      %-44s %8.1f MB/s  %10.0f ops/s  ratio: %5.1f%%\n",          \
            name_str,                                                          \
            mb_sec,                                                            \
            ops_sec,                                                           \
            ratio);                                                            \
  } while (0)

/* Benchmark decompression: reports MB/s and ops/s (no ratio) */
#define FIO_BENCH_DECOMPRESS(name_str, target_time_ms, orig_size, code_block)  \
  do {                                                                         \
    clock_t bench_start = clock();                                             \
    uint64_t bench_iterations = 0;                                             \
    for (;                                                                     \
         (clock() - bench_start) < ((target_time_ms)*CLOCKS_PER_SEC / 1000) || \
         bench_iterations < 50;                                                \
         ++bench_iterations) {                                                 \
      code_block;                                                              \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    clock_t bench_end = clock();                                               \
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;       \
    double ops_sec = bench_iterations / (elapsed > 0.0 ? elapsed : 0.0001);    \
    double mb_sec = (ops_sec * (double)(orig_size)) / (1024.0 * 1024.0);       \
    fprintf(stderr,                                                            \
            "      %-44s %8.1f MB/s  %10.0f ops/s\n",                          \
            name_str,                                                          \
            mb_sec,                                                            \
            ops_sec);                                                          \
  } while (0)

/* *****************************************************************************
Test Data Generation
***************************************************************************** */

/* HTML-like repetitive text data */
FIO_SFUNC void fio___bench_fill_html(uint8_t *buf, size_t len) {
  static const char html_pattern[] =
      "<div class=\"item\"><h2>Title</h2><p>Lorem ipsum dolor sit amet, "
      "consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut "
      "labore et dolore magna aliqua.</p><span class=\"meta\">Author: "
      "John</span></div>\n";
  size_t plen = sizeof(html_pattern) - 1;
  size_t pos = 0;
  while (pos + plen <= len) {
    FIO_MEMCPY(buf + pos, html_pattern, plen);
    pos += plen;
  }
  if (pos < len)
    FIO_MEMCPY(buf + pos, html_pattern, len - pos);
}

/* JSON-like repetitive data */
FIO_SFUNC void fio___bench_fill_json(uint8_t *buf, size_t len) {
  static const char json_pattern[] =
      "{\"id\":12345,\"name\":\"Alice Johnson\",\"email\":"
      "\"alice@example.com\",\"active\":true,\"score\":98.7,"
      "\"tags\":[\"admin\",\"user\",\"verified\"],"
      "\"address\":{\"city\":\"Springfield\",\"zip\":\"62704\"}},\n";
  size_t plen = sizeof(json_pattern) - 1;
  size_t pos = 0;
  while (pos + plen <= len) {
    FIO_MEMCPY(buf + pos, json_pattern, plen);
    pos += plen;
  }
  if (pos < len)
    FIO_MEMCPY(buf + pos, json_pattern, len - pos);
}

/* Highly repetitive data (best case for compression) */
FIO_SFUNC void fio___bench_fill_repetitive(uint8_t *buf, size_t len) {
  static const char rep_pattern[] = "AAAAAAAAAAAAAAAA";
  size_t plen = sizeof(rep_pattern) - 1;
  size_t pos = 0;
  while (pos + plen <= len) {
    FIO_MEMCPY(buf + pos, rep_pattern, plen);
    pos += plen;
  }
  if (pos < len)
    FIO_MEMSET(buf + pos, 'A', len - pos);
}

/* Data type descriptor */
typedef struct {
  const char *name;
  void (*fill)(uint8_t *, size_t);
  int is_random; /* use fio_rand_bytes instead of fill */
} fio___bench_data_type_s;

static const fio___bench_data_type_s fio___bench_data_types[] = {
    {"HTML-like text", fio___bench_fill_html, 0},
    {"JSON data", fio___bench_fill_json, 0},
    {"Random (incompressible)", NULL, 1},
    {"Highly repetitive", fio___bench_fill_repetitive, 0},
};
#define FIO___BENCH_DATA_TYPE_COUNT                                            \
  (sizeof(fio___bench_data_types) / sizeof(fio___bench_data_types[0]))

/* Benchmark sizes */
static const size_t fio___bench_sizes[] = {4096, 65536};
#define FIO___BENCH_SIZE_COUNT                                                 \
  (sizeof(fio___bench_sizes) / sizeof(fio___bench_sizes[0]))

/* *****************************************************************************
zlib Wrappers (raw DEFLATE via deflateInit2/inflateInit2 with windowBits=-15)
***************************************************************************** */

#ifdef HAVE_ZLIB

/* Raw DEFLATE compress via zlib (windowBits=-15 for no header) */
FIO_SFUNC size_t fio___zlib_deflate_compress(void *out,
                                             size_t out_len,
                                             const void *in,
                                             size_t in_len,
                                             int level) {
  z_stream strm;
  FIO_MEMSET(&strm, 0, sizeof(strm));
  strm.next_in = (Bytef *)(uintptr_t)in;
  strm.avail_in = (uInt)in_len;
  strm.next_out = (Bytef *)out;
  strm.avail_out = (uInt)out_len;
  if (deflateInit2(&strm, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) !=
      Z_OK)
    return 0;
  int ret = deflate(&strm, Z_FINISH);
  size_t result = (ret == Z_STREAM_END) ? strm.total_out : 0;
  deflateEnd(&strm);
  return result;
}

/* Raw DEFLATE decompress via zlib (windowBits=-15 for no header) */
FIO_SFUNC size_t fio___zlib_deflate_decompress(void *out,
                                               size_t out_len,
                                               const void *in,
                                               size_t in_len) {
  z_stream strm;
  FIO_MEMSET(&strm, 0, sizeof(strm));
  strm.next_in = (Bytef *)(uintptr_t)in;
  strm.avail_in = (uInt)in_len;
  strm.next_out = (Bytef *)out;
  strm.avail_out = (uInt)out_len;
  if (inflateInit2(&strm, -15) != Z_OK)
    return 0;
  int ret = inflate(&strm, Z_FINISH);
  size_t result = (ret == Z_STREAM_END) ? strm.total_out : 0;
  inflateEnd(&strm);
  return result;
}

/* Gzip compress via zlib (windowBits=15+16 for gzip header) */
FIO_SFUNC size_t fio___zlib_gzip_compress(void *out,
                                          size_t out_len,
                                          const void *in,
                                          size_t in_len,
                                          int level) {
  z_stream strm;
  FIO_MEMSET(&strm, 0, sizeof(strm));
  strm.next_in = (Bytef *)(uintptr_t)in;
  strm.avail_in = (uInt)in_len;
  strm.next_out = (Bytef *)out;
  strm.avail_out = (uInt)out_len;
  if (deflateInit2(&strm, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) !=
      Z_OK)
    return 0;
  int ret = deflate(&strm, Z_FINISH);
  size_t result = (ret == Z_STREAM_END) ? strm.total_out : 0;
  deflateEnd(&strm);
  return result;
}

/* Gzip decompress via zlib (windowBits=15+16 for gzip header) */
FIO_SFUNC size_t fio___zlib_gzip_decompress(void *out,
                                            size_t out_len,
                                            const void *in,
                                            size_t in_len) {
  z_stream strm;
  FIO_MEMSET(&strm, 0, sizeof(strm));
  strm.next_in = (Bytef *)(uintptr_t)in;
  strm.avail_in = (uInt)in_len;
  strm.next_out = (Bytef *)out;
  strm.avail_out = (uInt)out_len;
  if (inflateInit2(&strm, 15 + 16) != Z_OK)
    return 0;
  int ret = inflate(&strm, Z_FINISH);
  size_t result = (ret == Z_STREAM_END) ? strm.total_out : 0;
  inflateEnd(&strm);
  return result;
}

#endif /* HAVE_ZLIB */

/* *****************************************************************************
Section 1: DEFLATE Compression Benchmarks
***************************************************************************** */

FIO_SFUNC void fio_bench_deflate_compress(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  DEFLATE Compression\n"
          "---------------------------------------------------\n");

  for (size_t si = 0; si < FIO___BENCH_SIZE_COUNT; ++si) {
    size_t data_size = fio___bench_sizes[si];
    size_t comp_bound = fio_deflate_compress_bound(data_size);
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
    uint8_t *dst = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
#ifdef HAVE_ZLIB
    uint8_t *zdst = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
#endif

    for (size_t di = 0; di < FIO___BENCH_DATA_TYPE_COUNT; ++di) {
      const fio___bench_data_type_s *dt = &fio___bench_data_types[di];

      /* Fill source data */
      if (dt->is_random)
        fio_rand_bytes(src, data_size);
      else
        dt->fill(src, data_size);

      fprintf(stderr,
              "\n  Data: %s (%zu bytes)\n"
              "  ────────────────────────────────────────────────\n"
              "  Compression:\n",
              dt->name,
              data_size);

      /* fio_deflate_compress at levels 1, 6, 9 */
      int levels[] = {1, 6, 9};
      for (size_t li = 0; li < 3; ++li) {
        int level = levels[li];
        size_t clen = 0;
        char label[64];
        snprintf(label,
                 sizeof(label),
                 "fio_deflate_compress (level %d)",
                 level);

        /* Pre-run to get compressed size */
        clen = fio_deflate_compress(dst, comp_bound, src, data_size, level);

        FIO_BENCH_COMPRESS_RATIO(
            label,
            500,
            data_size,
            clen,
            clen = fio_deflate_compress(dst, comp_bound, src, data_size, level);
            src[0] ^= ((uint8_t *)dst)[0];);
      }

#ifdef HAVE_ZLIB
      /* zlib deflate at levels 1, 6, 9 */
      for (size_t li = 0; li < 3; ++li) {
        int level = levels[li];
        size_t clen = 0;
        char label[64];
        snprintf(label, sizeof(label), "zlib deflate (level %d)", level);

        clen = fio___zlib_deflate_compress(zdst,
                                           comp_bound,
                                           src,
                                           data_size,
                                           level);

        FIO_BENCH_COMPRESS_RATIO(label,
                                 500,
                                 data_size,
                                 clen,
                                 clen = fio___zlib_deflate_compress(zdst,
                                                                    comp_bound,
                                                                    src,
                                                                    data_size,
                                                                    level);
                                 src[0] ^= ((uint8_t *)zdst)[0];);
      }
#endif /* HAVE_ZLIB */
    }

    FIO_MEM_FREE(src, data_size);
    FIO_MEM_FREE(dst, comp_bound);
#ifdef HAVE_ZLIB
    FIO_MEM_FREE(zdst, comp_bound);
#endif
  }
}

/* *****************************************************************************
Section 2: DEFLATE Decompression Benchmarks
***************************************************************************** */

FIO_SFUNC void fio_bench_deflate_decompress(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  DEFLATE Decompression\n"
          "---------------------------------------------------\n");

  for (size_t si = 0; si < FIO___BENCH_SIZE_COUNT; ++si) {
    size_t data_size = fio___bench_sizes[si];
    size_t comp_bound = fio_deflate_compress_bound(data_size);
    size_t decomp_bound = fio_deflate_decompress_bound(comp_bound);
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
    uint8_t *compressed = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
    uint8_t *decompressed =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_bound, 0);

    for (size_t di = 0; di < FIO___BENCH_DATA_TYPE_COUNT; ++di) {
      const fio___bench_data_type_s *dt = &fio___bench_data_types[di];

      if (dt->is_random)
        fio_rand_bytes(src, data_size);
      else
        dt->fill(src, data_size);

      /* Pre-compress with level 6 */
      size_t clen =
          fio_deflate_compress(compressed, comp_bound, src, data_size, 6);
      if (!clen) {
        fprintf(stderr, "  ERROR: compression failed for %s\n", dt->name);
        continue;
      }

      fprintf(stderr,
              "\n  Data: %s (%zu bytes, compressed: %zu bytes)\n"
              "  ────────────────────────────────────────────────\n"
              "  Decompression:\n",
              dt->name,
              data_size,
              clen);

      /* fio_deflate_decompress */
      FIO_BENCH_DECOMPRESS("fio_deflate_decompress",
                           500,
                           data_size,
                           size_t dlen = fio_deflate_decompress(decompressed,
                                                                decomp_bound,
                                                                compressed,
                                                                clen);
                           ((uint8_t *)compressed)[0] ^=
                           ((uint8_t *)decompressed)[dlen > 0 ? 0 : 0];);

#ifdef HAVE_ZLIB
      /* zlib inflate — decompress fio-compressed data (raw DEFLATE is
       * interoperable) */
      FIO_BENCH_DECOMPRESS("zlib inflate",
                           500,
                           data_size,
                           size_t dlen =
                               fio___zlib_deflate_decompress(decompressed,
                                                             decomp_bound,
                                                             compressed,
                                                             clen);
                           ((uint8_t *)compressed)[0] ^=
                           ((uint8_t *)decompressed)[dlen > 0 ? 0 : 0];);
#endif /* HAVE_ZLIB */
    }

    FIO_MEM_FREE(src, data_size);
    FIO_MEM_FREE(compressed, comp_bound);
    FIO_MEM_FREE(decompressed, decomp_bound);
  }
}

/* *****************************************************************************
Section 3: Gzip Benchmarks
***************************************************************************** */

FIO_SFUNC void fio_bench_gzip(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Gzip (DEFLATE + gzip wrapper)\n"
          "---------------------------------------------------\n");

  for (size_t si = 0; si < FIO___BENCH_SIZE_COUNT; ++si) {
    size_t data_size = fio___bench_sizes[si];
    /* gzip adds ~18 bytes header+trailer */
    size_t comp_bound = fio_deflate_compress_bound(data_size) + 32;
    size_t decomp_bound = fio_deflate_decompress_bound(comp_bound);
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
    uint8_t *dst = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
    uint8_t *dec = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_bound, 0);
#ifdef HAVE_ZLIB
    uint8_t *zdst = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
    uint8_t *zdec = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_bound, 0);
#endif

    /* Use HTML data for gzip benchmarks (most common real-world use) */
    fio___bench_fill_html(src, data_size);

    fprintf(stderr,
            "\n  Data: HTML-like text (%zu bytes)\n"
            "  ────────────────────────────────────────────────\n",
            data_size);

    /* fio_gzip_compress level 6 */
    {
      size_t clen = fio_gzip_compress(dst, comp_bound, src, data_size, 6);
      fprintf(stderr, "  Compression:\n");
      FIO_BENCH_COMPRESS_RATIO(
          "fio_gzip_compress (level 6)",
          500,
          data_size,
          clen,
          clen = fio_gzip_compress(dst, comp_bound, src, data_size, 6);
          src[0] ^= dst[0];);

      /* fio_gzip_decompress */
      fprintf(stderr, "  Decompression:\n");
      FIO_BENCH_DECOMPRESS(
          "fio_gzip_decompress",
          500,
          data_size,
          size_t dlen = fio_gzip_decompress(dec, decomp_bound, dst, clen);
          dst[0] ^= dec[dlen > 0 ? 0 : 0];);
    }

#ifdef HAVE_ZLIB
    /* zlib gzip compress level 6 */
    {
      size_t clen =
          fio___zlib_gzip_compress(zdst, comp_bound, src, data_size, 6);
      fprintf(stderr, "  Compression:\n");
      FIO_BENCH_COMPRESS_RATIO(
          "zlib gzip compress (level 6)",
          500,
          data_size,
          clen,
          clen = fio___zlib_gzip_compress(zdst, comp_bound, src, data_size, 6);
          src[0] ^= zdst[0];);

      /* zlib gzip decompress */
      fprintf(stderr, "  Decompression:\n");
      FIO_BENCH_DECOMPRESS(
          "zlib gzip decompress",
          500,
          data_size,
          size_t dlen =
              fio___zlib_gzip_decompress(zdec, decomp_bound, zdst, clen);
          zdst[0] ^= zdec[dlen > 0 ? 0 : 0];);
    }
#endif /* HAVE_ZLIB */

    FIO_MEM_FREE(src, data_size);
    FIO_MEM_FREE(dst, comp_bound);
    FIO_MEM_FREE(dec, decomp_bound);
#ifdef HAVE_ZLIB
    FIO_MEM_FREE(zdst, comp_bound);
    FIO_MEM_FREE(zdec, decomp_bound);
#endif
  }
}

/* *****************************************************************************
Section 4: Brotli Compression Benchmarks
***************************************************************************** */

FIO_SFUNC void fio_bench_brotli_compress(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Brotli Compression\n"
          "---------------------------------------------------\n");

  for (size_t si = 0; si < FIO___BENCH_SIZE_COUNT; ++si) {
    size_t data_size = fio___bench_sizes[si];
    size_t comp_bound = fio_brotli_compress_bound(data_size);
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
    uint8_t *dst = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);

    for (size_t di = 0; di < FIO___BENCH_DATA_TYPE_COUNT; ++di) {
      const fio___bench_data_type_s *dt = &fio___bench_data_types[di];

      if (dt->is_random)
        fio_rand_bytes(src, data_size);
      else
        dt->fill(src, data_size);

      fprintf(stderr,
              "\n  Data: %s (%zu bytes)\n"
              "  ────────────────────────────────────────────────\n"
              "  Compression:\n",
              dt->name,
              data_size);

      /* fio_brotli_compress at quality 1 and 4 */
      int qualities[] = {1, 4};
      for (size_t qi = 0; qi < 2; ++qi) {
        int quality = qualities[qi];
        size_t clen = 0;
        char label[64];
        snprintf(label,
                 sizeof(label),
                 "fio_brotli_compress (quality %d)",
                 quality);

        clen = fio_brotli_compress(dst, comp_bound, src, data_size, quality);

        FIO_BENCH_COMPRESS_RATIO(
            label,
            500,
            data_size,
            clen,
            clen =
                fio_brotli_compress(dst, comp_bound, src, data_size, quality);
            src[0] ^= dst[0];);
      }
    }

    FIO_MEM_FREE(src, data_size);
    FIO_MEM_FREE(dst, comp_bound);
  }
}

/* *****************************************************************************
Section 5: Brotli Decompression Benchmarks
***************************************************************************** */

FIO_SFUNC void fio_bench_brotli_decompress(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Brotli Decompression\n"
          "---------------------------------------------------\n");

  for (size_t si = 0; si < FIO___BENCH_SIZE_COUNT; ++si) {
    size_t data_size = fio___bench_sizes[si];
    size_t comp_bound = fio_brotli_compress_bound(data_size);
    size_t decomp_bound = data_size + 4096; /* generous for decompression */
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
    uint8_t *compressed = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
    uint8_t *decompressed =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_bound, 0);

    for (size_t di = 0; di < FIO___BENCH_DATA_TYPE_COUNT; ++di) {
      const fio___bench_data_type_s *dt = &fio___bench_data_types[di];

      if (dt->is_random)
        fio_rand_bytes(src, data_size);
      else
        dt->fill(src, data_size);

      /* Pre-compress with quality 4 */
      size_t clen =
          fio_brotli_compress(compressed, comp_bound, src, data_size, 4);
      if (!clen) {
        fprintf(stderr,
                "  ERROR: brotli compression failed for %s\n",
                dt->name);
        continue;
      }

      fprintf(stderr,
              "\n  Data: %s (%zu bytes, compressed: %zu bytes)\n"
              "  ────────────────────────────────────────────────\n"
              "  Decompression:\n",
              dt->name,
              data_size,
              clen);

      FIO_BENCH_DECOMPRESS("fio_brotli_decompress",
                           500,
                           data_size,
                           size_t dlen = fio_brotli_decompress(decompressed,
                                                               decomp_bound,
                                                               compressed,
                                                               clen);
                           ((uint8_t *)compressed)[0] ^=
                           ((uint8_t *)decompressed)[dlen > 0 ? 0 : 0];);
    }

    FIO_MEM_FREE(src, data_size);
    FIO_MEM_FREE(compressed, comp_bound);
    FIO_MEM_FREE(decompressed, decomp_bound);
  }
}

/* *****************************************************************************
Section 6: Throughput Summary (key algorithms, 64KB HTML)
***************************************************************************** */

FIO_SFUNC void fio_bench_compression_summary(void) {
  fprintf(stderr,
          "\n"
          "===================================================\n"
          "  Throughput Summary — 64KB HTML-like text\n"
          "===================================================\n\n");

  size_t data_size = 65536;
  size_t deflate_bound = fio_deflate_compress_bound(data_size);
  size_t brotli_bound = fio_brotli_compress_bound(data_size);
  size_t gzip_bound = deflate_bound + 32;
  size_t decomp_bound = fio_deflate_decompress_bound(deflate_bound);

  uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, data_size, 0);
  uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, brotli_bound, 0);
  uint8_t *dec = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_bound, 0);
#ifdef HAVE_ZLIB
  uint8_t *zcomp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, deflate_bound, 0);
#endif

  fio___bench_fill_html(src, data_size);

  fprintf(stderr,
          "  %-36s %10s  %10s  %7s\n",
          "Algorithm",
          "Comp MB/s",
          "Decomp MB/s",
          "Ratio");
  fprintf(stderr,
          "  ──────────────────────────────────────────────────────────"
          "──────────\n");

  /* Helper macro for summary rows */
#define FIO___SUMMARY_ROW(algo_name, comp_expr, decomp_expr)                   \
  do {                                                                         \
    size_t clen = 0;                                                           \
    /* Measure compression */                                                  \
    clock_t cs = clock();                                                      \
    uint64_t ci = 0;                                                           \
    for (; (clock() - cs) < (500 * CLOCKS_PER_SEC / 1000) || ci < 50; ++ci) {  \
      clen = comp_expr;                                                        \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    double ce = (double)(clock() - cs) / CLOCKS_PER_SEC;                       \
    double c_mbps =                                                            \
        (ci / (ce > 0 ? ce : 0.0001) * (double)data_size) / (1024.0 * 1024.0); \
    double ratio = 100.0 * (double)clen / (double)data_size;                   \
    /* Measure decompression */                                                \
    clock_t ds = clock();                                                      \
    uint64_t di2 = 0;                                                          \
    for (; (clock() - ds) < (500 * CLOCKS_PER_SEC / 1000) || di2 < 50;         \
         ++di2) {                                                              \
      decomp_expr;                                                             \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    double de = (double)(clock() - ds) / CLOCKS_PER_SEC;                       \
    double d_mbps = (di2 / (de > 0 ? de : 0.0001) * (double)data_size) /       \
                    (1024.0 * 1024.0);                                         \
    fprintf(stderr,                                                            \
            "  %-36s %8.1f    %8.1f      %5.1f%%\n",                           \
            algo_name,                                                         \
            c_mbps,                                                            \
            d_mbps,                                                            \
            ratio);                                                            \
  } while (0)

  FIO___SUMMARY_ROW(
      "fio_deflate (level 1)",
      fio_deflate_compress(comp, deflate_bound, src, data_size, 1),
      fio_deflate_decompress(dec, decomp_bound, comp, clen));

  FIO___SUMMARY_ROW(
      "fio_deflate (level 6)",
      fio_deflate_compress(comp, deflate_bound, src, data_size, 6),
      fio_deflate_decompress(dec, decomp_bound, comp, clen));

  FIO___SUMMARY_ROW(
      "fio_deflate (level 9)",
      fio_deflate_compress(comp, deflate_bound, src, data_size, 9),
      fio_deflate_decompress(dec, decomp_bound, comp, clen));

#ifdef HAVE_ZLIB
  FIO___SUMMARY_ROW(
      "zlib deflate (level 1)",
      fio___zlib_deflate_compress(zcomp, deflate_bound, src, data_size, 1),
      fio___zlib_deflate_decompress(dec, decomp_bound, zcomp, clen));

  FIO___SUMMARY_ROW(
      "zlib deflate (level 6)",
      fio___zlib_deflate_compress(zcomp, deflate_bound, src, data_size, 6),
      fio___zlib_deflate_decompress(dec, decomp_bound, zcomp, clen));

  FIO___SUMMARY_ROW(
      "zlib deflate (level 9)",
      fio___zlib_deflate_compress(zcomp, deflate_bound, src, data_size, 9),
      fio___zlib_deflate_decompress(dec, decomp_bound, zcomp, clen));
#endif

  FIO___SUMMARY_ROW("fio_gzip (level 6)",
                    fio_gzip_compress(comp, gzip_bound, src, data_size, 6),
                    fio_gzip_decompress(dec, decomp_bound, comp, clen));

#ifdef HAVE_ZLIB
  FIO___SUMMARY_ROW(
      "zlib gzip (level 6)",
      fio___zlib_gzip_compress(zcomp, gzip_bound, src, data_size, 6),
      fio___zlib_gzip_decompress(dec, decomp_bound, zcomp, clen));
#endif

  FIO___SUMMARY_ROW("fio_brotli (quality 1)",
                    fio_brotli_compress(comp, brotli_bound, src, data_size, 1),
                    fio_brotli_decompress(dec, decomp_bound, comp, clen));

  FIO___SUMMARY_ROW("fio_brotli (quality 4)",
                    fio_brotli_compress(comp, brotli_bound, src, data_size, 4),
                    fio_brotli_decompress(dec, decomp_bound, comp, clen));

#undef FIO___SUMMARY_ROW

  fprintf(stderr, "\n");

  FIO_MEM_FREE(src, data_size);
  FIO_MEM_FREE(comp, brotli_bound);
  FIO_MEM_FREE(dec, decomp_bound);
#ifdef HAVE_ZLIB
  FIO_MEM_FREE(zcomp, deflate_bound);
#endif
}

/* *****************************************************************************
Platform / Build Info
***************************************************************************** */

FIO_SFUNC void fio_bench_compression_platform_info(void) {
  fprintf(stderr,
          "\n"
          "===================================================\n"
          "  Platform / Build Info\n"
          "===================================================\n\n");
  fprintf(stderr, "  Platform: ");
#if defined(__APPLE__)
  fprintf(stderr, "macOS");
#if defined(__aarch64__) || defined(__arm64__)
  fprintf(stderr, " (Apple Silicon)");
#endif
#elif defined(__linux__)
  fprintf(stderr, "Linux");
#elif defined(_WIN32)
  fprintf(stderr, "Windows");
#else
  fprintf(stderr, "Unknown");
#endif

  fprintf(stderr, "\n  Compiler: ");
#if defined(__clang__)
  fprintf(stderr,
          "clang %d.%d.%d",
          __clang_major__,
          __clang_minor__,
          __clang_patchlevel__);
#elif defined(__GNUC__)
  fprintf(stderr,
          "gcc %d.%d.%d",
          __GNUC__,
          __GNUC_MINOR__,
          __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
  fprintf(stderr, "MSVC %d", _MSC_VER);
#else
  fprintf(stderr, "unknown");
#endif

#ifdef HAVE_ZLIB
  fprintf(stderr, "\n  zlib: %s", ZLIB_VERSION);
#else
  fprintf(stderr, "\n  zlib: (unavailable)");
#endif

#ifdef HAVE_OPENSSL
  fprintf(stderr, "\n  OpenSSL: %s", OPENSSL_VERSION_TEXT);
#endif

  fprintf(stderr, "\n\n");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
#if defined(DEBUG) && (DEBUG)
  if (1) {
    fprintf(stderr, "\t- Skipped in DEBUG\n");
    return 0;
  }
#endif

  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  Compression Performance Benchmarks\n"
          "  facil.io DEFLATE & Brotli vs zlib\n"
          "=======================================================\n\n");

#ifdef HAVE_ZLIB
  fprintf(stderr,
          "  zlib %s detected — head-to-head comparison enabled\n\n",
          ZLIB_VERSION);
#else
  fprintf(stderr, "  zlib not detected — facil.io only benchmarks\n\n");
#endif

  /* Section 1: DEFLATE Compression */
  fio_bench_deflate_compress();

  /* Section 2: DEFLATE Decompression */
  fio_bench_deflate_decompress();

  /* Section 3: Gzip */
  fio_bench_gzip();

  /* Section 4: Brotli Compression */
  fio_bench_brotli_compress();

  /* Section 5: Brotli Decompression */
  fio_bench_brotli_decompress();

  /* Section 6: Summary */
  fio_bench_compression_summary();

  /* Platform info */
  fio_bench_compression_platform_info();

  return 0;
}
