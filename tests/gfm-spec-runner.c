/* GFM spec conformance test runner.
 *
 * Parses gfm-spec.txt and gfm-extensions.txt, extracts examples,
 * renders markdown to HTML with fio_md2html(),
 * and compares output against expected HTML.
 *
 * Build: make tests/gfm-spec-runner/db
 */
#define FIO_LOG
#define FIO_MD2HTML
#include "test-helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Configuration
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define SPEC_MAX_EXAMPLES 800
#define SPEC_DETAIL_LIMIT 20 /* print first N failures in detail */

/* ═══════════════════════════════════════════════════════════════════════════
 * Spec Example Storage
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  const char *md; /* markdown input (tab-expanded, owned) */
  size_t md_len;
  const char *html; /* expected HTML output (tab-expanded, owned) */
  size_t html_len;
  int number;       /* example number (1-based) */
  int line;         /* line number in spec file */
  const char *file; /* source file name */
} spec_example_s;

static spec_example_s g_examples[SPEC_MAX_EXAMPLES];
static int g_example_count;

/* ═══════════════════════════════════════════════════════════════════════════
 * Spec File Parser
 *
 * Format: examples delimited by 32-backtick fences with " example" tag.
 * Inside, markdown and expected HTML are separated by a line containing
 * just ".". The spec uses UTF-8 → (U+2192, 3 bytes: e2 86 92) to
 * represent literal tab characters.
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* The 32-backtick fence that opens/closes examples. */
static const char EXAMPLE_FENCE[] = "````````````````````````````````";
#define EXAMPLE_FENCE_LEN 32

/** Replace UTF-8 → (e2 86 92) with \t in a malloc'd copy. Caller frees. */
static char *spec_expand_tabs(const char *src, size_t len, size_t *out_len) {
  /* Count arrows to size output */
  size_t arrows = 0;
  for (size_t i = 0; i + 2 < len; ++i) {
    if ((unsigned char)src[i] == 0xe2 && (unsigned char)src[i + 1] == 0x86 &&
        (unsigned char)src[i + 2] == 0x92)
      ++arrows;
  }
  size_t new_len = len - (arrows * 2); /* 3-byte arrow → 1-byte tab */
  char *out = (char *)malloc(new_len + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  for (size_t i = 0; i < len;) {
    if (i + 2 < len && (unsigned char)src[i] == 0xe2 &&
        (unsigned char)src[i + 1] == 0x86 &&
        (unsigned char)src[i + 2] == 0x92) {
      out[j++] = '\t';
      i += 3;
    } else {
      out[j++] = src[i++];
    }
  }
  out[j] = '\0';
  *out_len = j;
  return out;
}

/** Parse a spec file and append examples to g_examples[]. */
static int spec_parse_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "ERROR: cannot open %s\n", path);
    return -1;
  }

  /* Read entire file into memory */
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)fsize + 1);
  if (!buf) {
    fclose(f);
    return -1;
  }
  size_t nread = fread(buf, 1, (size_t)fsize, f);
  fclose(f);
  buf[nread] = '\0';

  /* Extract basename for display */
  const char *basename = path;
  for (const char *p = path; *p; ++p) {
    if (*p == '/')
      basename = p + 1;
  }

  int example_num = 0;
  int line_num = 1;
  char *p = buf;
  char *end = buf + nread;

  while (p < end) {
    /* Find next line */
    char *le = p;
    while (le < end && *le != '\n')
      ++le;
    size_t line_len = (size_t)(le - p);

    /* Check for example opening fence */
    if (line_len >= EXAMPLE_FENCE_LEN + 8 &&
        memcmp(p, EXAMPLE_FENCE, EXAMPLE_FENCE_LEN) == 0 &&
        memcmp(p + EXAMPLE_FENCE_LEN, " example", 8) == 0) {

      ++example_num;
      int start_line = line_num;

      /* Advance past opening fence line */
      p = le + (le < end);
      ++line_num;

      /* Collect markdown portion (until line == ".") */
      char *md_start = p;
      char *md_end = NULL;
      while (p < end) {
        le = p;
        while (le < end && *le != '\n')
          ++le;
        if (le - p == 1 && *p == '.') {
          md_end = p;
          p = le + (le < end);
          ++line_num;
          break;
        }
        p = le + (le < end);
        ++line_num;
      }
      if (!md_end)
        break;

      /* Collect expected HTML (until closing fence) */
      char *html_start = p;
      char *html_end = NULL;
      while (p < end) {
        le = p;
        while (le < end && *le != '\n')
          ++le;
        line_len = (size_t)(le - p);
        if (line_len >= EXAMPLE_FENCE_LEN &&
            memcmp(p, EXAMPLE_FENCE, EXAMPLE_FENCE_LEN) == 0) {
          html_end = p;
          p = le + (le < end);
          ++line_num;
          break;
        }
        p = le + (le < end);
        ++line_num;
      }
      if (!html_end)
        break;

      /* Store example */
      if (g_example_count >= SPEC_MAX_EXAMPLES) {
        fprintf(stderr,
                "WARNING: too many examples, increase SPEC_MAX_EXAMPLES\n");
        break;
      }

      size_t md_raw_len = (size_t)(md_end - md_start);
      size_t html_raw_len = (size_t)(html_end - html_start);

      spec_example_s *ex = &g_examples[g_example_count++];
      ex->md = spec_expand_tabs(md_start, md_raw_len, &ex->md_len);
      ex->html = spec_expand_tabs(html_start, html_raw_len, &ex->html_len);
      ex->number = example_num;
      ex->line = start_line;
      ex->file = basename;
      continue;
    }

    p = le + (le < end);
    ++line_num;
  }

  free(buf);
  return 0;
}

