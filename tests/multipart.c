/* *****************************************************************************
MIME Multipart Parser Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_MULTIPART
#include "test-helpers.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* *****************************************************************************
Recording callbacks: every reported part is appended to a log, so the result
of any feeding pattern can be compared to a single complete parse.
***************************************************************************** */

typedef struct {
  char *log;
  size_t len;
  size_t capa;
  size_t strings;
  size_t files;
  int fail_on; /* return this from the n-th callback (`fail_at`), if set */
  size_t fail_at;
  size_t calls;
  fio_buf_info_s headers; /* the last file's headers (copied) */
  char headers_buf[512];
} mp_ctx_s;

static void mp_log(mp_ctx_s *c, const char *buf, size_t len) {
  if (c->len + len + 1 > c->capa) {
    size_t capa = (c->len + len + 1) * 2;
    c->log = (char *)realloc(c->log, capa);
    FIO_ASSERT(c->log, "allocation failed");
    c->capa = capa;
  }
  if (len)
    memcpy(c->log + c->len, buf, len);
  c->len += len;
  c->log[c->len] = 0;
}
static void mp_log_buf(mp_ctx_s *c, fio_buf_info_s b) {
  if (!b.buf) {
    mp_log(c, "(null)", 6);
    return;
  }
  mp_log(c, b.buf, b.len);
}
static int mp_fail(mp_ctx_s *c) {
  return (c->fail_on && ++c->calls == c->fail_at) ? c->fail_on : 0;
}
static int mp_on_string(void *udata,
                        fio_buf_info_s key,
                        fio_buf_info_s value,
                        fio_buf_info_s content_type) {
  mp_ctx_s *c = (mp_ctx_s *)udata;
  ++c->strings;
  mp_log(c, "s[", 2);
  mp_log_buf(c, key);
  mp_log(c, "][", 2);
  mp_log_buf(c, content_type);
  mp_log(c, "]=", 2);
  mp_log_buf(c, value);
  mp_log(c, "|", 1);
  return mp_fail(c);
}
static int mp_on_file(void *udata,
                      fio_buf_info_s key,
                      fio_buf_info_s filename,
                      fio_buf_info_s content_type,
                      fio_buf_info_s headers,
                      fio_buf_info_s data) {
  mp_ctx_s *c = (mp_ctx_s *)udata;
  ++c->files;
  mp_log(c, "f[", 2);
  mp_log_buf(c, key);
  mp_log(c, "][", 2);
  mp_log_buf(c, filename);
  mp_log(c, "][", 2);
  mp_log_buf(c, content_type);
  mp_log(c, "]=", 2);
  mp_log_buf(c, data);
  mp_log(c, "|", 1);
  if (headers.len < sizeof(c->headers_buf)) {
    memcpy(c->headers_buf, headers.buf, headers.len);
    c->headers = FIO_BUF_INFO2(c->headers_buf, headers.len);
  }
  return mp_fail(c);
}
static const fio_multipart_parser_callbacks_s MP_CB = {
    .on_string = mp_on_string,
    .on_file = mp_on_file,
};

static void mp_ctx_reset(mp_ctx_s *c) {
  free(c->log);
  *c = (mp_ctx_s){0};
}

#define MP_B FIO_BUF_INFO1((char *)"XyZ")

/** Returns the offset of `needle` in `hay` (binary safe); asserts it's found.
 */
static size_t mp_find(const char *hay, size_t len, const char *needle) {
  size_t n = strlen(needle);
  for (size_t i = 0; i + n <= len; ++i)
    if (!memcmp(hay + i, needle, n))
      return i;
  FIO_ASSERT(0, "test bug: \"%s\" not found", needle);
  return 0;
}

/** Parses all of `msg` in one call. */
static int mp_whole(mp_ctx_s *c, const char *msg, size_t len) {
  fio_multipart_s p;
  FIO_ASSERT(!fio_multipart_init(&p, MP_B), "init failed");
  fio_multipart_result_s r = fio_multipart_parse(&p, &MP_CB, c, msg, len, 1);
  if (!r.err)
    FIO_ASSERT(r.consumed == len, "eof success must consume everything");
  return r.err;
}

/**
 * Feeds `msg` in blocks of `block` bytes, keeping only unconsumed bytes (a
 * caller's buffer), with `eof` on the last call. Returns the error (0 = ok)
 * and the peak number of bytes kept between calls in `*peak` (if set).
 */
