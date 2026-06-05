/* GFM parser smoke test — vertical slice validation.
 * Build: make tests/gfm/db
 */
#define FIO_LOG
#define FIO_GFM
#include "test-helpers.h"

/* ── test infrastructure ── */

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__);                   \
      fprintf(stderr, __VA_ARGS__);                                            \
      fprintf(stderr, "\n");                                                   \
      ++g_fail;                                                                \
    } else {                                                                   \
      ++g_pass;                                                                \
    }                                                                          \
  } while (0)

/* ── event recorder ── */

typedef struct {
  uint8_t kind;  /* 0=push, 1=write, 2=pop */
  uint8_t type;  /* fio_gfm_type_e */
  uint8_t flags; /* FIO_GFM_F_* */
  uint8_t align; /* fio_gfm_align_e for table cells */
  size_t text_len;
  char text_buf[256];
} test_event_s;

#define MAX_EVENTS 64
static test_event_s g_events[MAX_EVENTS];
static int g_event_count;

static void reset_events(void) { g_event_count = 0; }

static void record(int kind, fio_gfm_event_s *e) {
  if (g_event_count >= MAX_EVENTS)
    return;
  test_event_s *ev = &g_events[g_event_count++];
  ev->kind = (uint8_t)kind;
  ev->type = e->type;
  ev->flags = e->flags;
  ev->align = e->align;
  ev->text_len = e->text.len;
  size_t n = e->text.len < 255 ? e->text.len : 255;
  if (n && e->text.buf)
    memcpy(ev->text_buf, e->text.buf, n);
  ev->text_buf[n] = 0;
}

static int test_push(fio_gfm_event_s *e) {
  record(0, e);
  return 0;
}
static int test_write(fio_gfm_event_s *e) {
  record(1, e);
  return 0;
}
static int test_pop(fio_gfm_event_s *e) {
  record(2, e);
  return 0;
}

static fio_gfm_callbacks_s test_cb = {
    .push = test_push,
    .write = test_write,
    .pop = test_pop,
};

/* ── tests ── */

static void test_empty_input(void) {
  fprintf(stderr, "test_empty_input\n");
  reset_events();
  size_t r = fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2(NULL, 0));
  TEST_ASSERT(r == 0, "empty input returns 0, got %zu", r);
  TEST_ASSERT(g_event_count == 0,
              "empty input emits no events, got %d",
              g_event_count);
}

static void test_single_paragraph(void) {
  fprintf(stderr, "test_single_paragraph\n");
  reset_events();
  const char *src = "Hello world\n";
  size_t r =
      fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(r == strlen(src), "consumed all, got %zu", r);
  TEST_ASSERT(g_event_count == 3,
              "paragraph = push+write+pop = 3 events, got %d",
              g_event_count);
  if (g_event_count >= 3) {
    TEST_ASSERT(g_events[0].kind == 0 && g_events[0].type == FIO_GFM_PARAGRAPH,
                "event 0: push(PARAGRAPH)");
    TEST_ASSERT(g_events[1].kind == 1 && g_events[1].type == FIO_GFM_TEXT,
                "event 1: write(TEXT)");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "Hello world") == 0,
                "text content = 'Hello world', got '%s'",
                g_events[1].text_buf);
    TEST_ASSERT(g_events[2].kind == 2 && g_events[2].type == FIO_GFM_PARAGRAPH,
                "event 2: pop(PARAGRAPH)");
  }
}

static void test_two_paragraphs(void) {
  fprintf(stderr, "test_two_paragraphs\n");
  reset_events();
  const char *src = "First\n\nSecond\n";
  size_t r =
      fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(r == strlen(src), "consumed all, got %zu", r);
  TEST_ASSERT(g_event_count == 6,
              "two paragraphs = 6 events, got %d",
              g_event_count);
  if (g_event_count >= 6) {
    TEST_ASSERT(g_events[0].kind == 0 && g_events[0].type == FIO_GFM_PARAGRAPH,
                "event 0: push(PARAGRAPH)");
    TEST_ASSERT(g_events[1].kind == 1, "event 1: write");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "First") == 0,
                "first para text = 'First', got '%s'",
                g_events[1].text_buf);
    TEST_ASSERT(g_events[2].kind == 2 && g_events[2].type == FIO_GFM_PARAGRAPH,
                "event 2: pop(PARAGRAPH)");
    TEST_ASSERT(g_events[3].kind == 0 && g_events[3].type == FIO_GFM_PARAGRAPH,
                "event 3: push(PARAGRAPH)");
    TEST_ASSERT(strcmp(g_events[4].text_buf, "Second") == 0,
                "second para text = 'Second', got '%s'",
                g_events[4].text_buf);
    TEST_ASSERT(g_events[5].kind == 2 && g_events[5].type == FIO_GFM_PARAGRAPH,
                "event 5: pop(PARAGRAPH)");
  }
}