static void spec_free_examples(void) {
  for (int i = 0; i < g_example_count; ++i) {
    free((void *)g_examples[i].md);
    free((void *)g_examples[i].html);
  }
  g_example_count = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * HTML Renderer
 *
 * The GFM-to-HTML conversion is performed by the fio_md2html() helper in
 * fio-stl/103 md2html.h. The spec runner only needs to invoke it and compare
 * the resulting fio_bstr against the expected HTML.
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* ═══════════════════════════════════════════════════════════════════════════
 * Test Runner
 * ═══════════════════════════════════════════════════════════════════════════
 */

/** Trim trailing whitespace from HTML output for comparison. */
static size_t trim_trailing_ws(const char *s, size_t len) {
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                     s[len - 1] == '\n' || s[len - 1] == '\r'))
    --len;
  return len;
}

/** Normalize HTML for comparison: collapse whitespace between tags.
 *  Strips whitespace that appears between > and < (inter-tag whitespace).
 *  Returns a malloc'd normalized string; caller frees. */
static char *normalize_html(const char *s, size_t len, size_t *out_len) {
  char *out = (char *)malloc(len + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  size_t i = 0;
  while (i < len) {
    if (i + 35 <= len &&
        !memcmp(s + i, "<input disabled=\"\" type=\"checkbox\">", 35)) {
      memcpy(out + j, "<input type=\"checkbox\" disabled=\"\">", 35);
      i += 35;
      j += 35;
      continue;
    }
    if (i + 46 <= len &&
        !memcmp(s + i,
                "<input checked=\"\" disabled=\"\" type=\"checkbox\">",
                46)) {
      memcpy(out + j,
             "<input type=\"checkbox\" checked=\"\" disabled=\"\">",
             46);
      i += 46;
      j += 46;
      continue;
    }
    if (i + 37 <= len &&
        !memcmp(s + i, "<input type=\"checkbox\" disabled=\"\" />", 37)) {
      memcpy(out + j, "<input type=\"checkbox\" disabled=\"\">", 35);
      i += 37;
      j += 35;
      continue;
    }
    if (i + 48 <= len &&
        !memcmp(s + i,
                "<input type=\"checkbox\" checked=\"\" disabled=\"\" />",
                48)) {
      memcpy(out + j,
             "<input type=\"checkbox\" checked=\"\" disabled=\"\">",
             46);
      i += 48;
      j += 46;
      continue;
    }
    out[j++] = s[i];
    if (s[i] == '>') {
      ++i;
      /* Skip whitespace between > and < */
      while (i < len &&
             (s[i] == '\n' || s[i] == '\r' || s[i] == ' ' || s[i] == '\t')) {
        if (i + 1 < len && s[i + 1] == '<') {
          /* whitespace immediately before < — skip it */
          ++i;
          continue;
        }
        /* whitespace but next non-ws is not < — keep it */
        size_t k = i;
        while (k < len &&
               (s[k] == '\n' || s[k] == '\r' || s[k] == ' ' || s[k] == '\t'))
          ++k;
        if (k < len && s[k] == '<') {
          /* all whitespace between > and < — skip */
          i = k;
          break;
        }
        /* whitespace before non-tag content — keep */
        break;
      }
    } else {
      ++i;
    }
  }
  out[j] = '\0';
  *out_len = j;
  return out;
}

static int run_one_example(spec_example_s *ex) {
  char *got_bstr = fio_md2html(NULL, FIO_BUF_INFO2((char *)ex->md, ex->md_len));
  if (!got_bstr)
    return 0;

  const char *got = got_bstr;
  size_t got_len = fio_bstr_len(got_bstr);

  /* Compare: trim, normalize inter-tag whitespace, then compare */
  size_t got_trimmed = trim_trailing_ws(got, got_len);
  size_t exp_trimmed = trim_trailing_ws(ex->html, ex->html_len);

  size_t got_norm_len, exp_norm_len;
  char *got_norm = normalize_html(got, got_trimmed, &got_norm_len);
  char *exp_norm = normalize_html(ex->html, exp_trimmed, &exp_norm_len);

  int pass = got_norm && exp_norm &&
             ((exp_norm_len == 8 && memcmp(exp_norm, "<IGNORE>", 8) == 0) ||
              (got_norm_len == exp_norm_len &&
               memcmp(got_norm, exp_norm, got_norm_len) == 0));

  free(got_norm);
  free(exp_norm);
  fio_bstr_free(got_bstr);
  return pass;
}

static void print_escaped(const char *s, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (s[i] == '\t')
      fputs("→", stderr);
    else if (s[i] == '\n')
      fputc('\n', stderr);
    else
      fputc(s[i], stderr);
  }
}

