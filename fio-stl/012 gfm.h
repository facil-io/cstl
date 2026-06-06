/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_GFM                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        GFM Markdown Parser (Flat-State)

  Non-recursive, zero-allocation GFM parser. Replaces `004 markdown.h`.

  Algorithm:
    Follows GFM spec appendix A exactly. Three steps per line:
      Step 1: Match open container continuations (outermost → innermost)
      Step 2: Check for new block starts / lazy continuation
      Step 3: Incorporate text into the deepest open block

    All container state lives in a flat array indexed by depth — no C
    recursion, no stack overflow risk. Inline parsing uses a forward-scan
    approach with a local open-section stack.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_GFM) && !defined(H___FIO_GFM___H)
#define H___FIO_GFM___H

/* Dependency: ML entity decoding for inline parsing. */
#ifndef H___FIO_ENTITY___H
#define FIO_ENTITY
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#endif

/* ---------------------------------------------------------------------------
 * Configuration macros — user may override before inclusion.
 * ------------------------------------------------------------------------- */

#ifndef FIO_GFM_MAX_DEPTH
/** Maximum container nesting depth (blockquote + list). */
#define FIO_GFM_MAX_DEPTH 255
#endif
FIO_ASSERT_STATIC(FIO_GFM_MAX_DEPTH < 256,
                  "FIO_GFM_MAX_DEPTH must fit in a single byte (<= 255)");

#ifndef FIO_GFM_MAX_TABLE_COLUMNS
/** Maximum table columns (2 bits each → 4 columns per byte). */
#define FIO_GFM_MAX_TABLE_COLUMNS 64
#endif
FIO_ASSERT_STATIC(FIO_GFM_MAX_TABLE_COLUMNS > 0,
                  "FIO_GFM_MAX_TABLE_COLUMNS too small");

#ifndef FIO_GFM_REF_CACHE_SIZE
/** Maximum reference definitions held in the fast cache. */
#define FIO_GFM_REF_CACHE_SIZE 128
#endif
FIO_ASSERT_STATIC(FIO_GFM_REF_CACHE_SIZE > 0,
                  "FIO_GFM_REF_CACHE_SIZE too small");

/* Inline delimiter stack is local to fio___gfm_inline_parse() — not in
 * the parser struct. See FIO___GFM_MAX_OPEN_SECTIONS in the inline parser
 * section. This keeps the parser struct ~4 KB smaller. */

/* ---------------------------------------------------------------------------
 * Internal constants
 * ------------------------------------------------------------------------- */

#define FIO___GFM_TAB_WIDTH         4U
#define FIO___GFM_MAX_MARKER_INDENT 3U
#define FIO___GFM_MIN_FENCE_LEN     3
#define FIO___GFM_MAX_ATX_HEADING   6
#define FIO___GFM_MAX_ENTITY_LEN    32

/** Error codes — negative values are parser-generated. */
#define FIO_GFM_ERR_GENERIC -1
#define FIO_GFM_ERR_DEPTH   -2
#define FIO_GFM_ERR_INPUT   -3

/* ---------------------------------------------------------------------------
 * Block flags (on container events via `fio_gfm_event_s.flags`)
 * ------------------------------------------------------------------------- */

/** List is tight (no blank lines between items). */
#define FIO_GFM_F_TIGHT ((uint8_t)1U << 0)
/** A blank line was seen between items — list is loose. */
#define FIO_GFM_F_LOOSE_SEEN ((uint8_t)1U << 1)
/** GFM task-list marker present. */
#define FIO_GFM_F_TASK ((uint8_t)1U << 2)
/** GFM task-list marker is checked. */
#define FIO_GFM_F_TASK_CHECKED ((uint8_t)1U << 3)

/* ---------------------------------------------------------------------------
 * Public API — self-contained, does NOT depend on 004 markdown.h.
 *
 * Callbacks: push / write / pop. Three callbacks, no error callback.
 * Errors are communicated via return value from push/write/pop (non-zero
 * aborts parsing) and the return value of fio_gfm_parse() (0 on error).
 * ------------------------------------------------------------------------- */

/** GFM node type. Covers block sections, inline sections, and text.
 *  Value 0 is reserved (no active type). All public types are nonzero. */
typedef enum {
  /* === Block sections (push/pop pairs) === */
  FIO_GFM_PARAGRAPH = 1,
  FIO_GFM_HEADING,
  FIO_GFM_THEMATIC_BREAK,
  FIO_GFM_BLOCKQUOTE,
  FIO_GFM_LIST_UNORDERED,
  FIO_GFM_LIST_ORDERED,
  FIO_GFM_LIST_ITEM,
  FIO_GFM_CODE_BLOCK, /* indented or fenced — e->info has language tag.
                        * Content emitted per-line as write(TEXT) events. */
  FIO_GFM_HTML_BLOCK, /* content emitted per-line as write(TEXT) events. */
  FIO_GFM_TABLE,
  FIO_GFM_TABLE_ROW,
  FIO_GFM_TABLE_CELL, /* content emitted per-cell via inline parse. */

  /* === Inline sections (push/pop pairs) === */
  FIO_GFM_EMPHASIS,      /* * or _ — single */
  FIO_GFM_STRONG,        /* ** or __ — double */
  FIO_GFM_STRIKETHROUGH, /* ~~ */
  FIO_GFM_LINK,

  /* === Text / leaf content (write only, no push/pop) === */
  FIO_GFM_TEXT,         /* literal text — raw source slice, zero-copy.
                         * Escapes and entities are NOT decoded; callbacks
                         * receive the original source bytes. */
  FIO_GFM_SOFT_BREAK,   /* line break → space in output */
  FIO_GFM_HARD_BREAK,   /* forced line break (trailing \\ or 2+ spaces) */
  FIO_GFM_CODE_SPAN,    /* inline code (`...`) — text is verbatim content */
  FIO_GFM_IMAGE,        /* image — destination, title, text (alt) set */
  FIO_GFM_AUTOLINK,     /* <url> or GFM extended autolink */
  FIO_GFM_INLINE_HTML,  /* raw inline HTML tag */
  FIO_GFM_FOOTNOTE_REF, /* GFM footnote reference [^label] */
} fio_gfm_type_e;

/** GFM table cell alignment. */
typedef enum {
  FIO_GFM_ALIGN_NONE = 0,
  FIO_GFM_ALIGN_LEFT,
  FIO_GFM_ALIGN_RIGHT,
  FIO_GFM_ALIGN_CENTER,
} fio_gfm_align_e;

/** GFM event. Passed by pointer to all callbacks.
 *
 * `udata` is live: the parser copies it back after each callback,
 * so the callback can update it (e.g., to push/pop render state). */
typedef struct {
  void *udata;                /* live: parser copies back after callback */
  fio_buf_info_s source;      /* full source slice for this event */
  fio_buf_info_s text;        /* text / label / code content */
  fio_buf_info_s marker;      /* marker slice (`#`, fence, list marker) */
  fio_buf_info_s info;        /* fenced code info string */
  fio_buf_info_s destination; /* link/image/autolink URL */
  fio_buf_info_s title;       /* link/image title */
  fio_buf_info_s reference;   /* reference label for ref-style links/images */
  uint32_t list_start;        /* ordered list start number */
  uint16_t columns;           /* table column count */
  uint16_t column;            /* table cell column index (0-based) */
  uint8_t type;               /* fio_gfm_type_e */
  uint8_t heading_level;      /* 1..6 for headings */
  uint8_t flags;              /* FIO_GFM_F_* */
  uint8_t align;              /* fio_gfm_align_e for table cells */
  uint8_t padding;            /* virtual leading spaces from tab expansion */
} fio_gfm_event_s;

/** GFM parser callbacks. Three callbacks, all optional.
 *
 * push — emitted when a section opens (block or inline).
 * write — emitted for text content and leaf nodes.
 * pop  — emitted when a section closes.
 *
 * Return 0 to continue, non-zero to abort parsing. The non-zero value
 * is propagated to any enclosing scope; positive values are user errors,
 * negative values are reserved for parser errors.
 */
typedef struct {
  int (*push)(fio_gfm_event_s *e);  /* section start */
  int (*write)(fio_gfm_event_s *e); /* text / leaf content */
  int (*pop)(fio_gfm_event_s *e);   /* section end */
} fio_gfm_callbacks_s;

/**
 * Parses a complete Markdown/GFM document and emits callback events.
 *
 * Non-streaming: `source` must contain the complete document.
 * Zero-copy: events are slices into `source`.
 * Zero-allocation: all state lives on the C stack (~12 KB).
 * Non-recursive: O(1) C stack depth.
 *
 * Returns the number of source bytes consumed. On error (callback returned
 * non-zero, depth overflow, or invalid input), returns the byte offset where
 * parsing stopped. Check the return value against `source.len` to detect
 * incomplete parsing.
 */
SFUNC size_t fio_gfm_parse(const fio_gfm_callbacks_s *callbacks,
                           void *udata,
                           fio_buf_info_s source);

/* *****************************************************************************
 * GFM Parser — Implementation
 *
 * The entire implementation lives inside FIO_EXTERN_COMPLETE || !FIO_EXTERN
 * so that when used as a header-only library the implementation compiles once.
 * ************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* ===========================================================================
 * Internal Types
 * ========================================================================= */

/** Reference definition cache entry. */
typedef struct {
  fio_buf_info_s label;
  fio_buf_info_s destination;
  fio_buf_info_s title;
} fio___gfm_ref_s;

/* Inline parser uses a forward-scan approach with a local open-section
 * stack (allocated on the C stack inside fio___gfm_inline_parse()).
 * No delimiter stack is stored in the parser struct. */

/* --- Internal container types (encode marker identity) --- */
#define FIO___GFM_CONT_BQ       0 /* blockquote */
#define FIO___GFM_CONT_LI       1 /* list item */
#define FIO___GFM_CONT_UL_DASH  2 /* unordered list, '-' bullet */
#define FIO___GFM_CONT_UL_PLUS  3 /* unordered list, '+' bullet */
#define FIO___GFM_CONT_UL_STAR  4 /* unordered list, '*' bullet */
#define FIO___GFM_CONT_OL_DOT   5 /* ordered list, '.' delimiter */
#define FIO___GFM_CONT_OL_PAREN 6 /* ordered list, ')' delimiter */

/** Map internal container type to public fio_gfm_type_e for events. */
FIO_IFUNC uint8_t fio___gfm_cont_public_type(uint8_t t) {
  static const uint8_t map[] = {
      FIO_GFM_BLOCKQUOTE,     /* 0: BQ */
      FIO_GFM_LIST_ITEM,      /* 1: LI */
      FIO_GFM_LIST_UNORDERED, /* 2: UL_DASH */
      FIO_GFM_LIST_UNORDERED, /* 3: UL_PLUS */
      FIO_GFM_LIST_UNORDERED, /* 4: UL_STAR */
      FIO_GFM_LIST_ORDERED,   /* 5: OL_DOT */
      FIO_GFM_LIST_ORDERED,   /* 6: OL_PAREN */
  };
  return (t < sizeof(map)) ? map[t] : 0;
}

/** Per-depth nesting entry. 4 bytes, no padding. */
typedef struct {
  uint8_t  type;   /* FIO___GFM_CONT_* — encodes container kind + marker */
  uint8_t  flags;  /* reserved (available for future per-depth state) */
  uint16_t indent; /* content indent (used by LIST_ITEM for continuation) */
} fio___gfm_nest_s;

/* ===========================================================================
 * The Flat State Struct
 *
 * ALL parser state lives here. No local variables in recursive frames.
 * This struct is allocated on the C stack in fio_gfm_parse().
 *
 * Cache-line layout (64-byte lines, single-threaded):
 *
 *   Line 0 (bytes 0-63):  HOT — touched every line of input.
 *     Constant: cb, start, end.
 *     Mutable:  udata, para_start, para_end, depth, matched_depth,
 *               leaf_type, para_open, fence_char, leaf_html_type,
 *               fence_len, fence_indent, err.
 *     Packed to exactly 64 bytes — no wasted padding.
 *
 *   Line 1+ (bytes 64-1083): WARM — touched during container walks.
 *     nest[] array, 4 bytes/entry, starts on cache line boundary.
 *     Step 1 walks nest[0..depth-1] sequentially each line.
 *     Typical depth 2-5 = 8-20 bytes = stays in one cache line.
 *
 *   After nest: WARM LEAF — touched only inside specific leaf types.
 *     fence_info, leaf_start (fenced code / HTML block paths only).
 *
 *   Cold tail: Reference cache, table alignment, footnotes, consumed.
 *     Touched only during inline parsing (ref lookup) or table rows.
 * ========================================================================= */

typedef struct {
  /* =================================================================
   * Cache line 0: HOT — every field here is accessed on most lines.
   * 6 pointers (48) + 1 int (4) + 4 uint16 (8) + 4 uint8 (4) = 64.
   * ================================================================= */

  /* --- Constant (set once at parse start, read every emit) --- */
  int (*push)(fio_gfm_event_s *);  /*  8 — inlined from callbacks_s */
  int (*write)(fio_gfm_event_s *); /*  8 — avoids double-deref through cb-> */
  int (*pop)(fio_gfm_event_s *);   /*  8 — all three in cache line 0 */
  char   *start;                   /*  8 — document start */
  char   *end;                     /*  8 — document end */

  /* --- Mutable (read/written most lines) --- */
  void   *udata;                   /*  8 — live: synced with events */
  char   *para_start;              /*  8 — paragraph text accumulator */
  char   *para_end;                /*  8 — end of accumulated para text */
                                   /* --- 64 bytes: 8 pointers --- */

  /* =================================================================
   * Cache line 1: Mutable control + nesting stack head.
   * depth/matched_depth are read then nest[] is walked — co-locate.
   * leaf_type/para_open branch immediately after Step 1.
   * Typical nest walk (depth 2-5) stays within this cache line.
   * ================================================================= */
  int      err;                    /*  4 — non-zero = abort */
  uint16_t depth;                  /*  2 — container nesting depth */
  uint16_t matched_depth;          /*  2 — deepest matched container */
  uint16_t fence_len;              /*  2 — opening fence length */
  uint16_t fence_indent;           /*  2 — opening fence indent */
  uint8_t  leaf_type;              /*  1 — current leaf (0 = none) */
  uint8_t  para_open;              /*  1 — 1 iff leaf_type == PARAGRAPH */
  uint8_t  fence_char;             /*  1 — '`'/'~' = fenced, 0 = indented */
  uint8_t  leaf_html_type;         /*  1 — HTML block sub-type (1-7) */
                                   /* --- 16 bytes, then nest[] follows --- */

  /* Container nesting stack. 4 bytes/entry.
   * Internal types encode marker identity (see FIO___GFM_CONT_*).
   * list_start not stored (passed on push, never re-read).
   * Tight/loose determined by lookahead, not tracked here.
   *
   * Example for "> - foo":
   *   nest[0] = { CONT_BQ,      .indent = 0 }
   *   nest[1] = { CONT_UL_DASH, .indent = 0 }
   *   nest[2] = { CONT_LI,      .indent = 4 }
   */
  fio___gfm_nest_s nest[FIO_GFM_MAX_DEPTH];

  /* =================================================================
   * WARM LEAF — only accessed inside fenced code / HTML block paths.
   * ================================================================= */

  /** Fenced code info string (language tag). Empty for indented code.
   *  fence_char/fence_len/fence_indent live in cache line 0 (hot). */
  fio_buf_info_s fence_info;
  /** Source start of the current leaf. */
  char *leaf_start;

  /* =================================================================
   * COLD — reference cache, table, footnotes, error bookkeeping.
   * Touched only during inline parsing or specific leaf types.
   * ================================================================= */

  size_t consumed;  /* bytes consumed (for error reporting) */

  /* Reference cache: lazy scan, 128 entries + 1 overflow slot. */
  fio___gfm_ref_s refs[FIO_GFM_REF_CACHE_SIZE];
  fio___gfm_ref_s ref_overflow_slot;
  char    *ref_scanned_to;
  uint16_t ref_count;
  uint8_t  ref_overflow;

  /* Table alignment (bit-packed, 2 bits/column). */
  uint8_t  table_align[(FIO_GFM_MAX_TABLE_COLUMNS + 3) >> 2];
  uint16_t table_columns;

  /* Footnote state (GFM extension). */
  uint8_t  in_footnote;
  uint8_t  footnote_depth;

  /* Indented code: pending blank lines (stripped if trailing per spec 4.4).
   * Store (content, le) pairs to preserve whitespace content. */
#define FIO___GFM_MAX_IC_BLANKS 32
  char *ic_blank_content[FIO___GFM_MAX_IC_BLANKS];
  char *ic_blank_le[FIO___GFM_MAX_IC_BLANKS];
  uint16_t ic_pending_blanks;

} fio___gfm_parser_s;

/* Verify layout: cache line 0 = 8 pointers (64 bytes).
 * Cache line 1 starts with 16 bytes of control fields, then nest[]. */
FIO_ASSERT_STATIC(
    offsetof(fio___gfm_parser_s, err) == 64,
    "fio___gfm_parser_s: mutable control must start at byte 64");

/* Forward declarations for cross-references between sections. */
FIO_SFUNC int fio___gfm_close_paragraph(fio___gfm_parser_s *st);
FIO_SFUNC int fio___gfm_close_leaf(fio___gfm_parser_s *st);
FIO_SFUNC int fio___gfm_inline_parse(fio___gfm_parser_s *st,
                                     char *start,
                                     char *end);
FIO_SFUNC int fio___gfm_emit_table_row(fio___gfm_parser_s *st,
                                        char *p,
                                        char *le);
FIO_SFUNC void fio___gfm_ref_cache_add(fio___gfm_parser_s *st,
                                       fio_buf_info_s label,
                                       fio_buf_info_s dest,
                                       fio_buf_info_s title);
