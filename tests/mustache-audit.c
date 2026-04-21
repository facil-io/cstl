#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIO_MUSTACHE
#include "../fio-stl.h"

static void run_load_only(char *buf, size_t len) {
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2(buf, len));
  if (m)
    fio_mustache_free(m);
}

static void run_load_and_build(char *buf, size_t len) {
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2(buf, len));
  if (!m)
    return;
  char *out = (char *)fio_mustache_build(m, .ctx = NULL);
  if (out) {
    fwrite(out, 1, fio_bstr_len(out), stdout);
    fputc('\n', stdout);
    fio_bstr_free(out);
  }
  fio_mustache_free(m);
}

int main(int argc, char **argv) {
  int r = 2;
  // if (argc != 2) {
  //   fprintf(stderr,
  //           "usage: %s "
  //           "<single_open|short_yaml|tag_eof|long_var|long_var_safe|nul_"
  //           "partial|self_partial>\n",
  //           argv[0]);
  //   return 2;
  // }

  if (argc < 2 || !strcmp(argv[1], "single_open")) {
    fprintf(stderr, "\t- single_open\n");
    char *buf = malloc(1);
    buf[0] = '{';
    run_load_only(buf, 1);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "short_yaml")) {
    fprintf(stderr, "\t- short_yaml\n");
    char *buf = malloc(3);
    memcpy(buf, "---", 3);
    run_load_only(buf, 3);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "tag_eof")) {
    fprintf(stderr, "\t- tag_eof\n");
    static const char tag[] = "{{name}}";
    char *buf = malloc(sizeof(tag) - 1);
    memcpy(buf, tag, sizeof(tag) - 1);
    run_load_only(buf, sizeof(tag) - 1);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "long_var")) {
    fprintf(stderr, "\t- long_var\n");
    size_t name_len = 70000;
    size_t total = name_len + 4;
    char *buf = malloc(total);
    buf[0] = '{';
    buf[1] = '{';
    memset(buf + 2, 'a', name_len);
    buf[total - 2] = '}';
    buf[total - 1] = '}';
    run_load_and_build(buf, total);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "long_var_safe")) {
    fprintf(stderr, "\t- long_var_safe\n");
    size_t name_len = 70000;
    size_t total = name_len + 5;
    char *buf = malloc(total);
    buf[0] = '{';
    buf[1] = '{';
    memset(buf + 2, 'a', name_len);
    buf[total - 3] = '}';
    buf[total - 2] = '}';
    buf[total - 1] = 'X';
    run_load_and_build(buf, total);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "nul_partial")) {
    fprintf(stderr, "\t- nul_partial\n");
    static const char prefix[] = "A{{>./tmp/np.mustache";
    static const char suffix[] = "junk}}B";
    size_t total = (sizeof(prefix) - 1) + 1 + (sizeof(suffix) - 1);
    char *buf = malloc(total);
    memcpy(buf, prefix, sizeof(prefix) - 1);
    buf[sizeof(prefix) - 1] = '\0';
    memcpy(buf + sizeof(prefix), suffix, sizeof(suffix) - 1);
    run_load_and_build(buf, total);
    free(buf);
    r = 0;
  }

  if (argc < 2 || !strcmp(argv[1], "self_partial")) {
    fprintf(stderr, "\t- self_partial\n");
    static const char buf[] = "{{>./tmp/self.mustache}}";
    run_load_and_build((char *)buf, sizeof(buf) - 1);
    r = 0;
  }
  if (r)
    fprintf(stderr, "unknown case: %s\n", argv[1]);
  return r;
}
