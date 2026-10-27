/* *****************************************************************************
Test: HTTP API security regressions (430 http api.h)

- API-01: FIO_HTTP_PATH_EACH must never write past its stack buffer when a
  section reaches the cutoff (plain, percent-encoded and invalid escapes).
- API-02: truncated escapes at the exact end of the input stay literal and
  never read (or form pointers) past the input.
- API-03: opaque pointer accessors address distinct members.
***************************************************************************** */
#define FIO_HTTP
#include "test-helpers.h"

/* the macro's buffer: `fio_buf_info_s buf[(4096 / sizeof(fio_buf_info_s)) - 2]` */
#define PATH_BUF_LEN                                                           \
  (sizeof(fio_buf_info_s) * ((4096 / sizeof(fio_buf_info_s)) - 2))
#define PATH_CUTOFF (PATH_BUF_LEN - 1)
#define CANARY_LEN  64

/* runs the macro helper on an exact-size heap copy (no trailing NUL) */
static size_t path_run(const char *src,
                       size_t len,
                       fio_buf_info_s *sections,
                       size_t max,
                       char (*copies)[PATH_BUF_LEN],
                       int *completed) {
  char *in = (char *)malloc(len ? len : 1);
  unsigned char *mem = (unsigned char *)malloc(PATH_BUF_LEN + CANARY_LEN);
  FIO_ASSERT(in && mem, "allocation failed");
  if (len)
    memcpy(in, src, len);
  memset(mem + PATH_BUF_LEN, 0xA5, CANARY_LEN);
  fio_buf_info_s rem = FIO_BUF_INFO2(in, len), pos;
  size_t count = 0;
  *completed = 1;
  for (;;) {
    int r = fio___http_path_each_next(&pos, &rem, mem);
    for (size_t i = 0; i < CANARY_LEN; ++i)
      FIO_ASSERT(mem[PATH_BUF_LEN + i] == 0xA5,
                 "path helper wrote past its buffer (len %zu, byte +%zu)",
                 len,
                 i);
    FIO_ASSERT(pos.len <= PATH_CUTOFF && pos.buf[pos.len] == 0,
               "section not terminated inside the buffer");
    if (!r) {
      /* stopped: record the partial section to check cutoff behavior */
      if (rem.len)
        *completed = 0;
      if (rem.len && count < max) {
        sections[count] = pos;
        memcpy(copies[count], pos.buf, pos.len + 1);
        sections[count].buf = copies[count];
        ++count;
      }
      break;
    }
    if (count < max) {
      memcpy(copies[count], pos.buf, pos.len + 1);
      sections[count] = FIO_BUF_INFO2(copies[count], pos.len);
    }
    ++count;
  }
  free(mem);
  free(in);
  return count;
}