static void test_multiline_paragraph(void) {
  fprintf(stderr, "test_multiline_paragraph\n");
  reset_events();
  const char *src = "Line one\nLine two\n";
  size_t r =
      fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(r == strlen(src), "consumed all, got %zu", r);
  /* push(PARA), write(TEXT,"Line one"), write(SOFT_BREAK),
   * write(TEXT,"Line two"), pop(PARA) = 5 */
  TEST_ASSERT(g_event_count == 5,
              "multiline para = push+3 writes+pop = 5, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(g_events[1].type == FIO_GFM_TEXT,
                "event 1 = TEXT, got type %d", g_events[1].type);
  }
}

static void test_no_trailing_newline(void) {
  fprintf(stderr, "test_no_trailing_newline\n");
  reset_events();
  const char *src = "No newline";
  size_t r =
      fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(r == strlen(src), "consumed all, got %zu", r);
  TEST_ASSERT(g_event_count == 3,
              "paragraph events = 3, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(strcmp(g_events[1].text_buf, "No newline") == 0,
                "text = 'No newline', got '%s'",
                g_events[1].text_buf);
  }
}

/* ── slice 3: headings + thematic break ── */

static void test_atx_heading(void) {
  fprintf(stderr, "test_atx_heading\n");
  reset_events();
  const char *src = "# Hello\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count == 3,
              "atx h1 = push+write+pop = 3, got %d",
              g_event_count);
  if (g_event_count >= 3) {
    TEST_ASSERT(g_events[0].kind == 0 && g_events[0].type == FIO_GFM_HEADING,
                "push(HEADING)");
    TEST_ASSERT(g_events[1].kind == 1 && g_events[1].type == FIO_GFM_TEXT,
                "write(TEXT)");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "Hello") == 0,
                "text = 'Hello', got '%s'",
                g_events[1].text_buf);
    TEST_ASSERT(g_events[2].kind == 2 && g_events[2].type == FIO_GFM_HEADING,
                "pop(HEADING)");
  }
}

static void test_atx_heading_levels(void) {
  fprintf(stderr, "test_atx_heading_levels\n");
  for (int level = 1; level <= 6; ++level) {
    reset_events();
    char src[32];
    int n = 0;
    for (int i = 0; i < level; ++i)
      src[n++] = '#';
    src[n++] = ' ';
    src[n++] = 'x';
    src[n++] = '\n';
    src[n] = 0;
    fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2(src, (size_t)n));
    TEST_ASSERT(g_event_count == 3,
                "h%d = 3 events, got %d",
                level,
                g_event_count);
  }
  /* Level 7 should be a paragraph, not heading */
  reset_events();
  const char *src7 = "####### foo\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src7, strlen(src7)));
  TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH,
              "7 # = paragraph, got type %d",
              g_events[0].type);
}

static void test_atx_no_space(void) {
  fprintf(stderr, "test_atx_no_space\n");
  reset_events();
  const char *src = "#foo\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH,
              "#foo = paragraph, got type %d",
              g_events[0].type);
}

static void test_atx_trailing_hashes(void) {
  fprintf(stderr, "test_atx_trailing_hashes\n");
  reset_events();
  const char *src = "## Hello ##\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count >= 2, "events >= 2, got %d", g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(strcmp(g_events[1].text_buf, "Hello") == 0,
                "trailing ## stripped, got '%s'",
                g_events[1].text_buf);
  }
}

static void test_thematic_break(void) {
  fprintf(stderr, "test_thematic_break\n");
  const char *variants[] = {"---\n", "***\n", "___\n", "- - -\n", "  ***\n"};
  for (int i = 0; i < 5; ++i) {
    reset_events();
    fio_gfm_parse(&test_cb,
                  NULL,
                  FIO_BUF_INFO2((char *)variants[i], strlen(variants[i])));
    TEST_ASSERT(g_event_count == 2,
                "'%s': push+pop = 2, got %d",
                variants[i],
                g_event_count);
    if (g_event_count >= 1) {
      TEST_ASSERT(g_events[0].type == FIO_GFM_THEMATIC_BREAK,
                  "'%s': type = THEMATIC_BREAK, got %d",
                  variants[i],
                  g_events[0].type);
    }
  }
}

static void test_thematic_break_not(void) {
  fprintf(stderr, "test_thematic_break_not\n");
  reset_events();
  const char *src = "--\n"; /* too few */
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH,
              "'--' = paragraph, got type %d",
              g_events[0].type);
}

