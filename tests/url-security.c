/* Exact-length URL inputs and secret-free TLS diagnostics. */
#include <stdio.h>

static FILE *url_log_stream;
#define FIO_STDERR_FILE (url_log_stream ? url_log_stream : stderr)
#define FIO_LOG
#define FIO_URL
#include "fio-stl/include.h"

static fio_url_s url_test_parse(const char *text, size_t len) {
  char *input = (char *)malloc(len ? len : 1);
  FIO_ASSERT(input, "URL test allocation failed");
  if (len)
    FIO_MEMCPY(input, text, len);
  fio_url_s u = fio_url_parse(input, len);
  fio_buf_info_s parts[] =
      {u.scheme, u.user, u.password, u.host, u.port, u.path, u.query, u.target};
  for (size_t i = 0; i < sizeof(parts) / sizeof(*parts); ++i) {
    FIO_ASSERT(!parts[i].len ||
                   (parts[i].buf >= input && parts[i].buf <= input + len &&
                    parts[i].len <= (size_t)((input + len) - parts[i].buf)),
               "URL component %zu outside %zu-byte input",
               i,
               len);
    FIO_ASSERT(parts[i].len || !parts[i].buf,
               "empty URL component %zu not normalized",
               i);
  }
  (void)fio_url_is_tls(u);
  /* Return only lengths/NULL state; the input storage is no longer owned. */
  free(input);
  u.scheme.buf = u.user.buf = u.password.buf = u.host.buf = NULL;
  u.port.buf = u.path.buf = u.query.buf = u.target.buf = NULL;
  return u;
}

static void url_test_host(void) {
  const char *vectors[] = {"@", "u@", ":@", "http://u@", "x:", "http://", ""};
  for (size_t i = 0; i < sizeof(vectors) / sizeof(*vectors); ++i)
    (void)url_test_parse(vectors[i], strlen(vectors[i]));
  fio_url_s u = fio_url_parse(NULL, 0);
  FIO_ASSERT(!u.host.len && !u.path.len && !u.scheme.len,
             "empty URL should have no components");
}

static void url_test_query(void) {
  const struct {
    const char *text;
    bool tls;
    bool material;
  } vectors[] = {{"tls", .tls = 1},
                 {"TlS", .tls = 1},
                 {"key", .tls = 0},
                 {"ssl", .tls = 1},
                 {"abc", .tls = 0},
                 {"%00", .tls = 0},
                 {"key=x&cert=y&tls", .tls = 1},
                 {"tls&", .tls = 1},
                 {"tls&abc", .tls = 1},
                 {"tls=", .tls = 1},
                 {"tls=1", .tls = 1},
                 {"tls=true", .tls = 1},
                 {"ssl=TRUE", .tls = 1},
                 {"TLS=k&pass=p", 1, 1},
                 {"key=&cert=", .tls = 0},
                 {"&&tls&&", .tls = 1},
                 {"cert=c&key=k&password=p&trust=t", 1, 1}};
  for (size_t i = 0; i < sizeof(vectors) / sizeof(*vectors); ++i) {
    size_t len = strlen(vectors[i].text);
    char *input = (char *)malloc(len);
    FIO_ASSERT(input, "query test allocation failed");
    FIO_MEMCPY(input, vectors[i].text, len);
    fio_url_tls_info_s tls =
        fio_url_is_tls((fio_url_s){.query = FIO_BUF_INFO2(input, len)});
    FIO_ASSERT(tls.tls == vectors[i].tls,
               "TLS query detection failed at vector %zu",
               i);
    if (vectors[i].material)
      FIO_ASSERT(tls.key.len == 1 && tls.cert.len == 1 && tls.pass.len == 1,
                 "TLS material lost at query vector %zu",
                 i);
    free(input);
  }
  const char *urls[] = {"/?tls", "/?key", "/?ssl", "/?abc", "/?%00"};
  for (size_t i = 0; i < sizeof(urls) / sizeof(*urls); ++i)
    (void)url_test_parse(urls[i], strlen(urls[i]));
  /* End-of-query/control handling must not form end + 1 or invent a target. */
  const char *ends[] = {"/?q", "/?q#", "/?q\x01suffix", "/?q#\x01suffix"};
  for (size_t i = 0; i < sizeof(ends) / sizeof(*ends); ++i) {
    fio_url_s u = url_test_parse(ends[i], strlen(ends[i]));
    FIO_ASSERT(u.path.len == 1 && u.query.len == 1 && !u.target.len,
               "query termination changed at vector %zu",
               i);
  }
}

static void url_test_scheme(void) {
  const char *vectors[] = {"wss",
                           "TLS",
                           "sSl",
                           "abc",
                           "sses",
                           "ssse",
                           "tcps",
                           "stcp",
                           "udps",
                           "sudp",
                           "Https",
                           "http"};
  for (size_t i = 0; i < sizeof(vectors) / sizeof(*vectors); ++i) {
    size_t len = strlen(vectors[i]);
    char *input = (char *)malloc(len);
    FIO_ASSERT(input, "scheme test allocation failed");
    FIO_MEMCPY(input, vectors[i], len);
    fio_url_tls_info_s tls =
        fio_url_is_tls((fio_url_s){.scheme = FIO_BUF_INFO2(input, len)});
    FIO_ASSERT(tls.tls == (i != 3 && i != 11),
               "TLS scheme detection failed at vector %zu",
               i);
    free(input);
  }
}

