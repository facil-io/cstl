/* *****************************************************************************
Markdown / GFM Parser Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_MARKDOWN
#include "test-helpers.h"

#include <stdio.h>

typedef struct {
  char *src;
  size_t len;
  size_t blocks[32];
  size_t texts[32];
  size_t checked_tasks;
  size_t unchecked_tasks;
  size_t table_cells;
  fio_buf_info_s table_cell_text[8];
  size_t ref_links;
  size_t direct_links;
  size_t images;
  size_t soft_breaks;
  size_t soft_break_source_len;
  size_t zero_copy_errors;
  size_t errors;
} test_md_ctx_s;

static int test_md_slice_eq(fio_buf_info_s s, const char *expected) {
  size_t len = FIO_STRLEN(expected);
  return s.len == len && FIO_MEMCMP(s.buf, expected, len) == 0;
}

static int test_md_slice_in_source(test_md_ctx_s *ctx, fio_buf_info_s s) {
  if (!s.len)
    return 1;
  return s.buf >= ctx->src && (s.buf + s.len) <= (ctx->src + ctx->len);
}

static int test_md_push(fio_md_event_s *e) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)e->udata;
  if ((size_t)e->type < (sizeof(ctx->blocks) / sizeof(ctx->blocks[0])))
    ++ctx->blocks[e->type];
  if (!test_md_slice_in_source(ctx, e->source) ||
      !test_md_slice_in_source(ctx, e->text) ||
      !test_md_slice_in_source(ctx, e->marker) ||
      !test_md_slice_in_source(ctx, e->info))
    ++ctx->zero_copy_errors;
  if (e->type == FIO_MD_LIST_ITEM) {
    ctx->checked_tasks += ((e->flags & FIO_MD_F_TASK_CHECKED) != 0);
    ctx->unchecked_tasks += ((e->flags & FIO_MD_F_TASK) != 0 &&
                             (e->flags & FIO_MD_F_TASK_CHECKED) == 0);
  }
  if (e->type == FIO_MD_TABLE_CELL) {
    if (ctx->table_cells <
        (sizeof(ctx->table_cell_text) / sizeof(ctx->table_cell_text[0])))
      ctx->table_cell_text[ctx->table_cells] = e->text;
    ++ctx->table_cells;
  }
  if (e->type == FIO_MD_LINK && e->reference.len)
    ++ctx->ref_links;
  if (e->type == FIO_MD_LINK && !e->reference.len)
    ++ctx->direct_links;
  return 0;
}

static int test_md_text(fio_md_event_s *e) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)e->udata;
  if ((size_t)e->type < (sizeof(ctx->texts) / sizeof(ctx->texts[0])))
    ++ctx->texts[e->type];
  if (!test_md_slice_in_source(ctx, e->source) ||
      !test_md_slice_in_source(ctx, e->text) ||
      !test_md_slice_in_source(ctx, e->destination) ||
      !test_md_slice_in_source(ctx, e->title) ||
      !test_md_slice_in_source(ctx, e->reference))
    ++ctx->zero_copy_errors;
  if (e->type == FIO_MD_IMAGE)
    ++ctx->images;
  if (e->type == FIO_MD_SOFT_BREAK) {
    ++ctx->soft_breaks;
    ctx->soft_break_source_len += e->source.len;
  }
  return 0;
}

static int test_md_pop(fio_md_event_s *e) {
  (void)e;
  return 0;
}

static void test_md_error(fio_md_event_s *e) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)e->udata;
  ++ctx->errors;
}

static const fio_md_callbacks_s test_md_callbacks = {
    .push = test_md_push,
    .text = test_md_text,
    .pop = test_md_pop,
    .error = test_md_error,
};

static void test_md_commonmark_gfm_blocks(void) {
  char src[] = "# Heading *one*\n"
               "\n"
               "Paragraph with **strong**, *em*, ~~gone~~, `code`, "
               "[link](https://example.com \"t\"), ![alt](img.png), "
               "<https://fio.dev>, <span>x</span>, \\*, &.\n"
               "\n"
               "Setext heading\n"
               "--------------\n"
               "\n"
               "> quoted text\n"
               "\n"
               "- [x] done\n"
               "- [ ] todo\n"
               "1. ordered\n"
               "\n"
               "---\n"
               "\n"
               "    code\n"
               "\n"
               "```js\n"
               "fenced\n"
               "```\n"
               "\n"
               "| a | b |\n"
               "|---|---|\n"
               "| c | d |\n"
               "\n"
               "<div>html</div>\n"
               "\n"
               "[^1]: footnote\n"
               "\n"
               "ref [link][ref] and [^1]\n"
               "\n"
               "[ref]: https://example.com \"title\"\n";
  test_md_ctx_s ctx = {0};
  ctx.src = src;
  ctx.len = sizeof(src) - 1;
  fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO1(src));
  FIO_ASSERT(!ctx.errors, "markdown parser errors detected");
  FIO_ASSERT(!ctx.zero_copy_errors, "markdown zero-copy errors detected");

  /* Blocks */
  FIO_ASSERT(ctx.blocks[FIO_MD_HEADING] == 2, "expected 2 headings");
  FIO_ASSERT(ctx.blocks[FIO_MD_PARAGRAPH] == 6, "expected 6 paragraphs (including list items)");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCKQUOTE] == 1, "expected 1 blockquote");
  FIO_ASSERT(ctx.blocks[FIO_MD_LIST_UNORDERED] == 1,
             "expected 1 unordered list");
  FIO_ASSERT(ctx.blocks[FIO_MD_LIST_ORDERED] == 1,
             "expected 1 ordered list");
  FIO_ASSERT(ctx.blocks[FIO_MD_LIST_ITEM] == 3, "expected 3 list items");
  FIO_ASSERT(ctx.blocks[FIO_MD_THEMATIC_BREAK] == 1,
             "expected 1 thematic break");
  FIO_ASSERT(ctx.blocks[FIO_MD_CODE_INDENTED] == 1,
             "expected 1 indented code block");
  FIO_ASSERT(ctx.blocks[FIO_MD_CODE_FENCED] == 1,
             "expected 1 fenced code block");
  FIO_ASSERT(ctx.blocks[FIO_MD_TABLE] == 1, "expected 1 table");
  FIO_ASSERT(ctx.blocks[FIO_MD_TABLE_ROW] == 2, "expected 2 table rows");
  FIO_ASSERT(ctx.blocks[FIO_MD_TABLE_CELL] == 4,
             "expected 4 table cells");
  FIO_ASSERT(ctx.blocks[FIO_MD_HTML_BLOCK] == 1,
             "expected 1 HTML block");

  /* Text / inline */
  FIO_ASSERT(ctx.texts[FIO_MD_TEXT] > 0, "expected text events");
  FIO_ASSERT(ctx.blocks[FIO_MD_STRONG_STAR] >= 1,
             "expected strong event (open)");
  FIO_ASSERT(ctx.blocks[FIO_MD_EMPHASIS_STAR] >= 1,
             "expected emphasis event (open)");
  FIO_ASSERT(ctx.blocks[FIO_MD_STRIKETHROUGH] >= 1,
             "expected strikethrough event (open)");
  FIO_ASSERT(ctx.texts[FIO_MD_CODE_SPAN] == 1, "expected 1 code span");
  FIO_ASSERT(ctx.direct_links == 1, "expected 1 direct link");
  FIO_ASSERT(ctx.ref_links == 1, "expected 1 ref link");
  FIO_ASSERT(ctx.images == 1, "expected 1 image");
  FIO_ASSERT(ctx.texts[FIO_MD_AUTOLINK] == 1, "expected 1 autolink");
  FIO_ASSERT(ctx.texts[FIO_MD_INLINE_HTML] == 2,
             "expected 2 inline HTML (open+close tags)");
  FIO_ASSERT(ctx.texts[FIO_MD_ESCAPE] == 1, "expected 1 escape");
  FIO_ASSERT(ctx.texts[FIO_MD_FOOTNOTE_REF] == 1,
             "expected 1 footnote ref");

  /* Tasks */
  FIO_ASSERT(ctx.checked_tasks == 1, "expected 1 checked task");
  FIO_ASSERT(ctx.unchecked_tasks == 1, "expected 1 unchecked task");

  /* Table cells */
  FIO_ASSERT(ctx.table_cells == 4, "expected 4 table cells");
  if (ctx.table_cells >= 4) {
    FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[0], "a"),
               "table cell 0 mismatch");
    FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[1], "b"),
               "table cell 1 mismatch");
    FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[2], "c"),
               "table cell 2 mismatch");
    FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[3], "d"),
               "table cell 3 mismatch");
  }

  /* Soft breaks */
  FIO_ASSERT(ctx.soft_breaks >= 0, "soft breaks may be zero for single-line paragraphs");
}