static void test_setext_heading(void) {
  fprintf(stderr, "test_setext_heading\n");
  /* Level 1 with '=' */
  reset_events();
  const char *src1 = "Heading\n===\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src1, strlen(src1)));
  TEST_ASSERT(g_event_count == 3,
              "setext h1 = 3 events, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_HEADING, "push(HEADING)");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "Heading") == 0,
                "text = 'Heading', got '%s'",
                g_events[1].text_buf);
  }

  /* Level 2 with '-' */
  reset_events();
  const char *src2 = "Heading\n---\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src2, strlen(src2)));
  TEST_ASSERT(g_event_count == 3,
              "setext h2 = 3 events, got %d",
              g_event_count);
  if (g_event_count >= 1) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_HEADING,
                "setext h2: type = HEADING, got %d",
                g_events[0].type);
  }
}

static void test_setext_priority_over_thematic(void) {
  fprintf(stderr, "test_setext_priority_over_thematic\n");
  /* "---" after a paragraph is setext h2, not thematic break */
  reset_events();
  const char *src = "Foo\n---\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count >= 1, "events >= 1");
  TEST_ASSERT(g_events[0].type == FIO_GFM_HEADING,
              "Foo/--- = HEADING (not THEMATIC_BREAK), got %d",
              g_events[0].type);
}

static void test_heading_interrupts_paragraph(void) {
  fprintf(stderr, "test_heading_interrupts_paragraph\n");
  reset_events();
  const char *src = "Hello\n# World\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Should be: push(PARA), write(TEXT,"Hello"), pop(PARA),
   *            push(HEADING), write(TEXT,"World"), pop(HEADING) */
  TEST_ASSERT(g_event_count == 6,
              "para + heading = 6 events, got %d",
              g_event_count);
  if (g_event_count >= 4) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH, "first = PARAGRAPH");
    TEST_ASSERT(g_events[3].type == FIO_GFM_HEADING, "second = HEADING");
  }
}

/* ── slice 4: blockquotes ── */

static void test_simple_blockquote(void) {
  fprintf(stderr, "test_simple_blockquote\n");
  reset_events();
  const char *src = "> Hello\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(BQ), push(PARA), write(TEXT), pop(PARA), pop(BQ) = 5 */
  TEST_ASSERT(g_event_count == 5,
              "blockquote para = 5 events, got %d",
              g_event_count);
  if (g_event_count >= 5) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_BLOCKQUOTE, "push(BQ)");
    TEST_ASSERT(g_events[1].type == FIO_GFM_PARAGRAPH, "push(PARA)");
    TEST_ASSERT(strcmp(g_events[2].text_buf, "Hello") == 0,
                "text = 'Hello', got '%s'",
                g_events[2].text_buf);
    TEST_ASSERT(g_events[3].type == FIO_GFM_PARAGRAPH, "pop(PARA)");
    TEST_ASSERT(g_events[4].type == FIO_GFM_BLOCKQUOTE, "pop(BQ)");
  }
}

static void test_multiline_blockquote(void) {
  fprintf(stderr, "test_multiline_blockquote\n");
  reset_events();
  const char *src = "> Line1\n> Line2\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(BQ), push(PARA), write(TEXT,"Line1"), write(SOFT_BREAK),
   * write(TEXT,"Line2"), pop(PARA), pop(BQ) = 7 */
  TEST_ASSERT(g_event_count == 7,
              "multiline bq = 7 events, got %d",
              g_event_count);
}

static void test_lazy_continuation(void) {
  fprintf(stderr, "test_lazy_continuation\n");
  reset_events();
  const char *src = "> foo\nbar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Lazy: "bar" continues the paragraph inside blockquote.
   * push(BQ), push(PARA), write(TEXT,"foo"), write(SOFT_BREAK),
   * write(TEXT,"bar"), pop(PARA), pop(BQ) = 7 */
  TEST_ASSERT(g_event_count == 7,
              "lazy continuation = 7 events, got %d",
              g_event_count);
  if (g_event_count >= 7) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_BLOCKQUOTE, "push(BQ)");
    TEST_ASSERT(g_events[2].type == FIO_GFM_TEXT, "write(TEXT,'foo')");
    TEST_ASSERT(g_events[6].type == FIO_GFM_BLOCKQUOTE, "pop(BQ)");
  }
}

static void test_blockquote_interrupts_paragraph(void) {
  fprintf(stderr, "test_blockquote_interrupts_paragraph\n");
  reset_events();
  const char *src = "Hello\n> World\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(PARA,"Hello"), pop(PARA), push(BQ), push(PARA,"World"), pop(PARA),
   * pop(BQ) = 8 */
  TEST_ASSERT(g_event_count == 8,
              "para + bq = 8 events, got %d",
              g_event_count);
  if (g_event_count >= 4) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH, "first = PARA");
    TEST_ASSERT(g_events[3].type == FIO_GFM_BLOCKQUOTE, "then = BQ");
  }
}

