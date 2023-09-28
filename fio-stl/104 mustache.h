/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MUSTACHE module    /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        Mustache-ish Template Engine




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MUSTACHE) && !defined(FIO___RECURSIVE_INCLUDE) &&              \
    !defined(H___FIO_MUSTACHE___H)
#define H___FIO_MUSTACHE___H

/* *****************************************************************************
Settings
***************************************************************************** */

#ifndef FIO_MUSTACHE_MAX_DEPTH
/** The maximum depth of a template's context */
#define FIO_MUSTACHE_MAX_DEPTH 128
#endif

#ifndef FIO_MUSTACHE_PRESERVE_PADDING
/** Preserves padding for stand-alone variables and partial templates */
#define FIO_MUSTACHE_PRESERVE_PADDING 0
#endif
#ifndef FIO_MUSTACHE_LAMBDA_SUPPORT
/** Supports raw text for lambda style languages. */
#define FIO_MUSTACHE_LAMBDA_SUPPORT 0
#endif
#ifndef FIO_MUSTACHE_ISOLATE_PARTIALS
/** Limits the scope of partial templates to the context of their section. */
#define FIO_MUSTACHE_ISOLATE_PARTIALS 1
#endif

/* *****************************************************************************
Mustache Parser / Builder API
***************************************************************************** */

typedef struct fio_mustache_s fio_mustache_s;
typedef struct fio_mustache_bargs_s fio_mustache_bargs_s;

typedef struct {
  /** The file's content (if pre-loaded) */
  fio_buf_info_s data;
  /** The file's name (even if preloaded, used for partials load paths) */
  fio_buf_info_s filename;
  /** Loads the file's content, returning a `fio_buf_info_s` structure. */
  fio_buf_info_s (*load_file_data)(fio_buf_info_s filename, void *udata);
  /** Frees the file's content from its `fio_buf_info_s` structure. */
  void (*free_file_data)(fio_buf_info_s file_data, void *udata);
  /** Called when YAML front matter data was found. */
  void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter, void *udata);
  /** Opaque user data. */
  void *udata;
} fio_mustache_load_args_s;

/* Allocates a new object on the heap and initializes it's memory. */
SFUNC fio_mustache_s *fio_mustache_load(fio_mustache_load_args_s settings);
/* Allocates a new object on the heap and initializes it's memory. */
#define fio_mustache_load(...)                                                 \
  fio_mustache_load((fio_mustache_load_args_s){__VA_ARGS__})

/* Frees the mustache template object (or reduces it's reference count). */
SFUNC void fio_mustache_free(fio_mustache_s *m);

/** Increases the mustache template's reference count. */
SFUNC fio_mustache_s *fio_mustache_dup(fio_mustache_s *m);

struct fio_mustache_bargs_s {
  /* callback should write `txt` to output and return updated `udata.` */
  void *(*write_text)(void *udata, fio_buf_info_s txt);
  /* same as `write_text`, but should also  HTML escape (sanitize) data. */
  void *(*write_text_escaped)(void *udata, fio_buf_info_s raw);
  /* callback should return a new context pointer with the value of `name`. */
  void *(*get_var)(void *ctx, fio_buf_info_s name);
  /* if context is an Array, should return its length. */
  size_t (*array_length)(void *ctx);
  /* if context is an Array, should return a context pointer @ index. */
  void *(*get_var_index)(void *ctx, size_t index);
  /* should return the String value of context `var` as a `fio_buf_info_s`. */
  fio_buf_info_s (*var2str)(void *var);
  /* should return non-zero if the context pointer refers to a valid value. */
  int (*var_is_truthful)(void *ctx);
  /* callback signals that the `ctx` context pointer is no longer in use. */
  void (*release_var)(void *ctx);
  /* returns non-zero if `ctx` is a lambda and handles section manually. */
  int (*is_lambda)(void **udata,
                   void *ctx,
                   fio_buf_info_s raw_template_section);
  /* the root context for finding named values. */
  void *ctx;
  /* opaque user data (settable as well as readable), the final return value. */
  void *udata;
};

/** Builds the template, returning the final value of `udata` (or NULL). */
SFUNC void *fio_mustache_build(fio_mustache_s *m, fio_mustache_bargs_s);
#define fio_mustache_build(m, ...)                                             \
  fio_mustache_build((m), ((fio_mustache_bargs_s){__VA_ARGS__}))

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
FIO___LEAK_COUNTER_DEF(fio_mustache_s)

/* *****************************************************************************
Instructions (relative state)

All instructions are 1 byte long with optional extra data.

Instruction refer to offsets rather than absolute values.
***************************************************************************** */
/* for ease of use, instructions are always a 1 byte numeral, */
typedef enum {
  FIO___MUSTACHE_I_STACK_POP,    /* 0 extra data (marks end of array / list)? */
  FIO___MUSTACHE_I_STACK_PUSH,   /* 32 bit extra data (goes to position) */
  FIO___MUSTACHE_I_GOTO_PUSH,    /* 32 bit extra data (goes to position) */
  FIO___MUSTACHE_I_TXT,          /* 16 bits length + data */
  FIO___MUSTACHE_I_VAR,          /* 16 bits length + data */
  FIO___MUSTACHE_I_VAR_RAW,      /* 16 bits length + data */
  FIO___MUSTACHE_I_ARY,          /* 16 bits length + 32 bit skip-pos + data */
  FIO___MUSTACHE_I_MISSING,      /* 16 bits length + 32 bit skip-pos + data */
  FIO___MUSTACHE_I_PADDING_PUSH, /* 16 bits length + data */
  FIO___MUSTACHE_I_PADDING_POP,  /* 0 extra data */
#if FIO_MUSTACHE_PRESERVE_PADDING
  FIO___MUSTACHE_I_VAR_PADDED,
  FIO___MUSTACHE_I_VAR_RAW_PADDED,
#endif
#if FIO_MUSTACHE_LAMBDA_SUPPORT
  FIO___MUSTACHE_I_METADATA, /* raw text data, written for lambda support */
#endif
} fio___mustache_inst_e;
/* *****************************************************************************
Instructions - Main processor
***************************************************************************** */

typedef struct fio___mustache_bldr_s {
  char *root;
  struct fio___mustache_bldr_s *prev;
  void *ctx;
  fio_buf_info_s padding;
  fio_mustache_bargs_s *args;
#if FIO_MUSTACHE_ISOLATE_PARTIALS
  uint32_t stop;
#endif
} fio___mustache_bldr_s;