static int mp_feed(mp_ctx_s *c,
                   const char *msg,
                   size_t len,
                   size_t block,
                   size_t *peak) {
  fio_multipart_s p;
  FIO_ASSERT(!fio_multipart_init(&p, MP_B), "init failed");
  char *buf = (char *)malloc(len + 1);
  FIO_ASSERT(buf, "allocation failed");
  size_t kept = 0, off = 0;
  int err = 0;
  if (peak)
    *peak = 0;
  for (;;) {
    size_t add = (len - off < block) ? (len - off) : block;
    memcpy(buf + kept, msg + off, add);
    kept += add;
    off += add;
    int eof = (off == len);
    fio_multipart_result_s r =
        fio_multipart_parse(&p, &MP_CB, c, buf, kept, eof);
    FIO_ASSERT(r.consumed <= kept, "consumed beyond the data");
    if (r.err) {
      err = r.err;
      break;
    }
    if (eof) {
      FIO_ASSERT(r.consumed == kept, "eof success must consume everything");
      break;
    }
    if (r.consumed) {
      memmove(buf, buf + r.consumed, kept - r.consumed);
      kept -= r.consumed;
    }
    if (peak && kept > *peak)
      *peak = kept;
  }
  free(buf);
  return err;
}

/* *****************************************************************************
Tests
***************************************************************************** */

static const char MP_MSG[] =
    "--XyZ\r\n"
    "Content-Disposition: form-data; name=\"a\"\r\n"
    "\r\n"
    "value-a\r\n"
    "--XyZ\r\n"
    "Content-Disposition: form-data; name=\"up\"; filename=\"t.txt\"\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "file\r\n-\r\n--Xy\r\nx--XyZ\0end\r\n"
    "--XyZ\r\n"
    "content-disposition: form-data; name=\"e\"\r\n"
    "Content-Type: text/x\r\n"
    "\r\n"
    "\r\n"
    "--XyZ\r\n"
    "Content-Disposition: form-data; name=\"empty\"; filename=\"\"\r\n"
    "\r\n"
    "\r\n"
    "--XyZ--\r\n"
    "epilogue --XyZ junk";

static const char MP_LOG[] =
    "s[a][(null)]=value-a|"
    "f[up][t.txt][text/plain]=file\r\n-\r\n--Xy\r\nx--XyZ\0end|"
    "s[e][text/x]=|"
    "f[empty][][(null)]=|";

FIO_SFUNC void mp_test_basic(void) {
  fprintf(stderr, "  * complete parts, borrowed values, headers\n");
  mp_ctx_s c = {0};
  FIO_ASSERT(!mp_whole(&c, MP_MSG, sizeof(MP_MSG) - 1), "parse failed");
  FIO_ASSERT(c.len == sizeof(MP_LOG) - 1 && !memcmp(c.log, MP_LOG, c.len),
             "unexpected parts: %s",
             c.log);
  FIO_ASSERT(c.strings == 2 && c.files == 2, "part counts");
  static const char hdr[] =
      "Content-Disposition: form-data; name=\"empty\"; filename=\"\"";
  FIO_ASSERT(c.headers.len == sizeof(hdr) - 1 &&
                 !memcmp(c.headers.buf, hdr, c.headers.len),
             "raw headers: %.*s",
             (int)c.headers.len,
             c.headers.buf);
  mp_ctx_reset(&c);

  /* a leading CRLF, an empty body and NULL callbacks (validated, skipped) */
  {
    static const char m[] = "\r\n--XyZ--";
    FIO_ASSERT(!mp_whole(&c, m, sizeof(m) - 1) && !c.len, "empty multipart");
    fio_multipart_s p;
    fio_multipart_init(&p, MP_B);
    fio_multipart_result_s r =
        fio_multipart_parse(&p, NULL, NULL, MP_MSG, sizeof(MP_MSG) - 1, 1);
    FIO_ASSERT(!r.err && r.consumed == sizeof(MP_MSG) - 1, "NULL callbacks");
    fio_multipart_init(&p, MP_B);
    r = fio_multipart_parse(&p, NULL, NULL, MP_MSG, 40, 1);
    FIO_ASSERT(r.err == FIO_MULTIPART_ERROR_GENERIC, "NULL callbacks: eof");
  }
  mp_ctx_reset(&c);
}