/* ── slice 5: lists ── */

static void test_bullet_list(void) {
  fprintf(stderr, "test_bullet_list\n");
  reset_events();
  const char *src = "- foo\n- bar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(UL), push(LI), push(PARA), write(TEXT,"foo"), pop(PARA), pop(LI),
   * push(LI), push(PARA), write(TEXT,"bar"), pop(PARA), pop(LI), pop(UL) = 12
   */
  TEST_ASSERT(g_event_count == 12,
              "two-item bullet list = 12 events, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_LIST_UNORDERED, "push(UL)");
    TEST_ASSERT(g_events[1].type == FIO_GFM_LIST_ITEM, "push(LI)");
  }
  if (g_event_count >= 12) {
    TEST_ASSERT(g_events[11].type == FIO_GFM_LIST_UNORDERED, "pop(UL)");
  }
}

static void test_ordered_list(void) {
  fprintf(stderr, "test_ordered_list\n");
  reset_events();
  const char *src = "1. foo\n2. bar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count == 12,
              "two-item ordered list = 12 events, got %d",
              g_event_count);
  if (g_event_count >= 1) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_LIST_ORDERED, "push(OL)");
    TEST_ASSERT(g_events[0].kind == 0, "push event");
  }
}

static void test_list_tight(void) {
  fprintf(stderr, "test_list_tight\n");
  reset_events();
  const char *src = "- foo\n- bar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  if (g_event_count >= 1) {
    TEST_ASSERT(g_events[0].flags & FIO_GFM_F_TIGHT,
                "no blank between items → tight");
  }
}

static void test_list_loose(void) {
  fprintf(stderr, "test_list_loose\n");
  reset_events();
  const char *src = "- foo\n\n- bar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  if (g_event_count >= 1) {
    TEST_ASSERT(!(g_events[0].flags & FIO_GFM_F_TIGHT),
                "blank between items → loose");
  }
}

static void test_different_markers_separate_lists(void) {
  fprintf(stderr, "test_different_markers_separate_lists\n");
  reset_events();
  const char *src = "- foo\n+ bar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Two separate lists:
   * push(UL), push(LI), push(PARA), write, pop(PARA),  [5]
   * pop(LI), pop(UL),                                   [7]
   * push(UL), push(LI), push(PARA), write, pop(PARA),   [12]
   * pop(LI), pop(UL)                                    [14] */
  TEST_ASSERT(g_event_count == 14,
              "different markers = 2 lists = 14 events, got %d",
              g_event_count);
  if (g_event_count >= 8) {
    TEST_ASSERT(g_events[6].kind == 2 &&
                    g_events[6].type == FIO_GFM_LIST_UNORDERED,
                "first list closed (pop UL)");
    TEST_ASSERT(g_events[7].kind == 0 &&
                    g_events[7].type == FIO_GFM_LIST_UNORDERED,
                "second list opened (push UL)");
  }
}

static void test_list_interrupts_paragraph(void) {
  fprintf(stderr, "test_list_interrupts_paragraph\n");
  reset_events();
  const char *src = "Hello\n- item\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(PARA), write, pop(PARA), push(UL), push(LI), push(PARA), write,
   * pop(PARA), pop(LI), pop(UL) = 10 */
  TEST_ASSERT(g_event_count == 10,
              "para + list = 10 events, got %d",
              g_event_count);
  if (g_event_count >= 4) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH, "first = PARA");
    TEST_ASSERT(g_events[3].type == FIO_GFM_LIST_UNORDERED, "then = UL");
  }
}

/* ── slice 6: code blocks ── */

static void test_fenced_code_backtick(void) {
  fprintf(stderr, "test_fenced_code_backtick\n");
  reset_events();
  const char *src = "```\ncode\n```\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(CODE_BLOCK), write(TEXT,"code"), pop(CODE_BLOCK) = 3 */
  TEST_ASSERT(g_event_count == 3,
              "fenced code = 3 events, got %d",
              g_event_count);
  if (g_event_count >= 3) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_CODE_BLOCK, "push(CODE_BLOCK)");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "code") == 0,
                "text = 'code', got '%s'",
                g_events[1].text_buf);
    TEST_ASSERT(g_events[2].type == FIO_GFM_CODE_BLOCK, "pop(CODE_BLOCK)");
  }
}

