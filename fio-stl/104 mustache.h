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
  fio_buf_info_s (*load_file_data)(fio_buf_info_s filename);
  void (*free_file_data)(fio_buf_info_s file_data);
  void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter);
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
  /* callback should return the String value of `name` as a `fio_buf_info_s`. */
  fio_buf_info_s (*get_val)(void *ctx, fio_buf_info_s name);
  /* callback should return the String value of `var` as a `fio_buf_info_s`. */
  fio_buf_info_s (*val2str)(void *var);
  /* callback should return the context for the value of `name[indx]`, if any */
  void *(*enter)(void *ctx, fio_buf_info_s name, size_t indx, fio_buf_info_s t);
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
Module Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All short term / type memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

All long-term / system memory allocations should use:
* FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE(ptr, size)

Module and File Names:
======================

00# XXX.h - the module is a core module, independent or doesn't define a type
1## XXX.h - the module doesn't define a type, but requires memory allocations
2## XXX.h - the module defines a type
3## XXX.h - hashes / crypto.
4## XXX.h - server related modules
5## XXX.h - FIOBJ related modules
9## XXX.h - testing (usually use 902 XXX.h unless tests depend on other tests)

When
*/

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
// FIO___LEAK_COUNTER_DEF(fio_mustache_s)

/* *****************************************************************************
Instructions (relative state)

All instructions are 1 byte long with optional extra data.

Instruction refer to offsets rather than absolute values.
***************************************************************************** */
/* for ease of use, instructions are always a 1 byte numeral, */
typedef enum {
  FIO___MUSTACHE_I_STACK_POP,  /* 0 extra data (marks end of array / list)? */
  FIO___MUSTACHE_I_STACK_PUSH, /* 32 bit extra data (goes to position) */
  FIO___MUSTACHE_I_GOTO_PUSH,  /* 32 bit extra data (goes to position) */
  FIO___MUSTACHE_I_TXT,        /* 16 bits length + data */
  FIO___MUSTACHE_I_VAR,        /* 16 bits length + data */
  FIO___MUSTACHE_I_VAR_RAW,    /* 16 bits length + data */
  FIO___MUSTACHE_I_ARY,        /* 16 bits length + 32 bit skip-pos + data */
  FIO___MUSTACHE_I_MISSING,    /* 16 bits length + 32 bit skip-pos + data */
#if FIO_MUSTACHE_PRESERVE_PADDING
  FIO___MUSTACHE_I_PADDING_PUSH, /* 16 bits length + data */
  FIO___MUSTACHE_I_PADDING_POP,  /* 0 extra data */
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
  fio_mustache_bargs_s *args;
  uint32_t padding;
} fio___mustache_bldr_s;