FIO_SFUNC void mp_test_blocks(void) {
  fprintf(stderr, "  * every block size gives the same parts once\n");
  const size_t len = sizeof(MP_MSG) - 1;
  for (size_t block = 1; block <= len; ++block) {
    mp_ctx_s c = {0};
    int err = mp_feed(&c, MP_MSG, len, block, NULL);
    FIO_ASSERT(!err && c.len == sizeof(MP_LOG) - 1 &&
                   !memcmp(c.log, MP_LOG, c.len),
               "block %zu: err %d, parts: %s",
               block,
               err,
               c.log);
    mp_ctx_reset(&c);
  }
}

FIO_SFUNC void mp_test_need_more(void) {
  fprintf(stderr, "  * need more: !consumed && !err, consumed parts only\n");
  fio_multipart_s p;
  mp_ctx_s c = {0};
  fio_multipart_init(&p, MP_B);
  fio_multipart_result_s r = fio_multipart_parse(&p, &MP_CB, &c, NULL, 0, 0);
  FIO_ASSERT(!r.err && !r.consumed, "no data: need more");
  r = fio_multipart_parse(&p, &MP_CB, &c, "--Xy", 4, 0);
  FIO_ASSERT(!r.err && !r.consumed, "partial first delimiter: need more");
  r = fio_multipart_parse(&p, &MP_CB, &c, "--Xa", 4, 0);
  FIO_ASSERT(r.err == FIO_MULTIPART_ERROR_GENERIC, "preamble rejected early");
  r = fio_multipart_parse(&p, &MP_CB, &c, MP_MSG, sizeof(MP_MSG) - 1, 1);
  FIO_ASSERT(r.err == FIO_MULTIPART_ERROR_GENERIC, "errors are permanent");

  /* the first part completes; the second is pending */
  size_t part1 = mp_find(MP_MSG, sizeof(MP_MSG) - 1, "value-a") + 7 +
                 9; /* past "\r\n--XyZ\r\n" */
  fio_multipart_init(&p, MP_B);
  r = fio_multipart_parse(&p, &MP_CB, &c, MP_MSG, part1 + 20, 0);
  FIO_ASSERT(!r.err && r.consumed == part1 && c.strings == 1,
             "consumed %zu (expected %zu), strings %zu",
             r.consumed,
             part1,
             c.strings);
  mp_ctx_reset(&c);
}

FIO_SFUNC void mp_test_truncation(void) {
  fprintf(stderr, "  * eof: every truncation fails, closing at eof is fine\n");
  const size_t len = sizeof(MP_MSG) - 1;
  const size_t closed = mp_find(MP_MSG, len, "--XyZ--") + 7;
  for (size_t i = 0; i < len; ++i) {
    mp_ctx_s c = {0};
    int err = mp_whole(&c, MP_MSG, i);
    int ok = (i == closed || i >= closed + 2); /* not after the lone CR */
    FIO_ASSERT(ok ? !err : err == FIO_MULTIPART_ERROR_GENERIC,
               "prefix %zu: err %d",
               i,
               err);
    if (ok)
      FIO_ASSERT(c.len == sizeof(MP_LOG) - 1, "prefix %zu: parts lost", i);
    mp_ctx_reset(&c);
  }
}

FIO_SFUNC void mp_test_closing_grammar(void) {
  fprintf(stderr, "  * closing delimiter line grammar (any split)\n");
  static const struct {
    const char *suffix;
    int ok;
  } cases[] = {
      {"", 1},
      {"\r\n", 1},
      {"\r\nepilogue\r\n--XyZ--junk", 1},
      {"  \t\r\n", 1},
      {"  ", 1},
      {"\t", 1},
      {"garbage", 0},
      {"  x", 0},
      {"\r", 0},
      {"\rx", 0},
      {" \r", 0},
      {"-", 0},
      {"\n", 0},
  };
  static const char head[] = "--XyZ\r\n"
                             "Content-Disposition: form-data; name=\"up\"; "
                             "filename=\"f\"\r\n\r\n"
                             "data\r\n--XyZ--";
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    char msg[256];
    size_t len =
        (size_t)snprintf(msg, sizeof(msg), "%s%s", head, cases[i].suffix);
    for (size_t block = 1; block <= len; ++block) {
      mp_ctx_s c = {0};
      int err = mp_feed(&c, msg, len, block, NULL);
      /* a part is reported only once its delimiter line is valid */
      FIO_ASSERT(cases[i].ok ? (!err && c.files == 1)
                             : (err == FIO_MULTIPART_ERROR_GENERIC && !c.files),
                 "suffix \"%s\", block %zu: err %d, files %zu",
                 cases[i].suffix,
                 block,
                 err,
                 c.files);
      mp_ctx_reset(&c);
    }
  }
  /* the same grammar between parts: "--XyZ" must be followed by CRLF */
  static const char bad[] = "--XyZ\r\nContent-Disposition: form-data; "
                            "name=\"a\"\r\n\r\nv\r\n--XyZx\r\n";
  mp_ctx_s c = {0};
  FIO_ASSERT(mp_whole(&c, bad, sizeof(bad) - 1) ==
                     FIO_MULTIPART_ERROR_GENERIC &&
                 !c.strings,
             "malformed delimiter line must fail before the part's callback");
  mp_ctx_reset(&c);
}