static void test_fenced_code_tilde(void) {
  fprintf(stderr, "test_fenced_code_tilde\n");
  reset_events();
  const char *src = "~~~\ncode\n~~~\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count == 3,
              "tilde fence = 3 events, got %d",
              g_event_count);
}

static void test_fenced_code_info_string(void) {
  fprintf(stderr, "test_fenced_code_info_string\n");
  reset_events();
  const char *src = "```javascript\nvar x = 1;\n```\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count == 3,
              "fenced code with info = 3 events, got %d",
              g_event_count);
  /* TODO: verify info string is "javascript" once event recording supports it
   */
}

static void test_fenced_code_multiline(void) {
  fprintf(stderr, "test_fenced_code_multiline\n");
  reset_events();
  const char *src = "```\nline1\nline2\n```\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(CODE_BLOCK), write(TEXT,"line1"), write(TEXT,"line2"), pop(CODE_BLOCK)
   * = 4 */
  TEST_ASSERT(g_event_count == 4,
              "multiline code = 4 events, got %d",
              g_event_count);
}

static void test_fenced_code_unclosed(void) {
  fprintf(stderr, "test_fenced_code_unclosed\n");
  reset_events();
  const char *src = "```\ncode\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Unclosed fence: push(CODE_BLOCK), write(TEXT,"code"), pop(CODE_BLOCK) = 3
   */
  TEST_ASSERT(g_event_count == 3,
              "unclosed fence = 3 events, got %d",
              g_event_count);
}

static void test_indented_code(void) {
  fprintf(stderr, "test_indented_code\n");
  reset_events();
  const char *src = "    code\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(CODE_BLOCK), write(TEXT,"code"), pop(CODE_BLOCK) = 3 */
  TEST_ASSERT(g_event_count == 3,
              "indented code = 3 events, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_CODE_BLOCK, "push(CODE_BLOCK)");
    TEST_ASSERT(strcmp(g_events[1].text_buf, "code") == 0,
                "4 spaces stripped, got '%s'",
                g_events[1].text_buf);
  }
}

/* ── slice 7: HTML blocks ── */

static void test_html_block_type1(void) {
  fprintf(stderr, "test_html_block_type1\n");
  reset_events();
  const char *src = "<script>\nvar x=1;\n</script>\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(HTML), write(line1), write(line2), write(line3), pop(HTML) = 5 */
  TEST_ASSERT(g_event_count >= 3,
              "HTML type 1: >= 3 events, got %d",
              g_event_count);
  TEST_ASSERT(g_events[0].type == FIO_GFM_HTML_BLOCK, "push(HTML_BLOCK)");
}

static void test_html_block_type2(void) {
  fprintf(stderr, "test_html_block_type2\n");
  reset_events();
  const char *src = "<!-- comment -->\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count >= 2,
              "HTML type 2: >= 2 events, got %d",
              g_event_count);
  TEST_ASSERT(g_events[0].type == FIO_GFM_HTML_BLOCK, "push(HTML_BLOCK)");
}

static void test_html_block_type6(void) {
  fprintf(stderr, "test_html_block_type6\n");
  reset_events();
  const char *src = "<div>\ncontent\n\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Type 6 closes on blank line */
  TEST_ASSERT(g_event_count >= 2,
              "HTML type 6: >= 2 events, got %d",
              g_event_count);
  TEST_ASSERT(g_events[0].type == FIO_GFM_HTML_BLOCK, "push(HTML_BLOCK)");
}

static void test_html_block_interrupts_paragraph(void) {
  fprintf(stderr, "test_html_block_interrupts_paragraph\n");
  reset_events();
  const char *src = "Hello\n<div>\n\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Type 6 can interrupt a paragraph */
  TEST_ASSERT(g_event_count >= 4,
              "para + html: >= 4 events, got %d",
              g_event_count);
  if (g_event_count >= 4) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH, "first = PARA");
  }
}

/* ── slice 8: tables ── */

static void test_simple_table(void) {
  fprintf(stderr, "test_simple_table\n");
  reset_events();
  const char *src = "| a | b |\n| - | - |\n| c | d |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(TABLE),
   *   push(TABLE_ROW), push(CELL), write(TEXT,"a"), pop(CELL),
   *                     push(CELL), write(TEXT,"b"), pop(CELL), pop(ROW),
   *   push(TABLE_ROW), push(CELL), write(TEXT,"c"), pop(CELL),
   *                     push(CELL), write(TEXT,"d"), pop(CELL), pop(ROW),
   * pop(TABLE)
   * = 1 + 2*(1 + 2*3 + 1) + 1 = 1 + 2*8 + 1 = 18 */
  TEST_ASSERT(g_event_count == 18,
              "simple 2x2 table = 18 events, got %d",
              g_event_count);
  if (g_event_count >= 2) {
    TEST_ASSERT(g_events[0].kind == 0 && g_events[0].type == FIO_GFM_TABLE,
                "event 0: push(TABLE)");
    TEST_ASSERT(g_events[1].kind == 0 && g_events[1].type == FIO_GFM_TABLE_ROW,
                "event 1: push(TABLE_ROW)");
  }
  if (g_event_count >= 18) {
    TEST_ASSERT(g_events[17].kind == 2 && g_events[17].type == FIO_GFM_TABLE,
                "last event: pop(TABLE)");
  }
}