static void test_path_cutoff(void) {
  fprintf(stderr, "  * FIO_HTTP_PATH_EACH cutoff stays inside its buffer\n");
  static char src[16384];
  static char copies[4][PATH_BUF_LEN];
  fio_buf_info_s s[4];
  int done;
  const size_t lens[] = {PATH_CUTOFF - 2,
                         PATH_CUTOFF - 1,
                         PATH_CUTOFF,
                         PATH_CUTOFF + 1,
                         PATH_CUTOFF + 2,
                         8000};
  /* the fallback write after a cutoff: plain, '%', valid and invalid escapes */
  const char *tails[] = {"", "%", "%4", "%41", "%zz", "a41", "/z"};
  for (size_t li = 0; li < sizeof(lens) / sizeof(lens[0]); ++li) {
    for (size_t ti = 0; ti < sizeof(tails) / sizeof(tails[0]); ++ti) {
      size_t tl = strlen(tails[ti]);
      src[0] = '/';
      memset(src + 1, 'x', lens[li]);
      memcpy(src + 1 + lens[li], tails[ti], tl);
      size_t len = 1 + lens[li] + tl;
      size_t n = path_run(src, len, s, 4, copies, &done);
      FIO_ASSERT(n >= 1, "no section for len %zu", lens[li]);
      if (lens[li] >= PATH_CUTOFF && tl && tails[ti][0] != '/') {
        /* section exceeds the cutoff: loop must stop early */
        FIO_ASSERT(!done && n == 1 && s[0].len == PATH_CUTOFF,
                   "cutoff not honored (len %zu tail %s): n=%zu slen=%zu",
                   lens[li],
                   tails[ti],
                   n,
                   s[0].len);
      }
    }
  }
  /* exact fits: 4062 'x' + "%41" decodes to exactly the cutoff */
  src[0] = '/';
  memset(src + 1, 'x', PATH_CUTOFF - 1);
  memcpy(src + PATH_CUTOFF, "%41/z", 5);
  size_t n = path_run(src, PATH_CUTOFF + 5, s, 4, copies, &done);
  FIO_ASSERT(done && n == 2 && s[0].len == PATH_CUTOFF &&
                 s[0].buf[PATH_CUTOFF - 1] == 'A' && s[1].len == 1 &&
                 s[1].buf[0] == 'z',
             "exact-fit escaped section mis-decoded (n=%zu)",
             n);
  /* all-escaped section crossing the cutoff */
  src[0] = '/';
  for (size_t i = 0; i < PATH_CUTOFF + 4; ++i)
    memcpy(src + 1 + i * 3, "%42", 3);
  n = path_run(src, 1 + (PATH_CUTOFF + 4) * 3, s, 4, copies, &done);
  FIO_ASSERT(!done && n == 1 && s[0].len == PATH_CUTOFF &&
                 s[0].buf[0] == 'B' && s[0].buf[PATH_CUTOFF - 1] == 'B',
             "escaped cutoff mis-handled");
  /* the macro itself, at and past the cutoff (ASan guards the stack) */
  for (size_t extra = 0; extra < 4; ++extra) {
    memset(src + 1, 'y', PATH_CUTOFF - 1 + extra);
    memcpy(src + PATH_CUTOFF + extra, "%41", 3);
    fio_str_info_s path = FIO_STR_INFO2(src, PATH_CUTOFF + extra + 3);
    size_t count = 0;
    FIO_HTTP_PATH_EACH(path, sec) {
      FIO_ASSERT(sec.len <= PATH_CUTOFF && !sec.buf[sec.len],
                 "macro section overflow");
      ++count;
    }
    FIO_ASSERT(count == (extra ? 0U : 1U), "macro cutoff count %zu", count);
  }
}

static void test_path_truncated_escape(void) {
  fprintf(stderr, "  * FIO_HTTP_PATH_EACH truncated escapes at input end\n");
  static char copies[4][PATH_BUF_LEN];
  fio_buf_info_s s[4];
  int done;
  struct {
    const char *in;
    const char *out[2];
  } cases[] = {
      {"/x%", {"x%", NULL}},
      {"/x%4", {"x%4", NULL}},
      {"/%", {"%", NULL}},
      {"/%4", {"%4", NULL}},
      {"/a/%", {"a", "%"}},
      {"/a%2/b", {"a%2", "b"}},
      {"/%00/%2F", {"\0", "/"}},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    size_t len = strlen(cases[i].in);
    size_t n = path_run(cases[i].in, len, s, 4, copies, &done);
    size_t expect = 1 + !!cases[i].out[1];
    FIO_ASSERT(done && n == expect, "%s: %zu sections", cases[i].in, n);
    for (size_t k = 0; k < n; ++k) {
      size_t el = cases[i].out[k][0] ? strlen(cases[i].out[k]) : 1;
      FIO_ASSERT(s[k].len == el && !memcmp(s[k].buf, cases[i].out[k], el),
                 "%s: section %zu mismatch",
                 cases[i].in,
                 k);
    }
  }
}

static void test_accessors(void) {
  fprintf(stderr, "  * opaque pointer accessors are independent\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new failed");
  fio_http_controller_s *c = fio_http_controller(h);
  FIO_ASSERT(c, "default controller missing");
  int a, b, d;
  FIO_ASSERT(fio_http_udata_set(h, &a) == &a, "udata_set return");
  FIO_ASSERT(fio_http_udata2_set(h, &b) == &b, "udata2_set return");
  FIO_ASSERT(fio_http_cdata_set(h, &d) == &d, "cdata_set return");
  FIO_ASSERT(fio_http_udata(h) == &a && fio_http_udata2(h) == &b &&
                 fio_http_cdata(h) == &d && fio_http_controller(h) == c,
             "accessor members overlap");
  fio_http_udata2_set(h, NULL);
  FIO_ASSERT(fio_http_udata(h) == &a && !fio_http_udata2(h) &&
                 fio_http_cdata(h) == &d && fio_http_controller(h) == c,
             "udata2_set changed another member");
  FIO_ASSERT(fio_http_controller_set(h, c) == c &&
                 fio_http_controller(h) == c && fio_http_cdata(h) == &d,
             "controller_set changed another member");
  fio_http_free(h);
}

int main(void) {
  test_path_cutoff();
  test_path_truncated_escape();
  test_accessors();
  return 0;
}