FIO_SFUNC void mp_test_callback_errors(void) {
  fprintf(stderr, "  * callback errors are opaque and propagate unchanged\n");
  static const int codes[] = {42, -7, 1, -1};
  for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); ++i) {
    for (size_t at = 1; at <= 4; ++at) {
      mp_ctx_s c = {.fail_on = codes[i], .fail_at = at};
      int err = mp_whole(&c, MP_MSG, sizeof(MP_MSG) - 1);
      FIO_ASSERT(err == codes[i] && c.strings + c.files == at,
                 "code %d at %zu: err %d, parts %zu",
                 codes[i],
                 at,
                 err,
                 c.strings + c.files);
      mp_ctx_reset(&c);
      c = (mp_ctx_s){.fail_on = codes[i], .fail_at = at};
      err = mp_feed(&c, MP_MSG, sizeof(MP_MSG) - 1, 7, NULL);
      FIO_ASSERT(err == codes[i] && c.strings + c.files == at,
                 "fed: code %d at %zu: err %d",
                 codes[i],
                 at,
                 err);
      mp_ctx_reset(&c);
    }
  }
}

FIO_SFUNC void mp_test_epilogue(void) {
  fprintf(stderr, "  * after the closing delimiter, data is ignored\n");
  fio_multipart_s p;
  mp_ctx_s c = {0};
  static const char m[] = "--XyZ--\r\n";
  fio_multipart_init(&p, MP_B);
  fio_multipart_result_s r =
      fio_multipart_parse(&p, &MP_CB, &c, m, sizeof(m) - 1, 0);
  FIO_ASSERT(!r.err && r.consumed == sizeof(m) - 1, "closing consumed");
  r = fio_multipart_parse(&p, &MP_CB, &c, MP_MSG, sizeof(MP_MSG) - 1, 0);
  FIO_ASSERT(!r.err && r.consumed == sizeof(MP_MSG) - 1 && !c.len,
             "epilogue must be consumed, never parsed");
  r = fio_multipart_parse(&p, &MP_CB, &c, NULL, 0, 1);
  FIO_ASSERT(!r.err && !r.consumed, "eof after completion");
  mp_ctx_reset(&c);
}

FIO_SFUNC void mp_test_init(void) {
  fprintf(stderr, "  * boundary validation and copy\n");
  fio_multipart_s p;
  char b[FIO_MULTIPART_MAX_BOUNDARY + 2];
  memset(b, 'b', sizeof(b));
  FIO_ASSERT(fio_multipart_init(&p, FIO_BUF_INFO2(b, 0)) == -1, "empty");
  FIO_ASSERT(fio_multipart_parse(&p, &MP_CB, NULL, "--", 2, 1).err ==
                 FIO_MULTIPART_ERROR_GENERIC,
             "a failed init fails parsing");
  FIO_ASSERT(fio_multipart_init(&p, FIO_BUF_INFO2(NULL, 3)) == -1, "NULL");
  FIO_ASSERT(fio_multipart_init(
                 &p,
                 FIO_BUF_INFO2(b, FIO_MULTIPART_MAX_BOUNDARY + 1)) == -1,
             "too long");
  FIO_ASSERT(fio_multipart_init(NULL, MP_B) == -1, "NULL parser");
  FIO_ASSERT(fio_multipart_parse(NULL, &MP_CB, NULL, "", 0, 1).err ==
                 FIO_MULTIPART_ERROR_GENERIC,
             "NULL parser");
  /* the longest boundary works, and the caller's copy may change */
  FIO_ASSERT(
      !fio_multipart_init(&p, FIO_BUF_INFO2(b, FIO_MULTIPART_MAX_BOUNDARY)),
      "max length");
  char msg[2 * FIO_MULTIPART_MAX_BOUNDARY + 128];
  size_t len = 0;
  msg[len++] = '-';
  msg[len++] = '-';
  memcpy(msg + len, b, FIO_MULTIPART_MAX_BOUNDARY);
  len += FIO_MULTIPART_MAX_BOUNDARY;
  static const char mid[] =
      "\r\nContent-Disposition: form-data; name=\"k\"\r\n\r\nv\r\n--";
  memcpy(msg + len, mid, sizeof(mid) - 1);
  len += sizeof(mid) - 1;
  memcpy(msg + len, b, FIO_MULTIPART_MAX_BOUNDARY);
  len += FIO_MULTIPART_MAX_BOUNDARY;
  msg[len++] = '-';
  msg[len++] = '-';
  memset(b, 'c', sizeof(b));
  mp_ctx_s c = {0};
  fio_multipart_result_s r = fio_multipart_parse(&p, &MP_CB, &c, msg, len, 1);
  FIO_ASSERT(!r.err && c.strings == 1, "max boundary parse: err %d", r.err);
  mp_ctx_reset(&c);
}