static void url_test_bytes(void) {
  const char *vectors[] = {("/a\xC3\xA9"
                            "b?q#f"),
                           "h\x80/",
                           "u\xFF@h/",
                           "u:p\x80@h/",
                           "h:8\xFF/p",
                           "/p?q\x80#f",
                           "tcp://h\x80/p",
                           "http://[a\x80]/"};
  const size_t lengths[][3] = {{5, 1, 1},
                               {1, 0, 0},
                               {1, 0, 0},
                               {1, 0, 0},
                               {2, 0, 0},
                               {2, 2, 1},
                               {2, 0, 0},
                               {1, 0, 0}};
  for (size_t i = 0; i < sizeof(vectors) / sizeof(*vectors); ++i) {
    fio_url_s u = url_test_parse(vectors[i], strlen(vectors[i]));
    FIO_ASSERT(u.path.len == lengths[i][0] && u.query.len == lengths[i][1] &&
                   u.target.len == lengths[i][2],
               "high byte truncated URL vector %zu",
               i);
    if (i == 1 || i == 6 || i == 7)
      FIO_ASSERT(u.host.len == 2, "high byte truncated host vector %zu", i);
    if (i == 2)
      FIO_ASSERT(u.user.len == 2, "high byte truncated username");
    if (i == 3)
      FIO_ASSERT(u.password.len == 2, "high byte truncated password");
    if (i == 4)
      FIO_ASSERT(u.port.len == 2, "high byte truncated port");
  }
  const char controls[] = {'\0', '\x01', '\x1F', ' '};
  for (size_t i = 0; i < sizeof(controls); ++i) {
    char text[] = {'/', 'a', controls[i], 'b'};
    fio_url_s u = url_test_parse(text, sizeof(text));
    FIO_ASSERT(u.path.len == (controls[i] == ' ' ? 4 : 2),
               "ASCII control/space policy changed");
  }
}

static void url_test_logging(void) {
  char url_tls_log[2048] = {0};
  url_log_stream = tmpfile();
  FIO_ASSERT(url_log_stream, "couldn't capture URL diagnostics");
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_DEBUG);
  const char *text = "tls://host/?key=private_marker&cert=cert_marker&"
                     "pass=passphrase_marker&trust=trust_marker";
  fio_url_tls_info_s tls = fio_url_is_tls(fio_url_parse(text, strlen(text)));
  FIO_ASSERT(tls.tls && tls.key.len && tls.cert.len && tls.pass.len &&
                 tls.trust.len,
             "TLS configuration not retained");
  FIO_ASSERT(!fflush(url_log_stream) && !fseek(url_log_stream, 0, SEEK_SET),
             "couldn't read URL diagnostics");
  size_t log_len =
      fread(url_tls_log, 1, sizeof(url_tls_log) - 1, url_log_stream);
  FIO_ASSERT(!ferror(url_log_stream), "URL diagnostic read failed");
  FIO_ASSERT(!fclose(url_log_stream), "URL diagnostic close failed");
  url_log_stream = NULL;
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
#ifdef DEBUG
  FIO_ASSERT(log_len && strstr(url_tls_log, "URL TLS detection"),
             "debug diagnostic was not captured");
#else
  (void)log_len;
#endif
#ifdef DEBUG
  FIO_ASSERT(strstr(url_tls_log, "key: 1") && strstr(url_tls_log, "cert: 1") &&
                 strstr(url_tls_log, "trust: 1") &&
                 strstr(url_tls_log, "pass: 1"),
             "TLS diagnostic presence flags lost");
#endif
  FIO_ASSERT(!strstr(url_tls_log, "private_marker") &&
                 !strstr(url_tls_log, "cert_marker") &&
                 !strstr(url_tls_log, "passphrase_marker") &&
                 !strstr(url_tls_log, "trust_marker"),
             "TLS diagnostics disclosed configuration material: %s",
             url_tls_log);
}

static void url_test_prefixes(void) {
  const char *vectors[] = {"scheme://user:password@[::1]:8080/path?tls#target",
                           "unix:///socket?tls",
                           "./relative?query#fragment",
                           "host:port?query#target",
                           "/?q\x01suffix",
                           "/?#"};
  for (size_t i = 0; i < sizeof(vectors) / sizeof(*vectors); ++i)
    for (size_t len = 0; len <= strlen(vectors[i]); ++len)
      (void)url_test_parse(vectors[i], len);
  uint32_t state = 1;
  for (size_t len = 1; len <= 32; ++len) {
    char text[32];
    for (size_t round = 0; round < 32; ++round) {
      for (size_t i = 0; i < len; ++i) {
        state = state * 1664525U + 1013904223U;
        text[i] = (char)(state >> 24);
      }
      (void)url_test_parse(text, len);
    }
  }
}

int main(int argc, char **argv) {
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
  struct {
    const char *name;
    void (*run)(void);
  } tests[] = {{"host", url_test_host},
               {"query", url_test_query},
               {"scheme", url_test_scheme},
               {"bytes", url_test_bytes},
               {"logging", url_test_logging},
               {"prefixes", url_test_prefixes}};
  for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    if (argc == 1 || !strcmp(argv[1], tests[i].name))
      tests[i].run();
  return 0;
}