FIO_SFUNC char *fio___mustache_i_stack_pop(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_stack_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_goto_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_txt(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_var(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_var_raw(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_ary(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_missing(char *p, fio___mustache_bldr_s *);
#if FIO_MUSTACHE_PRESERVE_PADDING
FIO_SFUNC char *fio___mustache_i_padding_push(char *p, fio___mustache_bldr_s *);
FIO_SFUNC char *fio___mustache_i_padding_pop(char *p, fio___mustache_bldr_s *);
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
#if FIO_MUSTACHE_PRESERVE_PADDING
    [FIO___MUSTACHE_I_PADDING_PUSH] = fio___mustache_i_padding_push,
    [FIO___MUSTACHE_I_PADDING_POP] = fio___mustache_i_padding_pop,
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

FIO_IFUNC fio_buf_info_s fio___mustache_get_var(fio___mustache_bldr_s *b,
                                                fio_buf_info_s val_name) {
  fio_buf_info_s r = FIO_BUF_INFO2(NULL, 0);
  if (val_name.len == 1 && val_name.buf[0] == '.')
    goto get_self;
  if (FIO_MEMCHR(val_name.buf, '.', val_name.len))
    goto limited_context;
  do {
    r = b->args->get_val(b->ctx, val_name);
    if (r.buf)
      return r;
  } while ((b = b->prev));
  return r;
get_self:
  r = b->args->val2str(b->ctx);
  return r;
limited_context:
  while (b && !b->ctx)
    b = b->prev;
  if (!b)
    return r;
  r = b->args->get_val(b->ctx, val_name);
  return r;
}

FIO_IFUNC void *fio___mustache_get_ctx(fio___mustache_bldr_s *b,
                                       fio_buf_info_s val_name,
                                       size_t i,
                                       uint32_t meta) {
  void *r = NULL;
#if FIO_MUSTACHE_LAMBDA_SUPPORT
  fio_buf_info_s t = {.buf = b->root + meta + 3,
                      .len = fio_buf2u16u(b->root + meta + 1)};
#else
  fio_buf_info_s t = {0};
#endif
  if (FIO_MEMCHR(val_name.buf, '.', val_name.len))
    goto limited_context;
  do {
    r = b->args->enter(b->ctx, val_name, i, t);
    if (r)
      return r;
  } while ((b = b->prev));
  return r;
  (void)meta; /* if unused */
limited_context:
  while (b && !b->ctx)
    b = b->prev;
  if (!b)
    return r;
  r = b->args->enter(b->ctx, val_name, i, t);
  return r;
}

#if FIO_MUSTACHE_PRESERVE_PADDING

FIO_SFUNC void fio___mustache_write_padding(fio___mustache_bldr_s *b) {
  while (b && b->padding) {
    if (b->padding > 1) {
      char *pos = b->root + b->padding;
      fio_buf_info_s pad = FIO_BUF_INFO2(pos + 3, fio_buf2u16u(pos + 1));
      b->args->udata = b->args->write_text(b->args->udata, pad);
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
  router[!b->padding](b, txt, writer);
}

#else /* FIO_MUSTACHE_PRESERVE_PADDING */

FIO_IFUNC void fio___mustache_writer_route(
    fio___mustache_bldr_s *b,
    fio_buf_info_s txt,
    void *(*writer)(void *, fio_buf_info_s txt)) {
  b->args->udata = writer(b->args->udata, txt);
}

#endif /* FIO_MUSTACHE_PRESERVE_PADDING */

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
  // .ctx = b->ctx,
#if FIO_MUSTACHE_PRESERVE_PADDING
    .padding = !!b->padding,
#endif
    .args = b->args,
  };
  fio___mustache_build_section(npos, builder);
  return p;
}
FIO_SFUNC char *fio___mustache_i_goto_push(char *p, fio___mustache_bldr_s *b) {
  char *npos = b->root + fio_buf2u32u(p + 1);
  fio___mustache_bldr_s builder = {
    .root = b->root,
    .prev = b,
  // .ctx = b->ctx,
#if FIO_MUSTACHE_PRESERVE_PADDING
    .padding = !!b->padding,
#endif
    .args = b->args,
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
FIO_SFUNC char *fio___mustache_i_var(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
  p = var.buf + var.len;
  var = fio___mustache_get_var(b, var);
  fio___mustache_writer_route(b, var, b->args->write_text_escaped);
  return p;
}
FIO_SFUNC char *fio___mustache_i_var_raw(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 3, fio_buf2u16u(p + 1));
  p = var.buf + var.len;
  var = fio___mustache_get_var(b, var);
  fio___mustache_writer_route(b, var, b->args->write_text);
  return p;
}
FIO_SFUNC char *fio___mustache_i_ary(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 7, fio_buf2u16u(p + 1));
  uint32_t skip_pos = fio_buf2u32u(p + 3);
  p = b->root + skip_pos;

  size_t index = 0;
  for (;;) {
#if FIO_MUSTACHE_LAMBDA_SUPPORT
    void *nctx = fio___mustache_get_ctx(b, var, index, skip_pos);
#else
    void *nctx = fio___mustache_get_ctx(b, var, index, 0);
#endif
    if (!nctx)
      break;
    ++index;
    fio___mustache_bldr_s builder = {
      .root = b->root,
      .prev = b,
      .ctx = nctx,
#if FIO_MUSTACHE_PRESERVE_PADDING
      .padding = !!b->padding,
#endif
      .args = b->args,
    };
    fio___mustache_build_section(var.buf + var.len, builder);
  }
  return p;
}
FIO_SFUNC char *fio___mustache_i_missing(char *p, fio___mustache_bldr_s *b) {
  fio_buf_info_s var = FIO_BUF_INFO2(p + 7, fio_buf2u16u(p + 1));
  uint32_t skip_pos = fio_buf2u32u(p + 3);
  p = b->root + skip_pos;
#if FIO_MUSTACHE_LAMBDA_SUPPORT
  void *nctx = fio___mustache_get_ctx(b, var, 0, skip_pos);
#else
  void *nctx = fio___mustache_get_ctx(b, var, 0, 0);
#endif
  if (!nctx) {
    fio___mustache_bldr_s builder = {
      .root = b->root,
      .prev = b,
    // .ctx = b->ctx,
#if FIO_MUSTACHE_PRESERVE_PADDING
      .padding = !!b->padding,
#endif
      .args = b->args,
    };
    fio___mustache_build_section(var.buf + var.len, builder);
  }
  return p;
}

#if FIO_MUSTACHE_PRESERVE_PADDING
FIO_SFUNC char *fio___mustache_i_padding_push(char *p,
                                              fio___mustache_bldr_s *b) {
  b->padding = (uint32_t)(p - b->root);
  return p + 3 + fio_buf2u16u(p + 1);
}
FIO_SFUNC char *fio___mustache_i_padding_pop(char *p,
                                             fio___mustache_bldr_s *b) {
  b->padding = !!(b->prev && b->prev->padding);
  return p + 1;
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
  fio_buf_info_s data;
  uint32_t starts_at;
  uint32_t depth;
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
  fio_buf_info_s const extensions[] = {FIO_BUF_INFO1((char *)".md"),
                                       FIO_BUF_INFO1((char *)".markdown"),
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
          r = p->args->load_file_data(FIO_STR2BUF_INFO(fn));
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
    r = p->args->load_file_data(FIO_STR2BUF_INFO(fn));
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
  p->args->free_file_data(d);
}

/* *****************************************************************************
Tag Helpers
***************************************************************************** */

/* forward declaration, implemented later */
FIO_SFUNC int fio___mustache_parse_block(fio___mustache_parser_s *p);
FIO_SFUNC int fio___mustache_parse_template_file(fio___mustache_parser_s *p);

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

#if FIO_MUSTACHE_LAMBDA_SUPPORT
  do
    --var.buf;
  while ((var.buf[0] != p->delim.in.buf[0] ||
          !p->delim.in.cmp(p->delim.in.buf, var.buf)) &&
         var.buf > p->prev->data.buf);
  old_var_name =
      FIO_BUF_INFO2(p->prev->data.buf, (size_t)(var.buf - p->prev->data.buf));
  if (old_var_name.len < (1U << 16)) {
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
                                                 fio_buf_info_s padding,
                                                 size_t inverted) {
  if (p->depth == FIO_MUSTACHE_MAX_DEPTH)
    return -1;
  fio___mustache_parser_s new_section = {
      .root = p->root,
      .prev = p,
      .delim = p->delim,
      .data = p->data,
      .args = p->args,
      .starts_at = (uint32_t)fio_bstr_len(p->root),
      .depth = p->depth + 1,
  };
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  for (size_t i = 0; i < 2; ++i) {
    size_t offset =
        (new_section.data.buf[0] == '\r' || new_section.data.buf[0] == '\n');
    new_section.data.buf += offset;
    new_section.data.len -= offset;
  }
  buf.u8[0] = FIO___MUSTACHE_I_ARY + inverted;
  fio_u2buf16u(buf.u8 + 1, var.len);
  /* + 32 bit value to be filled by closure. */
  new_section.root = fio_bstr_write2(p->root,
                                     FIO_STRING_WRITE_STR2(buf.u8, 7),
                                     FIO_STRING_WRITE_STR2(var.buf, var.len));
#if FIO_MUSTACHE_PRESERVE_PADDING
  buf.u8[0] = FIO___MUSTACHE_I_PADDING_PUSH;
  fio_u2buf16u(buf.u8 + 1, padding.len);
  new_section.root =
      fio_bstr_write2(new_section.root,
                      FIO_STRING_WRITE_STR2(buf.u8, 3),
                      FIO_STRING_WRITE_STR2(padding.buf, padding.len));
#endif
  int r = fio___mustache_parse_block(&new_section);
  p->root = new_section.root;
  p->data = new_section.data;
  return r;
  (void)padding; /* if unused */
}

FIO_IFUNC int fio___mustache_parse_partial(fio___mustache_parser_s *p,
                                           fio_buf_info_s filename,
                                           fio_buf_info_s padding) {
  if (p->depth == FIO_MUSTACHE_MAX_DEPTH)
    return -1;

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

#if FIO_MUSTACHE_PRESERVE_PADDING
  buf.u8[0] = FIO___MUSTACHE_I_PADDING_PUSH;
  fio_u2buf16u(buf.u8 + 1, padding.len);
  p->root = fio_bstr_write2(p->root,
                            FIO_STRING_WRITE_STR2(buf.u8, 3),
                            FIO_STRING_WRITE_STR2(padding.buf, padding.len));
#endif

  fio___mustache_parser_s new_section = {
      .root = p->root,
      .prev = p,
      .delim = fio___mustache_delimiter_init(),
      .path = fio_filename_parse2(filename.buf, filename.len).folder,
      .fname = filename,
      .data = file_content,
      .args = p->args,
      .starts_at = (uint32_t)fio_bstr_len(p->root),
      .depth = p->depth + 1,
  };

  int r = fio___mustache_parse_template_file(&new_section);
  buf.u8[0] = FIO___MUSTACHE_I_STACK_POP;
  p->root = fio_bstr_write(new_section.root, buf.u8, 1);
  fio_u2buf32u(p->root + ipos, fio_bstr_len(p->root));
  return r;
  (void)padding; /* if unused */
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
                                            fio_buf_info_s padding,
                                            size_t raw) {
  union {
    uint64_t u64[1];
    char u8[8];
  } buf;
  if (padding.len > ((1 << 16) - 1))
    padding.len = 0;

#if FIO_MUSTACHE_PRESERVE_PADDING
  if (padding.len)
    goto padded;
#else
  if (padding.len) {
    buf.u8[0] = (char)(FIO___MUSTACHE_I_TXT);
    fio_u2buf16u(buf.u8 + 1, padding.len);
    p->root = fio_bstr_write2(p->root,
                              FIO_STRING_WRITE_STR2(buf.u8, 3),
                              FIO_STRING_WRITE_STR2(padding.buf, padding.len));
  }
#endif
  buf.u8[0] = (char)(FIO___MUSTACHE_I_VAR + raw);
  fio_u2buf16u(buf.u8 + 1, var.len);
  p->root = fio_bstr_write2(p->root,
                            FIO_STRING_WRITE_STR2(buf.u8, 3),
                            FIO_STRING_WRITE_STR2(var.buf, var.len));
  return 0;
#if FIO_MUSTACHE_PRESERVE_PADDING
padded:
  buf.u8[0] = (char)(FIO___MUSTACHE_I_PADDING_PUSH);
  fio_u2buf16u(buf.u8 + 1, padding.len);
  buf.u8[3] = (char)(FIO___MUSTACHE_I_VAR + raw);
  fio_u2buf16u(buf.u8 + 4, var.len);
  buf.u8[6] = (char)(FIO___MUSTACHE_I_PADDING_POP);
  p->root = fio_bstr_write2(p->root,
                            FIO_STRING_WRITE_STR2(buf.u8, 3),
                            FIO_STRING_WRITE_STR2(padding.buf, padding.len),
                            FIO_STRING_WRITE_STR2(buf.u8 + 3, 3),
                            FIO_STRING_WRITE_STR2(var.buf, var.len),
                            FIO_STRING_WRITE_STR2(buf.u8 + 6, 1));
  return 0;
#endif
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

/* *****************************************************************************
Tag Consumer
***************************************************************************** */

FIO_SFUNC int fio___mustache_parse_consume_tag(fio___mustache_parser_s *p,
                                               fio_buf_info_s buf,
                                               fio_buf_info_s padding) {
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
    return fio___mustache_parse_var_name(p, buf, padding, 0); /* escaped var */

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
  case '#': return fio___mustache_parse_section_start(p, buf, padding, 0);
  case '^': return fio___mustache_parse_section_start(p, buf, padding, 1);
  case '>': return fio___mustache_parse_partial(p, buf, padding);
  case '!': return 0;
  case '=': /* fall through */
  case '{':
    do /* it is known that (buf.buf ends as '=' or '}')*/
      --buf.len;
    while (buf.buf[buf.len - 1] == ' ' || buf.buf[buf.len - 1] == '\t');

    if (id == '=')
      return fio___mustache_parse_set_delim(p, buf); /* fall through */
  default:                                           /* raw var */
    return fio___mustache_parse_var_name(p, buf, padding, 1);
  }
}

/* *****************************************************************************
File Consumer parser
***************************************************************************** */

FIO_SFUNC int fio___mustache_parse_block(fio___mustache_parser_s *p) {
  int r = 0;
  const char *pos = p->data.buf;
  const char *end = p->data.buf + p->data.len;
  unsigned dirty = 0;
  p->root = fio_bstr_reserve(p->root, (size_t)(end - p->data.buf));
  /* consume each line (in case it's a stand alone line) */
  while (pos < end) {
    if (FIO_UNLIKELY(*pos == p->delim.in.buf[0] &&
                     p->delim.in.cmp(pos, p->delim.in.buf))) {
      /* tag started */
      const char *start = pos;
      pos += p->delim.in.len;
      for (;;) {
        if (pos + p->delim.out.len > end) {
          FIO_LOG_ERROR(
              "(mustache) template error, un-terminated {{tag}}: %.*s",
              (int)(end - start > 10 ? (int)10 : (int)(end - start)),
              start);
          return (r = -1);
        }
        if (*pos == p->delim.out.buf[0] &&
            p->delim.out.cmp(pos, p->delim.out.buf))
          break;
        ++pos;
      }
      /* advance tag ending when triple mustache is detected. */
      pos += (pos < end && pos[0] == '}' &&
              p->delim.out.cmp(pos + 1, p->delim.out.buf));
      fio_buf_info_s stand_alone = {0};
      if (start != p->data.buf) {
        /* if stand-alone, add padding and than pop it, otherwise emit txt */
        if (!dirty &&
            (!pos[p->delim.out.len] || pos[p->delim.out.len] == '\n' ||
             pos[p->delim.out.len] == '\r')) {
          stand_alone =
              FIO_BUF_INFO2(p->data.buf, (size_t)(start - p->data.buf));
        } else { /* not stand-alone, add txt */
          fio___mustache_parse_add_text(
              p,
              FIO_BUF_INFO2(p->data.buf, (size_t)(start - p->data.buf)));
        }
      }
      start += p->delim.in.len;
      p->data.buf = (char *)pos + p->delim.out.len;
      size_t tmp_len = fio_bstr_len(p->root);
      if ((r = fio___mustache_parse_consume_tag(
               p,
               FIO_BUF_INFO2((char *)start, (size_t)(pos - start)),
               stand_alone)))
        goto done;
      p->data.buf += !dirty && p->data.buf[0] == '\r' &&
                     p->root[tmp_len] != FIO___MUSTACHE_I_TXT &&
#if FIO_MUSTACHE_PRESERVE_PADDING
                     p->root[tmp_len] != FIO___MUSTACHE_I_PADDING_PUSH &&
#endif
                     p->root[tmp_len] != FIO___MUSTACHE_I_VAR &&
                     p->root[tmp_len] != FIO___MUSTACHE_I_VAR_RAW;
      p->data.buf += !dirty && p->data.buf[0] == '\n' &&
                     p->root[tmp_len] != FIO___MUSTACHE_I_TXT &&
#if FIO_MUSTACHE_PRESERVE_PADDING
                     p->root[tmp_len] != FIO___MUSTACHE_I_PADDING_PUSH &&
#endif
                     p->root[tmp_len] != FIO___MUSTACHE_I_VAR &&
                     p->root[tmp_len] != FIO___MUSTACHE_I_VAR_RAW;
      pos = p->data.buf; /* may have progressed on ARY tag recursion */
      dirty = (p->data.buf[-1] != '\n');
      continue;
    }
    if (((pos + 1) - p->data.buf == ((1 << 16) - 1)) || *pos == '\n') {
      fio___mustache_parse_add_text(
          p,
          FIO_BUF_INFO2(p->data.buf, (size_t)((pos + 1) - p->data.buf)));
      p->data.buf = (char *)(pos + 1);
      dirty = (*pos != '\n');
    } else
      dirty |= (*pos != ' ' && *pos != '\t');
    ++pos;
  }
  /* print leftover text? */
  if (p->data.buf < end) {
    fio___mustache_parse_add_text(
        p,
        FIO_BUF_INFO2(p->data.buf, (size_t)(end - p->data.buf)));
  }
done:
  r += ((r == -2) << 1); /* end-tag stop shouldn't propagate onward. */
  return r;
}

FIO_SFUNC int fio___mustache_parse_template_file(fio___mustache_parser_s *p) {
  /* remove (possible) filename comment line */
  if (p->data.buf[0] == '@' && p->data.buf[1] == ':') {
    char *pos = (char *)FIO_MEMCHR(p->data.buf, '\n', p->data.len);
    if (!pos)
      return 0; /* done with file... though nothing happened. */
  }
  /* consume (possible) YAML front matter */
  if (p->data.buf[0] == '-' && p->data.buf[1] == '-' && p->data.buf[2] == '-' &&
      (p->data.buf[3] == '\n' || p->data.buf[3] == '\r')) {
    const char *end = p->data.buf + p->data.len;
    const char *pos = p->data.buf;
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
        FIO_BUF_INFO2(p->data.buf, (size_t)(pos - p->data.buf)));
    p->data.len = (size_t)(pos - p->data.buf);
    p->data.buf = (char *)pos;
  }
  return fio___mustache_parse_block(p);
}

/* *****************************************************************************
Default functions
***************************************************************************** */
FIO_SFUNC fio_buf_info_s fio___mustache_dflt_load_file_data(fio_buf_info_s fn) {
  char *data = fio_bstr_readfile(NULL, fn.buf, 0, 0);
  return fio_bstr_buf(data);
}

FIO_SFUNC void fio___mustache_dflt_free_file_data(fio_buf_info_s d) {
  fio_bstr_free(d.buf);
}

FIO_SFUNC void fio___mustache_dflt_on_yaml_front_matter(fio_buf_info_s y) {
  (void)y;
}

FIO_SFUNC void *fio___mustache_dflt_write_text(void *u, fio_buf_info_s txt) {
  return (void *)fio_bstr_write((char *)u, txt.buf, txt.len);
}

FIO_SFUNC void *fio___mustache_dflt_write_text_escaped(void *u,
                                                       fio_buf_info_s raw) {
  return (void *)fio_bstr_write_html_escape((char *)u, raw.buf, raw.len);
}

FIO_SFUNC fio_buf_info_s fio___mustache_dflt_get_val(void *ctx,
                                                     fio_buf_info_s name) {
  return FIO_BUF_INFO2(NULL, 0);
  (void)ctx, (void)name;
}
FIO_SFUNC fio_buf_info_s fio___mustache_dflt_val2str(void *ctx) {
  return FIO_BUF_INFO2(NULL, 0);
  (void)ctx;
}

FIO_SFUNC void *fio___mustache_dflt_enter(void *ctx,
                                          fio_buf_info_s name,
                                          size_t indx,
                                          fio_buf_info_s t) {
  return NULL;
  (void)ctx, (void)name, (void)indx, (void)t;
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
      a.data = a.load_file_data(FIO_STR2BUF_INFO(fn));
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
  parser.data = a.data;
  parser.path = base_path;
  if (fio___mustache_parse_template_file(&parser)) { /* parser failed(!) */
    fio_bstr_free(parser.root);
    parser.root = NULL;
  }
  /* No need to write FIO___MUSTACHE_I_STACK_POP, as the string ends with NUL */
  if (should_free_data)
    a.free_file_data(a.data);
  return (fio_mustache_s *)parser.root;
}

/* Frees the mustache template object (or reduces it's reference count). */
SFUNC void fio_mustache_free(fio_mustache_s *m) { fio_bstr_free((char *)m); }

/** Increases the mustache template's reference count. */
SFUNC fio_mustache_s *fio_mustache_dup(fio_mustache_s *m) {
  return (fio_mustache_s *)fio_bstr_copy((char *)m);
}

void fio_mustache_build___(void); /* IDE marker */
/** Builds the template, returning the final value of `udata` (or NULL). */
SFUNC void *fio_mustache_build FIO_NOOP(fio_mustache_s *m,
                                        fio_mustache_bargs_s args) {
  if (!args.write_text && !args.write_text_escaped) {
    args.write_text = fio___mustache_dflt_write_text;
    args.write_text_escaped = fio___mustache_dflt_write_text_escaped;
  }
  FIO_ASSERT(args.write_text_escaped && args.write_text,
             "(mustache) fio_mustache_build requires both writer "
             "callbacks!\n\t\t(or none, for a fio_bstr_s return)");
  if (!args.get_val)
    args.get_val = fio___mustache_dflt_get_val;
  if (!args.val2str)
    args.val2str = fio___mustache_dflt_val2str;
  if (!args.enter)
    args.enter = fio___mustache_dflt_enter;
  fio___mustache_bldr_s builder = {.root = (char *)m,
                                   .args = &args,
                                   .ctx = args.ctx};
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