FIO_SFUNC void mp_test_limits(void) {
  fprintf(stderr, "  * header and padding limits, missing disposition\n");
  const size_t max = FIO_MULTIPART_MAX_HEADER_SIZE;
  char *msg = (char *)malloc(max * 2 + 256);
  FIO_ASSERT(msg, "allocation failed");
  static const char cd[] = "Content-Disposition: form-data; name=\"k\"";
  for (size_t extra = 0; extra < 2; ++extra) {
    /* headers of exactly `max` (+ extra) bytes */
    size_t len = (size_t)sprintf(msg, "--XyZ\r\n%s\r\nX-Pad: ", cd);
    size_t pad = max + extra - (len - 7) - 0;
    memset(msg + len, 'p', pad);
    len += pad;
    len += (size_t)sprintf(msg + len, "\r\n\r\nv\r\n--XyZ--");
    for (size_t block = 0; block < 2; ++block) {
      mp_ctx_s c = {0};
      int err = block ? mp_feed(&c, msg, len, 1, NULL) : mp_whole(&c, msg, len);
      FIO_ASSERT(extra ? err == FIO_MULTIPART_ERROR_GENERIC
                       : (!err && c.strings == 1),
                 "header limit (+%zu, block %zu): err %d",
                 extra,
                 block,
                 err);
      mp_ctx_reset(&c);
    }
  }
  /* Only the spaces/tabs after the two closing hyphens count as padding.
   * Check both terminal forms, with and without an earlier part. */
  for (size_t extra = 0; extra < 2; ++extra) {
    for (size_t crlf = 0; crlf < 2; ++crlf) {
      for (size_t part = 0; part < 2; ++part) {
        size_t len = (size_t)sprintf(
            msg,
            part ? "--XyZ\r\nContent-Disposition: form-data; name=\"k\"\r\n"
                   "\r\nv\r\n--XyZ--"
                 : "--XyZ--");
        memset(msg + len, ' ', max + extra);
        len += max + extra;
        if (crlf) {
          msg[len++] = '\r';
          msg[len++] = '\n';
        }
        for (size_t block = 0; block < 2; ++block) {
          mp_ctx_s c = {0};
          int err = block ? mp_feed(&c, msg, len, 127, NULL)
                          : mp_whole(&c, msg, len);
          FIO_ASSERT(extra ? err == FIO_MULTIPART_ERROR_GENERIC
                           : (!err && c.strings == part),
                     "closing padding (+%zu, crlf %zu, part %zu, block %zu): "
                     "err %d, strings %zu",
                     extra,
                     crlf,
                     part,
                     block,
                     err,
                     c.strings);
          mp_ctx_reset(&c);
        }
      }
    }
  }
  {
    static const char m[] = "--XyZ\r\nContent-Type: text/plain\r\n\r\nv\r\n"
                            "--XyZ--";
    mp_ctx_s c = {0};
    FIO_ASSERT(mp_whole(&c, m, sizeof(m) - 1) == FIO_MULTIPART_ERROR_GENERIC,
               "missing Content-Disposition must fail");
    mp_ctx_reset(&c);
  }
  free(msg);
}