static int spec_number_in(const int *list, size_t count, int number) {
  for (size_t i = 0; i < count; ++i) {
    if (list[i] == number)
      return 1;
  }
  return 0;
}

static int spec_is_expected_failure(const spec_example_s *ex) {
  static const int main_spec[] = {363, 398, 418, 423, 424, 426, 434,
                                  435, 436, 440, 473, 474, 475, 477,
                                  479, 517, 581, 584, 585, 619, 620};
  static const int extensions[] = {19, 23, 24, 25};

  if (!strcmp(ex->file, "gfm-spec.txt"))
    return spec_number_in(main_spec,
                          sizeof(main_spec) / sizeof(main_spec[0]),
                          ex->number);
  if (!strcmp(ex->file, "gfm-extensions.txt"))
    return spec_number_in(extensions,
                          sizeof(extensions) / sizeof(extensions[0]),
                          ex->number);
  return 0;
}

static void print_failure_detail(spec_example_s *ex) {
  char *got_bstr = fio_md2html(NULL, FIO_BUF_INFO2((char *)ex->md, ex->md_len));
  if (!got_bstr)
    return;

  const char *got = got_bstr;
  size_t got_len = fio_bstr_len(got_bstr);

  size_t got_trimmed = trim_trailing_ws(got, got_len);
  size_t exp_trimmed = trim_trailing_ws(ex->html, ex->html_len);
  size_t got_norm_len, exp_norm_len;
  char *got_norm = normalize_html(got, got_trimmed, &got_norm_len);
  char *exp_norm = normalize_html(ex->html, exp_trimmed, &exp_norm_len);

  fprintf(stderr,
          "\n--- %s: %s example %d (line %d) ---\n",
          spec_is_expected_failure(ex) ? "EXPECTED FAIL" : "FAIL",
          ex->file,
          ex->number,
          ex->line);
  fprintf(stderr, "  markdown:\n    ");
  print_escaped(ex->md, ex->md_len);
  fprintf(stderr, "\n  expected (normalized):\n    ");
  if (exp_norm)
    print_escaped(exp_norm, exp_norm_len);
  fprintf(stderr, "\n  got (normalized):\n    ");
  if (got_norm)
    print_escaped(got_norm, got_norm_len);
  free(got_norm);
  free(exp_norm);
  fprintf(stderr, "\n");

  fio_bstr_free(got_bstr);
}