FIO_SFUNC char *fio___mustache_i_stack_pop(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_stack_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_goto_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_txt(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_var(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_var_raw(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_ary(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_missing(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_padding_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_padding_pop(char *p, fio___mustache_bldr_s *);
#if FIO_MUSTACHE_PRESERVE_PADDING
FIO_SFUNC char *fio___mustache_i_var_padded(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_var_raw_padded(char *,
                                                fio___mustache_bldr_s *);
#endif
#if FIO_MUSTACHE_LAMBDA_SUPPORT
FIO_SFUNC char *fio___mustache_i_metadata(char *p, fio___mustache_bldr_s *);
#endif

FIO_SFUNC void *fio___mustache_build_section(char *p, fio___mustache_bldr_s a) {
  static char *(*map[])(char *, fio___mustache_bldr_s *) = {
    [FIO___MUSTACHE_I_STACK_POP] = fio___mustache_i_stack_pop,
    [FIO___MUSTACHE_I_STACK_PUSH] = fio___mustache_i_stack_push,
    [FIO___MUSTACHE_I_GOTO_PUSH] = fio___mustache_i_goto_push,
    [FIO___MUSTACHE_I_TXT] = fio___mustache_i_txt,
    [FIO___MUSTACHE_I_VAR] = fio___mustache_i_var,
    [FIO___MUSTACHE_I_VAR_RAW] = fio___mustache_i_var_raw,
    [FIO___MUSTACHE_I_ARY] = fio___mustache_i_ary,
    [FIO___MUSTACHE_I_MISSING] = fio___mustache_i_missing,
    [FIO___MUSTACHE_I_PADDING_PUSH] = fio___mustache_i_padding_push,
    [FIO___MUSTACHE_I_PADDING_POP] = fio___mustache_i_padding_pop,
#if FIO_MUSTACHE_PRESERVE_PADDING
    [FIO___MUSTACHE_I_VAR_PADDED] = fio___mustache_i_var_padded,
    [FIO___MUSTACHE_I_VAR_RAW_PADDED] = fio___mustache_i_var_raw_padded,
#endif
#if FIO_MUSTACHE_LAMBDA_SUPPORT
    [FIO___MUSTACHE_I_METADATA] = fio___mustache_i_metadata,
#endif

  };
  while (p)
    p = map[(uint8_t)(p[0])](p, &a);
  return a.args->udata;
}

/* *****************************************************************************
Instructions - Helpers
***************************************************************************** */

/* consumes `val_name`, in whole or in part, returning the variable found.
 * sets `val_name` to the unconsumed partial remaining.
 */
FIO_IFUNC void *fio___mustache_get_var_in_context(fio_mustache_bargs_s *a,
                                                  void *ctx,
                                                  fio_buf_info_s *val_name) {
  void *v = ctx;
  v = a->get_var(ctx, *val_name);
  if (v) {
    val_name->len = 0;
    return v;
  }
  char *s = val_name->buf;
  char *end = val_name->buf + val_name->len;
  for (;;) {
    if (s == end)
      return v;
    s = (char *)FIO_MEMCHR(s, '.', (size_t)(end - s));
    if (!s)
      return v;
    v = a->get_var(ctx,
                   FIO_BUF_INFO2(val_name->buf, (size_t)(s - val_name->buf)));
    ++s;
    if (!v)
      continue;
    val_name->buf = s;
    val_name->len = (size_t)(end - s);
    return v;
  }
}

FIO_IFUNC void *fio___mustache_get_var(fio___mustache_bldr_s *b,
                                       fio_buf_info_s val_name) {
  void *v = b->ctx;
  if (val_name.len == 1 && val_name.buf[0] == '.')
    return v;
  for (;;) {
    if (b->ctx)
      v = fio___mustache_get_var_in_context(b->args, b->ctx, &val_name);
    if (v)
      break;
#if FIO_MUSTACHE_ISOLATE_PARTIALS
    if (b->stop)
      return v;
#endif
    b = b->prev;
    if (!b)
      return v;
  }
  while (val_name.len && v)
    v = fio___mustache_get_var_in_context(b->args, v, &val_name);
  return v;
}

FIO_SFUNC void fio___mustache_write_padding(fio___mustache_bldr_s *b) {
  while (b && b->padding.len) {
    if (b->padding.buf) {
      b->args->udata = b->args->write_text(b->args->udata, b->padding);
    }
    b = b->prev;
  }
}

FIO_SFUNC void fio___mustache_write_text_simple(
    fio___mustache_bldr_s *b,
    fio_buf_info_s txt,
    void *(*writer)(void *, fio_buf_info_s txt)) {
  b->args->udata = writer(b->args->udata, txt);
}

FIO_SFUNC void fio___mustache_write_text_complex(
    fio___mustache_bldr_s *b,
    fio_buf_info_s txt,
    void *(*writer)(void *, fio_buf_info_s txt)) {
  const char *end = txt.buf + txt.len;
  for (;;) {
    char *pos = (char *)FIO_MEMCHR(txt.buf, '\n', (size_t)(end - txt.buf));
    if (!pos)
      break;
    ++pos;
    if (txt.buf[0] != '\n' || (size_t)(pos - txt.buf) > 1)
      fio___mustache_write_padding(b);
    b->args->udata =
        writer(b->args->udata, FIO_BUF_INFO2(txt.buf, (size_t)(pos - txt.buf)));
    txt.buf = pos;
    if (pos < end)
      continue;
    return;
  }
  if (txt.buf < end) {
    fio___mustache_write_padding(b);
    b->args->udata =
        writer(b->args->udata, FIO_BUF_INFO2(txt.buf, (size_t)(end - txt.buf)));
  }
}

FIO_IFUNC void fio___mustache_writer_route(
    fio___mustache_bldr_s *b,
    fio_buf_info_s txt,
    void *(*writer)(void *, fio_buf_info_s txt)) {
  void (*router[2])(fio___mustache_bldr_s *,
                    fio_buf_info_s txt,
                    void *(*writer)(void *, fio_buf_info_s txt)) = {
      fio___mustache_write_text_complex,
      fio___mustache_write_text_simple};
  router[!(b->padding.len)](b, txt, writer);
}

/* *****************************************************************************
Instruction Implementations
***************************************************************************** */

FIO_SFUNC char *fio___mustache_i_stack_pop(char *p, fio___mustache_bldr_s *b) {
  return NULL;
  (void)p, (void)b;
}
FIO_SFUNC char *fio___mustache_i_stack_push(char *p, fio___mustache_bldr_s *b) {
  char *npos = p + 5;
  p = b->root + fio_buf2u32u(p + 1);
  fio___mustache_bldr_s builder = {
    .root = b->root,
    .prev = b,
#if FIO_MUSTACHE_ISOLATE_PARTIALS
    .ctx = b->ctx,
#endif
    .padding = FIO_BUF_INFO2(NULL, b->padding.len),
    .args = b->args,
#if FIO_MUSTACHE_ISOLATE_PARTIALS
    .stop = 1,
#endif
  };
  fio___mustache_build_section(npos, builder);
  return p;
}
FIO_SFUNC char *fio___mustache_i_goto_push(char *p, fio___mustache_bldr_s *b) {
  char *npos = b->root + fio_buf2u32u(p + 1);
  fio___mustache_bldr_s builder = {
    .root = b->root,
    .prev = b,
#if FIO_MUSTACHE_ISOLATE_PARTIALS
    .ctx = b->ctx,
#endif
    .padding = FIO_BUF_INFO2(NULL, b->padding.len),
    .args = b->args,
#if FIO_MUSTACHE_ISOLATE_PARTIALS
    .stop = 1,
#endif
  };
  fio___mustache_build_section(npos, builder);
  p += 5;
  return p;
}
FIO_SFUNC char *fio___mustache_i_txt(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s txt = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
  p = txt.buf + txt.len;
  fio___mustache_writer_route(b, txt, b->args->write_text);
  return p;
}

FIO_IFUNC char *fio___mustache_i_var_internal(
    char *p,
    fio___mustache_bldr_s *b,
    void *(*writer)(void *, fio_buf_info_s txt)) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
  p = var.buf + var.len;
  void *v = fio___mustache_get_var(b, var);
  if (!v)
    return p;
  var = b->args->var2str(v);
  b->args->release_var(v);
#if FIO_MUSTACHE_PRESERVE_PADDING
  fio___mustache_writer_route(b, var, writer);
#else
  fio___mustache_write_padding(b);
  b->args->udata = writer(b->args->udata, var);
#endif
  return p;
}
FIO_SFUNC char *fio___mustache_i_var(char *p, fio___mustache_bldr_s *b) {
  return fio___mustache_i_var_internal(p, b, b->args->write_text_escaped);
}
FIO_SFUNC char *fio___mustache_i_var_raw(char *p, fio___mustache_bldr_s *b) {
  return fio___mustache_i_var_internal(p, b, b->args->write_text);
}

FIO_SFUNC char *fio___mustache_i_ary(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 7, fio_buf2u16u(p + 1));
  uint32_t skip_pos = fio_buf2u32u(p + 3);
  p = b->root + skip_pos;
#if FIO_MUSTACHE_LAMBDA_SUPPORT
  fio_buf_info_s section_raw_txt = FIO_BUF_INFO2(NULL, 0);
  if (p[0] == FIO___MUSTACHE_I_METADATA) {
    section_raw_txt = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
    p = section_raw_txt.buf + section_raw_txt.len;
  }
#else
  const fio_buf_info_s section_raw_txt = FIO_BUF_INFO2(NULL, 0);
#endif

  void *v = fio___mustache_get_var(b, var);
  if (!(b->args->var_is_truthful(v)))
    return p;
  size_t index = 0;
  const size_t ary_len = b->args->array_length(v);
  void *nctx = v;
  if (ary_len)
    nctx = b->args->get_var_index(v, index);
  for (;;) {
    ++index;
    fio___mustache_bldr_s builder = {
        .root = b->root,
        .prev = b,
        .ctx = nctx,
        .padding = FIO_BUF_INFO2(NULL, b->padding.len),
        .args = b->args,
    };
    if (!b->args->is_lambda(&(b->args->udata), nctx, section_raw_txt)) {
      fio___mustache_build_section(var.buf + var.len, builder);
    }
    b->args->release_var(nctx);
    if (index >= ary_len) {
      if (nctx != v)
        b->args->release_var(v);
      return p;
    }
    nctx = b->args->get_var_index(v, index);
  }
}
FIO_SFUNC char *fio___mustache_i_missing(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 7, fio_buf2u16u(p + 1));
  uint32_t skip_pos = fio_buf2u32u(p + 3);
  p = b->root + skip_pos;

  void *v = fio___mustache_get_var(b, var);
  if (b->args->var_is_truthful(v)) {
    b->args->release_var(v);
    return p;
  }

  fio___mustache_bldr_s builder = {
      .root = b->root,
      .prev = b,
      .padding = FIO_BUF_INFO2(NULL, b->padding.len),
      .args = b->args,
  };
  fio___mustache_build_section(var.buf + var.len, builder);
  return p;
}

FIO_SFUNC char *fio___mustache_i_padding_push(char *p,
                                              fio___mustache_bldr_s *b) {
  b->padding = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
  return p + 3 + b->padding.len;
}
FIO_SFUNC char *fio___mustache_i_padding_pop(char *p,
                                             fio___mustache_bldr_s *b) {
  b->padding = FIO_BUF_INFO2(NULL, 0);
  if (b->prev)
    b->padding.len = b->prev->padding.len;
  return p + 1;
}

#if FIO_MUSTACHE_PRESERVE_PADDING

FIO_SFUNC char *fio___mustache_i_var_padded(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 5, fio_buf2u16u(p + 1));
  fio_buf_info_s padding = FIO_BUF_INFO2(p + 5 + var.len, fio_buf2u16u(p + 3));
  p = padding.buf + padding.len;
  void *v = fio___mustache_get_var(b, var);
  if (!v)
    return p;
  var = b->args->var2str(v);
  if (!var.len)
    goto done;
  fio___mustache_bldr_s b2 = *b;
  b2.padding = padding;
  fio___mustache_writer_route(&b2, var, b->args->write_text_escaped);
done:
  b->args->release_var(v);
  return p;
}
FIO_SFUNC char *fio___mustache_i_var_raw_padded(char *p,
                                                fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 5, fio_buf2u16u(p + 1));
  fio_buf_info_s padding = FIO_BUF_INFO2(p + 5 + var.len, fio_buf2u16u(p + 3));
  p = padding.buf + padding.len;
  void *v = fio___mustache_get_var(b, var);
  if (!v)
    return p;
  var = b->args->var2str(v);
  if (!var.len)
    goto done;
  fio___mustache_bldr_s b2 = *b;
  b2.padding = padding;
  fio___mustache_writer_route(&b2, var, b->args->write_text);
  b->args->release_var(v);
  return p;
}

#endif

#if FIO_MUSTACHE_LAMBDA_SUPPORT
FIO_SFUNC char *fio___mustache_i_metadata(char *p, fio___mustache_bldr_s *b) {
  uint32_t len = fio_buf2u16u(p + 1);
  return p + 3 + len;
  (void)b;
}
#endif

/* *****************************************************************************
Mustache delimiter testing
***************************************************************************** */
FIO_SFUNC _Bool fio___mustache_delcmp1(const char *restrict a,
                                       const char *restrict b) {
  return 1;
  (void)a, (void)b;
}
FIO_SFUNC _Bool fio___mustache_delcmp2(const char *restrict a,
                                       const char *restrict b) {
  return a[1] == b[1];
}
FIO_SFUNC _Bool fio___mustache_delcmp3(const char *restrict a,
                                       const char *restrict b) {
  return a[1] == b[1] && a[2] == b[2];
}
FIO_SFUNC _Bool fio___mustache_delcmp4(const char *restrict a,
                                       const char *restrict b) {
  return a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

typedef struct fio___mustache_delimiter_s {
  struct {
    _Bool (*cmp)(const char *restrict, const char *restrict);
    uint32_t len;
    char buf[4];
  } in, out;
} fio___mustache_delimiter_s;

FIO_IFUNC fio___mustache_delimiter_s fio___mustache_delimiter_init(void) {
  fio___mustache_delimiter_s r = {
      .in = {.cmp = fio___mustache_delcmp2, .len = 2, .buf = {'{', '{'}},
      .out = {.cmp = fio___mustache_delcmp2, .len = 2, .buf = {'}', '}'}},
  };
  return r;
}

/* *****************************************************************************
Parser type & helpers
***************************************************************************** */

typedef struct fio___mustache_parser_s {
  char *root;
  struct fio___mustache_parser_s *prev;
  fio_mustache_load_args_s *args;
  fio___mustache_delimiter_s delim;
  fio_buf_info_s fname;
  fio_buf_info_s path;
  fio_buf_info_s backwards;
  fio_buf_info_s forwards;
  uint32_t starts_at;
  uint32_t depth;
  uint32_t dirty;
} fio___mustache_parser_s;

/* *****************************************************************************
Template file loading
***************************************************************************** */
FIO_SFUNC fio_buf_info_s
fio___mustache_load_template(fio___mustache_parser_s *p, fio_buf_info_s fname) {
  /* Attempt to load templates in the following order:
   * 1. Calling template folder
   * 2. Parent calling folder (recursively)?
   * 3. Working folder.
   */
  fio_buf_info_s r = {0};
  fio_buf_info_s const extensions[] = {FIO_BUF_INFO1((char *)".mustache"),
                                       FIO_BUF_INFO1((char *)".html"),
                                       FIO_BUF_INFO2((char *)"", 0),
                                       {0}};
  FIO_STR_INFO_TMP_VAR(fn, (PATH_MAX | 2094));
  if (FIO_UNLIKELY(!fname.len || fname.len > (PATH_MAX - 1)))
    return r;
  fio___mustache_parser_s *tp = p;
  /* TODO: iterate file names to test for a match... */
  if (fname.buf[0] != FIO_FOLDER_SEPARATOR && fname.buf[0] != '/') {
    for (;;) { /* test and load file with a possible relative base path... */
      /* test if file was previously loaded (with this base-path) */
      for (;;) {
        if (FIO_BUF_INFO_IS_EQ(tp->fname, fname))
          goto already_exists;
        if (tp->path.buf)
          break; /* we arrived at current relative path root */
        tp = tp->prev;
        if (!tp)
          goto absolute_path_or_cwd;
      }
      /* test current relative path with each filename & extension combo */
      if (tp->path.len + fname.len + 32 < ((PATH_MAX | 2094) - 1)) {
        for (size_t i = 0; extensions[i].buf; ++i) {
          fn.len = 0;
          fio_string_write2(
              &fn,
              NULL,
              FIO_STRING_WRITE_STR2(tp->path.buf, tp->path.len),
              FIO_STRING_WRITE_STR2(fname.buf, fname.len),
              FIO_STRING_WRITE_STR2(extensions[i].buf, extensions[i].len));
          r = p->args->load_file_data(FIO_STR2BUF_INFO(fn), p->args->udata);
          if (r.len)
            goto file_loaded_successfully;
        }
      }
      tp = tp->prev;
      if (!tp)
        goto absolute_path_or_cwd;
    }
  }

absolute_path_or_cwd:
  /* possibly full-path specified + fallback to working folder */
  for (size_t i = 0; extensions[i].buf; ++i) {
    fn.len = 0;
    fio_string_write2(
        &fn,
        NULL,
        FIO_STRING_WRITE_STR2(fname.buf, fname.len),
        FIO_STRING_WRITE_STR2(extensions[i].buf, extensions[i].len));
    r = p->args->load_file_data(FIO_STR2BUF_INFO(fn), p->args->udata);
    if (r.len)
      goto file_loaded_successfully;
  }

file_loaded_successfully:
  return r;

already_exists:
  fn.len = 5; /* TODO: fixme? */
  fn.buf[0] = FIO___MUSTACHE_I_GOTO_PUSH;
  fio_u2buf32u(fn.buf + 1, tp->starts_at);
  p->root = fio_bstr_write(p->root, fn.buf, fn.len);
  return r;
}

FIO_SFUNC void fio___mustache_free_template(fio___mustache_parser_s *p,
                                            fio_buf_info_s d) {
  p->args->free_file_data(d, p->args->udata);
}

/* *****************************************************************************
Tag Helpers
***************************************************************************** */

/* forward declaration, implemented later */
FIO_SFUNC int fio___mustache_parse_block(fio___mustache_parser_s *p);
FIO_SFUNC int fio___mustache_parse_template_file(fio___mustache_parser_s *p);

FIO_IFUNC void fio___mustache_stand_alone_skip_eol(fio___mustache_parser_s *p) {
  size_t offset =
      !p->dirty && p->forwards.buf[0] == '\r' && p->forwards.buf[1] == '\n';
  p->forwards.buf += offset;
  p->forwards.len -= offset;
  offset = !p->dirty && p->forwards.buf[0] == '\n';
  p->forwards.buf += offset;
  p->forwards.len -= offset;
}

FIO_IFUNC int fio___mustache_parse_add_text(fio___mustache_parser_s *p,
                                            fio_buf_info_s txt) {
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  buf.u8[0] = FIO___MUSTACHE_I_TXT;
  FIO_ASSERT_DEBUG(txt.len < (1 << 16),
                   "(mustache) text instruction overflow!");
  fio_u2buf16u(buf.u8 + 1, txt.len);
  p->root = fio_bstr_write2(p->root,
                            FIO_STRING_WRITE_STR2(buf.u8, 3),
                            FIO_STRING_WRITE_STR2(txt.buf, txt.len));
  return 0;
}

FIO_IFUNC int fio___mustache_parse_comment(fio___mustache_parser_s *p,
                                           fio_buf_info_s comment) {
  fio___mustache_stand_alone_skip_eol(p);
  return 0;
  (void)comment;
}

FIO_IFUNC int fio___mustache_parse_section_end(fio___mustache_parser_s *p,
                                               fio_buf_info_s var) {
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  char *prev = NULL;
  fio_buf_info_s old_var_name;
  buf.u8[0] = FIO___MUSTACHE_I_STACK_POP;
  if (!p->prev)
    goto section_not_open;
  prev = p->root + p->starts_at;
  if (*prev != FIO___MUSTACHE_I_ARY && *prev != FIO___MUSTACHE_I_MISSING)
    goto section_not_open;
  old_var_name = FIO_BUF_INFO2(prev + 7, (size_t)fio_buf2u16u(prev + 1));
  if (!FIO_BUF_INFO_IS_EQ(old_var_name, var))
    goto value_name_mismatch;

  fio_u2buf32u(prev + 3, fio_bstr_len(p->root) + 1);
  fio___mustache_stand_alone_skip_eol(p);

#if FIO_MUSTACHE_LAMBDA_SUPPORT
  old_var_name =
      FIO_BUF_INFO2(p->prev->forwards.buf,
                    (size_t)(p->backwards.buf - p->prev->forwards.buf));
  old_var_name.len -= (old_var_name.len && old_var_name.buf[-1] == '\n');
  old_var_name.len -= (old_var_name.len && old_var_name.buf[-1] == '\r');
  if (old_var_name.len && old_var_name.len < (1U << 16)) {
    buf.u8[1] = FIO___MUSTACHE_I_METADATA;
    fio_u2buf16u(buf.u8 + 2, old_var_name.len);
    p->root = fio_bstr_write2(
        p->root,
        FIO_STRING_WRITE_STR2(buf.u8, 4),
        FIO_STRING_WRITE_STR2(old_var_name.buf, old_var_name.len));
  } else
#endif
    p->root = fio_bstr_write(p->root, buf.u8, 1);
  return -2;

value_name_mismatch:
  FIO_LOG_ERROR(
      "(mustache) template section end tag doesn't match section start:"
      "\n\t\t%.*s != %.*s",
      (int)var.len,
      var.buf,
      (int)old_var_name.len,
      old_var_name.buf);
  return -1;

section_not_open:
  FIO_LOG_ERROR("(mustache) section end tag with no section opening tag?"
                "\n\t\t%.*s",
                (int)var.len,
                var.buf);
  return -1;
}

FIO_IFUNC int fio___mustache_parse_section_start(fio___mustache_parser_s *p,
                                                 fio_buf_info_s var,
                                                 size_t inverted) {
  if (p->depth == FIO_MUSTACHE_MAX_DEPTH)
    return -1;
  fio___mustache_stand_alone_skip_eol(p);

  fio___mustache_parser_s new_section = {
      .root = p->root,
      .prev = p,
      .args = p->args,
      .delim = p->delim,
      .forwards = p->forwards,
      .starts_at = (uint32_t)fio_bstr_len(p->root),
      .depth = p->depth + 1,
      .dirty = p->dirty,
  };
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  buf.u8[0] = FIO___MUSTACHE_I_ARY + inverted;
  fio_u2buf16u(buf.u8 + 1, var.len);
  /* + 32 bit value to be filled by closure. */
  new_section.root = fio_bstr_write2(new_section.root,
                                     FIO_STRING_WRITE_STR2(buf.u8, 7),
                                     FIO_STRING_WRITE_STR2(var.buf, var.len));
#if FIO_MUSTACHE_PRESERVE_PADDING
  if (!p->dirty && p->backwards.len) {
    buf.u8[0] = FIO___MUSTACHE_I_PADDING_PUSH;
    fio_u2buf16u(buf.u8 + 1, p->backwards.len);
    new_section.root = fio_bstr_write2(
        new_section.root,
        FIO_STRING_WRITE_STR2(buf.u8, 3),
        FIO_STRING_WRITE_STR2(p->backwards.buf, p->backwards.len));
  }
#endif
  int r = fio___mustache_parse_block(&new_section);
  p->root = new_section.root;
  p->forwards = new_section.forwards;
  return r;
}

FIO_IFUNC int fio___mustache_parse_partial(fio___mustache_parser_s *p,
                                           fio_buf_info_s filename) {
  if (p->depth == FIO_MUSTACHE_MAX_DEPTH)
    return -1;

  fio___mustache_stand_alone_skip_eol(p);

  fio_buf_info_s file_content = fio___mustache_load_template(p, filename);
  if (!file_content.len)
    return 0;

  union {
    uint64_t u64[1];
    char u8[8];
  } buf;

  buf.u8[0] = FIO___MUSTACHE_I_STACK_PUSH;
  size_t ipos = fio_bstr_len(p->root) + 1;
  p->root = fio_bstr_write(p->root, buf.u8, 5);

  if (!p->dirty && p->backwards.len) {
    buf.u8[0] = FIO___MUSTACHE_I_PADDING_PUSH;
    fio_u2buf16u(buf.u8 + 1, p->backwards.len);
    p->root = fio_bstr_write2(
        p->root,
        FIO_STRING_WRITE_STR2(buf.u8, 3),
        FIO_STRING_WRITE_STR2(p->backwards.buf, p->backwards.len));
  }

  fio___mustache_parser_s new_section = {
      .root = p->root,
      .prev = p,
      .args = p->args,
      .delim = fio___mustache_delimiter_init(),
      .fname = filename,
      .path = fio_filename_parse2(filename.buf, filename.len).folder,
      .forwards = file_content,
      .starts_at = (uint32_t)fio_bstr_len(p->root),
      .depth = p->depth + 1,
      .dirty = 0,
  };

  int r = fio___mustache_parse_template_file(&new_section);
  buf.u8[0] = FIO___MUSTACHE_I_STACK_POP;
  p->root = fio_bstr_write(new_section.root, buf.u8, 1);
  fio_u2buf32u(p->root + ipos, fio_bstr_len(p->root));
  fio___mustache_free_template(p, file_content);
  return r;
}

FIO_IFUNC int fio___mustache_parse_set_delim(fio___mustache_parser_s *p,
                                             fio_buf_info_s buf) {
  struct {
    uint32_t len;
    void *(*cpy)(void *a, const void *b);
    _Bool (*cmp)(const char *restrict, const char *restrict);
  } const len_map[] = {
      {0},
      {1, fio_memcpy1, fio___mustache_delcmp1},
      {2, fio_memcpy2, fio___mustache_delcmp2},
      {3, fio_memcpy3, fio___mustache_delcmp3},
      {4, fio_memcpy4, fio___mustache_delcmp4},
  };

  fio___mustache_stand_alone_skip_eol(p);

  char *end = buf.buf + buf.len;
  char *pos = buf.buf;
  while (pos < end && *pos != ' ' && *pos != '\t')
    ++pos;
  if (pos == end)
    goto delim_tag_error;
  buf.len = (size_t)(pos - buf.buf);
  while (*pos == ' ' || *pos == '\t')
    ++pos;
  if (pos >= end)
    goto delim_tag_error;

  if ((size_t)(end - pos) > 4UL || !(size_t)(end - pos) || !buf.len ||
      buf.len > 4UL)
    goto delim_tag_error;
  len_map[buf.len].cpy(p->delim.in.buf, buf.buf);
  len_map[(size_t)(end - pos)].cpy(p->delim.out.buf, pos);
  p->delim.in.cmp = len_map[buf.len].cmp;
  p->delim.out.cmp = len_map[(size_t)(end - pos)].cmp;
  p->delim.in.len = len_map[buf.len].len;
  p->delim.out.len = len_map[(size_t)(end - pos)].len;
  return 0;

delim_tag_error:
  FIO_LOG_ERROR("(mustache) delimiter tag error: %.*s",
                (int)(end - buf.buf),
                buf.buf);
  return -1;
}

FIO_IFUNC int fio___mustache_parse_var_name(fio___mustache_parser_s *p,
                                            fio_buf_info_s var,
                                            size_t raw) {
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  if (p->backwards.len > ((1 << 16) - 1))
    p->backwards.len = 0;

#if FIO_MUSTACHE_PRESERVE_PADDING
  if (p->backwards.len)
    goto padded;
#else
  fio___mustache_parse_add_text(p, p->backwards);
#endif
  buf.u8[0] = (char)(FIO___MUSTACHE_I_VAR + raw);
  fio_u2buf16u(buf.u8 + 1, var.len);
  p->root = fio_bstr_write2(p->root,
                            FIO_STRING_WRITE_STR2(buf.u8, 3),
                            FIO_STRING_WRITE_STR2(var.buf, var.len));
  return 0;
#if FIO_MUSTACHE_PRESERVE_PADDING
padded:
  /* TODO: fixme (what if padding value is already used?) */
  buf.u8[0] = (char)(FIO___MUSTACHE_I_VAR_PADDED + raw);
  fio_u2buf16u(buf.u8 + 1, var.len);
  fio_u2buf16u(buf.u8 + 3, p->backwards.len);
  p->root = fio_bstr_write2(
      p->root,
      FIO_STRING_WRITE_STR2(buf.u8, 5),
      FIO_STRING_WRITE_STR2(var.buf, var.len),
      FIO_STRING_WRITE_STR2(p->backwards.buf, p->backwards.len));
  return 0;
#endif
}

/* *****************************************************************************
Tag Consumer
***************************************************************************** */

FIO_SFUNC int fio___mustache_parse_consume_tag(fio___mustache_parser_s *p,
                                               fio_buf_info_s buf) {
  /* remove white-space from name */
  for (; buf.len &&
         (buf.buf[buf.len - 1] == ' ' || buf.buf[buf.len - 1] == '\t');)
    --buf.len;
  if (!buf.len) {
    FIO_LOG_ERROR("(mustache) template tags must contain a value!");
    return -1;
  }

  while (buf.buf[0] == ' ' || buf.buf[0] == '\t') {
    ++buf.buf;
    --buf.len;
  }
  char id = buf.buf[0];
  if (!(id == '/' || id == '#' || id == '^' || id == '>' || id == '!' ||
        id == '&' || (id == '=' && buf.buf[buf.len - 1] == '=') ||
        (id == '{' && buf.buf[buf.len - 1] == '}')))
    return fio___mustache_parse_var_name(p, buf, 0); /* escaped var */

  /* tag starts with a marker, seek new tag starting point */
  do {
    ++buf.buf;
    --buf.len;
    if (buf.len)
      continue;
    FIO_LOG_ERROR("(mustache) template tags must contain a value!");
    return -1;
  } while (buf.buf[0] == ' ' || buf.buf[0] == '\t');
  /* test for tag type and route to handler */
  switch (id) {
  case '/': return fio___mustache_parse_section_end(p, buf);
  case '#': return fio___mustache_parse_section_start(p, buf, 0);
  case '^': return fio___mustache_parse_section_start(p, buf, 1);
  case '>': return fio___mustache_parse_partial(p, buf);
  case '!': return fio___mustache_parse_comment(p, buf);
  case '=': /* fall through */
  case '{':
    do /* it is known that (buf.buf ends as '=' or '}')*/
      --buf.len;
    while (buf.buf[buf.len - 1] == ' ' || buf.buf[buf.len - 1] == '\t');

    if (id == '=')
      return fio___mustache_parse_set_delim(p, buf); /* fall through */
  default: /* raw var */ return fio___mustache_parse_var_name(p, buf, 1);
  }
}

/* *****************************************************************************
File Consumer parser
***************************************************************************** */

FIO_SFUNC int fio___mustache_parse_block(fio___mustache_parser_s *p) {
  int r = 0;
  const char *end = p->forwards.buf + p->forwards.len;
  fio_buf_info_s tag;
  p->backwards = FIO_BUF_INFO2(p->forwards.buf, 0);
  p->root = fio_bstr_reserve(p->root, p->forwards.len);
  /* consume each line (in case it's a stand alone line) */
  for (;;) {
    p->backwards.len = (size_t)(p->forwards.buf - p->backwards.buf);
    if (p->forwards.buf >= end)
      break;
    if (FIO_UNLIKELY(*p->forwards.buf == p->delim.in.buf[0] &&
                     p->delim.in.cmp(p->forwards.buf, p->delim.in.buf))) {
      /* tag started */
      p->forwards.buf += p->delim.in.len;
      tag = FIO_BUF_INFO2(p->forwards.buf, 0);
      for (;;) {
        if (p->forwards.buf + p->delim.out.len > end)
          goto incomplete_tag_error;
        if (p->forwards.buf[0] == p->delim.out.buf[0] &&
            p->delim.out.cmp(p->forwards.buf, p->delim.out.buf))
          break;
        ++(p->forwards.buf);
      }
      /* advance tag ending when triple mustache is detected. */
      p->forwards.buf +=
          ((p->forwards.buf + p->delim.out.len) < end &&
           p->forwards.buf[0] == '}' &&
           p->delim.out.cmp(p->forwards.buf + 1, p->delim.out.buf));
      /* finalize tag */
      tag.len = p->forwards.buf - tag.buf;
      if (!tag.len)
        goto empty_tag_error;
      p->forwards.buf += p->delim.out.len;
      p->forwards.len = (size_t)(end - p->forwards.buf);
      p->dirty |= (unsigned)(p->forwards.buf[0] && p->forwards.buf[0] != '\r' &&
                             p->forwards.buf[0] != '\n');
      if (p->dirty && p->backwards.len) { /* not stand-alone, add txt */
        fio___mustache_parse_add_text(p, p->backwards);
        p->backwards = FIO_BUF_INFO2((p->backwards.buf + p->backwards.len), 0);
      }
      if ((r = fio___mustache_parse_consume_tag(p, tag)))
        goto done;
      p->backwards = FIO_BUF_INFO2(p->forwards.buf, 0);
      continue;
    }
    p->dirty = (unsigned)(p->forwards.buf[0] != '\n') &
               (p->dirty | (unsigned)(p->forwards.buf[0] != ' ' &&
                                      p->forwards.buf[0] != '\t'));
    ++p->forwards.buf;
    if (p->backwards.len == ((1 << 16) - 2) || p->forwards.buf[-1] == '\n') {
      p->backwards.len = p->forwards.buf - p->backwards.buf;
      fio___mustache_parse_add_text(p, p->backwards);
      p->backwards.buf = p->forwards.buf;
    }
  }
  /* print leftover text? */
  if (p->backwards.len)
    fio___mustache_parse_add_text(p, p->backwards);

done:
  r += ((r == -2) << 1); /* end-tag stop shouldn't propagate onward. */
  return r;
incomplete_tag_error:
  FIO_LOG_ERROR("(mustache) template error, un-terminated {{tag}}:\n\t%.*s",
                (int)(end - (p->backwards.buf + p->backwards.len) > 32
                          ? (int)32
                          : (int)(end - (p->backwards.buf + p->backwards.len))),
                (p->backwards.buf + p->backwards.len));
  return (r = -1);
empty_tag_error:
  FIO_LOG_ERROR("(mustache) template error, empty {{tag}}:\n\t%.*s",
                (int)(end - (p->backwards.buf + p->backwards.len) > 32
                          ? (int)32
                          : (int)(end - (p->backwards.buf + p->backwards.len))),
                (p->backwards.buf + p->backwards.len));
  return (r = -1);
}

FIO_SFUNC int fio___mustache_parse_template_file(fio___mustache_parser_s *p) {
  /* remove (possible) filename comment line */
  if (p->forwards.buf[0] == '@' && p->forwards.buf[1] == ':') {
    char *pos = (char *)FIO_MEMCHR(p->forwards.buf, '\n', p->forwards.len);
    if (!pos)
      return 0; /* done with file... though nothing happened. */
  }
  /* consume (possible) YAML front matter */
  if (p->forwards.buf[0] == '-' && p->forwards.buf[1] == '-' &&
      p->forwards.buf[2] == '-' &&
      (p->forwards.buf[3] == '\n' || p->forwards.buf[3] == '\r')) {
    const char *end = p->forwards.buf + p->forwards.len;
    const char *pos = p->forwards.buf;
    for (;;) {
      pos = (const char *)FIO_MEMCHR(pos, '\n', end - pos);
      if (!pos)
        return 0; /* done with file... though nothing happened. */
      ++pos;
      if (pos[0] == '-' && pos[1] == '-' && pos[2] == '-' &&
          (pos[3] == '\n' || pos[3] == '\r' || !pos[3])) {
        pos += 4;
        pos += pos[0] == '\n';
        break;
      }
    }
    p->args->on_yaml_front_matter(
        FIO_BUF_INFO2(p->forwards.buf, (size_t)(pos - p->forwards.buf)),
        p->args->udata);
    p->forwards.len = (size_t)(pos - p->forwards.buf);
    p->forwards.buf = (char *)pos;
  }
  return fio___mustache_parse_block(p);
}

/* *****************************************************************************
Default functions
***************************************************************************** */
FIO_SFUNC fio_buf_info_s fio___mustache_dflt_load_file_data(fio_buf_info_s fn,
                                                            void *udata) {
  char *data = fio_bstr_readfile(NULL, fn.buf, 0, 0);
  return fio_bstr_buf(data);
  (void)udata;
}

FIO_SFUNC void fio___mustache_dflt_free_file_data(fio_buf_info_s d,
                                                  void *udata) {
  fio_bstr_free(d.buf);
  (void)udata;
}

FIO_SFUNC void fio___mustache_dflt_on_yaml_front_matter(fio_buf_info_s y,
                                                        void *udata) {
  (void)y, (void)udata;
}

FIO_SFUNC void *fio___mustache_dflt_write_text(void *u, fio_buf_info_s txt) {
  return (void *)fio_bstr_write((char *)u, txt.buf, txt.len);
}

FIO_SFUNC void *fio___mustache_dflt_write_text_escaped(void *u,
                                                       fio_buf_info_s raw) {
  return (void *)fio_bstr_write_html_escape((char *)u, raw.buf, raw.len);
}

/* callback should return a new context pointer with the value of `name`. */
FIO_SFUNC void *fio___mustache_dflt_get_var(void *ctx, fio_buf_info_s name) {
  return NULL;
  (void)ctx, (void)name;
}

/* if context is an Array, should return its length. */
FIO_SFUNC size_t fio___mustache_dflt_array_length(void *ctx) {
  return 0;
  (void)ctx;
}

/* if context is an Array, should return a context pointer @ index. */
FIO_SFUNC void *fio___mustache_dflt_get_var_index(void *ctx, size_t index) {
  return NULL;
  (void)ctx, (void)index;
}
/* should return the String value of context `var` as a `fio_buf_info_s`. */
FIO_SFUNC fio_buf_info_s fio___mustache_dflt_var2str(void *var) {
  return FIO_BUF_INFO2(NULL, 0);
  (void)var;
}

FIO_SFUNC int fio___mustache_dflt_var_is_truthful(void *v) { return !!v; }

FIO_IFUNC void fio___mustache_dflt_release_var(void *ctx) { (void)ctx; }

/* returns non-zero if `ctx` is a lambda and handles section manually. */
FIO_SFUNC int fio___mustache_dflt_is_lambda(
    void **udata,
    void *ctx,
    fio_buf_info_s raw_template_section) {
  return 0;
  (void)raw_template_section, (void)ctx, (void)udata;
}

/* *****************************************************************************
Public API
***************************************************************************** */

void fio_mustache_load___(void); /* IDE Marker */
/* Allocates a new object on the heap and initializes it's memory. */
SFUNC fio_mustache_s *fio_mustache_load FIO_NOOP(fio_mustache_load_args_s a) {
  uint8_t should_free_data = 0;
  fio_buf_info_s base_path = {0};
  fio___mustache_parser_s parser = {0};
  if (!a.load_file_data && !a.free_file_data) {
    a.load_file_data = fio___mustache_dflt_load_file_data;
    a.free_file_data = fio___mustache_dflt_free_file_data;
  }
  if (!a.filename.buf && !a.data.buf)
    return NULL;
  if (!a.on_yaml_front_matter)
    a.on_yaml_front_matter = fio___mustache_dflt_on_yaml_front_matter;
  if (a.filename.buf) {
    fio_filename_s pathname;
    FIO_STR_INFO_TMP_VAR(fn, (PATH_MAX | 2096));
    if (!a.filename.len)
      a.filename.len = FIO_STRLEN(a.filename.buf);
    if (a.filename.buf[a.filename.len])
      fio_string_write(&fn, NULL, a.filename.buf, a.filename.len);
    else
      fn = FIO_BUF2STR_INFO(a.filename);
    if (!a.data.buf && fn.buf) {
      a.data = a.load_file_data(FIO_STR2BUF_INFO(fn), a.udata);
      if (!a.data.buf)
        return NULL;
      should_free_data = 1;
    }
    pathname = fio_filename_parse2(a.filename.buf, a.filename.len);
    base_path = pathname.folder;
  }
  parser.args = &a;
  parser.root = NULL;
  parser.delim = fio___mustache_delimiter_init();
  parser.depth = 0;
  parser.fname = a.filename;
  parser.forwards = a.data;
  parser.path = base_path;
  if (fio___mustache_parse_template_file(&parser)) { /* parser failed(!) */
    fio_bstr_free(parser.root);
    parser.root = NULL;
  }
  /* No need to write FIO___MUSTACHE_I_STACK_POP, as the string ends with NUL */
  if (should_free_data)
    a.free_file_data(a.data, a.udata);
  FIO___LEAK_COUNTER_ON_ALLOC(fio_mustache_s);
  return (fio_mustache_s *)parser.root;
}

/* Frees the mustache template object (or reduces it's reference count). */
SFUNC void fio_mustache_free(fio_mustache_s *m) {
  FIO___LEAK_COUNTER_ON_FREE(fio_mustache_s);
  fio_bstr_free((char *)m);
}

/** Increases the mustache template's reference count. */
SFUNC fio_mustache_s *fio_mustache_dup(fio_mustache_s *m) {
  FIO___LEAK_COUNTER_ON_ALLOC(fio_mustache_s);
  return (fio_mustache_s *)fio_bstr_copy((char *)m);
}

void fio_mustache_build___(void); /* IDE marker */
/** Builds the template, returning the final value of `udata` (or NULL). */
SFUNC void *fio_mustache_build FIO_NOOP(fio_mustache_s *m,
                                        fio_mustache_bargs_s args) {
  if (!m)
    return args.udata;
  if (!args.write_text && !args.write_text_escaped) {
    args.write_text = fio___mustache_dflt_write_text;
    args.write_text_escaped = fio___mustache_dflt_write_text_escaped;
  }
  FIO_ASSERT(args.write_text_escaped && args.write_text,
             "(mustache) fio_mustache_build requires both writer "
             "callbacks!\n\t\t(or none, for a fio_bstr_s return)");
  if (!args.get_var)
    args.get_var = fio___mustache_dflt_get_var;
  if (!args.array_length)
    args.array_length = fio___mustache_dflt_array_length;
  if (!args.get_var_index)
    args.get_var_index = fio___mustache_dflt_get_var_index;
  if (!args.var2str)
    args.var2str = fio___mustache_dflt_var2str;
  if (!args.var_is_truthful)
    args.var_is_truthful = fio___mustache_dflt_var_is_truthful;
  if (!args.release_var)
    args.release_var = fio___mustache_dflt_release_var;
  if (!args.is_lambda)
    args.is_lambda = fio___mustache_dflt_is_lambda;

  fio___mustache_bldr_s builder = {
      .root = (char *)m,
      .ctx = args.ctx,
      .args = &args,
  };
  return fio___mustache_build_section((char *)m, builder);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MODULE_PTR
#undef FIO_MUSTACHE
#undef FIO___UNTAG_T
#endif /* FIO_MUSTACHE */