static void test_table_alignment(void) {
  fprintf(stderr, "test_table_alignment\n");
  reset_events();
  const char *src = "| a | b | c | d |\n| :-- | --: | :-: | --- |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Header-only table (no body rows). 4 columns.
   * push(TABLE), push(ROW), 4*(push(CELL)+write+pop(CELL)), pop(ROW),
   * pop(TABLE) = 1 + 1 + 4*3 + 1 + 1 = 16 */
  TEST_ASSERT(g_event_count == 16,
              "header-only 4-col table = 16 events, got %d",
              g_event_count);
  if (g_event_count >= 16) {
    /* Check alignment of cells: left, right, center, none */
    /* Cell events are at indices 2, 5, 8, 11 (push(TABLE_CELL)) */
    TEST_ASSERT(g_events[2].align == FIO_GFM_ALIGN_LEFT,
                "col 0 align = LEFT, got %d", g_events[2].align);
    TEST_ASSERT(g_events[5].align == FIO_GFM_ALIGN_RIGHT,
                "col 1 align = RIGHT, got %d", g_events[5].align);
    TEST_ASSERT(g_events[8].align == FIO_GFM_ALIGN_CENTER,
                "col 2 align = CENTER, got %d", g_events[8].align);
    TEST_ASSERT(g_events[11].align == FIO_GFM_ALIGN_NONE,
                "col 3 align = NONE, got %d", g_events[11].align);
  }
}

static void test_table_no_leading_trailing_pipe(void) {
  fprintf(stderr, "test_table_no_leading_trailing_pipe\n");
  reset_events();
  const char *src = "a | b\n- | -\nc | d\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count == 18,
              "no-pipe 2x2 table = 18 events, got %d",
              g_event_count);
  if (g_event_count >= 1) {
    TEST_ASSERT(g_events[0].type == FIO_GFM_TABLE,
                "push(TABLE), got type %d", g_events[0].type);
  }
}

static void test_table_column_mismatch(void) {
  fprintf(stderr, "test_table_column_mismatch\n");
  reset_events();
  /* Header has 2 columns, delimiter has 1 → NOT a table */
  const char *src = "| a | b |\n| --- |\n| c |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Should be a single paragraph containing all three lines */
  TEST_ASSERT(g_event_count >= 1, "at least 1 event");
  TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH,
              "column mismatch = PARAGRAPH, got %d", g_events[0].type);
}

static void test_table_multiline_para_not_table(void) {
  fprintf(stderr, "test_table_multiline_para_not_table\n");
  reset_events();
  /* Multi-line paragraph before delimiter → NOT a table */
  const char *src = "line1\nline2\n| --- |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  TEST_ASSERT(g_event_count >= 1, "at least 1 event");
  TEST_ASSERT(g_events[0].type == FIO_GFM_PARAGRAPH,
              "multi-line para + delimiter = PARAGRAPH, got %d",
              g_events[0].type);
}

static void test_table_body_row_missing_cells(void) {
  fprintf(stderr, "test_table_body_row_missing_cells\n");
  reset_events();
  const char *src = "| a | b |\n| - | - |\n| c |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Body row has 1 cell → 1 real + 1 empty cell (no write for empty).
   * push(TABLE), header row (8), body row (7), pop(TABLE) = 17 */
  TEST_ASSERT(g_event_count == 17,
              "missing cell padded = 17 events, got %d",
              g_event_count);
}

static void test_table_body_row_excess_cells(void) {
  fprintf(stderr, "test_table_body_row_excess_cells\n");
  reset_events();
  const char *src = "| a | b |\n| - | - |\n| c | d | e |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Excess cell "e" ignored — only 2 cells emitted for body row.
   * Same count as simple 2x2: 18 events */
  TEST_ASSERT(g_event_count == 18,
              "excess cell ignored = 18 events, got %d",
              g_event_count);
}