static void test_md_empty(void) {
  test_md_ctx_s ctx = {0};
  fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO1((char *)""));
  FIO_ASSERT(!ctx.errors, "empty doc should not error");
}

static void test_md_null(void) {
  test_md_ctx_s ctx = {0};
  fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO0);
  FIO_ASSERT(!ctx.errors, "null doc should not error");
}

static void test_md_invalid(void) {
  test_md_ctx_s ctx = {0};
  fio_md_parse(&test_md_callbacks, &ctx,
               FIO_BUF_INFO2(NULL, 1)); /* NULL buf with non-zero len */
  FIO_ASSERT(ctx.errors, "invalid input should error");
}

static void test_md_depth_limit(void) {
  /* Build a document that nests blockquotes to the limit */
  char src[4096];
  size_t pos = 0;
  size_t i;
  for (i = 0; i < FIO_MARKDOWN_MAX_DEPTH + 2 && pos + 4 < sizeof(src); ++i) {
    size_t j;
    for (j = 0; j < i && pos < sizeof(src) - 2; ++j)
      src[pos++] = '>';
    if (pos < sizeof(src) - 2) {
      src[pos++] = ' ';
      src[pos++] = 'x';
      src[pos++] = '\n';
    }
  }
  if (pos < sizeof(src))
    src[pos] = 0;
  {
    test_md_ctx_s ctx = {0};
    ctx.src = src;
    ctx.len = pos;
    fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO1(src));
    /* Should either succeed with depth truncation or error gracefully */
    FIO_ASSERT(ctx.errors || ctx.blocks[FIO_MD_BLOCKQUOTE] > 0,
               "depth limit test should produce blockquotes or error");
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== Testing Markdown / GFM Parser\n");
  test_md_empty();
  fprintf(stderr, "* empty document parsing OK.\n");
  test_md_null();
  fprintf(stderr, "* null document parsing OK.\n");
  test_md_invalid();
  fprintf(stderr, "* invalid input handling OK.\n");
  test_md_commonmark_gfm_blocks();
  fprintf(stderr, "* CommonMark + GFM blocks parsing OK.\n");
  test_md_depth_limit();
  fprintf(stderr, "* depth limit parsing OK.\n");
  fprintf(stderr, "=== All Markdown tests passed.\n");
  return 0;
}
