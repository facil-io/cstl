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
  size_t blocks[16];
  size_t leaves[16];
  size_t enters[16];
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

static int test_md_slice_in_source(test_md_ctx_s *ctx, fio_buf_info_s s) {
  if (!s.len)
    return 1;
  return s.buf >= ctx->src && (s.buf + s.len) <= (ctx->src + ctx->len);
}

static int test_md_slice_eq(fio_buf_info_s s, const char *expected) {
  size_t len = FIO_STRLEN(expected);
  return s.len == len && FIO_MEMCMP(s.buf, expected, len) == 0;
}

static int test_md_doc_start(void *udata, fio_buf_info_s source) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  if (source.buf != ctx->src || source.len != ctx->len)
    ++ctx->zero_copy_errors;
  return 0;
}

static int test_md_doc_end(void *udata, fio_buf_info_s source) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  if (source.buf != ctx->src || source.len != ctx->len)
    ++ctx->zero_copy_errors;
  return 0;
}

static int test_md_block_enter(void *udata, const fio_md_block_s *b) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  if ((size_t)b->type < (sizeof(ctx->blocks) / sizeof(ctx->blocks[0])))
    ++ctx->blocks[b->type];
  if (!test_md_slice_in_source(ctx, b->source) ||
      !test_md_slice_in_source(ctx, b->content) ||
      !test_md_slice_in_source(ctx, b->marker) ||
      !test_md_slice_in_source(ctx, b->info))
    ++ctx->zero_copy_errors;
  if (b->type == FIO_MD_BLOCK_LIST_ITEM) {
    ctx->checked_tasks += ((b->flags & FIO_MD_BLOCK_F_TASK_CHECKED) != 0);
    ctx->unchecked_tasks += ((b->flags & FIO_MD_BLOCK_F_TASK) != 0 &&
                             (b->flags & FIO_MD_BLOCK_F_TASK_CHECKED) == 0);
  }
  if (b->type == FIO_MD_BLOCK_TABLE_CELL) {
    if (ctx->table_cells <
        (sizeof(ctx->table_cell_text) / sizeof(ctx->table_cell_text[0])))
      ctx->table_cell_text[ctx->table_cells] = b->content;
    ++ctx->table_cells;
  }
  return 0;
}

static int test_md_block_leave(void *udata, const fio_md_block_s *b) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  if (!test_md_slice_in_source(ctx, b->source) ||
      !test_md_slice_in_source(ctx, b->content) ||
      !test_md_slice_in_source(ctx, b->marker) ||
      !test_md_slice_in_source(ctx, b->info))
    ++ctx->zero_copy_errors;
  return 0;
}

static int test_md_inline(void *udata, const fio_md_inline_s *i) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  if (!test_md_slice_in_source(ctx, i->source) ||
      !test_md_slice_in_source(ctx, i->text) ||
      !test_md_slice_in_source(ctx, i->destination) ||
      !test_md_slice_in_source(ctx, i->title) ||
      !test_md_slice_in_source(ctx, i->reference))
    ++ctx->zero_copy_errors;
  if ((size_t)i->type < (sizeof(ctx->leaves) / sizeof(ctx->leaves[0]))) {
    if (i->event == FIO_MD_EVENT_ENTER)
      ++ctx->enters[i->type];
    if (i->event == FIO_MD_EVENT_LEAF)
      ++ctx->leaves[i->type];
  }
  if (i->type == FIO_MD_INLINE_LINK && i->event == FIO_MD_EVENT_ENTER) {
    if (i->reference.len)
      ++ctx->ref_links;
    else
      ++ctx->direct_links;
  }
  if (i->type == FIO_MD_INLINE_IMAGE)
    ++ctx->images;
  if (i->type == FIO_MD_INLINE_SOFT_BREAK) {
    ++ctx->soft_breaks;
    ctx->soft_break_source_len += i->source.len;
  }
  return 0;
}

static void test_md_error(void *udata, int err, size_t consumed) {
  test_md_ctx_s *ctx = (test_md_ctx_s *)udata;
  (void)err;
  (void)consumed;
  ++ctx->errors;
}