static void test_table_closes_on_blank(void) {
  fprintf(stderr, "test_table_closes_on_blank\n");
  reset_events();
  const char *src = "| a |\n| - |\n| b |\n\nParagraph\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Table (1 col, 1 body row) then paragraph.
   * push(TABLE), ROW(3), ROW(3), pop(TABLE), push(PARA), write, pop(PARA)
   * = 1 + 4 + 4 + 1 + 3 = 13 */
  TEST_ASSERT(g_event_count >= 4, "at least 4 events, got %d", g_event_count);
  /* Find the paragraph after the table */
  int found_para = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_PARAGRAPH && g_events[i].kind == 0) {
      found_para = 1;
      break;
    }
  }
  TEST_ASSERT(found_para, "paragraph found after table");
}

static void test_table_breaks_on_blockquote(void) {
  fprintf(stderr, "test_table_breaks_on_blockquote\n");
  reset_events();
  const char *src = "| a |\n| - |\n| b |\n> quote\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* Table should close, then blockquote should open */
  int found_bq = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_BLOCKQUOTE && g_events[i].kind == 0) {
      found_bq = 1;
      break;
    }
  }
  TEST_ASSERT(found_bq, "blockquote found after table break");
}

static void test_table_header_only(void) {
  fprintf(stderr, "test_table_header_only\n");
  reset_events();
  const char *src = "| a | b |\n| - | - |\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* push(TABLE), push(ROW), 2*(push(CELL)+write+pop(CELL)), pop(ROW),
   * pop(TABLE) = 1 + 1 + 2*3 + 1 + 1 = 10 */
  TEST_ASSERT(g_event_count == 10,
              "header-only table = 10 events, got %d",
              g_event_count);
}

/* ── slice 9: references ── */

static void test_ref_def_basic(void) {
  fprintf(stderr, "test_ref_def_basic\n");
  reset_events();
  const char *src = "[foo]: /url\n\n[foo]\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* The [foo] line should become a paragraph with a LINK inside */
  int found_link = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_LINK && g_events[i].kind == 0)
      found_link = 1;
  }
  TEST_ASSERT(found_link, "ref-style link resolved");
}

static void test_ref_def_case_insensitive(void) {
  fprintf(stderr, "test_ref_def_case_insensitive\n");
  reset_events();
  const char *src = "[FOO]: /url\n\n[foo]\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_link = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_LINK && g_events[i].kind == 0)
      found_link = 1;
  }
  TEST_ASSERT(found_link, "case-insensitive ref match");
}

/* ── slice 10: inline parser ── */

static void test_inline_emphasis(void) {
  fprintf(stderr, "test_inline_emphasis\n");
  reset_events();
  const char *src = "Hello *world*\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_em = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_EMPHASIS && g_events[i].kind == 0)
      found_em = 1;
  }
  TEST_ASSERT(found_em, "emphasis found");
}

static void test_inline_strong(void) {
  fprintf(stderr, "test_inline_strong\n");
  reset_events();
  const char *src = "Hello **world**\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_strong = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_STRONG && g_events[i].kind == 0)
      found_strong = 1;
  }
  TEST_ASSERT(found_strong, "strong found");
}

static void test_inline_code_span(void) {
  fprintf(stderr, "test_inline_code_span\n");
  reset_events();
  const char *src = "Use `code` here\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_code = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_CODE_SPAN && g_events[i].kind == 1) {
      found_code = 1;
      TEST_ASSERT(strcmp(g_events[i].text_buf, "code") == 0,
                  "code span text = 'code', got '%s'", g_events[i].text_buf);
    }
  }
  TEST_ASSERT(found_code, "code span found");
}

static void test_inline_code_span_double_backtick(void) {
  fprintf(stderr, "test_inline_code_span_double_backtick\n");
  reset_events();
  const char *src = "`` foo ` bar ``\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_code = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_CODE_SPAN && g_events[i].kind == 1) {
      found_code = 1;
      TEST_ASSERT(strcmp(g_events[i].text_buf, "foo ` bar") == 0,
                  "double backtick code = 'foo ` bar', got '%s'",
                  g_events[i].text_buf);
    }
  }
  TEST_ASSERT(found_code, "double backtick code span found");
}

static void test_inline_escape(void) {
  fprintf(stderr, "test_inline_escape\n");
  reset_events();
  const char *src = "\\*not emphasis\\*\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  /* No emphasis should be found — the * are escaped */
  int found_em = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_EMPHASIS) found_em = 1;
  }
  TEST_ASSERT(!found_em, "escaped stars = no emphasis");
}

static void test_inline_hard_break_spaces(void) {
  fprintf(stderr, "test_inline_hard_break_spaces\n");
  reset_events();
  const char *src = "foo  \nbar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_hard = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_HARD_BREAK) found_hard = 1;
  }
  TEST_ASSERT(found_hard, "2+ trailing spaces = hard break");
}