FIO_SFUNC char *fio___gfm_try_parse_ref_def(char *p,
                                            char *end,
                                            fio_buf_info_s *label,
                                            fio_buf_info_s *dest,
                                            fio_buf_info_s *title);
FIO_SFUNC void fio___gfm_ref_scan_document(fio___gfm_parser_s *st);

/* ===========================================================================
 * Nesting Stack Macros
 *
 * nest[] is 0-indexed: nest[0] is depth 1 (outermost), nest[depth-1] is
 * innermost. depth==0 means no containers open.
 * ========================================================================= */

#define FIO___GFM_DEPTH(st)      ((st)->depth)
#define FIO___GFM_TYPE(st, d)    ((st)->nest[(d)].type)
#define FIO___GFM_TOP(st)        ((st)->nest[(st)->depth - 1])
#define FIO___GFM_TOP_TYPE(st)   ((st)->nest[(st)->depth - 1].type)

/** Push a container. Sets err on overflow. */
#define FIO___GFM_PUSH(st, type_val, indent_val)                               \
  do {                                                                         \
    if ((st)->depth >= FIO_GFM_MAX_DEPTH) {                                    \
      (st)->err = FIO_GFM_ERR_DEPTH;                                           \
      break;                                                                   \
    }                                                                          \
    (st)->nest[(st)->depth++] = (fio___gfm_nest_s){                            \
        .type = (type_val),                                                    \
        .indent = (indent_val),                                                \
    };                                                                         \
  } while (0)

/** Pop the innermost container. */
#define FIO___GFM_POP(st) (--(st)->depth)

/* ===========================================================================
 * Table Alignment Macros (identical to 004 markdown.h)
 * ========================================================================= */

#define FIO___GFM_TABLE_ALIGN_GET(st, col)                                     \
  (((st)->table_align[(col) >> 2] >> (((col)&3) << 1)) & 3)
#define FIO___GFM_TABLE_ALIGN_SET(st, col, val)                                \
  do {                                                                         \
    uint8_t _shift = ((col)&3) << 1;                                           \
    (st)->table_align[(col) >> 2] &= ~(3 << _shift);                           \
    (st)->table_align[(col) >> 2] |= ((val)&3) << _shift;                      \
  } while (0)

/* ===========================================================================
 * Event Emission Helpers
 *
 * Three callbacks: push / write / pop. No error callback.
 * Errors are signaled by non-zero return from callbacks or by comparing
 * the return value of fio_gfm_parse() against source.len.
 *
 * `udata` is live: the parser reads it back from the event after each
 * callback, so callbacks can mutate it (e.g., push/pop render state).
 * ========================================================================= */

FIO_IFUNC void fio___gfm_event_init(fio___gfm_parser_s *st,
                                    fio_gfm_event_s *e) {
  FIO_MEMSET(e, 0, sizeof(*e));
  e->udata = st->udata;
}

/** Emit a push (section open) event. Returns callback result. */
FIO_IFUNC int fio___gfm_emit_push(fio___gfm_parser_s *st, fio_gfm_event_s *e) {
  if (!st->push)
    return 0;
  e->udata = st->udata;
  int r = st->push(e);
  st->udata = e->udata;
  return r;
}

/** Emit a write (text / leaf content) event. Returns callback result. */
FIO_IFUNC int fio___gfm_emit_write(fio___gfm_parser_s *st, fio_gfm_event_s *e) {
  if (!st->write)
    return 0;
  e->udata = st->udata;
  int r = st->write(e);
  st->udata = e->udata;
  return r;
}

/** Emit a pop (section close) event. Returns callback result. */
FIO_IFUNC int fio___gfm_emit_pop(fio___gfm_parser_s *st, fio_gfm_event_s *e) {
  if (!st->pop)
    return 0;
  e->udata = st->udata;
  int r = st->pop(e);
  st->udata = e->udata;
  return r;
}

/* ===========================================================================
 * Low-Level Text Helpers
 *
 * These operate on raw source bytes. They handle tab expansion, virtual
 * column computation, line boundary detection, and whitespace classification.
 * ========================================================================= */

/** Find the end of the current line (\n, \r, or end of document). */
FIO_IFUNC char *fio___gfm_line_end(char *p, char *end) {
  while (p < end && *p != '\n' && *p != '\r')
    ++p;
  return p;
}

/** Advance past the current line ending (\r\n, \n, \r, or stay at end). */
FIO_IFUNC char *fio___gfm_line_next(char *le, char *end) {
  le += (le < end && *le == '\r');
  le += (le < end && *le == '\n');
  return le;
}

/** Compute the virtual column at position `target` starting from `start`.
 *  Tabs expand to the next multiple of 4. */
FIO_IFUNC uint32_t fio___gfm_vcol(char *start, char *target) {
  uint32_t col = 0;
  while (start < target) {
    uint32_t is_tab = (*start == '\t');
    /* tab: jump to next multiple of 4. non-tab: advance by 1. */
    col += is_tab *
               (FIO___GFM_TAB_WIDTH - (col & (FIO___GFM_TAB_WIDTH - 1U)) - 1U) +
           1U;
    ++start;
  }
  return col;
}

/** Compute the virtual column of the first non-whitespace character.
 *  Equivalent to fio___gfm_vcol(p, ltrim(p, le)). */
FIO_IFUNC uint32_t fio___gfm_indent(char *p, char *le) {
  uint32_t col = 0;
  while (p < le && (*p == ' ' || *p == '\t')) {
    uint32_t is_tab = (*p == '\t');
    col += is_tab *
               (FIO___GFM_TAB_WIDTH - (col & (FIO___GFM_TAB_WIDTH - 1U)) - 1U) +
           1U;
    ++p;
  }
  return col;
}

/** Skip whitespace, consuming up to `columns` virtual columns.
 *  Returns pointer to position after consuming, or NULL if the line
 *  has fewer than `columns` of whitespace. Sets *padding to the
 *  overshoot if a tab crosses the column boundary. */
FIO_IFUNC char *fio___gfm_skip_indent(char *p,
                                      char *le,
                                      uint32_t columns,
                                      uint8_t *padding) {
  uint32_t col = 0;
  uint8_t pad = 0;
  while (p < le && col < columns) {
    uint32_t advance =
        (*p == '\t') ? FIO___GFM_TAB_WIDTH - (col & (FIO___GFM_TAB_WIDTH - 1U))
                     : (*p == ' ');
    if (!advance)
      break;
    col += advance;
    ++p;
  }
  pad = (uint8_t)((col - columns) & -(col > columns));
  if (padding)
    *padding = pad;
  return col >= columns ? p : NULL;
}

/** Left-trim whitespace. Only use before emitting TEXT events (G5). */
FIO_IFUNC char *fio___gfm_ltrim(char *p, char *le) {
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  return p;
}

/** Right-trim whitespace. */
FIO_IFUNC char *fio___gfm_rtrim(char *start, char *end) {
  while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
    --end;
  return end;
}

/** Test if a line is blank (only whitespace between p and le). */
FIO_IFUNC int fio___gfm_is_blank(char *p, char *le) {
  return fio___gfm_ltrim(p, le) == le;
}

/* ASCII lowercase — use the core constant-time helper. */
#define fio___gfm_tolower(c) fio_ct_tolower(c)