static const fio_md_callbacks_s test_md_callbacks = {
    .on_document_start = test_md_doc_start,
    .on_document_end = test_md_doc_end,
    .on_block_enter = test_md_block_enter,
    .on_block_leave = test_md_block_leave,
    .on_inline = test_md_inline,
    .on_error = test_md_error,
};

static void test_md_commonmark_gfm_blocks(void) {
  char src[] = "# Heading *one*\n"
               "\n"
               "Paragraph with **strong**, *em*, ~~gone~~, `code`, "
               "[link](https://example.com \"t\"), ![alt](img.png), "
               "<https://fio.dev>, <span>x</span>, \\*, &amp;.\n"
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
               "```c\n"
               "int main(void) { return 0; }\n"
               "```\n"
               "\n"
               "<div>raw</div>\n"
               "\n"
               "| a | b |\n"
               "| :- | -: |\n"
               "| 1 | 2 |\n"
               "\n"
               "[ref text][id]\n"
               "\n"
               "[id]: https://ref.example \"ref title\"\n";
  test_md_ctx_s ctx = {.src = src, .len = FIO_STRLEN(src)};
  size_t consumed =
      fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO2(src, ctx.len));
  FIO_ASSERT(consumed == ctx.len,
             "Markdown consumed %zu / %zu",
             consumed,
             ctx.len);
  FIO_ASSERT(ctx.zero_copy_errors == 0, "All slices should point into source");
  FIO_ASSERT(ctx.errors == 0, "No error callbacks expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_DOCUMENT] == 1, "Document block expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_HEADING] >= 2,
             "ATX and setext headings expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_PARAGRAPH] >= 4, "Paragraphs expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_BLOCK_QUOTE] == 1, "Block quote expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_LIST_UNORDERED] >= 1,
             "Unordered list block expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_LIST_ORDERED] >= 1,
             "Ordered list block expected");
  FIO_ASSERT(ctx.checked_tasks == 1 && ctx.unchecked_tasks == 1,
             "Task markers expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_THEMATIC_BREAK] == 1,
             "Thematic break expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_CODE_INDENTED] == 1,
             "Indented code expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_CODE_FENCED] == 1, "Fenced code expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_HTML] == 1, "HTML block expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_TABLE] == 1, "GFM table expected");
  FIO_ASSERT(ctx.table_cells == 4,
             "Expected 4 table cells, got %zu",
             ctx.table_cells);
  FIO_ASSERT(ctx.enters[FIO_MD_INLINE_EMPHASIS] >= 1, "Emphasis expected");
  FIO_ASSERT(ctx.enters[FIO_MD_INLINE_STRONG] >= 1, "Strong expected");
  FIO_ASSERT(ctx.enters[FIO_MD_INLINE_STRIKETHROUGH] >= 1, "Strike expected");
  FIO_ASSERT(ctx.leaves[FIO_MD_INLINE_CODE_SPAN] >= 1, "Code span expected");
  FIO_ASSERT(ctx.direct_links >= 1, "Inline link expected");
  FIO_ASSERT(ctx.ref_links == 1, "Reference link expected");
  FIO_ASSERT(ctx.images == 1, "Image expected");
  FIO_ASSERT(ctx.leaves[FIO_MD_INLINE_AUTOLINK] >= 1, "Autolink expected");
  FIO_ASSERT(ctx.leaves[FIO_MD_INLINE_HTML] >= 1, "Inline HTML expected");
  FIO_ASSERT(ctx.leaves[FIO_MD_INLINE_ESCAPE] >= 1, "Escape expected");
  FIO_ASSERT(ctx.leaves[FIO_MD_INLINE_ENTITY] >= 1, "Entity expected");
}

static void test_md_blockquote_multiline_paragraph(void) {
  char src[] = "> alpha\n> beta\n";
  test_md_ctx_s ctx = {.src = src, .len = FIO_STRLEN(src)};
  size_t consumed =
      fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO2(src, ctx.len));
  FIO_ASSERT(consumed == ctx.len, "Markdown consumed full block quote");
  FIO_ASSERT(ctx.errors == 0, "No error callbacks expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_BLOCK_QUOTE] == 1,
             "One block quote expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_PARAGRAPH] == 1,
             "Multi-line block quote should contain one paragraph");
  FIO_ASSERT(ctx.soft_breaks == 1 && ctx.soft_break_source_len > 0,
             "Soft break should expose the source line ending");
}