static void test_inline_hard_break_backslash(void) {
  fprintf(stderr, "test_inline_hard_break_backslash\n");
  reset_events();
  const char *src = "foo\\\nbar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_hard = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_HARD_BREAK) found_hard = 1;
  }
  TEST_ASSERT(found_hard, "backslash before newline = hard break");
}

static void test_inline_soft_break(void) {
  fprintf(stderr, "test_inline_soft_break\n");
  reset_events();
  const char *src = "foo\nbar\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_soft = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_SOFT_BREAK) found_soft = 1;
  }
  TEST_ASSERT(found_soft, "plain newline = soft break");
}

static void test_inline_link(void) {
  fprintf(stderr, "test_inline_link\n");
  reset_events();
  const char *src = "[link](/url)\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_link = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_LINK && g_events[i].kind == 0)
      found_link = 1;
  }
  TEST_ASSERT(found_link, "inline link found");
}

static void test_inline_image(void) {
  fprintf(stderr, "test_inline_image\n");
  reset_events();
  const char *src = "![alt](/img.png)\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_img = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_IMAGE && g_events[i].kind == 1)
      found_img = 1;
  }
  TEST_ASSERT(found_img, "image found");
}

static void test_inline_autolink(void) {
  fprintf(stderr, "test_inline_autolink\n");
  reset_events();
  /* Autolink must be inside paragraph text, not standalone (which would be
   * HTML block type 7). Precede with text so it's a paragraph. */
  const char *src = "See <http://example.com> here\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_AUTOLINK) found = 1;
  }
  TEST_ASSERT(found, "autolink found");
}

static void test_inline_strikethrough(void) {
  fprintf(stderr, "test_inline_strikethrough\n");
  reset_events();
  const char *src = "~~deleted~~\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_STRIKETHROUGH && g_events[i].kind == 0)
      found = 1;
  }
  TEST_ASSERT(found, "strikethrough found");
}

static void test_inline_nested_emphasis(void) {
  fprintf(stderr, "test_inline_nested_emphasis\n");
  reset_events();
  const char *src = "***bold and italic***\n";
  fio_gfm_parse(&test_cb, NULL, FIO_BUF_INFO2((char *)src, strlen(src)));
  int found_em = 0, found_strong = 0;
  for (int i = 0; i < g_event_count; ++i) {
    if (g_events[i].type == FIO_GFM_EMPHASIS && g_events[i].kind == 0) found_em = 1;
    if (g_events[i].type == FIO_GFM_STRONG && g_events[i].kind == 0) found_strong = 1;
  }
  TEST_ASSERT(found_em && found_strong, "*** = emphasis + strong");
}

/* ── main ── */

int main(void) {
  fprintf(stderr, "=== GFM Parser Tests ===\n");
  test_empty_input();
  test_single_paragraph();
  test_two_paragraphs();
  test_multiline_paragraph();
  test_no_trailing_newline();
  test_atx_heading();
  test_atx_heading_levels();
  test_atx_no_space();
  test_atx_trailing_hashes();
  test_thematic_break();
  test_thematic_break_not();
  test_setext_heading();
  test_setext_priority_over_thematic();
  test_heading_interrupts_paragraph();
  test_simple_blockquote();
  test_multiline_blockquote();
  test_lazy_continuation();
  test_blockquote_interrupts_paragraph();
  test_bullet_list();
  test_ordered_list();
  test_list_tight();
  test_list_loose();
  test_different_markers_separate_lists();
  test_list_interrupts_paragraph();
  test_fenced_code_backtick();
  test_fenced_code_tilde();
  test_fenced_code_info_string();
  test_fenced_code_multiline();
  test_fenced_code_unclosed();
  test_indented_code();
  test_html_block_type1();
  test_html_block_type2();
  test_html_block_type6();
  test_html_block_interrupts_paragraph();
  test_simple_table();
  test_table_alignment();
  test_table_no_leading_trailing_pipe();
  test_table_column_mismatch();
  test_table_multiline_para_not_table();
  test_table_body_row_missing_cells();
  test_table_body_row_excess_cells();
  test_table_closes_on_blank();
  test_table_breaks_on_blockquote();
  test_table_header_only();
  test_ref_def_basic();
  test_ref_def_case_insensitive();
  test_inline_emphasis();
  test_inline_strong();
  test_inline_code_span();
  test_inline_code_span_double_backtick();
  test_inline_escape();
  test_inline_hard_break_spaces();
  test_inline_hard_break_backslash();
  test_inline_soft_break();
  test_inline_link();
  test_inline_image();
  test_inline_autolink();
  test_inline_strikethrough();
  test_inline_nested_emphasis();
  fprintf(stderr, "\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