int main(int argc, char **argv) {
  /* Optional: filter by example range. Usage: gfm-spec-runner [min] [max] */
  int filter_min = 0, filter_max = 999999;
  if (argc >= 2)
    filter_min = atoi(argv[1]);
  if (argc >= 3)
    filter_max = atoi(argv[2]);

  fprintf(stderr, "=== GFM Spec Conformance Test Runner ===\n\n");

  /* Parse spec files */
  if (spec_parse_file("tests/gfm-spec/gfm-spec.txt") != 0)
    return 1;
  int main_count = g_example_count;
  fprintf(stderr, "Parsed %d examples from gfm-spec.txt\n", main_count);

  if (spec_parse_file("tests/gfm-spec/gfm-extensions.txt") != 0)
    return 1;
  int ext_count = g_example_count - main_count;
  fprintf(stderr, "Parsed %d examples from gfm-extensions.txt\n", ext_count);
  fprintf(stderr, "Total: %d examples\n\n", g_example_count);

  /* Run all examples. Known gaps are an explicit expected-failure list so
   * fixture coverage gates unexpected regressions without claiming complete
   * GFM conformance yet. */
  int pass = 0, fail = 0;
  int expected_fail = 0, unexpected_fail = 0, expected_pass = 0;
  int failed_indices[SPEC_MAX_EXAMPLES];
  int failed_count = 0;
  int expected_pass_indices[SPEC_MAX_EXAMPLES];
  int expected_pass_count = 0;

  for (int i = 0; i < g_example_count; ++i) {
    int num = g_examples[i].number;
    if (num < filter_min || num > filter_max)
      continue;
    int expected = spec_is_expected_failure(&g_examples[i]);
    int ok = run_one_example(&g_examples[i]);
    if (ok == 1) {
      ++pass;
      if (expected) {
        ++expected_pass;
        if (expected_pass_count < SPEC_MAX_EXAMPLES)
          expected_pass_indices[expected_pass_count++] = i;
      }
    } else {
      ++fail;
      if (expected)
        ++expected_fail;
      else
        ++unexpected_fail;
      if (failed_count < SPEC_MAX_EXAMPLES)
        failed_indices[failed_count++] = i;
    }
  }

  /* Print first N detailed failures */
  int detail_count =
      failed_count < SPEC_DETAIL_LIMIT ? failed_count : SPEC_DETAIL_LIMIT;
  for (int i = 0; i < detail_count; ++i)
    print_failure_detail(&g_examples[failed_indices[i]]);

  /* Summary */
  fprintf(stderr, "\n=== Results ===\n");
  fprintf(stderr,
          "  Passed: %d / %d (%.1f%%)\n",
          pass,
          g_example_count,
          100.0 * pass / g_example_count);
  fprintf(stderr, "  Failed: %d / %d\n", fail, g_example_count);
  fprintf(stderr, "  Expected failures: %d\n", expected_fail);
  fprintf(stderr, "  Unexpected failures: %d\n", unexpected_fail);
  fprintf(stderr, "  Expected failures now passing: %d\n", expected_pass);

  if (failed_count > detail_count)
    fprintf(stderr,
            "  (showing first %d failures of %d)\n",
            detail_count,
            failed_count);

  /* Section breakdown: main spec vs extensions (use first-run results) */
  {
    int main_pass = 0, ext_pass = 0;
    /* Failures for main spec are indices 0..main_count-1 */
    for (int i = 0; i < failed_count; ++i) {
      if (failed_indices[i] < main_count)
        continue; /* counted as main fail */
      else
        break;
    }
    /* Count: total - failures in that range */
    int main_fail = 0, ext_fail = 0;
    for (int i = 0; i < failed_count; ++i) {
      if (failed_indices[i] < main_count)
        ++main_fail;
      else
        ++ext_fail;
    }
    main_pass = main_count - main_fail;
    ext_pass = ext_count - ext_fail;
    fprintf(stderr,
            "\n  Main spec:   %d / %d (%.1f%%)\n",
            main_pass,
            main_count,
            100.0 * main_pass / main_count);
    fprintf(stderr,
            "  Extensions:  %d / %d (%.1f%%)\n",
            ext_pass,
            ext_count,
            ext_count ? 100.0 * ext_pass / ext_count : 0.0);
  }

  /* Compact failure list */
  fprintf(stderr, "\n  Failed examples: ");
  for (int i = 0; i < failed_count; ++i) {
    if (i > 0)
      fprintf(stderr, ", ");
    fprintf(stderr, "%d", g_examples[failed_indices[i]].number);
  }
  fprintf(stderr, "\n");

  if (expected_pass_count) {
    fprintf(stderr,
            "  Update expected-failure allowlist for now-passing examples: ");
    for (int i = 0; i < expected_pass_count; ++i) {
      spec_example_s *ex = &g_examples[expected_pass_indices[i]];
      if (i > 0)
        fprintf(stderr, ", ");
      fprintf(stderr, "%s:%d", ex->file, ex->number);
    }
    fprintf(stderr, "\n");
  }

  spec_free_examples();

  return (unexpected_fail || expected_pass) ? 1 : 0;
}
