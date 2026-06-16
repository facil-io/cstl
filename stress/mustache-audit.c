/* *****************************************************************************
Stress/Audit - Mustache parser fuzz/audit harness (104 mustache.h)

Audit harness ported from tests-old/mustache-audit.c.
Runs manually or on request. Exercises the Mustache loader/builder with
large inputs, embedded NUL bytes, partial path traversal, and other
fuzz-like inputs that go beyond the correctness tests in ./tests/mustache.c
and ./tests/mustache-spec.c.

No external processes are spawned.
***************************************************************************** */
#include "tests/test-helpers.h"

#define FIO_MUSTACHE
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Configuration
***************************************************************************** */

#define MUS_AUDIT_LONG_LEN 70000
#define MUS_AUDIT_SELF_PARTIAL_PATH "./tmp/self.mustache"

/* *****************************************************************************
Helpers
***************************************************************************** */

static void run_load_only(const char *buf, size_t len) {
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2((char *)buf, len));
  if (m)
    fio_mustache_free(m);
}

static void run_load_and_build(const char *buf, size_t len) {
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2((char *)buf, len));
  if (!m)
    return;
  char *out = (char *)fio_mustache_build(m, .ctx = NULL);
  if (out)
    fio_bstr_free(out);
  fio_mustache_free(m);
}

/* *****************************************************************************
Audit cases
***************************************************************************** */

static int audit_single_open(void) {
  fprintf(stderr, "\t- single_open\n");
  char buf[1] = {'{'};
  run_load_only(buf, 1);
  return 0;
}

static int audit_short_yaml(void) {
  fprintf(stderr, "\t- short_yaml\n");
  static const char buf[] = "---";
  run_load_only(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_tag_eof(void) {
  fprintf(stderr, "\t- tag_eof\n");
  static const char buf[] = "{{name}}";
  run_load_only(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_long_var(void) {
  fprintf(stderr, "\t- long_var\n");
  size_t name_len = MUS_AUDIT_LONG_LEN;
  size_t total = name_len + 4;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  buf[0] = '{';
  buf[1] = '{';
  FIO_MEMSET(buf + 2, 'a', name_len);
  buf[total - 2] = '}';
  buf[total - 1] = '}';
  run_load_and_build(buf, total);
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_long_var_safe(void) {
  fprintf(stderr, "\t- long_var_safe\n");
  size_t name_len = MUS_AUDIT_LONG_LEN;
  size_t total = name_len + 5;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  buf[0] = '{';
  buf[1] = '{';
  FIO_MEMSET(buf + 2, 'a', name_len);
  buf[total - 3] = '}';
  buf[total - 2] = '}';
  buf[total - 1] = 'X';
  run_load_and_build(buf, total);
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_nul_partial(void) {
  fprintf(stderr, "\t- nul_partial\n");
  static const char prefix[] = "A{{>./tmp/np.mustache";
  static const char suffix[] = "junk}}B";
  size_t total = (sizeof(prefix) - 1) + 1 + (sizeof(suffix) - 1);
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, prefix, sizeof(prefix) - 1);
  buf[sizeof(prefix) - 1] = '\0';
  FIO_MEMCPY(buf + sizeof(prefix), suffix, sizeof(suffix) - 1);
  run_load_and_build(buf, total);
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_self_partial(void) {
  fprintf(stderr, "\t- self_partial\n");
  static const char content[] = "{{>" MUS_AUDIT_SELF_PARTIAL_PATH "}}";
  /* Create a self-referential partial file to exercise recursion handling. */
  fio_filename_overwrite(MUS_AUDIT_SELF_PARTIAL_PATH,
                         content,
                         sizeof(content) - 1);
  static const char buf[] = "{{>" MUS_AUDIT_SELF_PARTIAL_PATH "}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  remove(MUS_AUDIT_SELF_PARTIAL_PATH);
  return 0;
}

static int audit_path_traversal_leading(void) {
  fprintf(stderr, "\t- path_traversal_leading\n");
  static const char buf[] = "{{>../etc/passwd}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_path_traversal_middle(void) {
  fprintf(stderr, "\t- path_traversal_middle\n");
  static const char buf[] = "{{>foo/../../etc/passwd}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_path_traversal_double_dot(void) {
  fprintf(stderr, "\t- path_traversal_double_dot\n");
  static const char buf[] = "{{>..}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_path_traversal_dot_dot_slash(void) {
  fprintf(stderr, "\t- path_traversal_dot_dot_slash\n");
  static const char buf[] = "{{>../secret}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_path_traversal_nested(void) {
  fprintf(stderr, "\t- path_traversal_nested\n");
  static const char buf[] = "{{>a/b/../c/d}}";
  run_load_and_build(buf, sizeof(buf) - 1);
  return 0;
}

static int audit_many_tags(void) {
  fprintf(stderr, "\t- many_tags\n");
  size_t count = 10000;
  size_t total = count * 5;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  for (size_t i = 0; i < count; ++i) {
    buf[i * 5 + 0] = '{';
    buf[i * 5 + 1] = '{';
    buf[i * 5 + 2] = 'x';
    buf[i * 5 + 3] = '}';
    buf[i * 5 + 4] = '}';
  }
  run_load_and_build(buf, total);
  FIO_MEM_FREE(buf, total);
  return 0;
}

/* *****************************************************************************
Case registry
***************************************************************************** */

typedef struct {
  const char *name;
  int (*fn)(void);
} mus_audit_case_s;

static const mus_audit_case_s mus_audit_cases[] = {
    {.name = "single_open", .fn = audit_single_open},
    {.name = "short_yaml", .fn = audit_short_yaml},
    {.name = "tag_eof", .fn = audit_tag_eof},
    {.name = "long_var", .fn = audit_long_var},
    {.name = "long_var_safe", .fn = audit_long_var_safe},
    {.name = "nul_partial", .fn = audit_nul_partial},
    {.name = "self_partial", .fn = audit_self_partial},
    {.name = "path_traversal_leading", .fn = audit_path_traversal_leading},
    {.name = "path_traversal_middle", .fn = audit_path_traversal_middle},
    {.name = "path_traversal_double_dot", .fn = audit_path_traversal_double_dot},
    {.name = "path_traversal_dot_dot_slash", .fn = audit_path_traversal_dot_dot_slash},
    {.name = "path_traversal_nested", .fn = audit_path_traversal_nested},
    {.name = "many_tags", .fn = audit_many_tags},
    {NULL, NULL},
};

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(int argc, char **argv) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== Mustache audit harness ===\n");

  int r = 0;
  if (argc > 1) {
    for (const mus_audit_case_s *c = mus_audit_cases; c->name; ++c) {
      if (!strcmp(argv[1], c->name)) {
        r = c->fn();
        fprintf(stderr, "=== Mustache audit harness %s ===\n",
                r ? "FAILED" : "passed");
        return r;
      }
    }
    fprintf(stderr, "unknown case: %s\n", argv[1]);
    return 2;
  }

  for (const mus_audit_case_s *c = mus_audit_cases; c->name; ++c) {
    if (c->fn()) {
      r = 1;
    }
  }

  fprintf(stderr,
          "=== Mustache audit harness %s ===\n",
          r ? "FAILED" : "passed");
  return r;
}