/** Large parts fed in small blocks: linear time, one part kept at a time. */
FIO_SFUNC void mp_test_linear(void) {
  fprintf(stderr, "  * large parts in small blocks: linear, one part kept\n");
  const size_t big = (size_t)1 << 22; /* 4 MiB per part */
  static const char h1[] =
      "--XyZ\r\nContent-Disposition: form-data; name=\"f\"; filename=\"b\"\r\n"
      "\r\n";
  static const char h2[] =
      "\r\n--XyZ\r\nContent-Disposition: form-data; name=\"s\"\r\n\r\n";
  static const char tail[] = "\r\n--XyZ--\r\n";
  size_t len =
      (sizeof(h1) - 1) + big + (sizeof(h2) - 1) + big + (sizeof(tail) - 1);
  char *msg = (char *)malloc(len);
  FIO_ASSERT(msg, "allocation failed");
  char *pos = msg;
  memcpy(pos, h1, sizeof(h1) - 1);
  pos += sizeof(h1) - 1;
  for (size_t i = 0; i < big; ++i) /* near misses: "\r\n--XyY" */
    pos[i] = "\r\n--XyY"[i % 7];
  pos += big;
  memcpy(pos, h2, sizeof(h2) - 1);
  pos += sizeof(h2) - 1;
  for (size_t i = 0; i < big; ++i)
    pos[i] = (char)('a' + (i % 23));
  pos += big;
  memcpy(pos, tail, sizeof(tail) - 1);

  mp_ctx_s whole = {0};
  FIO_ASSERT(!mp_whole(&whole, msg, len), "whole parse failed");
  mp_ctx_s c = {0};
  size_t peak = 0;
  clock_t start = clock();
  int err = mp_feed(&c, msg, len, 61, &peak);
  double secs = (double)(clock() - start) / CLOCKS_PER_SEC;
  FIO_ASSERT(!err && c.len == whole.len && !memcmp(c.log, whole.log, c.len),
             "block parse differs from a whole parse (err %d)",
             err);
  FIO_ASSERT(peak < big + 256,
             "more than one part kept: %zu bytes (part %zu)",
             peak,
             big);
  /* a rescan per block would take minutes (~8 * 10^11 byte compares) */
  FIO_ASSERT(secs < 10.0, "parsing in blocks isn't linear (%.2fs)", secs);
  fprintf(stderr, "    (8 MiB in 61 byte blocks: %.3fs)\n", secs);
  mp_ctx_reset(&c);
  mp_ctx_reset(&whole);
  free(msg);
}

/** Callers breaking the contract (data not kept) get a rescan, not a crash. */
FIO_SFUNC void mp_test_contract(void) {
  fprintf(stderr, "  * shorter data than already searched: rescanned\n");
  fio_multipart_s p;
  mp_ctx_s c = {0};
  const size_t len = sizeof(MP_MSG) - 1;
  const size_t part1 = mp_find(MP_MSG, len, "value-a") + 7 + 9;
  fio_multipart_init(&p, MP_B);
  /* the second part's headers (87 bytes) are complete, its body pending */
  fio_multipart_result_s r =
      fio_multipart_parse(&p, &MP_CB, &c, MP_MSG, part1 + 100, 0);
  FIO_ASSERT(!r.err && r.consumed == part1, "first part");
  r = fio_multipart_parse(&p, &MP_CB, &c, MP_MSG + part1, 10, 0);
  FIO_ASSERT(!r.err && !r.consumed, "shorter data: need more");
  r = fio_multipart_parse(&p, &MP_CB, &c, MP_MSG + part1, len - part1, 1);
  FIO_ASSERT(!r.err && c.len == sizeof(MP_LOG) - 1 &&
                 !memcmp(c.log, MP_LOG, c.len),
             "resumed parse: %s",
             c.log);
  mp_ctx_reset(&c);
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

void fio_test_multipart(void) {
  fprintf(stderr, "Testing MIME multipart parser\n");
  mp_test_basic();
  mp_test_blocks();
  mp_test_need_more();
  mp_test_truncation();
  mp_test_closing_grammar();
  mp_test_callback_errors();
  mp_test_epilogue();
  mp_test_init();
  mp_test_limits();
  mp_test_contract();
  mp_test_linear();
}

#ifndef FIO_TEST_ALL
int main(void) {
  fio_test_multipart();
  return 0;
}
#endif