/** Test if character is ASCII punctuation (per CommonMark). */
FIO_IFUNC int fio___gfm_is_punct(char c) {
  return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') ||
         (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

/* ===========================================================================
 * Block Detection Helpers
 *
 * Each helper tests whether the current content (after container markers
 * have been consumed) starts a specific block type. They operate on
 * virtual columns, never on raw byte offsets.
 *
 * TODO: Implement each of these. The signatures and contracts are final.
 * ========================================================================= */

/** Test for ATX heading: 1-6 '#' chars followed by space or end-of-line,
 *  at indent <= 3.
 *  Returns heading level (1-6) or 0 if not a heading. */
FIO_SFUNC int fio___gfm_is_atx_heading(char *p, char *le) {
  /* GFM spec section 4.2: 1-6 '#' then space/tab/EOL. */
  if (p >= le || *p != '#')
    return 0;
  int level = 0;
  while (p < le && *p == '#') {
    ++level;
    ++p;
  }
  if (level > FIO___GFM_MAX_ATX_HEADING)
    return 0;
  /* Must be followed by space, tab, or end of line */
  if (p < le && *p != ' ' && *p != '\t')
    return 0;
  return level;
}

/** Test for thematic break: 3+ of same char (-, _, *) with optional spaces.
 *  Returns 1 if thematic break, 0 otherwise. */
FIO_SFUNC int fio___gfm_is_thematic_break(char *p, char *le) {
  /* GFM spec section 4.1: 3+ of same char (- _ *), optional spaces. */
  if (p >= le)
    return 0;
  char ch = *p;
  if (ch != '-' && ch != '_' && ch != '*')
    return 0;
  int count = 0;
  while (p < le) {
    count += (*p == ch);
    if (*p != ch && *p != ' ' && *p != '\t')
      return 0;
    ++p;
  }
  return count >= 3;
}

/** Test for blockquote marker: '>' at indent <= 3.
 *  Returns 1 if blockquote marker found, 0 otherwise.
 *  Sets *after to the position after '>' + optional space. */
FIO_SFUNC int fio___gfm_is_blockquote(char *p, char *le, char **after) {
  /* GFM spec section 5.1: '>' optionally followed by single space. */
  if (p >= le || *p != '>')
    return 0;
  ++p;
  /* Consume optional single space or tab after '>' */
  p += (p < le && (*p == ' ' || *p == '\t'));
  *after = p;
  return 1;
}

/** Test for fenced code opening: 3+ backticks or tildes at indent <= 3.
 *  Returns fence length or 0 if not a fence opener.
 *  Sets *fence_ch to the fence character. */
FIO_SFUNC uint16_t fio___gfm_is_fenced_code_open(char *p,
                                                 char *le,
                                                 char *fence_ch) {
  /* GFM spec section 4.5: 3+ backticks or tildes. */
  if (p >= le || (*p != '`' && *p != '~'))
    return 0;
  char ch = *p;
  uint16_t len = 0;
  while (p < le && *p == ch) {
    ++len;
    ++p;
  }
  if (len < FIO___GFM_MIN_FENCE_LEN)
    return 0;
  /* Backtick fences: info string must not contain backtick */
  if (ch == '`') {
    for (char *s = p; s < le; ++s) {
      if (*s == '`')
        return 0;
    }
  }
  *fence_ch = ch;
  return len;
}

/** Test for closing fence: same char, length >= opening, indent <= 3.
 *  Returns 1 if valid closing fence, 0 otherwise. */
FIO_SFUNC int fio___gfm_is_fenced_code_close(char *p,
                                             char *le,
                                             char fence_ch,
                                             uint16_t fence_len) {
  /* GFM spec section 4.5: closing fence = same char, >= opener length. */
  /* Skip up to 3 spaces of indent */
  uint32_t ind = 0;
  while (p < le && (*p == ' ' || *p == '\t') && ind < FIO___GFM_TAB_WIDTH) {
    ind += (*p == '\t')
               ? FIO___GFM_TAB_WIDTH - (ind & (FIO___GFM_TAB_WIDTH - 1U))
               : 1;
    ++p;
  }
  if (ind > FIO___GFM_MAX_MARKER_INDENT)
    return 0;
  if (p >= le || *p != fence_ch)
    return 0;
  uint16_t count = 0;
  while (p < le && *p == fence_ch) {
    ++count;
    ++p;
  }
  if (count < fence_len)
    return 0;
  /* Rest of line must be whitespace */
  while (p < le) {
    if (*p != ' ' && *p != '\t')
      return 0;
    ++p;
  }
  return 1;
}

/** Parsed list marker result. */
typedef struct {
  char *content_start;     /* position where item content begins */
  uint32_t start_num;      /* ordered list start number (0 for bullet) */
  uint8_t type;            /* FIO___GFM_CONT_* internal type */
  uint8_t marker_char;     /* '-', '+', '*', '.', or ')' */
  uint16_t content_indent; /* virtual column of content start */
} fio___gfm_list_marker_s;

/** Test for list marker at current position.
 *  Returns 1 if valid list marker found, 0 otherwise.
 *  Fills *info with marker details.
 *
 *  Bullet markers: -, +, * followed by 1-4 spaces then content.
 *  Ordered markers: 1-9 digits followed by . or ) then 1-4 spaces.
 *
 *  GFM spec: "the spaces after the list marker determine how much
 *  relative indentation is needed." Content indent is the column of
 *  the first non-whitespace character after the marker + spaces.
 *  If the line after the marker is blank, content indent = marker width + 1.
 */
FIO_SFUNC int fio___gfm_is_list_marker(char *p,
                                       char *le,
                                       uint32_t base_col,
                                       fio___gfm_list_marker_s *info) {
  /* GFM spec section 5.2: bullet (- + *) or ordered (digits + . or )). */
  uint8_t type;
  uint32_t start_num = 0;
  uint32_t marker_width;

  if (p < le && (*p == '-' || *p == '+' || *p == '*')) {
    /* Bullet list */
    info->marker_char = *p;
    type = (*p == '-') ? FIO___GFM_CONT_UL_DASH
         : (*p == '+') ? FIO___GFM_CONT_UL_PLUS
                       : FIO___GFM_CONT_UL_STAR;
    marker_width = 1;
    ++p;
  } else {
    /* Try ordered: 1-9 digits then '.' or ')' */
    char *d = p;
    int digits = 0;
    while (d < le && *d >= '0' && *d <= '9' && digits < 10) {
      start_num = start_num * 10 + (uint32_t)(*d - '0');
      ++digits;
      ++d;
    }
    if (digits < 1 || digits > 9 || d >= le || (*d != '.' && *d != ')'))
      return 0;
    info->marker_char = *d;
    type = (*d == '.') ? FIO___GFM_CONT_OL_DOT : FIO___GFM_CONT_OL_PAREN;
    marker_width = (uint32_t)digits + 1;
    p = d + 1;
  }

  /* After marker: must have space/tab or be at EOL */
  if (p < le && *p != ' ' && *p != '\t')
    return 0;

  /* Count whitespace after marker (in virtual columns) */
  uint32_t after_col = base_col + marker_width;
  char *scan = p;
  uint32_t spaces = 0;
  while (scan < le && (*scan == ' ' || *scan == '\t')) {
    spaces += (*scan == '\t')
                  ? FIO___GFM_TAB_WIDTH -
                        ((after_col + spaces) & (FIO___GFM_TAB_WIDTH - 1U))
                  : 1;
    ++scan;
  }

  int blank_after = (scan >= le);

  if (blank_after || spaces > 4) {
    info->content_indent = (uint16_t)(base_col + marker_width + 1);
    /* Consume just one space past marker (or stay at EOL) */
    info->content_start = (p < le) ? p + 1 : p;
  } else {
    info->content_indent = (uint16_t)(base_col + marker_width + spaces);
    info->content_start = scan;
  }

  info->start_num = start_num;
  info->type = type;
  return 1;
}

/** Test for HTML block start. Returns HTML block type (1-7) or 0.
 *
 *  Types 1-6 can interrupt a paragraph. Type 7 cannot.
 *  The `can_interrupt_para` parameter controls which types are checked. */
/** Case-insensitive prefix match. Returns pointer past match or NULL. */
FIO_SFUNC char *fio___gfm_ci_match(char *p, char *le, const char *word) {
  while (*word && p < le) {
    if (fio___gfm_tolower(*p) != *word)
      return NULL;
    ++p;
    ++word;
  }
  return (*word == 0) ? p : NULL;
}

/** Check if tag name matches a block-level tag (GFM spec section 4.6 type 6).
 *  `p` points at first char of tag name, `le` is line end.
 *  Returns pointer past the tag name, or NULL if not a block tag. */
FIO_SFUNC char *fio___gfm_is_block_tag(char *p, char *le) {
  /* Must be at least 1 char */
  if (p >= le)
    return NULL;
  /* Extract tag name (lowercase ASCII letters and digits) */
  char *start = p;
  while (p < le && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                    (*p >= '0' && *p <= '9')))
    ++p;
  size_t len = (size_t)(p - start);
  if (!len || len > 10)
    return NULL;
  /* Build lowercase copy */
  char tag[11];
  for (size_t i = 0; i < len; ++i)
    tag[i] = (char)fio___gfm_tolower(start[i]);
  tag[len] = 0;
  /* Check against block-level tags (sorted for readability, linear scan) */
  static const char *const tags[] = {
      "address",    "article",  "aside",   "base",      "basefont",
      "blockquote", "body",     "caption", "center",    "col",
      "colgroup",   "dd",       "details", "dialog",    "dir",
      "div",        "dl",       "dt",      "fieldset",  "figcaption",
      "figure",     "footer",   "form",    "frame",     "frameset",
      "h1",         "h2",       "h3",      "h4",        "h5",
      "h6",         "head",     "header",  "hr",        "html",
      "iframe",     "legend",   "li",      "link",      "main",
      "menu",       "menuitem", "nav",     "noframes",  "ol",
      "optgroup",   "option",   "p",       "param",     "search",
      "section",    "summary",  "table",   "tbody",     "td",
      "tfoot",      "th",       "thead",   "title",     "tr",
      "track",      "ul",       NULL};
  for (int i = 0; tags[i]; ++i) {
    if (len == FIO_STRLEN(tags[i]) && FIO_MEMCMP(tag, tags[i], len) == 0)
      return p;
  }
  return NULL;
}

FIO_SFUNC int fio___gfm_is_html_block_start(char *p,
                                            char *le,
                                            int can_interrupt_para) {
  /* GFM spec section 4.6: HTML blocks, types 1-7. */
  if (p >= le || *p != '<')
    return 0;
  char *after = p + 1;

  /* Type 2: <!-- */
  if (le - after >= 3 && after[0] == '!' && after[1] == '-' && after[2] == '-')
    return 2;

  /* Type 3: <? */
  if (after < le && *after == '?')
    return 3;

  /* Type 5: <![CDATA[ */
  if (fio___gfm_ci_match(after, le, "![cdata["))
    return 5;

  /* Type 4: <! + uppercase letter */
  if (le - after >= 2 && after[0] == '!' && after[1] >= 'A' && after[1] <= 'Z')
    return 4;

  /* Type 1: <script, <pre, <style (case-insensitive) + whitespace/>/>  */
  {
    char *m;
    if ((m = fio___gfm_ci_match(after, le, "script")) ||
        (m = fio___gfm_ci_match(after, le, "pre")) ||
        (m = fio___gfm_ci_match(after, le, "style"))) {
      if (m >= le || *m == ' ' || *m == '\t' || *m == '>' || *m == '/')
        return 1;
    }
  }

  /* Type 6: </ or < + block-level tag + whitespace/>/> */
  {
    char *tag_start = after;
    int closing = 0;
    if (tag_start < le && *tag_start == '/') {
      closing = 1;
      ++tag_start;
    }
    char *tag_end = fio___gfm_is_block_tag(tag_start, le);
    if (tag_end) {
      if (tag_end >= le || *tag_end == ' ' || *tag_end == '\t' ||
          *tag_end == '>' || *tag_end == '/' ||
          (closing && *tag_end == '>'))
        return 6;
    }
  }

  /* Type 7: complete open/close tag on its own line (no script/pre/style).
   * Only when can_interrupt_para is set (= NOT interrupting a paragraph).
   * Tag name: letter followed by letters/digits/hyphens only. Must end at
   * whitespace, '>', '/>' or attributes. Autolinks (<scheme://...>) are NOT
   * valid HTML tags and must not match here. */
  if (can_interrupt_para) {
    char *t = after;
    int is_close = 0;
    if (t < le && *t == '/') {
      is_close = 1;
      ++t;
    }
    if (t < le && ((*t >= 'a' && *t <= 'z') || (*t >= 'A' && *t <= 'Z'))) {
      char *tag_name = t;
      /* Tag name: letters, digits, hyphens */
      while (t < le && ((*t >= 'a' && *t <= 'z') || (*t >= 'A' && *t <= 'Z') ||
                        (*t >= '0' && *t <= '9') || *t == '-'))
        ++t;
      /* Tag name must be non-empty and followed by valid continuation */
      if (t > tag_name && t < le) {
        if (is_close) {
          /* Closing tag: optional whitespace then > */
          while (t < le && (*t == ' ' || *t == '\t'))
            ++t;
          if (t < le && *t == '>') {
            ++t;
            while (t < le && (*t == ' ' || *t == '\t'))
              ++t;
            if (t >= le)
              return 7;
          }
        } else {
          /* Open tag: after tag name, must see whitespace, '>', or '/' */
          if (*t == ' ' || *t == '\t' || *t == '>' || *t == '/') {
            while (t < le && *t != '>')
              ++t;
            if (t < le && *t == '>') {
              ++t;
              while (t < le && (*t == ' ' || *t == '\t'))
                ++t;
              if (t >= le)
                return 7;
            }
          }
        }
      }
    }
  }

  return 0;
}

/** Check if a line contains an HTML block close condition for the given type.
 *  Returns 1 if the close condition is found, 0 otherwise.
 *  Types 6-7 close on blank line, not by scanning content. */
FIO_SFUNC int fio___gfm_html_block_has_close(char *p, char *le, int html_type) {
  switch (html_type) {
  case 1: /* </script>, </pre>, </style> */
    for (char *s = p; s + 2 < le; ++s) {
      if (*s == '<' && s[1] == '/') {
        if (fio___gfm_ci_match(s + 2, le, "script>") ||
            fio___gfm_ci_match(s + 2, le, "pre>") ||
            fio___gfm_ci_match(s + 2, le, "style>"))
          return 1;
      }
    }
    return 0;
  case 2: /* --> */
    for (char *s = p; s + 2 < le; ++s) {
      if (s[0] == '-' && s[1] == '-' && s[2] == '>')
        return 1;
    }
    return 0;
  case 3: /* ?> */
    for (char *s = p; s + 1 < le; ++s) {
      if (s[0] == '?' && s[1] == '>')
        return 1;
    }
    return 0;
  case 4: /* > */
    for (char *s = p; s < le; ++s) {
      if (*s == '>')
        return 1;
    }
    return 0;
  case 5: /* ]]> */
    for (char *s = p; s + 2 < le; ++s) {
      if (s[0] == ']' && s[1] == ']' && s[2] == '>')
        return 1;
    }
    return 0;
  default: return 0;
  }
}

/** Test for setext heading underline: line of '=' or '-' only.
 *  Returns heading level (1 for '=', 2 for '-') or 0.
 *  Only valid when a paragraph is open. */
FIO_SFUNC int fio___gfm_is_setext_underline(char *p, char *le) {
  /* GFM spec section 4.3: line of '=' (level 1) or '-' (level 2). */
  if (p >= le)
    return 0;
  char ch = *p;
  if (ch != '=' && ch != '-')
    return 0;
  while (p < le && *p == ch)
    ++p;
  /* Trailing whitespace is allowed */
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  if (p < le)
    return 0; /* non-matching chars remain */
  return (ch == '=') + (ch == '-') * 2;
}

/** Advance past one table cell (pipe-separated).
 *  Handles backslash escapes and backtick code spans so that escaped
 *  pipes and pipes inside code are not treated as separators.
 *  Returns pointer to the pipe separator or le if end of line.
 *  Sets *cell_start and *cell_end to the trimmed cell content. */
FIO_SFUNC char *fio___gfm_table_cell_next(char *p,
                                           char *le,
                                           char **cell_start,
                                           char **cell_end) {
  char *cs = p;
  while (p < le) {
    if (*p == '\\' && p + 1 < le) {
      p += 2;
      continue;
    }
    if (*p == '`') {
      char *bq = p;
      while (p < le && *p == '`')
        ++p;
      uint16_t bq_len = (uint16_t)(p - bq);
      while (p < le) {
        if (*p == '`') {
          char *cr = p;
          while (p < le && *p == '`')
            ++p;
          if ((uint16_t)(p - cr) == bq_len)
            break;
        } else {
          ++p;
        }
      }
      continue;
    }
    if (*p == '|')
      break;
    ++p;
  }
  char *ce = p;
  while (ce > cs && (ce[-1] == ' ' || ce[-1] == '\t'))
    --ce;
  while (cs < ce && (*cs == ' ' || *cs == '\t'))
    ++cs;
  *cell_start = cs;
  *cell_end = ce;
  return p;
}

/** Count pipe-separated cells in a table row (header or body).
 *  Handles leading/trailing pipes. Used to validate header column count. */
FIO_SFUNC uint16_t fio___gfm_count_table_cells(char *p, char *le) {
  uint16_t cols = 0;
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  p += (p < le && *p == '|');
  while (p < le) {
    char *cs, *ce;
    p = fio___gfm_table_cell_next(p, le, &cs, &ce);
    ++cols;
    if (p < le && *p == '|') {
      ++p;
      char *rest = p;
      while (rest < le && (*rest == ' ' || *rest == '\t'))
        ++rest;
      if (rest >= le)
        break;
    } else {
      break;
    }
  }
  return cols;
}

/** Test if a line is a valid table delimiter row.
 *  Returns column count or 0 if not a delimiter row.
 *  Parses alignment markers and stores them in st->table_align[]. */
FIO_SFUNC uint16_t fio___gfm_is_table_delimiter(fio___gfm_parser_s *st,
                                                char *p,
                                                char *le) {
  uint16_t cols = 0;
  /* Skip leading whitespace */
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  /* Optional leading pipe */
  p += (p < le && *p == '|');

  while (p < le) {
    /* Skip whitespace before cell */
    while (p < le && (*p == ' ' || *p == '\t'))
      ++p;
    if (p >= le)
      break;

    /* Parse delimiter cell: optional ':', one+ '-', optional ':' */
    int has_left = (p < le && *p == ':');
    p += has_left;

    int dashes = 0;
    while (p < le && *p == '-') {
      ++dashes;
      ++p;
    }
    if (!dashes)
      return 0; /* each cell must have at least one dash */

    int has_right = (p < le && *p == ':');
    p += has_right;

    uint8_t align = (has_left && has_right) ? FIO_GFM_ALIGN_CENTER
                  : has_left                ? FIO_GFM_ALIGN_LEFT
                  : has_right               ? FIO_GFM_ALIGN_RIGHT
                                            : FIO_GFM_ALIGN_NONE;

    /* Skip whitespace after cell */
    while (p < le && (*p == ' ' || *p == '\t'))
      ++p;

    /* Store alignment */
    if (cols < FIO_GFM_MAX_TABLE_COLUMNS)
      FIO___GFM_TABLE_ALIGN_SET(st, cols, align);
    ++cols;

    /* Expect pipe or end of line */
    if (p < le && *p == '|') {
      ++p;
      /* Check if this was a trailing pipe */
      char *rest = p;
      while (rest < le && (*rest == ' ' || *rest == '\t'))
        ++rest;
      if (rest >= le)
        break;
    } else if (p < le) {
      return 0; /* unexpected character */
    }
  }

  if (cols)
    st->table_columns = cols;
  return cols;
}

/* ===========================================================================
 * Container Operations
 *
 * These manage the nesting stack: pushing new containers, popping closed
 * ones, and closing all containers deeper than `matched_depth`.
 * ========================================================================= */

/** Push a blockquote container onto the nesting stack and emit PUSH event.
 *  `content_after` points to the position after '>' + optional space. */
FIO_SFUNC int fio___gfm_push_blockquote(fio___gfm_parser_s *st,
                                        uint16_t content_col) {
  FIO___GFM_PUSH(st, FIO___GFM_CONT_BQ, 0);
  if (st->err)
    return st->err;
  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_BLOCKQUOTE;
  (void)content_col;
  return fio___gfm_emit_push(st, &e);
}

/** Push a new list + list item onto the nesting stack.
 *
 *  Pushes TWO entries: LIST container + first LIST_ITEM.
 *  Tight/loose determined by lookahead, passed on the LIST push event.
 *  list_start passed on the LIST push event, not stored in nest[].
 *
 *  Emits: PUSH(LIST_ORDERED/LIST_UNORDERED), PUSH(LIST_ITEM). */
/** Lookahead: check if the list starting at the current position is tight.
 *  Scans forward to the second list item. If a blank line is found between
 *  items 1 and 2 (outside nested containers/code), the list is loose. */
FIO_SFUNC uint8_t fio___gfm_list_lookahead_tight(fio___gfm_parser_s *st,
                                                 fio___gfm_list_marker_s *info) {
  /* Scan forward from the current position to find the second item.
   * If a blank line appears before the second item, the list is loose.
   * For single-item lists, if a blank line appears between two
   * block-level elements within the item, the list is also loose
   * (GFM spec: "directly contain two block-level elements with a
   * blank line between them"). */
  char *p = info->content_start;
  char *end = st->end;
  int saw_blank = 0;
  int content_after_blank = 0;
  int in_fence = 0;

  while (p < end) {
    char *le = fio___gfm_line_end(p, end);
    char *next = fio___gfm_line_next(le, end);

    if (in_fence) {
      /* Inside fenced code — check for closing fence */
      char *ft = fio___gfm_ltrim(p, le);
      if (ft < le && (*ft == '`' || *ft == '~'))
        in_fence = 0; /* simplified: any fence-like line ends code */
      p = next;
      continue;
    }

    if (fio___gfm_is_blank(p, le)) {
      saw_blank = 1;
      p = next;
      continue;
    }

    /* Non-blank content after a blank → potential loose trigger */
    content_after_blank |= saw_blank;

    /* Check for fence open */
    char *ft = fio___gfm_ltrim(p, le);
    if (ft < le && (*ft == '`' || *ft == '~')) {
      char fc;
      if (fio___gfm_is_fenced_code_open(ft, le, &fc))
        in_fence = 1;
    }

    /* Check if this line starts a new item of the same type */
    uint32_t vcol = fio___gfm_indent(p, le);
    if (vcol <= FIO___GFM_MAX_MARKER_INDENT) {
      fio___gfm_list_marker_s m2;
      if (fio___gfm_is_list_marker(ft, le, vcol, &m2) && m2.type == info->type)
        return saw_blank ? 0 : FIO_GFM_F_TIGHT;
    }

    p = next;
  }
  /* Single-item list: loose if blank line separates block elements. */
  return content_after_blank ? 0 : FIO_GFM_F_TIGHT;
}

FIO_SFUNC int fio___gfm_push_list_and_item(fio___gfm_parser_s *st,
                                           fio___gfm_list_marker_s *info) {
  int r;
  if (st->depth + 2 > FIO_GFM_MAX_DEPTH) {
    st->err = FIO_GFM_ERR_DEPTH;
    return st->err;
  }

  /* Lookahead for tight/loose */
  uint8_t tight = fio___gfm_list_lookahead_tight(st, info);

  /* Push LIST container */
  FIO___GFM_PUSH(st, info->type, 0);
  if (st->err)
    return st->err;
  /* Store tight/loose in the LIST container's flags */
  st->nest[st->depth - 1].flags = tight;

  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = fio___gfm_cont_public_type(info->type);
  e.list_start = info->start_num;
  e.flags = tight;
  r = fio___gfm_emit_push(st, &e);
  if (r)
    return r;

  /* Push LIST_ITEM — propagate tight flag from parent LIST. */
  FIO___GFM_PUSH(st, FIO___GFM_CONT_LI, info->content_indent);
  if (st->err)
    return st->err;
  st->nest[st->depth - 1].flags = tight;

  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_LIST_ITEM;
  e.flags = tight;
  return fio___gfm_emit_push(st, &e);
}

/** Push a new list item into an existing list.
 *
 *  Closes the current LIST_ITEM (and any nested containers),
 *  then pushes a new LIST_ITEM.
 *
 *  Emits: POP(LIST_ITEM) [+ POPs for nested], PUSH(LIST_ITEM). */
FIO_SFUNC int fio___gfm_push_new_item(fio___gfm_parser_s *st,
                                      uint16_t list_depth,
                                      fio___gfm_list_marker_s *info) {
  int r = 0;

  /* Close open leaf first */
  if (st->leaf_type) {
    r = fio___gfm_close_leaf(st);
    if (r)
      return r;
  }

  /* Pop containers from innermost down to (but not including) the LIST.
   * list_depth is 1-indexed (nest[list_depth-1] is the LIST).
   * We need to pop everything deeper: LI + any nested containers. */
  while (FIO___GFM_DEPTH(st) > list_depth) {
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = fio___gfm_cont_public_type(FIO___GFM_TOP_TYPE(st));
    /* Propagate tight flag on LIST_ITEM pops */
    e.flags = st->nest[st->depth - 1].flags;
    r = fio___gfm_emit_pop(st, &e);
    FIO___GFM_POP(st);
    if (r) {
      st->err = r;
      return r;
    }
  }

  /* Get tight flag from parent LIST container */
  uint8_t tight = st->nest[list_depth - 1].flags & FIO_GFM_F_TIGHT;

  /* Push new LIST_ITEM — propagate tight from parent LIST. */
  FIO___GFM_PUSH(st, FIO___GFM_CONT_LI, info->content_indent);
  if (st->err)
    return st->err;
  st->nest[st->depth - 1].flags = tight;

  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_LIST_ITEM;
  e.flags = tight;
  return fio___gfm_emit_push(st, &e);
}

/** Close the open leaf block (paragraph, fenced code, etc.).
 *
 *  For paragraphs: extracts reference definitions first, then emits
 *  the paragraph (or nothing if all text was ref defs).
 *  For other leaves: emits POP event. */
FIO_SFUNC int fio___gfm_close_leaf(fio___gfm_parser_s *st) {
  if (!st->leaf_type)
    return 0;
  int r = 0;
  switch (st->leaf_type) {
  case FIO_GFM_PARAGRAPH:
    r = fio___gfm_close_paragraph(st);
    return r;
  case FIO_GFM_CODE_BLOCK: {
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_CODE_BLOCK;
    r = fio___gfm_emit_pop(st, &e);
    st->leaf_type = 0;
    st->fence_char = 0;
    st->fence_len = 0;
    st->fence_indent = 0;
    st->fence_info = (fio_buf_info_s){0};
    st->ic_pending_blanks = 0;
    return r;
  }
  case FIO_GFM_HTML_BLOCK: {
    /* TODO: implement HTML block close */
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_HTML_BLOCK;
    r = fio___gfm_emit_pop(st, &e);
    st->leaf_type = 0;
    st->leaf_html_type = 0;
    return r;
  }
  case FIO_GFM_TABLE: {
    /* TODO: implement table close */
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_TABLE;
    r = fio___gfm_emit_pop(st, &e);
    st->leaf_type = 0;
    st->table_columns = 0;
    return r;
  }
  default: break;
  }
  st->leaf_type = 0;
  return r;
}

/** Close all containers deeper than `matched_depth`.
 *
 *  This is called in Step 2 when we've determined that unmatched
 *  containers must be closed (not a lazy continuation line).
 *
 *  Closes from innermost to outermost, emitting POP events.
 *  Also closes any open leaf first. */
FIO_SFUNC int fio___gfm_close_unmatched(fio___gfm_parser_s *st) {
  int r = 0;

  /* Close open leaf first (paragraph, code, etc.) */
  if (st->leaf_type) {
    r = fio___gfm_close_leaf(st);
    if (r)
      return r;
  }

  /* Pop containers from innermost to matched_depth.
   * Propagate flags (tight/loose) on POP events so the renderer
   * can suppress <p> tags in tight list items. */
  while (FIO___GFM_DEPTH(st) > st->matched_depth) {
    fio_gfm_event_s e = {
        .udata = st->udata,
        .type = fio___gfm_cont_public_type(FIO___GFM_TOP_TYPE(st)),
        .flags = st->nest[st->depth - 1].flags,
    };
    r = fio___gfm_emit_pop(st, &e);
    FIO___GFM_POP(st);
    if (r) {
      st->err = r;
      return r;
    }
  }
  return 0;
}

/** Close orphaned LIST containers at the top of the nesting stack.
 *  When an LI is closed by close_unmatched but the parent LIST survives
 *  (because LIST containers always match in Step 1), the LIST should
 *  also close for document-level blocks (thematic break, heading, etc.)
 *  that shouldn't be nested inside a list. */
FIO_SFUNC int fio___gfm_close_orphan_lists(fio___gfm_parser_s *st) {
  int r = 0;
  while (FIO___GFM_DEPTH(st) > 0) {
    uint8_t t = FIO___GFM_TOP_TYPE(st);
    if (t < FIO___GFM_CONT_UL_DASH || t > FIO___GFM_CONT_OL_PAREN)
      break;
    fio_gfm_event_s e = {
        .udata = st->udata,
        .type = fio___gfm_cont_public_type(t),
        .flags = st->nest[st->depth - 1].flags,
    };
    r = fio___gfm_emit_pop(st, &e);
    FIO___GFM_POP(st);
    if (r) { st->err = r; return r; }
  }
  return 0;
}

/** Close ALL open blocks. Called at end of document. */
FIO_SFUNC int fio___gfm_close_all(fio___gfm_parser_s *st) {
  st->matched_depth = 0;
  return fio___gfm_close_unmatched(st);
}

/* ===========================================================================
 * Paragraph Operations
 * ========================================================================= */

/** Open a new paragraph starting at `content`. */
FIO_SFUNC void fio___gfm_open_paragraph(fio___gfm_parser_s *st,
                                        char *content,
                                        char *le) {
  /* NOTE: No PUSH event is emitted here. The PUSH is deferred until
   * paragraph close, because:
   *   1. The paragraph might be retroactively converted to a setext heading.
   *   2. The paragraph might be retroactively converted to a table.
   *   3. Leading lines might be reference definitions, not paragraph text.
   *
   * We just accumulate text and resolve the type on close.
   */
  st->leaf_type = FIO_GFM_PARAGRAPH;
  st->para_open = 1;
  st->para_start = content;
  st->para_end = le;
  st->leaf_start = content;
}

/** Append a line to the open paragraph. */
FIO_SFUNC void fio___gfm_append_paragraph(fio___gfm_parser_s *st,
                                          char *le_or_next) {
  /* Extend the paragraph's accumulated text to include this line.
   * `le_or_next` should be the line_next position (past the newline)
   * so that the newline is included in the accumulated text. */
  st->para_end = le_or_next;
}

/** Close the open paragraph. Extracts ref defs, emits paragraph events. */
FIO_SFUNC int fio___gfm_close_paragraph(fio___gfm_parser_s *st) {
  int r = 0;
  char *text = st->para_start;
  char *text_end = st->para_end;

  for (;;) {
    fio_buf_info_s lbl, dst, ttl;
    char *after = fio___gfm_try_parse_ref_def(text, text_end, &lbl, &dst, &ttl);
    if (!after)
      break;
    fio___gfm_ref_cache_add(st, lbl, dst, ttl);
    text = after;
  }

  /* Left-trim leading whitespace from first line */
  while (text < text_end && (*text == ' ' || *text == '\t'))
    ++text;
  /* Right-trim trailing whitespace/newlines from paragraph text */
  while (text_end > text &&
         (text_end[-1] == '\n' || text_end[-1] == '\r' ||
          text_end[-1] == ' ' || text_end[-1] == '\t'))
    --text_end;

  /* Emit paragraph if any text remains after ref def extraction */
  if (text < text_end) {
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_PARAGRAPH;
    e.source = FIO_BUF_INFO2(text, (size_t)(text_end - text));
    r = fio___gfm_emit_push(st, &e);
    if (!r)
      r = fio___gfm_inline_parse(st, text, text_end);
    if (!r) {
      fio___gfm_event_init(st, &e);
      e.type = FIO_GFM_PARAGRAPH;
      r = fio___gfm_emit_pop(st, &e);
    }
  }

  st->leaf_type = 0;
  st->para_open = 0;
  st->para_start = NULL;
  st->para_end = NULL;
  return r;
}

/** Convert open paragraph to a setext heading. */
FIO_SFUNC int fio___gfm_convert_to_setext(fio___gfm_parser_s *st,
                                          char *underline,
                                          char *le,
                                          int level) {
  int r = 0;
  char *text = st->para_start;
  char *text_end = st->para_end;
  (void)underline;
  (void)le;

  /* TODO: extract leading ref defs (enable when ref cache is implemented) */

  /* Left-trim leading whitespace */
  while (text < text_end && (*text == ' ' || *text == '\t'))
    ++text;
  /* Right-trim paragraph text */
  while (text_end > text &&
         (text_end[-1] == '\n' || text_end[-1] == '\r' ||
          text_end[-1] == ' ' || text_end[-1] == '\t'))
    --text_end;

  if (text < text_end) {
    /* Emit as heading */
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_HEADING;
    e.heading_level = (uint8_t)level;
    e.source = FIO_BUF_INFO2(text, (size_t)(text_end - text));
    r = fio___gfm_emit_push(st, &e);
    if (!r)
      r = fio___gfm_inline_parse(st, text, text_end);
    if (!r) {
      fio___gfm_event_init(st, &e);
      e.type = FIO_GFM_HEADING;
      e.heading_level = (uint8_t)level;
      r = fio___gfm_emit_pop(st, &e);
    }
  } else if (level == 2) {
    /* All text was ref defs and underline is '---' → thematic break */
    fio_gfm_event_s e;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_THEMATIC_BREAK;
    r = fio___gfm_emit_push(st, &e);
    if (!r) {
      fio___gfm_event_init(st, &e);
      e.type = FIO_GFM_THEMATIC_BREAK;
      r = fio___gfm_emit_pop(st, &e);
    }
  }
  /* If '=' and all ref defs → underline is ignored (per spec) */

  st->leaf_type = 0;
  st->para_open = 0;
  st->para_start = NULL;
  st->para_end = NULL;
  return r;
}

/** Convert open paragraph to a table (GFM extension). */
FIO_SFUNC int fio___gfm_convert_to_table(fio___gfm_parser_s *st,
                                         char *delim_line,
                                         char *delim_le) {
  int r = 0;
  char *text = st->para_start;
  char *text_end = st->para_end;
  (void)delim_line;
  (void)delim_le;

  /* Paragraph must be a single line (no embedded newlines).
   * Multi-line paragraphs cannot become table headers. */
  for (char *s = text; s < text_end; ++s) {
    if (*s == '\n' || *s == '\r')
      return 0;
  }

  /* Header column count must match delimiter column count
   * (is_table_delimiter already stored count in st->table_columns). */
  uint16_t header_cols = fio___gfm_count_table_cells(text, text_end);
  if (header_cols != st->table_columns || !header_cols)
    return 0;

  /* --- Conversion confirmed: emit TABLE + header row --- */

  /* Clear paragraph state */
  st->para_open = 0;
  st->para_start = NULL;
  st->para_end = NULL;

  /* Emit PUSH(TABLE) */
  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_TABLE;
  e.columns = st->table_columns;
  r = fio___gfm_emit_push(st, &e);
  if (r)
    return r;

  /* Emit header row (cells parsed via inline_parse) */
  r = fio___gfm_emit_table_row(st, text, text_end);
  if (r)
    return r;

  /* Set leaf type for body row processing by the main loop */
  st->leaf_type = FIO_GFM_TABLE;
  return 0;
}

/* ===========================================================================
 * Blank Line Handler
 * ========================================================================= */

/** Handle a blank line. Context-dependent behavior. */
FIO_SFUNC int fio___gfm_handle_blank_line(fio___gfm_parser_s *st) {
  switch (st->leaf_type) {
  case FIO_GFM_PARAGRAPH:
    return fio___gfm_close_paragraph(st);
  case FIO_GFM_CODE_BLOCK:
    /* Blank line inside code block — part of content.
     * Fenced: always continues. Indented: trailing blanks stripped on close.
     * TODO: emit blank line as text when code block emission is wired. */
    return 0;
  case FIO_GFM_HTML_BLOCK:
    /* Types 6-7: blank line closes HTML block. Types 1-5: continues. */
    if (st->leaf_html_type >= 6)
      return fio___gfm_close_leaf(st);
    return 0;
  case FIO_GFM_TABLE:
    return fio___gfm_close_leaf(st);
  default: break;
  }
  /* No open leaf — mark enclosing list as loose if inside a list item. */
  for (uint16_t d = st->depth; d > 0; --d) {
    uint8_t t = st->nest[d - 1].type;
    if (t == FIO___GFM_CONT_LI)
      continue;
    if (t >= FIO___GFM_CONT_UL_DASH && t <= FIO___GFM_CONT_OL_PAREN) {
      st->nest[d - 1].flags |= FIO_GFM_F_LOOSE_SEEN;
      break;
    }
    break;
  }
  return 0;
}

/* ===========================================================================
 * Reference Cache
 * ========================================================================= */

/** Compare two reference labels with GFM normalization:
 *  ASCII case-fold, collapse internal whitespace to single space,
 *  strip leading/trailing whitespace. Returns 1 if equal, 0 otherwise. */
FIO_SFUNC int fio___gfm_label_eq(fio_buf_info_s a, fio_buf_info_s b) {
  char *ap = a.buf, *ae = a.buf + a.len;
  char *bp = b.buf, *be = b.buf + b.len;
  /* skip leading whitespace */
  while (ap < ae && (*ap == ' ' || *ap == '\t' || *ap == '\n' || *ap == '\r'))
    ++ap;
  while (bp < be && (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r'))
    ++bp;
  /* strip trailing whitespace */
  while (ae > ap && (ae[-1] == ' ' || ae[-1] == '\t' || ae[-1] == '\n' ||
                     ae[-1] == '\r'))
    --ae;
  while (be > bp && (be[-1] == ' ' || be[-1] == '\t' || be[-1] == '\n' ||
                     be[-1] == '\r'))
    --be;
  /* walk both labels comparing with case-fold and whitespace collapse */
  while (ap < ae && bp < be) {
    int a_ws = (*ap == ' ' || *ap == '\t' || *ap == '\n' || *ap == '\r');
    int b_ws = (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r');
    if (a_ws && b_ws) {
      /* collapse both runs */
      while (ap < ae && (*ap == ' ' || *ap == '\t' || *ap == '\n' ||
                         *ap == '\r'))
        ++ap;
      while (bp < be && (*bp == ' ' || *bp == '\t' || *bp == '\n' ||
                         *bp == '\r'))
        ++bp;
      continue;
    }
    if (a_ws != b_ws)
      return 0;
    if (fio___gfm_tolower(*ap) != fio___gfm_tolower(*bp))
      return 0;
    ++ap;
    ++bp;
  }
  return (ap == ae) & (bp == be);
}

/** Add a reference definition to the cache. First definition wins. */
FIO_SFUNC void fio___gfm_ref_cache_add(fio___gfm_parser_s *st,
                                       fio_buf_info_s label,
                                       fio_buf_info_s dest,
                                       fio_buf_info_s title) {
  /* First definition wins: check if label already exists. */
  for (uint16_t i = 0; i < st->ref_count; ++i) {
    if (fio___gfm_label_eq(st->refs[i].label, label))
      return;
  }
  if (st->ref_count < FIO_GFM_REF_CACHE_SIZE) {
    st->refs[st->ref_count++] = (fio___gfm_ref_s){
        .label = label,
        .destination = dest,
        .title = title,
    };
  } else {
    st->ref_overflow = 1;
  }
}

/** Try to parse a reference definition from text.
 *  Returns pointer past the ref def on success, NULL on failure.
 *  On success, fills *label, *dest, *title. */
FIO_SFUNC char *fio___gfm_try_parse_ref_def(char *p,
                                            char *end,
                                            fio_buf_info_s *label,
                                            fio_buf_info_s *dest,
                                            fio_buf_info_s *title) {
  char *s = p;
  /* 1. Skip 0-3 spaces indent */
  {
    int sp = 0;
    while (s < end && (*s == ' ' || *s == '\t') && sp < 4) {
      sp += (*s == '\t') ? (4 - (sp & 3)) : 1;
      s += (sp <= 3);
      if (sp > 3) break;
    }
    if (sp > 3)
      return NULL;
  }
  /* 2. Parse [label] */
  if (s >= end || *s != '[')
    return NULL;
  ++s;
  char *lbl_start = s;
  int lbl_empty = 1;
  while (s < end && *s != ']') {
    if (*s == '[')
      return NULL; /* no nested unescaped [ */
    if (*s == '\\' && s + 1 < end) {
      lbl_empty = 0;
      s += 2;
      continue;
    }
    lbl_empty &= (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r');
    ++s;
  }
  if (s >= end || lbl_empty)
    return NULL;
  char *lbl_end = s;
  ++s; /* skip ']' */
  /* 3. Expect ':' immediately after ']' */
  if (s >= end || *s != ':')
    return NULL;
  ++s;
  /* 4. Optional whitespace (including up to one line ending) */
  while (s < end && (*s == ' ' || *s == '\t'))
    ++s;
  if (s < end && (*s == '\n' || *s == '\r')) {
    s += (s < end && *s == '\r');
    s += (s < end && *s == '\n');
    while (s < end && (*s == ' ' || *s == '\t'))
      ++s;
  }
  /* 5. Parse destination */
  char *dst_start = NULL, *dst_end = NULL;
  if (s < end && *s == '<') {
    /* angle-bracket form */
    ++s;
    dst_start = s;
    while (s < end && *s != '>' && *s != '\n' && *s != '\r') {
      if (*s == '\\' && s + 1 < end) {
        s += 2;
        continue;
      }
      if (*s == '<')
        return NULL;
      ++s;
    }
    if (s >= end || *s != '>')
      return NULL;
    dst_end = s;
    ++s; /* skip '>' */
  } else {
    /* bare destination: balanced parens, no spaces */
    dst_start = s;
    int paren_depth = 0;
    while (s < end && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') {
      if (*s == '\\' && s + 1 < end) {
        s += 2;
        continue;
      }
      if (*s == '(') {
        ++paren_depth;
      } else if (*s == ')') {
        if (paren_depth == 0)
          break;
        --paren_depth;
      }
      /* ASCII control chars invalid in bare destination */
      if ((unsigned char)*s < 0x20)
        return NULL;
      ++s;
    }
    if (paren_depth != 0)
      return NULL;
    dst_end = s;
    if (dst_start == dst_end)
      return NULL; /* bare form requires non-empty destination */
  }
  /* 6. Optional whitespace (including up to one line ending).
   *    Save position before title attempt so we can fall back. */
  char *before_title_ws = s;
  while (s < end && (*s == ' ' || *s == '\t'))
    ++s;
  char *ttl_start = NULL, *ttl_end = NULL;
  char *after_dest_line = s; /* position after dest + horizontal ws */
  int had_line_ending = 0;
  if (s < end && (*s == '\n' || *s == '\r')) {
    had_line_ending = 1;
    s += (*s == '\r');
    s += (s < end && *s == '\n');
    while (s < end && (*s == ' ' || *s == '\t'))
      ++s;
  }
  /* 7. Optional title in "...", '...', or (...) */
  if (s < end && (*s == '"' || *s == '\'' || *s == '(')) {
    char open_ch = *s;
    char close_ch = (open_ch == '(') ? ')' : open_ch;
    ++s;
    ttl_start = s;
    while (s < end) {
      if (*s == '\\' && s + 1 < end) {
        s += 2;
        continue;
      }
      if (*s == close_ch) {
        ttl_end = s;
        ++s;
        break;
      }
      /* check for blank line inside title — invalidates ref def */
      if (*s == '\n' || *s == '\r') {
        char *nl = s;
        nl += (*nl == '\r');
        nl += (nl < end && *nl == '\n');
        if (nl >= end || fio___gfm_is_blank(nl, fio___gfm_line_end(nl, end))) {
          /* blank line in title — title parse failed */
          ttl_start = NULL;
          ttl_end = NULL;
          break;
        }
      }
      ++s;
    }
    if (!ttl_end) {
      /* Title didn't close — no title. Fall back to position after dest. */
      ttl_start = NULL;
      s = after_dest_line;
      /* If we consumed a line ending, the ref def ended at end-of-dest line */
      if (had_line_ending)
        s = before_title_ws;
    }
  }
  /* 8. No further non-whitespace on the final line */
  if (ttl_start) {
    /* we have a title — rest of line must be whitespace */
    while (s < end && (*s == ' ' || *s == '\t'))
      ++s;
    if (s < end && *s != '\n' && *s != '\r')
      return NULL;
  } else {
    /* no title — rest of dest line must be whitespace */
    s = after_dest_line;
    if (had_line_ending) {
      s = before_title_ws;
    }
    while (s < end && (*s == ' ' || *s == '\t'))
      ++s;
    if (s < end && *s != '\n' && *s != '\r')
      return NULL;
  }
  /* skip final line ending */
  if (s < end && (*s == '\n' || *s == '\r')) {
    s += (*s == '\r');
    s += (s < end && *s == '\n');
  }
  /* Fill outputs */
  *label = FIO_BUF_INFO2(lbl_start, (size_t)(lbl_end - lbl_start));
  *dest = FIO_BUF_INFO2(dst_start, (size_t)(dst_end - dst_start));
  if (ttl_start && ttl_end)
    *title = FIO_BUF_INFO2(ttl_start, (size_t)(ttl_end - ttl_start));
  else
    *title = (fio_buf_info_s){0};
  return s;
}

/** Resolve a reference label to its definition.
 *  Returns pointer to the ref entry, or NULL if not found.
 *  Triggers lazy document scan on first cache miss. */
FIO_SFUNC fio___gfm_ref_s *fio___gfm_ref_resolve(fio___gfm_parser_s *st,
                                                 fio_buf_info_s label) {
  /* Stage 1: Linear scan of cache. */
  for (uint16_t i = 0; i < st->ref_count; ++i) {
    if (fio___gfm_label_eq(st->refs[i].label, label))
      return &st->refs[i];
  }
  /* Stage 2: If not yet scanned, scan entire document. */
  if (!st->ref_scanned_to) {
    fio___gfm_ref_scan_document(st);
    for (uint16_t i = 0; i < st->ref_count; ++i) {
      if (fio___gfm_label_eq(st->refs[i].label, label))
        return &st->refs[i];
    }
  }
  return NULL;
}

/** Scan the entire document for reference definitions (lazy, one-shot). */
FIO_SFUNC void fio___gfm_ref_scan_document(fio___gfm_parser_s *st) {
  char *p = st->start;
  char *doc_end = st->end;
  int in_fence = 0;
  char fence_ch = 0;
  uint16_t fence_len = 0;

  while (p < doc_end) {
    char *le = fio___gfm_line_end(p, doc_end);
    char *next = fio___gfm_line_next(le, doc_end);

    /* Strip blockquote markers for scanning (simplified: one level) */
    char *line = p;
    while (line < le && (*line == ' ' || *line == '\t'))
      ++line;
    if (line < le && *line == '>') {
      ++line;
      line += (line < le && (*line == ' ' || *line == '\t'));
    }

    uint32_t ind = fio___gfm_indent(line, le);

    if (in_fence) {
      /* Check for closing fence */
      char *ft = fio___gfm_ltrim(line, le);
      if (ind <= FIO___GFM_MAX_MARKER_INDENT && ft < le && *ft == fence_ch) {
        uint16_t cnt = 0;
        char *fc = ft;
        while (fc < le && *fc == fence_ch) {
          ++cnt;
          ++fc;
        }
        /* closing fence: same char, >= opener len, rest is whitespace */
        if (cnt >= fence_len && fio___gfm_is_blank(fc, le))
          in_fence = 0;
      }
      p = next;
      continue;
    }

    /* Skip indented code (indent >= 4) */
    if (ind >= 4) {
      p = next;
      continue;
    }

    /* Check for fence open */
    {
      char *ft = fio___gfm_ltrim(line, le);
      if (ft < le && (*ft == '`' || *ft == '~')) {
        char fc;
        uint16_t fl = fio___gfm_is_fenced_code_open(ft, le, &fc);
        if (fl) {
          in_fence = 1;
          fence_ch = fc;
          fence_len = fl;
          p = next;
          continue;
        }
      }
    }

    /* Try ref def if line starts with '[' at indent <= 3 */
    {
      char *ft = fio___gfm_ltrim(line, le);
      if (ft < le && *ft == '[') {
        fio_buf_info_s lbl, dst, ttl;
        char *after = fio___gfm_try_parse_ref_def(line, doc_end, &lbl, &dst, &ttl);
        if (after) {
          fio___gfm_ref_cache_add(st, lbl, dst, ttl);
          /* A ref def might span multiple lines; advance past it */
          p = after;
          continue;
        }
      }
    }

    p = next;
  }
  st->ref_scanned_to = st->end;
}

/* ===========================================================================
 * Inline Parser (Forward-Scan — Single Pass, No Recursion)
 *
 * DOCUMENT-ORDER GUARANTEE:
 *   All callbacks (push/write/pop) are emitted in strict left-to-right
 *   document order. For "Hello **World**", the callback sequence is:
 *
 *     write(TEXT, "Hello ")
 *     push(STRONG)
 *     write(TEXT, "World")
 *     pop(STRONG)
 *
 *   This allows a renderer to sequentially append to a string buffer
 *   without any backtracking, delete, undo, or random-access patching.
 *
 * HOW IT WORKS (single pass, forward scanning):
 *
 *   Walk the text left-to-right. When a potential opener is found
 *   (*, _, ~~, [, ![), scan FORWARD to find its matching closer
 *   BEFORE emitting any push event. This resolves nesting order at
 *   discovery time — no speculative emission, no retroactive correction.
 *
 *   After emitting the push, the main loop continues scanning the
 *   INTERIOR of the construct. Interior emphasis, links, code spans
 *   etc. are handled by the same loop — no recursion needed. When the
 *   closer position is reached, the corresponding pop is emitted.
 *
 *   For combined delimiter runs like `***`:
 *     1. Forward-scan finds the first matching closer (e.g., `**`).
 *     2. This tells us: 2 chars consumed → STRONG, 1 remaining → EMPHASIS.
 *     3. Emit push(EMPHASIS), push(STRONG) in the correct nesting order.
 *     4. When `**` is reached: pop(STRONG).
 *     5. When `*` is reached: pop(EMPHASIS).
 *     No guessing, no resetting — the forward scan determines the
 *     nesting before any push is emitted.
 *
 *   Code spans, autolinks, HTML tags, escapes, entities, and breaks
 *   are resolved immediately during the scan (they are unambiguous).
 *
 * WHY FORWARD-SCAN INSTEAD OF DELIMITER STACK + TWO-PHASE:
 *   - Single pass: no temporary event array (saves ~16 KB stack).
 *   - No resolution phase: push/pop emitted immediately, in order.
 *   - No delete/undo needed: nesting resolved before committing.
 *   - Simpler mental model: "look ahead, then commit."
 *   - Delimiter stack shrinks to a simple open-section tracker.
 *
 * COMPLEXITY:
 *   O(n) for typical documents. O(n²) worst case for pathological
 *   inputs (many unclosed openers). Bounded by MAX_DELIM_STACK — when
 *   the stack overflows, excess delimiters emit as literal text.
 *
 * Entry point: fio___gfm_inline_parse()
 *   Called when a leaf closes (paragraph, heading, table cell).
 *   Processes the text region [start..end) for inline elements.
 * ========================================================================= */

/** Forward-scan for emphasis closer.
 *
 *  Given an opener with `marker` char and `opener_len` chars,
 *  scans [search_start..boundary) for a matching closer.
 *
 *  `boundary` is the closer position of the enclosing section, or
 *  end-of-text if no section is open. This is CRITICAL: inner constructs
 *  must not find closers past their parent's closer. Without this bound,
 *  overlapping spans like `<em>foo <strong>bar</em> baz</strong>` can
 *  occur. With the bound, `**` inside an `*` emphasis cannot match a
 *  `**` that lies past the `*` closer — so it becomes literal text.
 *
 *  Returns: pointer to the closer run, or NULL if no closer found.
 *  On success, *closer_len is set to the closer's run length.
 *
 *  Matching rules:
 *  - Closer must be a right-flanking delimiter run of the same char.
 *  - Rule of three: if closer can also open, (opener_len + closer_len)
 *    must not be a multiple of 3 unless both are multiples of 3.
 *  - For ~~: only exact length 2 matches.
 *
 *  The forward scan skips over code spans (backtick pairs) to avoid
 *  false matches inside code. */
FIO_SFUNC char *fio___gfm_find_closer(char *search_start,
                                      char *boundary,
                                      char marker,
                                      uint16_t opener_len,
                                      uint16_t *closer_len) {
  char *s = search_start;
  int depth = 0; /* tracks inner openers that consume closers */
  while (s < boundary) {
    /* Skip backtick code spans */
    if (*s == '`') {
      char *bq = s;
      while (s < boundary && *s == '`')
        ++s;
      uint16_t bq_len = (uint16_t)(s - bq);
      int found_close = 0;
      while (s < boundary) {
        if (*s == '`') {
          char *cr = s;
          while (s < boundary && *s == '`')
            ++s;
          if ((uint16_t)(s - cr) == bq_len) {
            found_close = 1;
            break;
          }
        } else {
          ++s;
        }
      }
      if (!found_close)
        s = bq + bq_len;
      continue;
    }
    /* Skip backslash escapes */
    if (*s == '\\' && s + 1 < boundary) {
      s += 2;
      continue;
    }
    /* Check for run of marker */
    if (*s == marker) {
      char *run_start = s;
      while (s < boundary && *s == marker)
        ++s;
      uint16_t run_len = (uint16_t)(s - run_start);
      /* Classify flanking */
      char before = (run_start > search_start) ? run_start[-1] : (char)marker;
      int before_ws = (before == ' ' || before == '\t' || before == '\n' ||
                       before == '\r');
      int before_punct = fio___gfm_is_punct(before);
      char after = (s < boundary) ? *s : ' ';
      int after_ws =
          (after == ' ' || after == '\t' || after == '\n' || after == '\r');
      int after_punct = fio___gfm_is_punct(after);
      int left_flanking = !after_ws && (!after_punct || before_ws || before_punct);
      int right_flanking = !before_ws && (!before_punct || after_ws || after_punct);

      /* Determine can_open / can_close per delimiter type */
      int can_open = left_flanking;
      int can_close = right_flanking;
      if (marker == '_') {
        can_open = left_flanking && (!right_flanking || before_punct);
        can_close = right_flanking && (!left_flanking || after_punct);
      }
      if (marker == '~') {
        can_close = can_close && (run_len == 2);
        can_open = can_open && (run_len == 2);
      }

      /* Nesting: pure opener increments depth, closer decrements */
      if (can_open && !can_close) {
        ++depth;
        continue;
      }
      if (!can_close)
        continue;

      /* This run can close. If inner openers consumed, decrement depth. */
      if (depth > 0) {
        --depth;
        continue;
      }

      /* Rule of three check */
      if (marker == '*' || marker == '_') {
        if (can_open) {
          uint16_t sum = opener_len + run_len;
          if ((sum % 3 == 0) && (opener_len % 3 != 0 || run_len % 3 != 0))
            continue;
        }
      }
      *closer_len = run_len;
      return run_start;
    }
    ++s;
  }
  return NULL;
}

/** Forward-scan for link/image: find ']' and validate link syntax.
 *
 *  Given a '[' or '![' at `opener`, scans forward for the matching ']',
 *  then checks for inline link `(url "title")`, reference `[label]`,
 *  collapsed `[]`, or shortcut (nothing after ']').
 *
 *  Returns: pointer past the full link construct on success (after the
 *  closing ')' or ']'), or NULL if this is not a valid link/image.
 *
 *  On success, fills *dest, *title, *ref_label with the link metadata.
 *  The link text spans [text_start..bracket_close). */
FIO_SFUNC char *fio___gfm_find_link(fio___gfm_parser_s *st,
                                    char *opener,
                                    char *end,
                                    int is_image,
                                    char **text_start,
                                    char **bracket_close,
                                    fio_buf_info_s *dest,
                                    fio_buf_info_s *title,
                                    fio_buf_info_s *ref_label) {
  (void)is_image;
  char *s = opener + 1; /* past '[' */
  *text_start = s;
  *dest = (fio_buf_info_s){0};
  *title = (fio_buf_info_s){0};
  *ref_label = (fio_buf_info_s){0};

  /* Scan for unescaped ']' — handle escapes, code spans, and images.
   * Images (![...]) are allowed inside link text; bare '[' is not. */
  while (s < end && *s != ']') {
    if (*s == '\\' && s + 1 < end) {
      s += 2;
      continue;
    }
    if (*s == '!' && s + 1 < end && s[1] == '[') {
      /* Skip over image construct: find matching ] then skip (url) or [ref] */
      char *img_start = s + 1; /* points at '[' */
      char *img_s = img_start + 1;
      /* Find closing ']' for image text */
      while (img_s < end && *img_s != ']') {
        if (*img_s == '\\' && img_s + 1 < end) { img_s += 2; continue; }
        ++img_s;
      }
      if (img_s < end) {
        ++img_s; /* past ']' */
        /* Skip (url) or [ref] or [] */
        if (img_s < end && *img_s == '(') {
          ++img_s;
          int paren_depth = 1;
          while (img_s < end && paren_depth > 0) {
            paren_depth += (*img_s == '(') - (*img_s == ')');
            ++img_s;
          }
          s = img_s;
          continue;
        }
        if (img_s < end && *img_s == '[') {
          ++img_s;
          while (img_s < end && *img_s != ']')
            ++img_s;
          s = img_s + (img_s < end);
          continue;
        }
        s = img_s; /* collapsed ref: ![text][] or shortcut ![text] */
        continue;
      }
      /* Not a valid image — treat '!' as literal */
      ++s;
      continue;
    }
    if (*s == '[')
      return NULL; /* no nested unescaped [ in link text */
    if (*s == '`') {
      /* skip code span */
      char *bq = s;
      while (s < end && *s == '`')
        ++s;
      uint16_t bq_len = (uint16_t)(s - bq);
      while (s < end) {
        if (*s == '`') {
          char *cr = s;
          while (s < end && *s == '`')
            ++s;
          if ((uint16_t)(s - cr) == bq_len)
            break;
        } else {
          ++s;
        }
      }
      continue;
    }
    ++s;
  }
  if (s >= end)
    return NULL;
  *bracket_close = s;
  ++s; /* past ']' */

  /* (a) Inline link: ( destination "title" ) */
  if (s < end && *s == '(') {
    char *lp = s + 1;
    /* skip whitespace */
    while (lp < end && (*lp == ' ' || *lp == '\t' || *lp == '\n' ||
                        *lp == '\r'))
      ++lp;
    /* parse destination */
    char *ds = NULL, *de = NULL;
    if (lp < end && *lp == '<') {
      ++lp;
      ds = lp;
      while (lp < end && *lp != '>' && *lp != '\n' && *lp != '\r') {
        if (*lp == '\\' && lp + 1 < end) {
          lp += 2;
          continue;
        }
        ++lp;
      }
      if (lp >= end || *lp != '>')
        goto try_ref;
      de = lp;
      ++lp;
    } else if (lp < end && *lp != ')') {
      ds = lp;
      int pd = 0;
      while (lp < end && *lp != ' ' && *lp != '\t' && *lp != '\n' &&
             *lp != '\r' && *lp != ')') {
        if (*lp == '\\' && lp + 1 < end) {
          lp += 2;
          continue;
        }
        if (*lp == '(')
          ++pd;
        else if (*lp == ')') {
          if (pd == 0)
            break;
          --pd;
        }
        ++lp;
      }
      de = lp;
    } else {
      ds = lp;
      de = lp; /* empty destination */
    }
    /* skip whitespace before optional title */
    while (lp < end && (*lp == ' ' || *lp == '\t' || *lp == '\n' ||
                        *lp == '\r'))
      ++lp;
    /* optional title */
    char *ts = NULL, *te = NULL;
    if (lp < end && (*lp == '"' || *lp == '\'' || *lp == '(')) {
      char open_ch = *lp;
      char close_ch = (open_ch == '(') ? ')' : open_ch;
      ++lp;
      ts = lp;
      while (lp < end && *lp != close_ch) {
        if (*lp == '\\' && lp + 1 < end) {
          lp += 2;
          continue;
        }
        ++lp;
      }
      if (lp >= end)
        goto try_ref;
      te = lp;
      ++lp;
    }
    /* skip whitespace, expect ')' */
    while (lp < end && (*lp == ' ' || *lp == '\t' || *lp == '\n' ||
                        *lp == '\r'))
      ++lp;
    if (lp < end && *lp == ')') {
      ++lp;
      if (ds)
        *dest = FIO_BUF_INFO2(ds, (size_t)(de - ds));
      if (ts && te)
        *title = FIO_BUF_INFO2(ts, (size_t)(te - ts));
      return lp;
    }
  }

try_ref:
  /* (b) Full reference: [label] */
  if (s < end && *s == '[') {
    char *rs = s + 1;
    char *re = rs;
    while (re < end && *re != ']') {
      if (*re == '\\' && re + 1 < end) {
        re += 2;
        continue;
      }
      ++re;
    }
    if (re < end && re > rs) {
      fio_buf_info_s rl = FIO_BUF_INFO2(rs, (size_t)(re - rs));
      fio___gfm_ref_s *ref = fio___gfm_ref_resolve(st, rl);
      if (ref) {
        *dest = ref->destination;
        *title = ref->title;
        *ref_label = rl;
        return re + 1;
      }
    }
    /* (c) Collapsed reference: [] */
    if (re < end && rs == re) {
      fio_buf_info_s rl = FIO_BUF_INFO2(
          *text_start, (size_t)(*bracket_close - *text_start));
      fio___gfm_ref_s *ref = fio___gfm_ref_resolve(st, rl);
      if (ref) {
        *dest = ref->destination;
        *title = ref->title;
        *ref_label = rl;
        return re + 1;
      }
    }
  }

  /* (d) Shortcut reference: label = link text */
  {
    fio_buf_info_s rl = FIO_BUF_INFO2(
        *text_start, (size_t)(*bracket_close - *text_start));
    fio___gfm_ref_s *ref = fio___gfm_ref_resolve(st, rl);
    if (ref) {
      *dest = ref->destination;
      *title = ref->title;
      *ref_label = rl;
      return *bracket_close + 1;
    }
  }

  return NULL;
}

/** Open-section stack entry.
 *
 *  Tracks what push events are currently open so that closers can be
 *  matched. This is NOT the CommonMark delimiter stack — it's a simple
 *  stack of "this type is open, pop it when closer is reached."
 */
typedef struct {
  char *closer_pos;    /* source position where the closer starts */
  char *advance_to;    /* if non-NULL, advance here after pop (links) */
  uint16_t closer_len; /* byte length of the closer delimiter run */
  uint8_t type;        /* fio_gfm_type_e being pushed */
  uint8_t marker;      /* delimiter char (* _ ~) */
} fio___gfm_open_section_s;

#ifndef FIO___GFM_MAX_OPEN_SECTIONS
#define FIO___GFM_MAX_OPEN_SECTIONS 64
#endif

/** Parse a region of text for inline elements.
 *
 *  Main inline parsing entry point. Called when a leaf closes.
 *  Processes [start..end) and emits push/write/pop callbacks in strict
 *  left-to-right document order.
 *
 *  NO RECURSION. Single pass with forward lookahead.
 *
 *  DOCUMENT-ORDER GUARANTEE: For input "Hello **World**":
 *    write(TEXT, "Hello ") → push(STRONG) → write(TEXT, "World") → pop(STRONG)
 */
FIO_SFUNC int fio___gfm_inline_parse(fio___gfm_parser_s *st,
                                     char *start,
                                     char *end) {
  int r = 0;
  char *p = start;          /* current scan position */
  char *text_start = start; /* start of pending literal text */

  /* Count blockquote nesting depth for stripping '>' from continuation lines.
   * When paragraph text spans multiple source lines inside a blockquote,
   * the raw text slice contains embedded '>' markers that must be skipped. */
  uint16_t bq_depth = 0;
  for (uint16_t d = 0; d < st->depth; ++d)
    bq_depth += (st->nest[d].type == FIO___GFM_CONT_BQ);

  /* Open-section stack: tracks active push events awaiting their pop.
   * When the scan reaches a closer_pos, the corresponding pop is emitted.
   * Stack is ordered innermost-on-top (LIFO). */
  fio___gfm_open_section_s open[FIO___GFM_MAX_OPEN_SECTIONS];
  uint16_t open_top = 0;

/* Helper: emit pending literal text from text_start to pos. */
#define FIO___GFM_FLUSH_TEXT(pos)                                              \
  do {                                                                         \
    if ((pos) > text_start) {                                                  \
      fio_gfm_event_s _e = {                                                   \
          .udata = st->udata,                                                  \
          .text = FIO_BUF_INFO2(text_start, (size_t)((pos)-text_start)),          \
          .type = FIO_GFM_TEXT,                                                \
      };                                                                       \
      r = fio___gfm_emit_write(st, &_e);                                       \
      if (r)                                                                   \
        goto done;                                                             \
    }                                                                          \
    text_start = (pos);                                                        \
  } while (0)

  while (p < end) {
    /* (a) Closer check: pop any open sections whose closer we've reached */
    while (open_top > 0 && p >= open[open_top - 1].closer_pos) {
      fio___gfm_open_section_s *sec = &open[open_top - 1];
      FIO___GFM_FLUSH_TEXT(sec->closer_pos);
      char *adv = sec->advance_to;
      uint8_t sec_type = sec->type;
      p = sec->closer_pos + sec->closer_len;
      text_start = p;
      --open_top;
      fio_gfm_event_s ev;
      fio___gfm_event_init(st, &ev);
      ev.type = sec_type;
      r = fio___gfm_emit_pop(st, &ev);
      if (r)
        goto done;
      if (adv) {
        p = adv;
        text_start = p;
      }
    }
    if (p >= end)
      break;

    char c = *p;

    /* (b) Backslash escape */
    if (c == '\\' && p + 1 < end) {
      char nc = p[1];
      if (nc == '\n' || nc == '\r') {
        /* hard break */
        FIO___GFM_FLUSH_TEXT(p);
        p += 1; /* skip backslash */
        p += (*p == '\r');
        p += (p < end && *p == '\n');
        text_start = p;
        if (p < end) {
          fio_gfm_event_s ev;
          fio___gfm_event_init(st, &ev);
          ev.type = FIO_GFM_HARD_BREAK;
          r = fio___gfm_emit_write(st, &ev);
          if (r) goto done;
        }
        continue;
      }
      if (fio___gfm_is_punct(nc)) {
        /* escaped punctuation: flush text before '\', skip '\',
         * restart text from the escaped char (exclude backslash). */
        FIO___GFM_FLUSH_TEXT(p);
        p += 1; /* skip backslash */
        text_start = p; /* text resumes at the escaped char */
        p += 1; /* advance past the escaped char */
        continue;
      }
      ++p;
      continue;
    }

    /* (b2) Ampersand — ML entity decode */
    if (c == '&') {
      char decoded[8];
      size_t decoded_len = fio_entity(decoded, p, (size_t)(end - p));
      if (decoded_len) {
        /* Find the ';' to advance past the entity */
        char *semi = p + 1;
        while (semi < end && *semi != ';')
          ++semi;
        FIO___GFM_FLUSH_TEXT(p);
        fio_gfm_event_s ev;
        fio___gfm_event_init(st, &ev);
        ev.type = FIO_GFM_TEXT;
        ev.text = FIO_BUF_INFO2(decoded, decoded_len);
        r = fio___gfm_emit_write(st, &ev);
        if (r) goto done;
        p = semi + 1;
        text_start = p;
        continue;
      }
      ++p;
      continue;
    }

    /* (c) Backtick — code span */
    if (c == '`') {
      char *bt_start = p;
      while (p < end && *p == '`')
        ++p;
      uint16_t bt_len = (uint16_t)(p - bt_start);
      /* forward-scan for matching closing run */
      char *scan = p;
      char *code_closer = NULL;
      while (scan < end) {
        if (*scan == '`') {
          char *cr = scan;
          while (scan < end && *scan == '`')
            ++scan;
          if ((uint16_t)(scan - cr) == bt_len) {
            code_closer = cr;
            break;
          }
        } else {
          ++scan;
        }
      }
      if (code_closer) {
        FIO___GFM_FLUSH_TEXT(bt_start);
        /* code span interior: [p..code_closer) */
        char *cs = p;
        char *ce = code_closer;
        /* strip one leading + trailing space/newline if both present and
         * content is not all whitespace (CommonMark: line endings = spaces) */
        if (ce - cs >= 2 &&
            (*cs == ' ' || *cs == '\n' || *cs == '\r') &&
            (ce[-1] == ' ' || ce[-1] == '\n' || ce[-1] == '\r')) {
          int all_ws = 1;
          for (char *t = cs; t < ce && all_ws; ++t)
            all_ws &= (*t == ' ' || *t == '\n' || *t == '\r');
          if (!all_ws) {
            ++cs;
            --ce;
          }
        }
        /* Replace internal newlines with spaces for code span */
        fio_gfm_event_s ev;
        fio___gfm_event_init(st, &ev);
        ev.type = FIO_GFM_CODE_SPAN;
        ev.text = FIO_BUF_INFO2(cs, (size_t)(ce - cs));
        r = fio___gfm_emit_write(st, &ev);
        if (r) goto done;
        p = code_closer + bt_len;
        text_start = p;
      }
      /* else: no closer found, p already advanced past backticks (literal) */
      continue;
    }

    /* (d) Newline */
    if (c == '\n' || c == '\r') {
      /* check for hard break: 2+ trailing spaces before newline */
      char *line_text_end = p;
      int trailing_spaces = 0;
      while (line_text_end > text_start &&
             (line_text_end[-1] == ' ' || line_text_end[-1] == '\t')) {
        --line_text_end;
        ++trailing_spaces;
      }
      int is_hard = (trailing_spaces >= 2);
      if (is_hard) {
        FIO___GFM_FLUSH_TEXT(line_text_end);
      } else {
        FIO___GFM_FLUSH_TEXT(p);
      }
      /* skip line ending */
      p += (*p == '\r');
      p += (p < end && *p == '\n');
      /* Strip blockquote continuation markers from paragraph text.
       * When a paragraph spans multiple lines inside a blockquote,
       * continuation lines in the raw source contain '>' markers
       * consumed by Step 1 but physically present in the text slice.
       * Strip up to bq_depth levels of '>' markers. */
      for (uint16_t bq = 0; bq < bq_depth && p < end; ++bq) {
        char *t = p;
        uint32_t sp = 0;
        while (t < end && (*t == ' ' || *t == '\t') && sp < 3) {
          sp += (*t == '\t')
                    ? FIO___GFM_TAB_WIDTH - (sp & (FIO___GFM_TAB_WIDTH - 1U))
                    : 1;
          ++t;
        }
        if (t < end && *t == '>') {
          p = t + 1;
          p += (p < end && (*p == ' ' || *p == '\t'));
        } else {
          break; /* lazy continuation — no marker at this level */
        }
      }
      /* strip leading whitespace from continuation line */
      while (p < end && (*p == ' ' || *p == '\t'))
        ++p;
      text_start = p;
      if (p < end) {
        fio_gfm_event_s ev;
        fio___gfm_event_init(st, &ev);
        ev.type = is_hard ? FIO_GFM_HARD_BREAK : FIO_GFM_SOFT_BREAK;
        r = fio___gfm_emit_write(st, &ev);
        if (r) goto done;
      }
      continue;
    }

    /* (e) Less-than — autolink or inline HTML */
    if (c == '<') {
      char *gt = p + 1;
      while (gt < end && *gt != '>')
        ++gt;
      if (gt < end) {
        /* check for autolink: has ':' or '@' inside, no spaces/newlines */
        int has_colon = 0, has_at = 0, has_space = 0;
        for (char *sc = p + 1; sc < gt; ++sc) {
          has_colon |= (*sc == ':');
          has_at |= (*sc == '@');
          has_space |= (*sc == ' ' || *sc == '\t' || *sc == '\n' ||
                        *sc == '\r' || *sc == '<');
        }
        if ((has_colon || has_at) && !has_space) {
          FIO___GFM_FLUSH_TEXT(p);
          fio_gfm_event_s ev;
          fio___gfm_event_init(st, &ev);
          ev.type = FIO_GFM_AUTOLINK;
          ev.text = FIO_BUF_INFO2(p + 1, (size_t)(gt - (p + 1)));
          ev.destination = ev.text;
          r = fio___gfm_emit_write(st, &ev);
          if (r) goto done;
          p = gt + 1;
          text_start = p;
          continue;
        }
        /* check for inline HTML */
        char fc = (p + 1 < gt) ? p[1] : 0;
        int is_html = ((fc >= 'a' && fc <= 'z') || (fc >= 'A' && fc <= 'Z') ||
                       fc == '/' || fc == '!' || fc == '?');
        if (is_html) {
          FIO___GFM_FLUSH_TEXT(p);
          fio_gfm_event_s ev;
          fio___gfm_event_init(st, &ev);
          ev.type = FIO_GFM_INLINE_HTML;
          ev.text = FIO_BUF_INFO2(p, (size_t)(gt + 1 - p));
          r = fio___gfm_emit_write(st, &ev);
          if (r) goto done;
          p = gt + 1;
          text_start = p;
          continue;
        }
      }
      ++p;
      continue;
    }

    /* (f) Star / underscore — emphasis / strong */
    if (c == '*' || c == '_') {
      char marker = c;
      char *run_start = p;
      while (p < end && *p == marker)
        ++p;
      uint16_t run_len = (uint16_t)(p - run_start);

      /* classify left-flanking */
      char before = (run_start > start) ? run_start[-1] : ' ';
      int before_ws = (before == ' ' || before == '\t' || before == '\n' ||
                       before == '\r');
      int before_punct = fio___gfm_is_punct(before);
      char after = (p < end) ? *p : ' ';
      int after_ws = (after == ' ' || after == '\t' || after == '\n' ||
                      after == '\r');
      int after_punct = fio___gfm_is_punct(after);
      int left_flanking = !after_ws && (!after_punct || before_ws || before_punct);

      int can_open = left_flanking;
      if (marker == '_') {
        /* _ can open only if left-flanking AND
         * (not right-flanking OR preceded by punctuation) */
        int right_flanking = !before_ws && (!before_punct || after_ws || after_punct);
        can_open = left_flanking && (!right_flanking || before_punct);
      }

      if (can_open && open_top < FIO___GFM_MAX_OPEN_SECTIONS) {
        char *boundary = open_top > 0 ? open[open_top - 1].closer_pos : end;
        uint16_t cl = 0;
        char *closer = fio___gfm_find_closer(p, boundary, marker, run_len, &cl);
        if (closer) {
          /* determine consume: 2 for STRONG if both >= 2, else 1 for EMPHASIS */
          uint16_t consume = (run_len >= 2 && cl >= 2) ? 2 : 1;
          uint8_t inner_type = (consume == 2) ? FIO_GFM_STRONG : FIO_GFM_EMPHASIS;
          uint16_t remaining = run_len - consume;

          FIO___GFM_FLUSH_TEXT(run_start);

          if (remaining > 0 && open_top + 1 < FIO___GFM_MAX_OPEN_SECTIONS) {
            uint16_t outer_consume = remaining;
            uint8_t outer_type =
                (outer_consume >= 2) ? FIO_GFM_STRONG : FIO_GFM_EMPHASIS;
            if (outer_consume > 2)
              outer_consume = 2;

            /* Check if closer run has enough chars for both layers */
            if (cl > consume) {
              /* Closer run splits: inner takes first `consume`,
               * outer takes chars at closer+consume (len = cl-consume). */
              uint16_t outer_cl_len = cl - consume;
              if (outer_cl_len > outer_consume)
                outer_cl_len = outer_consume;
              /* push outer, then inner (LIFO: inner on top) */
              fio_gfm_event_s ev;
              fio___gfm_event_init(st, &ev);
              ev.type = outer_type;
              r = fio___gfm_emit_push(st, &ev);
              if (r) goto done;
              open[open_top++] = (fio___gfm_open_section_s){
                  .closer_pos = closer + consume,
                  .closer_len = outer_cl_len,
                  .type = outer_type,
                  .marker = (uint8_t)marker,
              };

              fio___gfm_event_init(st, &ev);
              ev.type = inner_type;
              r = fio___gfm_emit_push(st, &ev);
              if (r) goto done;
              open[open_top++] = (fio___gfm_open_section_s){
                  .closer_pos = closer,
                  .closer_len = consume,
                  .type = inner_type,
                  .marker = (uint8_t)marker,
              };

              p = run_start + run_len;
              text_start = p;
              continue;
            }
            /* Closer run has no extra chars — search for separate outer */
            uint16_t outer_cl = 0;
            char *outer_closer = fio___gfm_find_closer(
                closer + cl, boundary, marker, outer_consume, &outer_cl);
            if (outer_closer) {
              /* push outer, then inner (LIFO: inner on top) */
              fio_gfm_event_s ev;
              fio___gfm_event_init(st, &ev);
              ev.type = outer_type;
              r = fio___gfm_emit_push(st, &ev);
              if (r) goto done;
              open[open_top++] = (fio___gfm_open_section_s){
                  .closer_pos = outer_closer,
                  .closer_len = outer_consume,
                  .type = outer_type,
                  .marker = (uint8_t)marker,
              };

              fio___gfm_event_init(st, &ev);
              ev.type = inner_type;
              r = fio___gfm_emit_push(st, &ev);
              if (r) goto done;
              open[open_top++] = (fio___gfm_open_section_s){
                  .closer_pos = closer,
                  .closer_len = consume,
                  .type = inner_type,
                  .marker = (uint8_t)marker,
              };

              p = run_start + run_len;
              text_start = p;
              continue;
            }
            /* no outer closer: emit remaining as text, push inner only */
            {
              fio_gfm_event_s ev;
              fio___gfm_event_init(st, &ev);
              ev.type = FIO_GFM_TEXT;
              ev.text = FIO_BUF_INFO2(run_start, remaining);
              r = fio___gfm_emit_write(st, &ev);
              if (r) goto done;
            }
          } else if (remaining > 0) {
            /* stack full for outer: emit remaining as text */
            fio_gfm_event_s ev;
            fio___gfm_event_init(st, &ev);
            ev.type = FIO_GFM_TEXT;
            ev.text = FIO_BUF_INFO2(run_start, remaining);
            r = fio___gfm_emit_write(st, &ev);
            if (r) goto done;
          }

          /* push inner */
          {
            fio_gfm_event_s ev;
            fio___gfm_event_init(st, &ev);
            ev.type = inner_type;
            r = fio___gfm_emit_push(st, &ev);
            if (r) goto done;
          }
          open[open_top++] = (fio___gfm_open_section_s){
              .closer_pos = closer,
              .closer_len = consume,
              .type = inner_type,
              .marker = (uint8_t)marker,
          };

          p = run_start + run_len;
          text_start = p;
          continue;
        }
      }
      /* no closer or cannot open: literal text (p already advanced) */
      continue;
    }

    /* (g) Tilde — strikethrough */
    if (c == '~') {
      char *run_start = p;
      while (p < end && *p == '~')
        ++p;
      uint16_t run_len = (uint16_t)(p - run_start);
      if (run_len == 2 && open_top < FIO___GFM_MAX_OPEN_SECTIONS) {
        char before = (run_start > start) ? run_start[-1] : ' ';
        int before_ws = (before == ' ' || before == '\t' || before == '\n' ||
                         before == '\r');
        int before_punct = fio___gfm_is_punct(before);
        char after = (p < end) ? *p : ' ';
        int after_ws = (after == ' ' || after == '\t' || after == '\n' ||
                        after == '\r');
        int after_punct = fio___gfm_is_punct(after);
        int left_flanking = !after_ws && (!after_punct || before_ws || before_punct);
        if (left_flanking) {
          char *boundary = open_top > 0 ? open[open_top - 1].closer_pos : end;
          uint16_t cl = 0;
          char *closer = fio___gfm_find_closer(p, boundary, '~', 2, &cl);
          if (closer) {
            FIO___GFM_FLUSH_TEXT(run_start);
            fio_gfm_event_s ev;
            fio___gfm_event_init(st, &ev);
            ev.type = FIO_GFM_STRIKETHROUGH;
            r = fio___gfm_emit_push(st, &ev);
            if (r) goto done;
            open[open_top++] = (fio___gfm_open_section_s){
                .closer_pos = closer,
                .closer_len = 2,
                .type = FIO_GFM_STRIKETHROUGH,
                .marker = '~',
            };
            text_start = p;
            continue;
          }
        }
      }
      /* literal tildes (p already advanced) */
      continue;
    }

    /* (h) Open bracket — link / image */
    if (c == '[' || (c == '!' && p + 1 < end && p[1] == '[')) {
      int is_image = (c == '!');
      char *bracket = p + is_image; /* points at '[' */
      char *link_text_start = NULL;
      char *bracket_close = NULL;
      fio_buf_info_s l_dest, l_title, l_ref;
      char *link_end = fio___gfm_find_link(st, bracket, end, is_image,
                                           &link_text_start, &bracket_close,
                                           &l_dest, &l_title, &l_ref);
      if (link_end) {
        FIO___GFM_FLUSH_TEXT(p);
        if (is_image) {
          /* IMAGE: emit as single write event */
          fio_gfm_event_s ev;
          fio___gfm_event_init(st, &ev);
          ev.type = FIO_GFM_IMAGE;
          ev.text = FIO_BUF_INFO2(link_text_start,
                                  (size_t)(bracket_close - link_text_start));
          ev.destination = l_dest;
          ev.title = l_title;
          ev.reference = l_ref;
          r = fio___gfm_emit_write(st, &ev);
          if (r) goto done;
          p = link_end;
          text_start = p;
        } else {
          /* LINK: push, scan interior, pop at ']' then advance to link_end */
          fio_gfm_event_s ev;
          fio___gfm_event_init(st, &ev);
          ev.type = FIO_GFM_LINK;
          ev.destination = l_dest;
          ev.title = l_title;
          ev.reference = l_ref;
          r = fio___gfm_emit_push(st, &ev);
          if (r) goto done;
          if (open_top < FIO___GFM_MAX_OPEN_SECTIONS) {
            open[open_top++] = (fio___gfm_open_section_s){
                .closer_pos = bracket_close,
                .advance_to = link_end,
                .closer_len = 1,
                .type = FIO_GFM_LINK,
                .marker = '[',
            };
          }
          p = link_text_start;
          text_start = p;
        }
        continue;
      }
      /* not a valid link/image — literal text */
      p += 1 + is_image;
      continue;
    }

    /* (i) Default: advance */
    ++p;
  }

  /* flush remaining text */
  FIO___GFM_FLUSH_TEXT(end);

done:
  /* If there are unclosed sections (pathological input), pop them now.
   * This ensures invariant I2 (every push has a matching pop). */
  while (open_top > 0) {
    --open_top;
    fio_gfm_event_s e = {
        .udata = st->udata,
        .type = open[open_top].type,
    };
    fio___gfm_emit_pop(st, &e);
  }

#undef FIO___GFM_FLUSH_TEXT
  return r;
}

/* ===========================================================================
 * Block Emission Helpers (used by main loop)
 * ========================================================================= */

/** Emit an ATX heading: parse content, PUSH(HEADING) + inline + POP(HEADING).
 *  `trimmed` points to the first '#', `le` is line end. */
FIO_SFUNC int fio___gfm_emit_atx_heading(fio___gfm_parser_s *st,
                                         char *trimmed,
                                         char *le,
                                         int level) {
  int r = 0;
  /* Skip '#' markers */
  char *content = trimmed + level;
  /* Skip required space/tab after markers */
  if (content < le && (*content == ' ' || *content == '\t'))
    ++content;
  /* Strip leading whitespace from content */
  while (content < le && (*content == ' ' || *content == '\t'))
    ++content;
  /* Strip trailing '#' run preceded by space */
  char *cend = le;
  while (cend > content && (cend[-1] == ' ' || cend[-1] == '\t'))
    --cend;
  if (cend > content && cend[-1] == '#') {
    char *h = cend - 1;
    while (h > content && h[-1] == '#')
      --h;
    if (h == content || h[-1] == ' ' || h[-1] == '\t')
      cend = h;
    while (cend > content && (cend[-1] == ' ' || cend[-1] == '\t'))
      --cend;
  }

  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_HEADING;
  e.heading_level = (uint8_t)level;
  e.source = FIO_BUF_INFO2(trimmed, (size_t)(le - trimmed));
  r = fio___gfm_emit_push(st, &e);
  if (!r && content < cend)
    r = fio___gfm_inline_parse(st, content, cend);
  if (!r) {
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_HEADING;
    e.heading_level = (uint8_t)level;
    r = fio___gfm_emit_pop(st, &e);
  }
  return r;
}

/** Emit a thematic break: PUSH(THEMATIC_BREAK) + POP(THEMATIC_BREAK). */
FIO_SFUNC int fio___gfm_emit_thematic_break(fio___gfm_parser_s *st,
                                            char *trimmed,
                                            char *le) {
  int r;
  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_THEMATIC_BREAK;
  e.source = FIO_BUF_INFO2(trimmed, (size_t)(le - trimmed));
  r = fio___gfm_emit_push(st, &e);
  if (!r) {
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_THEMATIC_BREAK;
    r = fio___gfm_emit_pop(st, &e);
  }
  return r;
}

/** Open a fenced code block. Parses info string and emits PUSH(CODE_BLOCK).
 *  `trimmed` points at the fence chars, `le` is line end. */
FIO_SFUNC int fio___gfm_open_fenced_code(fio___gfm_parser_s *st,
                                         char *trimmed,
                                         char *le,
                                         uint32_t vcol,
                                         char fc,
                                         uint16_t fl) {
  st->leaf_type = FIO_GFM_CODE_BLOCK;
  st->fence_char = (uint8_t)fc;
  st->fence_len = fl;
  st->fence_indent = (uint16_t)vcol;

  /* Parse info string: text after fence chars, trimmed */
  char *info_start = trimmed + fl;
  while (info_start < le && (*info_start == ' ' || *info_start == '\t'))
    ++info_start;
  char *info_end = le;
  while (info_end > info_start &&
         (info_end[-1] == ' ' || info_end[-1] == '\t'))
    --info_end;
  st->fence_info = FIO_BUF_INFO2(info_start, (size_t)(info_end - info_start));
  st->leaf_start = trimmed;

  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_CODE_BLOCK;
  e.info = st->fence_info;
  e.source = FIO_BUF_INFO2(trimmed, (size_t)(le - trimmed));
  return fio___gfm_emit_push(st, &e);
}

/** Emit a code content line. Strips up to `strip_cols` virtual columns. */
FIO_SFUNC int fio___gfm_emit_code_line(fio___gfm_parser_s *st,
                                       char *p,
                                       char *le,
                                       uint32_t strip_cols) {
  /* Strip leading indentation */
  uint8_t padding = 0;
  if (strip_cols > 0) {
    char *after = fio___gfm_skip_indent(p, le, strip_cols, &padding);
    if (after)
      p = after;
  }
  fio_gfm_event_s e;
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_TEXT;
  e.text = FIO_BUF_INFO2(p, (size_t)(le - p));
  e.padding = padding;
  return fio___gfm_emit_write(st, &e);
}

/** Emit a table row: PUSH(TABLE_ROW), cells, POP(TABLE_ROW).
 *  Each cell: PUSH(TABLE_CELL), inline_parse(content), POP(TABLE_CELL).
 *  Missing cells (fewer than table_columns) are emitted as empty.
 *  Excess cells (more than table_columns) are silently ignored. */
FIO_SFUNC int fio___gfm_emit_table_row(fio___gfm_parser_s *st,
                                        char *p,
                                        char *le) {
  int r = 0;
  fio_gfm_event_s e;

  /* PUSH(TABLE_ROW) */
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_TABLE_ROW;
  e.columns = st->table_columns;
  r = fio___gfm_emit_push(st, &e);
  if (r)
    return r;

  /* Skip leading whitespace + optional leading pipe */
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  p += (p < le && *p == '|');

  uint16_t col = 0;
  while (p < le && col < st->table_columns) {
    char *cs, *ce;
    p = fio___gfm_table_cell_next(p, le, &cs, &ce);

    /* PUSH(TABLE_CELL) */
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_TABLE_CELL;
    e.column = col;
    e.columns = st->table_columns;
    e.align = FIO___GFM_TABLE_ALIGN_GET(st, col);
    r = fio___gfm_emit_push(st, &e);
    if (r)
      return r;

    /* Inline parse cell content */
    if (cs < ce) {
      r = fio___gfm_inline_parse(st, cs, ce);
      if (r)
        return r;
    }

    /* POP(TABLE_CELL) */
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_TABLE_CELL;
    r = fio___gfm_emit_pop(st, &e);
    if (r)
      return r;

    ++col;

    /* Advance past pipe separator */
    if (p < le && *p == '|') {
      ++p;
      /* Check for trailing pipe (only whitespace remains) */
      char *rest = p;
      while (rest < le && (*rest == ' ' || *rest == '\t'))
        ++rest;
      if (rest >= le)
        break;
    } else {
      break;
    }
  }

  /* Emit empty cells for missing columns */
  while (col < st->table_columns) {
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_TABLE_CELL;
    e.column = col;
    e.columns = st->table_columns;
    e.align = FIO___GFM_TABLE_ALIGN_GET(st, col);
    r = fio___gfm_emit_push(st, &e);
    if (r)
      return r;
    fio___gfm_event_init(st, &e);
    e.type = FIO_GFM_TABLE_CELL;
    r = fio___gfm_emit_pop(st, &e);
    if (r)
      return r;
    ++col;
  }

  /* POP(TABLE_ROW) */
  fio___gfm_event_init(st, &e);
  e.type = FIO_GFM_TABLE_ROW;
  r = fio___gfm_emit_pop(st, &e);
  return r;
}

/* ===========================================================================
 * THE MAIN LOOP
 *
 * This is the core of the flat-state parser. It implements the three-step-
 * per-line algorithm from the GFM spec appendix A.
 *
 * The key insight that makes this work WITHOUT recursion:
 *
 *   In a recursive-descent parser, container state is carried in C stack
 *   frames. Entering a blockquote means calling parse_blockquote(), which
 *   calls parse_blocks() recursively. The C stack IS the container stack.
 *
 *   In this flat parser, container state is carried in a single struct
 *   array (nest[] — 4 bytes per depth: type, flags, indent).
 *   The main loop walks this array with a simple `for` loop instead of
 *   recursing. This eliminates stack overflow risk and keeps all state
 *   inspectable in a single struct.
 *
 *   The trick is the three-step-per-line algorithm:
 *     Step 1: Walk existing containers, consuming their markers.
 *     Step 2: At the point where matching stopped, check for new blocks.
 *             New containers push onto the stack and we loop back to
 *             check for MORE new blocks (the `try_new_block` loop).
 *             This replaces the recursive "enter container, parse blocks"
 *             pattern with an iterative "push container, loop" pattern.
 *     Step 3: Whatever text remains goes into the deepest open block.
 *
 *   Lazy continuation is the subtlest part: when a paragraph is open,
 *   unmatched containers are NOT closed. The line becomes paragraph
 *   continuation text. This is checked BEFORE closing unmatched
 *   containers, which is why we track `matched_depth` separately.
 * ========================================================================= */

SFUNC size_t fio_gfm_parse(const fio_gfm_callbacks_s *callbacks,
                           void *udata,
                           fio_buf_info_s source) {
  fio___gfm_parser_s st;
  FIO_MEMSET(&st, 0, sizeof(st));
  if (callbacks) {
    st.push = callbacks->push;
    st.write = callbacks->write;
    st.pop = callbacks->pop;
  }
  st.udata = udata;
  st.start = source.buf;
  st.end = source.buf + source.len;

  /* --- Input validation --- */
  if (!source.buf && source.len) {
    st.err = FIO_GFM_ERR_INPUT;
    return 0;
  }
  if (!source.len)
    return 0;

  char *p = st.start;

  /* =======================================================================
   * MAIN LOOP: process one line per iteration.
   *
   * Invariants maintained at loop entry:
   *   - p points to the start of the current line in the source.
   *   - st.nest[0] is the current container depth.
   *   - st.leaf_type indicates the open leaf (0 = none).
   *   - st.para_open == 1 iff st.leaf_type == FIO_GFM_PARAGRAPH.
   *   - st.err == 0 (non-zero exits the loop).
   * ===================================================================== */
  while (p < st.end && !st.err) {
    char *le = fio___gfm_line_end(p, st.end);
    char *next = fio___gfm_line_next(le, st.end);

    /* =================================================================
     * STEP 1: MATCH OPEN CONTAINER CONTINUATIONS
     *
     * Walk from outermost (depth 1) to innermost (depth N).
     * For each container, test if this line satisfies its continuation
     * condition. Consume the container's marker from the line as we go.
     *
     * `content` tracks the current position in the line AFTER consumed
     * markers. It advances as each container's marker is consumed.
     *
     * `matched_depth` records the deepest container that matched.
     * If matching stops early, deeper containers are "unmatched" but
     * NOT yet closed (they may survive via lazy continuation).
     * ================================================================= */
    char *content = p;
    st.matched_depth = 0;
    int from_push = 0; /* set to 1 when entering try_new_block via push */

    for (uint16_t d = 0; d < st.depth; ++d) {
      uint8_t ctype = st.nest[d].type;

      if (ctype == FIO___GFM_CONT_BQ) {
        /* Blockquote continuation: expect '>' within first 3 columns. */
        uint32_t ind = fio___gfm_indent(content, le);
        if (ind > FIO___GFM_MAX_MARKER_INDENT)
          break;
        char *trimmed = fio___gfm_ltrim(content, le);
        if (trimmed >= le || *trimmed != '>')
          break;
        content = trimmed + 1;
        content += (content < le && (*content == ' ' || *content == '\t'));
        st.matched_depth = d + 1;

      } else if (ctype >= FIO___GFM_CONT_UL_DASH &&
                 ctype <= FIO___GFM_CONT_OL_PAREN) {
        /* LIST containers always match — their child LIST_ITEM decides. */
        st.matched_depth = d + 1;

      } else if (ctype == FIO___GFM_CONT_LI) {
        /* List item continuation: indent >= item's content indent. */
        if (fio___gfm_is_blank(content, le)) {
          st.matched_depth = d + 1;
        } else {
          uint32_t line_col = fio___gfm_indent(content, le);
          if (line_col >= st.nest[d].indent) {
            uint8_t padding = 0;
            char *after = fio___gfm_skip_indent(
                content, le, st.nest[d].indent, &padding);
            if (after)
              content = after;
            st.matched_depth = d + 1;
          } else {
            break;
          }
        }
      }
    } /* end Step 1 */

    /* =================================================================
     * STEP 2: HANDLE CONTINUATION OF OPEN LEAVES
     *
     * Some leaf blocks consume all subsequent lines regardless of what
     * they contain (fenced code, HTML blocks). Handle these first,
     * before checking for new block starts.
     *
     * If all containers matched (matched_depth == nest[0]), the leaf
     * continues normally. If some containers were unmatched, the leaf
     * is implicitly closed by the container breakout.
     * ================================================================= */

    if (st.leaf_type == FIO_GFM_CODE_BLOCK) {
      if (st.matched_depth != FIO___GFM_DEPTH(&st)) {
        /* Container breakout — code block implicitly closed. */
        fio___gfm_close_leaf(&st);
        fio___gfm_close_unmatched(&st);
        goto step2_new_blocks;
      }
      if (st.fence_char) {
        /* --- Fenced code: consumes all lines until closing fence --- */
        if (fio___gfm_is_fenced_code_close(content,
                                           le,
                                           st.fence_char,
                                           st.fence_len)) {
          fio___gfm_close_leaf(&st);
        } else {
          /* Content line. Strip up to fence_indent columns of indent. */
          fio___gfm_emit_code_line(&st, content, le, st.fence_indent);
        }
        p = next;
        continue;
      }
      /* --- Indented code: continues while indent >= 4 or blank --- */
      {
        uint32_t vcol_ic = fio___gfm_indent(content, le);
        int is_blank_ic = fio___gfm_is_blank(content, le);
        if (vcol_ic >= 4 || is_blank_ic) {
          if (is_blank_ic) {
            /* Buffer blank lines — strip if trailing (spec 4.4). */
            if (st.ic_pending_blanks < FIO___GFM_MAX_IC_BLANKS) {
              st.ic_blank_content[st.ic_pending_blanks] = content;
              st.ic_blank_le[st.ic_pending_blanks] = le;
            }
            ++st.ic_pending_blanks;
          } else {
            /* Non-blank code line: emit buffered blank lines first */
            for (uint16_t bi = 0; bi < st.ic_pending_blanks; ++bi) {
              uint16_t idx = (bi < FIO___GFM_MAX_IC_BLANKS) ? bi : 0;
              fio___gfm_emit_code_line(&st,
                                       st.ic_blank_content[idx],
                                       st.ic_blank_le[idx], 4);
            }
            st.ic_pending_blanks = 0;
            fio___gfm_emit_code_line(&st, content, le, 4);
          }
          p = next;
          continue;
        }
      }
      /* Indent < 4 and not blank — close (discard pending blanks). */
      st.ic_pending_blanks = 0;
      fio___gfm_close_leaf(&st);
    }

    if (st.leaf_type == FIO_GFM_HTML_BLOCK) {
      if (st.matched_depth != FIO___GFM_DEPTH(&st)) {
        fio___gfm_close_leaf(&st);
        fio___gfm_close_unmatched(&st);
        goto step2_new_blocks;
      }
      /* Types 6-7: blank line closes HTML block.
       * Must check here because the HTML continuation catches all lines
       * before the blank line handler at step2_new_blocks. */
      if (st.leaf_html_type >= 6 && fio___gfm_is_blank(content, le)) {
        fio___gfm_close_leaf(&st);
        goto step2_new_blocks;
      }
      /* Emit line as HTML content */
      {
        fio_gfm_event_s e;
        fio___gfm_event_init(&st, &e);
        e.type = FIO_GFM_TEXT;
        e.text = FIO_BUF_INFO2(content, (size_t)(le - content));
        fio___gfm_emit_write(&st, &e);
      }
      /* Check close conditions by type (types 1-5 have in-line close) */
      if (fio___gfm_html_block_has_close(content, le, st.leaf_html_type))
        fio___gfm_close_leaf(&st);
      p = next;
      continue;
    }

    if (st.leaf_type == FIO_GFM_TABLE) {
      if (st.matched_depth == FIO___GFM_DEPTH(&st)) {
        if (fio___gfm_is_blank(content, le)) {
          fio___gfm_close_leaf(&st);
          /* Fall through — blank line after table */
        } else {
          /* Check if this line starts a block structure (breaks table).
           * Per GFM spec: "The table is broken at the first empty line,
           * or beginning of another block-level structure." */
          {
            uint32_t tvcol = fio___gfm_indent(content, le);
            if (tvcol <= FIO___GFM_MAX_MARKER_INDENT) {
              char *tt = fio___gfm_ltrim(content, le);
              char *bq_dummy;
              char fc_dummy;
              fio___gfm_list_marker_s lm_dummy;
              if (fio___gfm_is_atx_heading(tt, le) ||
                  fio___gfm_is_thematic_break(tt, le) ||
                  fio___gfm_is_blockquote(tt, le, &bq_dummy) ||
                  fio___gfm_is_fenced_code_open(tt, le, &fc_dummy) ||
                  fio___gfm_is_html_block_start(tt, le, 1) ||
                  fio___gfm_is_list_marker(tt, le, tvcol, &lm_dummy)) {
                fio___gfm_close_leaf(&st);
                goto step2_new_blocks;
              }
            }
          }
          fio___gfm_emit_table_row(&st, content, le);
          p = next;
          continue;
        }
      } else {
        fio___gfm_close_leaf(&st);
        fio___gfm_close_unmatched(&st);
      }
      /* Fall through to step2_new_blocks */
    }

    /* =================================================================
     * STEP 2 CONTINUED: NEW BLOCK DETECTION
     *
     * At this point, `content` points to the line content after all
     * matched container markers have been consumed. `matched_depth`
     * tells us how many containers matched.
     *
     * Two cases:
     *   A) Paragraph is open → use special precedence rules.
     *   B) No paragraph open → use full block detection.
     * ================================================================= */
  step2_new_blocks:

    /* Handle blank lines (they interact with many block types) */
    if (fio___gfm_is_blank(content, le)) {
      fio___gfm_close_unmatched(&st);
      fio___gfm_handle_blank_line(&st);
      p = next;
      continue;
    }

    /* --- CASE A: PARAGRAPH IS OPEN --- */
    if (st.para_open) {
      uint32_t vcol = fio___gfm_indent(content, le);
      char *trimmed = fio___gfm_ltrim(content, le);

      /* A1: Setext heading underline?
       *     Only valid when ALL containers matched — a setext underline
       *     must be in the same container context as the paragraph.
       *     e.g., "> foo\n---" — the --- is outside the blockquote,
       *     so it's a thematic break, not a setext heading.
       *
       *     CRITICAL: This check must come BEFORE thematic break.
       *     "---" after a paragraph is a setext heading (level 2),
       *     NOT a thematic break — but only within matched containers. */
      if (vcol <= FIO___GFM_MAX_MARKER_INDENT &&
          st.matched_depth == FIO___GFM_DEPTH(&st)) {
        int setext_level = fio___gfm_is_setext_underline(trimmed, le);
        if (setext_level) {
          /* Convert paragraph to heading FIRST (consumes para state),
           * THEN close unmatched containers. close_unmatched would
           * otherwise close the paragraph as a regular paragraph. */
          fio___gfm_convert_to_setext(&st, content, le, setext_level);
          fio___gfm_close_unmatched(&st);
          p = next;
          continue;
        }
      }

      /* A2: Table delimiter row?
       *     If paragraph has exactly one line and this line is a valid
       *     delimiter row, convert the paragraph to a table.
       *     Only valid when all containers matched and indent <= 3. */
      if (st.para_start && st.para_end &&
          vcol <= FIO___GFM_MAX_MARKER_INDENT &&
          st.matched_depth == FIO___GFM_DEPTH(&st)) {
        uint16_t delim_cols = fio___gfm_is_table_delimiter(&st, trimmed, le);
        if (delim_cols > 0) {
          int tr = fio___gfm_convert_to_table(&st, content, le);
          if (tr) {
            st.err = tr;
            break;
          }
          if (st.leaf_type == FIO_GFM_TABLE) {
            p = next;
            continue;
          }
          /* Conversion declined (column mismatch / multi-line para) —
           * fall through to paragraph continuation. */
        }
      }

      /* A3: Can any block type interrupt this paragraph?
       *
       *     The GFM spec restricts which blocks can interrupt a paragraph:
       *       YES: ATX heading, fenced code, thematic break, blockquote,
       *            HTML block types 1-6, bullet list, ordered list (start=1)
       *       NO:  Indented code, HTML type 7, ref defs, ordered list start>1
       *
       *     If nothing interrupts: this is lazy continuation or paragraph
       *     continuation text. */
      if (vcol <= FIO___GFM_MAX_MARKER_INDENT) {
        /* ATX heading? */
        int atx_level = fio___gfm_is_atx_heading(trimmed, le);
        if (atx_level) {
          fio___gfm_close_unmatched(&st);
          fio___gfm_close_orphan_lists(&st);
          fio___gfm_close_paragraph(&st);
          fio___gfm_emit_atx_heading(&st, trimmed, le, atx_level);
          p = next;
          continue;
        }

        /* Thematic break? (checked AFTER setext above) */
        if (fio___gfm_is_thematic_break(trimmed, le)) {
          fio___gfm_close_unmatched(&st);
          fio___gfm_close_orphan_lists(&st);
          fio___gfm_close_paragraph(&st);
          fio___gfm_emit_thematic_break(&st, trimmed, le);
          p = next;
          continue;
        }

        /* Fenced code? */
        char fc;
        uint16_t fl = fio___gfm_is_fenced_code_open(trimmed, le, &fc);
        if (fl) {
          fio___gfm_close_unmatched(&st);
          fio___gfm_close_orphan_lists(&st);
          fio___gfm_close_paragraph(&st);
          fio___gfm_open_fenced_code(&st, trimmed, le, vcol, fc, fl);
          p = next;
          continue;
        }

        /* Blockquote? */
        char *bq_after;
        if (fio___gfm_is_blockquote(trimmed, le, &bq_after)) {
          fio___gfm_close_unmatched(&st);
          fio___gfm_close_orphan_lists(&st);
          fio___gfm_close_paragraph(&st);
          uint16_t bq_content_col = fio___gfm_vcol(p, bq_after);
          fio___gfm_push_blockquote(&st, bq_content_col);
          if (st.err)
            break;
          content = bq_after;
          from_push = 1;
          goto try_new_block;
        }

        /* HTML block types 1-6? (type 7 CANNOT interrupt a paragraph) */
        int html_type = fio___gfm_is_html_block_start(trimmed, le, 0);
        if (html_type >= 1 && html_type <= 6) {
          fio___gfm_close_unmatched(&st);
          fio___gfm_close_orphan_lists(&st);
          fio___gfm_close_paragraph(&st);
          st.leaf_type = FIO_GFM_HTML_BLOCK;
          st.leaf_html_type = (uint8_t)html_type;
          st.leaf_start = content;
          {
            fio_gfm_event_s e;
            fio___gfm_event_init(&st, &e);
            e.type = FIO_GFM_HTML_BLOCK;
            e.source = FIO_BUF_INFO2(content, (size_t)(le - content));
            fio___gfm_emit_push(&st, &e);
          }
          /* First line is also content */
          {
            fio_gfm_event_s e;
            fio___gfm_event_init(&st, &e);
            e.type = FIO_GFM_TEXT;
            e.text = FIO_BUF_INFO2(content, (size_t)(le - content));
            fio___gfm_emit_write(&st, &e);
          }
          /* Check same-line close (e.g., <style>...</style> on one line) */
          if (fio___gfm_html_block_has_close(content, le, html_type))
            fio___gfm_close_leaf(&st);
          p = next;
          continue;
        }

        /* List marker? */
        fio___gfm_list_marker_s marker_info;
        if (fio___gfm_is_list_marker(trimmed, le, vcol, &marker_info)) {
          /* Bullet lists always interrupt. Ordered only if start == 1. */
        {
          /* Check for existing list of same type FIRST.
           * A new item in an existing list always interrupts (even ordered
           * with start > 1). Only a NEW list is restricted.
           * Don't match a list whose LI was matched (d < matched_depth),
           * since that means we're INSIDE the LI → create nested list. */
          uint16_t list_depth = 0;
          int has_diff_list = 0;
          uint16_t depth_a = FIO___GFM_DEPTH(&st);
          for (uint16_t d = depth_a; d >= 1; --d) {
            uint8_t t = st.nest[d - 1].type;
            if (t >= FIO___GFM_CONT_UL_DASH && t <= FIO___GFM_CONT_OL_PAREN) {
              if (t == marker_info.type && d >= st.matched_depth)
                list_depth = d;
              else
                has_diff_list = 1;
              break;
            }
            if (t == FIO___GFM_CONT_BQ)
              break;
          }
          /* New item in existing list: always allowed.
           * New list: bullet or ordered start=1 only.
           * Different delimiter (e.g., '.' vs ')') always starts new. */
          if (list_depth || has_diff_list ||
              marker_info.start_num == 0 || marker_info.start_num == 1) {
            fio___gfm_close_paragraph(&st);
            fio___gfm_close_unmatched(&st);
            /* After close_unmatched, the found list may no longer exist
             * (e.g., "> - foo\n- bar" — BQ and its list were closed).
             * In that case, treat as a new list. */
            if (list_depth && list_depth <= FIO___GFM_DEPTH(&st)) {
              fio___gfm_push_new_item(&st, list_depth, &marker_info);
            } else {
              /* Close existing different-type list at current depth */
              if (FIO___GFM_DEPTH(&st) > 0) {
                uint8_t tt = FIO___GFM_TOP_TYPE(&st);
                if (tt >= FIO___GFM_CONT_UL_DASH &&
                    tt <= FIO___GFM_CONT_OL_PAREN) {
                  st.matched_depth = FIO___GFM_DEPTH(&st) - 1;
                  fio___gfm_close_unmatched(&st);
                }
              }
              fio___gfm_push_list_and_item(&st, &marker_info);
            }
            if (st.err)
              break;
            content = marker_info.content_start;
            from_push = 1;
            goto try_new_block;
          }
        }
          /* Ordered with start > 1: cannot interrupt paragraph.
           * Fall through to lazy continuation. */
        }
      }

      /* A4: LAZY CONTINUATION
       *
       *     None of the above matched. This line is paragraph continuation.
       *     Even if some containers were unmatched (matched_depth < nest[0]),
       *     we do NOT close them. The paragraph absorbs the line.
       *
       *     This is the heart of lazy continuation:
       *       > foo       ← opens blockquote + paragraph
       *       bar         ← no '>', but paragraph open → lazy continuation
       *                     blockquote stays open
       *
       *     IMPORTANT: We use `le` (not `next`) because the newline is
       *     part of the line ending, not paragraph content. But we store
       *     `next` so that paragraph text includes the newline for proper
       *     inline parsing of soft/hard breaks. */
      fio___gfm_append_paragraph(&st, next);
      p = next;
      continue;
    }

    /* --- CASE B: NO PARAGRAPH OPEN — FULL BLOCK DETECTION --- */

    /* Close unmatched containers now.
     * (With no paragraph, there's no lazy continuation to preserve.) */
    fio___gfm_close_unmatched(&st);

    /* Try to open new blocks. This loop handles nested container starts:
     * e.g., "> - foo" opens a blockquote, then inside it opens a list.
     * After each container is pushed, we loop back to check if the
     * remaining content starts ANOTHER container.
     *
     * from_push tracks whether we arrived via a container push (1) or
     * from the top of the line (0). When from a push, list markers
     * ALWAYS start new nested lists — never match parent lists.
     *
     * This iterative loop replaces the recursive pattern where
     * parse_blockquote() calls parse_blocks() which calls parse_list()
     * which calls parse_blocks() again. */
  try_new_block:;
    uint32_t vcol = fio___gfm_indent(content, le);
    char *trimmed = fio___gfm_ltrim(content, le);

    /* B1: Indented code? (only when no paragraph is open)
     *     A line with indent >= 4 that doesn't start another block. */
    if (vcol >= 4) {
      st.leaf_type = FIO_GFM_CODE_BLOCK;
      st.fence_char = 0; /* indented = no fence char */
      st.fence_len = 0;
      st.fence_indent = 0;
      st.leaf_start = content;
      fio_gfm_event_s e;
      fio___gfm_event_init(&st, &e);
      e.type = FIO_GFM_CODE_BLOCK;
      e.source = FIO_BUF_INFO2(content, (size_t)(le - content));
      fio___gfm_emit_push(&st, &e);
      fio___gfm_emit_code_line(&st, content, le, 4);
      p = next;
      continue;
    }

    /* B2: ATX heading? */
    {
      int atx_level = fio___gfm_is_atx_heading(trimmed, le);
      if (atx_level) {
        fio___gfm_emit_atx_heading(&st, trimmed, le, atx_level);
        p = next;
        continue;
      }
    }

    /* B3: Thematic break? */
    if (fio___gfm_is_thematic_break(trimmed, le)) {
      fio___gfm_emit_thematic_break(&st, trimmed, le);
      p = next;
      continue;
    }

    /* B4: Fenced code? */
    {
      char fc;
      uint16_t fl = fio___gfm_is_fenced_code_open(trimmed, le, &fc);
      if (fl) {
        fio___gfm_open_fenced_code(&st, trimmed, le, vcol, fc, fl);
        p = next;
        continue;
      }
    }

    /* B5: Blockquote? → push container, loop back for nested blocks */
    {
      char *bq_after;
      if (fio___gfm_is_blockquote(trimmed, le, &bq_after)) {
        uint16_t bq_content_col = fio___gfm_vcol(p, bq_after);
        fio___gfm_push_blockquote(&st, bq_content_col);
        if (st.err)
          break;
        content = bq_after;
        from_push = 1;
        goto try_new_block; /* check for nested blocks inside blockquote */
      }
    }

    /* B6: HTML block? (all 7 types when no paragraph is open) */
    {
      int html_type = fio___gfm_is_html_block_start(trimmed, le, 1);
      if (html_type) {
        st.leaf_type = FIO_GFM_HTML_BLOCK;
        st.leaf_html_type = (uint8_t)html_type;
        st.leaf_start = content;
        fio_gfm_event_s e;
        fio___gfm_event_init(&st, &e);
        e.type = FIO_GFM_HTML_BLOCK;
        e.source = FIO_BUF_INFO2(content, (size_t)(le - content));
        fio___gfm_emit_push(&st, &e);
        /* First line is also content */
        fio___gfm_event_init(&st, &e);
        e.type = FIO_GFM_TEXT;
        e.text = FIO_BUF_INFO2(content, (size_t)(le - content));
        fio___gfm_emit_write(&st, &e);
        /* Check same-line close (e.g., <style>...</style> on one line) */
        if (fio___gfm_html_block_has_close(content, le, html_type))
          fio___gfm_close_leaf(&st);
        p = next;
        continue;
      }
    }

    /* B7: List item? → push LIST + LIST_ITEM containers, loop back */
    {
      fio___gfm_list_marker_s marker_info;
      if (fio___gfm_is_list_marker(trimmed, le, vcol, &marker_info)) {
        /* Determine if this is a new item in an existing list or a new list.
         *
         * Check the current nesting stack:
         *   - If innermost container is a LIST with the same marker type,
         *     this is a new item in that list.
         *   - Otherwise, this starts a new list.
         *
         * "Same marker type" means:
         *   - Same bullet character (-, +, *) for unordered lists, OR
         *   - Same delimiter (., )) for ordered lists.
         *
         * A different character starts a DIFFERENT list:
         *   "- foo\n+ bar" = two separate lists.
         */
        uint16_t list_depth = 0;
        uint16_t depth = FIO___GFM_DEPTH(&st);

        /* Walk backwards to find enclosing LIST of same type.
         * When from_push is set, we're processing content INSIDE a
         * just-opened container — list markers always create nested
         * lists, never match parent lists. */
        if (!from_push) {
          for (uint16_t d = depth; d >= 1; --d) {
            uint8_t t = st.nest[d - 1].type; /* 0-indexed */
            if (t >= FIO___GFM_CONT_UL_DASH && t <= FIO___GFM_CONT_OL_PAREN) {
              if (t == marker_info.type)
                list_depth = d;
              break; /* stop at first list container we find */
            }
            if (t == FIO___GFM_CONT_BQ)
              break; /* don't look past blockquote boundaries */
          }
        }

        if (list_depth) {
          /* New item in existing list */
          fio___gfm_push_new_item(&st, list_depth, &marker_info);
        } else {
          /* Close existing different-type list at current depth */
          if (FIO___GFM_DEPTH(&st) > 0) {
            uint8_t tt = FIO___GFM_TOP_TYPE(&st);
            if (tt >= FIO___GFM_CONT_UL_DASH && tt <= FIO___GFM_CONT_OL_PAREN) {
              st.matched_depth = FIO___GFM_DEPTH(&st) - 1;
              fio___gfm_close_unmatched(&st);
            }
          }
          /* New list */
          fio___gfm_push_list_and_item(&st, &marker_info);
        }

        if (st.err)
          break;

        /* Advance content past the list marker */
        content = marker_info.content_start;
        from_push = 1;
        goto try_new_block; /* check for nested blocks inside list item */
      }
    }

    /* B8: Default — open a new paragraph.
     * Skip if content >= le (e.g., empty list item marker line). */
    if (content < le)
      fio___gfm_open_paragraph(&st, content, le);
    p = next;
    continue;

  } /* end main loop */

  /* =======================================================================
   * END OF DOCUMENT: Close all open blocks.
   * ===================================================================== */
  fio___gfm_close_all(&st);

  return st.err ? st.consumed : (size_t)(st.end - st.start);
}

#endif /* FIO_EXTERN_COMPLETE || !FIO_EXTERN */
#endif /* FIO_GFM && !H___FIO_GFM___H */

/* *****************************************************************************
Clean up macros
***************************************************************************** */
#undef FIO_GFM
#undef FIO___GFM_TAB_WIDTH
#undef FIO___GFM_MAX_MARKER_INDENT
#undef FIO___GFM_MIN_FENCE_LEN
#undef FIO___GFM_MAX_ATX_HEADING
#undef FIO___GFM_MAX_ENTITY_LEN