static void test_md_nested_lists(void) {
  char src[] = " - foo\n   - bar\n\t - baz\n";
  test_md_ctx_s ctx = {.src = src, .len = FIO_STRLEN(src)};
  size_t consumed =
      fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO2(src, ctx.len));
  FIO_ASSERT(consumed == ctx.len, "Markdown consumed full nested list");
  FIO_ASSERT(ctx.errors == 0, "No error callbacks expected");
  FIO_ASSERT(ctx.zero_copy_errors == 0, "Nested list slices stay zero-copy");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_LIST_UNORDERED] == 3,
             "Expected 3 nested unordered lists, got %zu",
             ctx.blocks[FIO_MD_BLOCK_LIST_UNORDERED]);
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_LIST_ITEM] == 3,
             "Expected 3 nested list items, got %zu",
             ctx.blocks[FIO_MD_BLOCK_LIST_ITEM]);
}

static void test_md_table_escaped_and_code_pipes(void) {
  char src[] = "| a\\|b | `c|d` |\n"
               "| --- | --- |\n"
               "| x | y |\n";
  test_md_ctx_s ctx = {.src = src, .len = FIO_STRLEN(src)};
  size_t consumed =
      fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO2(src, ctx.len));
  FIO_ASSERT(consumed == ctx.len, "Markdown consumed full table");
  FIO_ASSERT(ctx.errors == 0, "No error callbacks expected");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_TABLE] == 1, "GFM table expected");
  FIO_ASSERT(ctx.table_cells == 4,
             "Expected 4 table cells, got %zu",
             ctx.table_cells);
  FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[0], "a\\|b"),
             "Escaped pipe must not split a cell");
  FIO_ASSERT(test_md_slice_eq(ctx.table_cell_text[1], "`c|d`"),
             "Code-span pipe must not split a cell");
}

static void test_md_backtick_fence_info_rejects_backtick(void) {
  char src[] = "``` bad`info\nnot code\n";
  test_md_ctx_s ctx = {.src = src, .len = FIO_STRLEN(src)};
  size_t consumed =
      fio_md_parse(&test_md_callbacks, &ctx, FIO_BUF_INFO2(src, ctx.len));
  FIO_ASSERT(consumed == ctx.len, "Markdown consumed full invalid fence");
  FIO_ASSERT(ctx.blocks[FIO_MD_BLOCK_CODE_FENCED] == 0,
             "Backtick fence info strings containing backticks are invalid");
}

typedef struct {
  int error_seen;
  int err;
  size_t consumed;
} abort_ctx_s;

static int abort_on_inline(void *udata, const fio_md_inline_s *i) {
  (void)udata;
  (void)i;
  return 77;
}

static void abort_on_error(void *udata, int err, size_t consumed) {
  abort_ctx_s *ctx = (abort_ctx_s *)udata;
  ctx->err = err;
  ctx->consumed = consumed;
  ctx->error_seen = 1;
}

static void test_md_abort(void) {
  char src[] = "hello *world*\n";
  abort_ctx_s ctx = {0};
  static const fio_md_callbacks_s callbacks = {
      .on_inline = abort_on_inline,
      .on_error = abort_on_error,
  };
  size_t consumed = fio_md_parse(&callbacks, &ctx, FIO_BUF_INFO1(src));
  FIO_ASSERT(ctx.err == 77, "Expected propagated callback error");
  FIO_ASSERT(consumed == ctx.consumed, "Expected matching consumed value");
  FIO_ASSERT(ctx.error_seen == 1, "Expected on_error callback");
}

int main(void) {
  test_md_commonmark_gfm_blocks();
  test_md_blockquote_multiline_paragraph();
  test_md_nested_lists();
  test_md_table_escaped_and_code_pipes();
  test_md_backtick_fence_info_rejects_backtick();
  test_md_abort();
  fprintf(stderr, "Markdown parser tests passed.\n");
  return 0;
}
