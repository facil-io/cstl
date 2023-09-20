/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_CLI                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  CLI helpers - command line interface parsing


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_CLI) && !defined(H___FIO_CLI___H) &&                           \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_CLI___H 1

/* *****************************************************************************
Internal Macro Implementation
***************************************************************************** */

/** Used internally. */
typedef enum {
  FIO___CLI_STRING,
  FIO___CLI_BOOL,
  FIO___CLI_INT,
  FIO___CLI_PRINT,
  FIO___CLI_PRINT_LINE,
  FIO___CLI_PRINT_HEADER,
} fio___cli_line_e;

typedef struct {
  fio___cli_line_e t;
  const char *l;
} fio___cli_line_s;

/** Indicates the CLI argument should be a String (default). */
#define FIO_CLI_STRING(line)                                                   \
  ((fio___cli_line_s){.t = FIO___CLI_STRING, .l = line})
/** Indicates the CLI argument is a Boolean value. */
#define FIO_CLI_BOOL(line) ((fio___cli_line_s){.t = FIO___CLI_BOOL, .l = line})
/** Indicates the CLI argument should be an Integer (numerical). */
#define FIO_CLI_INT(line) ((fio___cli_line_s){.t = FIO___CLI_INT, .l = line})
/** Indicates the CLI string should be printed as is with proper offset. */
#define FIO_CLI_PRINT(line)                                                    \
  ((fio___cli_line_s){.t = FIO___CLI_PRINT, .l = line})
/** Indicates the CLI string should be printed as is with no offset. */
#define FIO_CLI_PRINT_LINE(line)                                               \
  ((fio___cli_line_s){.t = FIO___CLI_PRINT_LINE, .l = line})
/** Indicates the CLI string should be printed as a header. */
#define FIO_CLI_PRINT_HEADER(line)                                             \
  ((fio___cli_line_s){.t = FIO___CLI_PRINT_HEADER, .l = line})

/* *****************************************************************************
CLI API
***************************************************************************** */

/**
 * This function parses the Command Line Interface (CLI), creating a temporary
 * "dictionary" that allows easy access to the CLI using their names or aliases.
 *
 * Command line arguments may be typed. If an optional type requirement is
 * provided and the provided arument fails to match the required type, execution
 * will end and an error message will be printed along with a short "help".
 *
 * The function / macro accepts the following arguments:
 * - `argc`: command line argument count.
 * - `argv`: command line argument list (array).
 * - `unnamed_min`: the required minimum of un-named arguments.
 * - `unnamed_max`: the maximum limit of un-named arguments.
 * - `description`: a C string containing the program's description.
 * - named arguments list: a list of C strings describing named arguments.
 *
 * The following optional type requirements are:
 *
 * * FIO_CLI_STRING(desc_line)       - (default) string argument.
 * * FIO_CLI_BOOL(desc_line)         - boolean argument (no value).
 * * FIO_CLI_INT(desc_line)          - integer argument.
 * * FIO_CLI_PRINT_HEADER(desc_line) - extra header for output.
 * * FIO_CLI_PRINT(desc_line)        - extra information for output.
 *
 * Argument names MUST start with the '-' character. The first word starting
 * without the '-' character will begin the description for the CLI argument.
 *
 * The arguments "-?", "-h", "-help" and "--help" are automatically handled
 * unless overridden.
 *
 * Un-named arguments shouldn't be listed in the named arguments list.
 *
 * Example use:
 *
 *    fio_cli_start(argc, argv, 0, 0, "The NAME example accepts the following:",
 *                        FIO_CLI_PRINT_HREADER("Concurrency:"),
 *                        FIO_CLI_INT("-t -thread number of threads to run."),
 *                        FIO_CLI_INT("-w -workers number of workers to run."),
 *                        FIO_CLI_PRINT_HREADER("Address Binding:"),
 *                        "-b, -address the address to bind to.",
 *                        FIO_CLI_INT("-p,-port the port to bind to."),
 *                        FIO_CLI_PRINT("\t\tset port to zero (0) for Unix s."),
 *                        FIO_CLI_PRINT_HREADER("Logging:"),
 *                        FIO_CLI_BOOL("-v -log enable logging.")
 *                  );
 *
 *
 * This would allow access to the named arguments:
 *
 *      fio_cli_get("-b") == fio_cli_get("-address");
 *
 *
 * Once all the data was accessed, free the parsed data dictionary using:
 *
 *      fio_cli_end();
 *
 * It should be noted, arguments will be recognized in a number of forms, i.e.:
 *
 *      app -t=1 -p3000 -a localhost
 *
 * This function is NOT thread safe.
 */
#define fio_cli_start(argc, argv, unnamed_min, unnamed_max, description, ...)  \
  fio_cli_start((argc),                                                        \
                (argv),                                                        \
                (unnamed_min),                                                 \
                (unnamed_max),                                                 \
                (description),                                                 \
                (fio___cli_line_s[]){__VA_ARGS__, {0}})
/**
 * Never use the function directly, always use the MACRO, because the macro
 * attaches a NULL marker at the end of the `names` argument collection.
 */
SFUNC void fio_cli_start FIO_NOOP(int argc,
                                  char const *argv[],
                                  int unnamed_min,
                                  int unnamed_max,
                                  char const *description,
                                  fio___cli_line_s *arguments);
/**
 * Clears the memory used by the CLI dictionary, removing all parsed data.
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_end(void);

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name);

/** Returns the argument's value as an integer. */
SFUNC int64_t fio_cli_get_i(char const *name);

/** This MACRO returns the argument's value as a boolean. */
#define fio_cli_get_bool(name) (fio_cli_get((name)) != NULL)

/** Returns the number of unnamed argument. */
SFUNC unsigned int fio_cli_unnamed_count(void);

/** Returns the unnamed argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index);

/**
 * Sets the argument's value as a NUL terminated C String.
 *
 *     fio_cli_set("-p", "hello");
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set(char const *name, char const *value);

/**
 * Sets the argument's value as a NUL terminated C String.
 *
 *     fio_cli_start(argc, argv,
 *                  "this is example accepts the following options:",
 *                  "-p -port the port to bind to", FIO_CLI_INT;
 *
 *     fio_cli_set("-p", "hello"); // fio_cli_get("-p") == fio_cli_get("-port");
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set_i(char const *name, int64_t i);

/** Sets / adds an unnamed argument to the 0 based array of unnamed elements. */
SFUNC unsigned int fio_cli_set_unnamed(unsigned int index, const char *);

/* *****************************************************************************
CLI Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(fio_cli_str)
FIO___LEAK_COUNTER_DEF(fio_cli_ary)
FIO___LEAK_COUNTER_DEF(fio_cli_help_writer)

/* *****************************************************************************
String for CLI
***************************************************************************** */

typedef struct {
  uint8_t em;
  uint8_t pad[3];
  uint32_t len;
  char *str;
} fio___cli_str_s;

FIO_SFUNC void fio___cli_str_destroy(fio___cli_str_s *s) {
  if (!s || s->em || !s->str)
    return;
  FIO___LEAK_COUNTER_ON_FREE(fio_cli_str);
  FIO_MEM_FREE_(s->str, s->len);
  *s = (fio___cli_str_s){0};
}

/* tmp copy */
FIO_IFUNC fio_str_info_s fio___cli_str_info(fio___cli_str_s *s) {
  fio_str_info_s r = {0};
  if (s && (s->em || s->len))
    r = ((s->em) & 127)
            ? ((fio_str_info_s){.buf = (char *)s->pad, .len = (size_t)s->em})
            : ((fio_str_info_s){.buf = s->str, .len = (size_t)s->len});
  return r;
}

/* copy */
FIO_SFUNC fio___cli_str_s fio___cli_str_copy(fio_str_info_s s) {
  fio___cli_str_s r = {0};
  if (s.len < sizeof(r) - 1) {
    r.em = s.len;
    FIO_MEMCPY(r.pad, s.buf, s.len);
    return r;
  }
  r.len = (uint32_t)s.len;
  r.str = (char *)FIO_MEM_REALLOC_(NULL, 0, s.len + 1, 0);
  FIO_ASSERT_ALLOC(r.str);
  FIO___LEAK_COUNTER_ON_ALLOC(fio_cli_str);
  FIO_MEMCPY(r.str, s.buf, s.len);
  r.str[r.len] = 0;
  return r;
}

/* tmp copy */
FIO_SFUNC fio___cli_str_s fio___cli_str(fio_str_info_s s) {
  fio___cli_str_s r = {0};
  if (s.len < sizeof(r) - 2) {
    r.em = s.len;
    FIO_MEMCPY(r.pad, s.buf, s.len);
    return r;
  }
  r.em = 128;
  r.len = (uint32_t)s.len;
  r.str = s.buf;
  return r;
}

/* *****************************************************************************
String array for CLI
***************************************************************************** */

typedef struct {
  fio___cli_str_s *ary;
  uint32_t capa;
  uint32_t w;
} fio___cli_ary_s;

FIO_SFUNC void fio___cli_ary_destroy(fio___cli_ary_s *a) {
  if (!a || !a->ary)
    return;
  for (size_t i = 0; i < a->w; ++i)
    fio___cli_str_destroy(a->ary + i);
  FIO_MEM_FREE_(a->ary, sizeof(*a->ary) * a->capa);
  FIO___LEAK_COUNTER_ON_FREE(fio_cli_ary);
  *a = (fio___cli_ary_s){0};
}
FIO_SFUNC uint32_t fio___cli_ary_new_index(fio___cli_ary_s *a) {
  FIO_ASSERT(a, "Internal CLI Error - no CLI array given!");
  if (a->w == a->capa) {
    /* increase capacity */
    if (!a->ary)
      FIO___LEAK_COUNTER_ON_ALLOC(fio_cli_ary);
    size_t new_capa = a->capa + 8;
    fio___cli_str_s *tmp =
        (fio___cli_str_s *)FIO_MEM_REALLOC_(a->ary,
                                            sizeof(*a->ary) * a->capa,
                                            sizeof(*a->ary) * new_capa,
                                            a->capa);
    FIO_ASSERT_ALLOC(tmp);
    a->ary = tmp;
    a->capa = new_capa;
    if (!(FIO_MEM_REALLOC_IS_SAFE_))
      FIO_MEMSET(a->ary + a->w, 0, sizeof(*a->ary) * (new_capa - a->w));
  }
  FIO_ASSERT_DEBUG(a->w < a->capa, "CLI array index error!");
  return a->w++;
}

FIO_IFUNC fio_str_info_s fio___cli_ary_get(fio___cli_ary_s *a, uint32_t index) {
  fio_str_info_s r = {0};
  if (index >= a->w)
    return r;
  return fio___cli_str_info(a->ary + index);
}
FIO_IFUNC void fio___cli_ary_set(fio___cli_ary_s *a,
                                 uint32_t index,
                                 fio_str_info_s str) {
  FIO_ASSERT(a, "Internal CLI Error - no CLI array given!");
  if (index >= a->w)
    return;
  fio___cli_str_destroy(a->ary + index);
  a->ary[index] = fio___cli_str_copy(str);
}

/* *****************************************************************************
CLI Alias Index Map
***************************************************************************** */

typedef struct {
  fio___cli_str_s name;
  fio___cli_line_e t;
  uint32_t index;
} fio___cli_aliases_s;

#define FIO___CLI_ALIAS_HASH(o)                                                \
  fio_risky_hash(fio___cli_str_info(&o->name).buf,                             \
                 fio___cli_str_info(&o->name).len,                             \
                 (uint64_t)(uintptr_t)fio___cli_str)
#define FIO___CLI_ALIAS_IS_EQ(a, b)                                            \
  FIO_STR_INFO_IS_EQ(fio___cli_str_info(&a->name), fio___cli_str_info(&b->name))
FIO_TYPEDEF_IMAP_ARRAY(fio___cli_amap,
                       fio___cli_aliases_s,
                       uint32_t,
                       FIO___CLI_ALIAS_HASH,
                       FIO___CLI_ALIAS_IS_EQ,
                       FIO_IMAP_ALWAYS_VALID)
#undef FIO___CLI_ALIAS_HASH
#undef FIO___CLI_ALIAS_IS_EQ

/* *****************************************************************************
CLI Alias and Value Data Store
***************************************************************************** */

static struct fio___cli_data_s {
  /* maps alias names to value indexes (array) */
  fio___cli_amap_s aliases;
  fio___cli_ary_s indexed, unnamed;
  const char *description;
  fio___cli_line_s *args;
  const char *app_name;
} fio___cli_data = {{0}};

FIO_SFUNC void fio___cli_data_destroy(void) {
  fio___cli_ary_destroy(&fio___cli_data.indexed);
  fio___cli_ary_destroy(&fio___cli_data.unnamed);
  FIO_IMAP_EACH(fio___cli_amap, &fio___cli_data.aliases, i) {
    fio___cli_str_destroy(&fio___cli_data.aliases.ary[i].name);
  }
  fio___cli_amap_destroy(&fio___cli_data.aliases);
  fio___cli_data = (struct fio___cli_data_s){{0}};
}

FIO_SFUNC void fio___cli_data_alias(fio_str_info_s key,
                                    fio_str_info_s alias,
                                    fio___cli_line_e t) {
  fio___cli_aliases_s o = {.name = fio___cli_str(key)};
  fio___cli_aliases_s *a = fio___cli_amap_get(&fio___cli_data.aliases, o);
  if (!a) {
    o.name = fio___cli_str_copy(key);
    o.index = fio___cli_ary_new_index(&fio___cli_data.indexed);
    o.t = t;
    fio___cli_amap_set(&fio___cli_data.aliases, o, 1);
  }
  if (!alias.len)
    return;
  o.name = fio___cli_str(alias);
  fio___cli_aliases_s *old = fio___cli_amap_get(&fio___cli_data.aliases, o);
  if (old) {
    FIO_LOG_WARNING("(fio_cli) CLI alias %s already exists! overwriting...",
                    fio___cli_str_info(&o.name).buf);
    old->index = a->index;
  } else {
    o.name = fio___cli_str_copy(alias);
    o.index = a->index;
    o.t = a->t;
    fio___cli_amap_set(&fio___cli_data.aliases, o, 1);
  }
}

FIO_SFUNC void fio___cli_print_help(void);

FIO_SFUNC void fio___cli_data_set(fio_str_info_s key, fio_str_info_s value) {
  fio___cli_aliases_s o = {.name = fio___cli_str(key)};
  fio___cli_aliases_s *a = fio___cli_amap_get(&fio___cli_data.aliases, o);
  if (!a) {
    fio___cli_data_alias(key, (fio_str_info_s){0}, FIO___CLI_STRING);
    a = fio___cli_amap_get(&fio___cli_data.aliases, o);
  }
  FIO_ASSERT(a && a->index < fio___cli_data.indexed.w,
             "(fio_cli) CLI alias initialization error!");
  if (a->t == FIO___CLI_INT) {
    char *start = value.buf;
    fio_atol(&start);
    if (start != value.buf + value.len) {
      FIO_LOG_FATAL("(CLI) %.*s should be an integer!",
                    (int)value.len,
                    value.buf);
      fio___cli_print_help();
    }
  }
  fio___cli_ary_set(&fio___cli_data.indexed, a->index, value);
}

FIO_SFUNC fio_str_info_s fio___cli_data_get(fio_str_info_s key) {
  fio_str_info_s r = {0};
  fio___cli_aliases_s o = {.name = fio___cli_str(key)};
  fio___cli_aliases_s *a = fio___cli_amap_get(&fio___cli_data.aliases, o);
  if (a)
    r = fio___cli_ary_get(&fio___cli_data.indexed, a->index);
  return r;
}

FIO_SFUNC uint32_t fio___cli_data_get_index(fio_str_info_s key) {
  uint32_t r = (uint32_t)-1;
  fio___cli_aliases_s o = {.name = fio___cli_str(key)};
  fio___cli_aliases_s *a = fio___cli_amap_get(&fio___cli_data.aliases, o);
  if (a)
    r = a->index;
  return r;
}

/* *****************************************************************************
CLI Destruction
***************************************************************************** */

SFUNC void __attribute__((destructor)) fio_cli_end(void) {
  fio___cli_data_destroy();
}

/* *****************************************************************************
CLI Public Get/Set API
***************************************************************************** */

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name) {
  if (!name)
    return fio_cli_unnamed(0);
  fio_str_info_s key = FIO_STR_INFO1((char *)name);
  return fio___cli_data_get(key).buf;
}

/** Returns the argument's value as an integer. */
SFUNC int64_t fio_cli_get_i(char const *name) {
  char *val = (char *)fio_cli_get(name);
  if (!val)
    return 0;
  return fio_atol(&val);
}

/** Returns the number of unnamed argument. */
SFUNC unsigned int fio_cli_unnamed_count(void) {
  return fio___cli_data.unnamed.w;
}

/** Returns the unnamed argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index) {
  if (index >= fio___cli_data.unnamed.w)
    return NULL;
  return fio___cli_ary_get(&fio___cli_data.unnamed, (uint32_t)index).buf;
}

/**
 * Sets the argument's value as a NUL terminated C String.
 *
 *     fio_cli_set("-p", "hello");
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set(char const *name, char const *value) {
  fio_str_info_s key = FIO_STR_INFO1((char *)name);
  fio_str_info_s val = FIO_STR_INFO1((char *)value);
  if (!name) {
    if (!value)
      return;
    uint32_t i = fio___cli_ary_new_index(&fio___cli_data.unnamed);
    fio___cli_ary_set(&fio___cli_data.unnamed, i, val);
    return;
  }
  fio___cli_data_set(key, val);
}

/**
 * Sets the argument's value as a NUL terminated C String.
 *
 *     fio_cli_start(argc, argv,
 *                  "this is example accepts the following options:",
 *                  "-p -port the port to bind to", FIO_CLI_INT;
 *
 *     fio_cli_set("-p", "hello"); // fio_cli_get("-p") == fio_cli_get("-port");
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set_i(char const *name, int64_t i) {
  char buf[32];
  size_t len = fio_digits10(i);
  fio_ltoa10(buf, i, len);
  buf[len] = 0;
  fio_cli_set(name, buf);
}

/** Sets / adds an unnamed argument to the 0 based array of unnamed elements. */
SFUNC unsigned int fio_cli_set_unnamed(unsigned int index, const char *value) {
  if (!value)
    return (uint32_t)-1;
  fio_str_info_s val = FIO_STR_INFO1((char *)value);
  if (!val.len)
    return (uint32_t)-1;
  if (index >= fio___cli_data.unnamed.w)
    index = fio___cli_ary_new_index(&fio___cli_data.unnamed);
  fio___cli_ary_set(&fio___cli_data.unnamed, index, val);
  return index;
}

/* *****************************************************************************
CLI Name Iterator
***************************************************************************** */

typedef struct {
  fio___cli_line_s *args;
  fio_str_info_s line;
  fio_str_info_s desc;
  size_t index;
  fio___cli_line_e line_type;
} fio___cli_iterator_args_s;

#define FIO___CLI_EACH_ARG(args_, i)                                           \
  for (fio___cli_iterator_args_s i =                                           \
           {                                                                   \
               .args = args_,                                                  \
              .line = FIO_STR_INFO1((char *)((args_)[0].l)),                   \
              .line_type = (args_)[0].t,                                       \
           };                                                                  \
       i.line.buf;                                                             \
       (++i.index,                                                             \
        i.line = FIO_STR_INFO1((char *)i.args[i.index].l),                     \
        i.line_type = i.args[i.index].t))

typedef struct {
  fio_str_info_s line;
  fio_str_info_s alias;
} fio___cli_iterator_alias_s;

FIO_IFUNC fio_str_info_s fio___cli_iterator_alias_first(fio___cli_line_s *arg,
                                                        fio_str_info_s line) {
  fio_str_info_s a = {0};
  if (arg->t > FIO___CLI_INT)
    return a;
  if (!line.buf || line.buf[0] != '-')
    return a;
  char *pos = (char *)FIO_MEMCHR(line.buf, ' ', line.len);
  if (!pos)
    pos = line.buf + line.len;
  a = FIO_STR_INFO2(line.buf, (size_t)(pos - line.buf));
  return a;
}
FIO_IFUNC fio_str_info_s fio___cli_iterator_alias_next(fio_str_info_s line,
                                                       fio_str_info_s prev) {
  fio_str_info_s a = {0};
  if (!prev.buf[prev.len])
    return a; /* eol */
  prev.buf += prev.len + 1;
  if (prev.buf[0] != '-')
    return a; /* no more aliases */
  char *pos =
      (char *)FIO_MEMCHR(prev.buf, ' ', ((line.buf + line.len) - prev.buf));
  if (!pos)
    pos = line.buf + line.len;
  a = FIO_STR_INFO2(prev.buf, (size_t)(pos - prev.buf));
  return a;
}

#define FIO___CLI_EACH_ALIAS(i, alias)                                         \
  for (fio_str_info_s alias =                                                  \
           fio___cli_iterator_alias_first(i.args + i.index, i.line);           \
       alias.buf;                                                              \
       alias = fio___cli_iterator_alias_next(i.line, alias))

FIO_IFUNC fio_str_info_s
fio___cli_iterator_default_val(fio___cli_iterator_args_s *i) {
  fio_str_info_s a = {0};
  fio_str_info_s line = i->line;
  if (!line.buf || line.buf[0] != '-') {
    i->desc = line;
    return a;
  }
  for (;;) {
    char *pos = (char *)FIO_MEMCHR(line.buf, ' ', line.len);
    if (!pos)
      return a;
    ++pos;
    if (pos[0] == '-') {
      line.len = (line.buf + line.len) - pos;
      line.buf = pos;
      continue;
    }
    if (pos[0] != '(') {
      i->desc.len = (line.buf + line.len) - pos;
      i->desc.buf = pos;
      return a;
    }
    if (pos[1] == '"') {
      pos += 2;
      a.buf = pos;
      while (*pos && !(pos[0] == '"' && pos[1] == ')'))
        ++pos;
      if (!pos[0]) {
        /* no default value? */
        i->desc.len = (line.buf + line.len) - a.buf;
        i->desc.buf = a.buf;
        a = (fio_str_info_s){0};
        return a;
      }
      a.len = pos - a.buf;
      pos += 2;
    } else {
      pos += 1;
      a.buf = pos;
      while (*pos && pos[0] != ')')
        ++pos;
      if (!pos[0]) {
        /* no default value? */
        i->desc.len = (line.buf + line.len) - a.buf;
        i->desc.buf = a.buf;
        a = (fio_str_info_s){0};
        return a;
      }
      a.len = pos - a.buf;
      ++pos;
    }
    pos += *pos == ' ';
    line = i->line;
    i->desc.len = (line.buf + line.len) - pos;
    i->desc.buf = pos;
    return a;
  }
}

#define FIO___CLI_EACH_DESC(i, desc_)                                          \
  for (fio_str_info_s desc_ = i.desc;                                          \
       desc_.len || i.args[i.index + 1].t == FIO___CLI_PRINT;                  \
       desc_ = (i.args[i.index + 1].t == FIO___CLI_PRINT                       \
                    ? (++i.index, FIO_STR_INFO1((char *)i.args[i.index].l))    \
                    : FIO_STR_INFO2(0, 0)))

/* *****************************************************************************
CLI Build + Parsing Arguments
***************************************************************************** */

FIO_SFUNC void fio___cli_build_argument_aliases(char const *argv[],
                                                char const *description,
                                                fio___cli_line_s *args) {
  /**   Setup the CLI argument alias indexing   **/
  fio___cli_data.description = description;
  fio___cli_data.args = args;
  fio___cli_data.app_name = argv[0];
  FIO___CLI_EACH_ARG(args, i) {
    fio_str_info_s first_alias = {0};
    fio_str_info_s def = fio___cli_iterator_default_val(&i);
    switch (i.line_type) {
    case FIO___CLI_STRING: /* fall through */
    case FIO___CLI_BOOL:   /* fall through */
    case FIO___CLI_INT:    /* fall through */
      FIO_ASSERT(
          i.line.buf[0] == '-',
          "(CLI) argument lines MUST start with an '-argument-name':\n\t%s",
          i.line.buf);
      FIO___CLI_EACH_ALIAS(i, alias) {
        if (first_alias.buf) {
          fio___cli_data_alias(first_alias, alias, i.line_type);
          continue;
        }
        fio___cli_data_alias(alias, first_alias, i.line_type);
        first_alias = alias;
      }
      if (def.len) {
        FIO_ASSERT(
            i.line_type != FIO___CLI_BOOL,
            "(CLI) boolean CLI arguments cannot have a default value:\n\t%s",
            i.line.buf);
        fio___cli_data_set(first_alias, def);
      }
      continue;
    case FIO___CLI_PRINT:      /* fall through */
    case FIO___CLI_PRINT_LINE: /* fall through */
    case FIO___CLI_PRINT_HEADER: continue;
    }
  }
}

void fio_cli_start___(void); /* sublime text marker */
SFUNC void fio_cli_start FIO_NOOP(int argc,
                                  char const *argv[],
                                  int unnamed_min,
                                  int unnamed_max,
                                  char const *description,
                                  fio___cli_line_s *args) {
  uint32_t help_value32 = fio_buf2u32u("help");

  fio___cli_build_argument_aliases(argv, description, args);
  if (unnamed_min == -1) {
    unnamed_max = -1;
    unnamed_min = 0;
  }

  /**   Consume Arguments   **/
  for (size_t i = 1; i < (size_t)argc; ++i) {
    fio_str_info_s key = FIO_STR_INFO1((char *)argv[i]);
    fio_str_info_s value = {0};
    fio___cli_aliases_s *a = NULL;
    if (!key.buf || !key.len)
      continue;
    if (key.buf[0] != '-')
      goto process_unnamed;
    /* --help / -h / -? */
    if ((key.len == 2 && ((key.buf[1] | 32) == 'h' || key.buf[1] == '?')) ||
        (key.len == 5 &&
         (fio_buf2u32u(key.buf + 1) | 0x20202020UL) == help_value32) ||
        (key.len == 6 && key.buf[1] == '-' &&
         (fio_buf2u32u(key.buf + 2) | 0x20202020UL) == help_value32))
      fio___cli_print_help();
    /* look for longest argument match for argument (find, i.e. -arg=val) */
    for (;;) {
      fio___cli_aliases_s o = {.name = fio___cli_str(key)};
      a = fio___cli_amap_get(&fio___cli_data.aliases, o);
      if (a)
        break;
      ++value.len;
      --key.len;
      value.buf = key.buf + key.len;
      if (!key.len) {
        key = value;
        goto process_unnamed;
      }
    }
    /* boolean values can be chained, but cannot have an actual value. */
    if (a->t == FIO___CLI_BOOL) {
      fio_str_info_s bool_value = FIO_STR_INFO2((char *)"1", 1);
      char bool_buf[3] = {'-', 0, 0};
      for (;;) {
        fio___cli_ary_set(&fio___cli_data.indexed, a->index, bool_value);
        while (value.len && value.buf[0] == ',')
          (--value.len, ++value.buf);
        if (!value.len)
          break;
        bool_buf[1] = value.buf[0];
        --value.len;
        ++value.buf;
        key = FIO_STR_INFO2(bool_buf, 2);
        fio___cli_aliases_s o = {.name = fio___cli_str(key)};
        a = fio___cli_amap_get(&fio___cli_data.aliases, o);
        if (!a || a->t != FIO___CLI_BOOL) {
          FIO_LOG_FATAL(
              "(CLI) unrecognized boolean value (%s) embedded in argument %s",
              bool_buf,
              argv[i]);
          fio___cli_print_help();
        }
      }
      continue;
    }

    if (value.len) { /* values such as `-arg34` / `-arg=32` */
      value.len -= (value.buf[0] == '=');
      value.buf += (value.buf[0] == '=');
    } else { /* values such as `-arg 32` (using 2 argv elements)*/
      if ((i + 1) == (size_t)argc) {
        FIO_LOG_FATAL("(CLI) argument value missing for (%s)",
                      key.buf,
                      argv[i]);
        fio___cli_print_help();
      }
      ++i;
      value = FIO_STR_INFO1((char *)argv[i]);
    }
    fio___cli_data_set(key, value); /* use this for type validation */
    continue;
  process_unnamed:

    if (!unnamed_max) {
      FIO_LOG_FATAL("(CLI) unnamed arguments limit reached at argument: %s",
                    key.buf);
      fio___cli_print_help();
    }
    fio___cli_ary_set(&fio___cli_data.unnamed,
                      fio___cli_ary_new_index(&fio___cli_data.unnamed),
                      key);
    --unnamed_max;
    continue;
  }
  if (unnamed_min && fio___cli_data.unnamed.w < (uint32_t)unnamed_min) {
    FIO_LOG_FATAL("(CLI) missing required arguments");
    fio___cli_print_help();
  }
}

/* *****************************************************************************
CLI Help Output
***************************************************************************** */

FIO_IFUNC fio_str_info_s fio___cli_write2line(fio_str_info_s d,
                                              fio_buf_info_s s,
                                              uint8_t static_memory) {
  if (d.len + s.len + 2 > d.capa) {
    size_t new_capa = (d.len + s.len) << 1;
    char *tmp = (char *)FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
    FIO_ASSERT_ALLOC(tmp);
    FIO___LEAK_COUNTER_ON_ALLOC(fio_cli_help_writer);
    FIO_MEMCPY(tmp, d.buf, d.len);
    if (!static_memory) {
      FIO_MEM_FREE_(d.buf, d.capa);
      FIO___LEAK_COUNTER_ON_FREE(fio_cli_help_writer);
    }
    d.capa = new_capa;
    d.buf = tmp;
  }
  FIO_MEMCPY(d.buf + d.len, s.buf, s.len);
  d.len += s.len;
  return d;
}

FIO_SFUNC fio_str_info_s fio___cli_write2line_finalize(fio_str_info_s d,
                                                       fio_buf_info_s app_name,
                                                       uint8_t static_memory) {
  /* replace "NAME" with `app_name` */
  size_t additional_bytes = app_name.len > 4 ? app_name.len - 4 : 0;
  char *pos = (char *)FIO_MEMCHR(d.buf, 'N', d.len);
  uint32_t name_val = fio_buf2u32u("NAME");
  while (pos) {
    if (fio_buf2u32u(pos) != name_val) {
      if (pos + 4 > d.buf + d.len)
        break;
      pos = (char *)FIO_MEMCHR(pos + 1, 'N', (d.buf + d.len) - pos);
      continue;
    }
    if (d.len + additional_bytes + 2 > d.capa) { /* not enough room? */
      size_t new_capa = d.len + ((additional_bytes + 2) << 2);
      char *tmp = (char *)FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
      FIO_ASSERT_ALLOC(tmp);
      FIO___LEAK_COUNTER_ON_ALLOC(fio_cli_help_writer);
      FIO_MEMCPY(tmp, d.buf, d.len);
      if (!static_memory) {
        FIO_MEM_FREE_(d.buf, d.capa);
        FIO___LEAK_COUNTER_ON_FREE(fio_cli_help_writer);
      }
      static_memory = 0;
      pos = tmp + (pos - d.buf);
      d.capa = new_capa;
      d.buf = tmp;
    }
    FIO_MEMMOVE(pos + app_name.len, pos + 4, ((d.buf + d.len) - (pos + 4)));
    FIO_MEMCPY(pos, app_name.buf, app_name.len);
    d.len -= 4;
    d.len += app_name.len;
    pos += app_name.len;
  }
  d.buf[d.len] = 0;
  return d;
}

FIO_SFUNC void fio___cli_print_help(void) {
  char const *description = fio___cli_data.description;
  fio___cli_line_s *args = fio___cli_data.args;

  fio_buf_info_s app_name = {
      .buf = (char *)fio___cli_data.app_name,
      .len =
          (fio___cli_data.app_name ? FIO_STRLEN(fio___cli_data.app_name) : 0)};
  FIO_STR_INFO_TMP_VAR(help, 8191);
  fio_str_info_s help_org_state = help;

  help = fio___cli_write2line(help,
                              FIO_BUF_INFO1((char *)"\n"),
                              help_org_state.buf == help.buf);
  help = fio___cli_write2line(help,
                              FIO_BUF_INFO1((char *)description),
                              help_org_state.buf == help.buf);
  help = fio___cli_write2line(help,
                              FIO_BUF_INFO1((char *)"\n"),
                              help_org_state.buf == help.buf);

  FIO___CLI_EACH_ARG(args, i) {
    fio_str_info_s first_alias = {0};
    fio_str_info_s def = fio___cli_iterator_default_val(&i);
    fio_buf_info_s argument_type_txt = {0};
    switch (i.line_type) {
    case FIO___CLI_STRING:
      argument_type_txt = FIO_BUF_INFO1(
          (char *)"\x1B[0m \x1B[2m<string value>"); /* fall through */
    case FIO___CLI_BOOL:
      FIO_ASSERT(i.line_type != FIO___CLI_BOOL || !def.len,
                 "(CLI) boolean values cannot have a default value:\n\t%s",
                 i.line.buf);
      if (!argument_type_txt.buf)
        argument_type_txt = FIO_BUF_INFO1(
            (char *)"\x1B[0m \x1B[2m(boolean flag)"); /* fall through */
    case FIO___CLI_INT:
      if (!argument_type_txt.buf)
        argument_type_txt =
            FIO_BUF_INFO1((char *)"\x1B[0m \x1B[2m<integer value>");
      FIO_ASSERT(
          i.line.buf[0] == '-',
          "(CLI) argument lines MUST start with an '-argument-name':\n\t%s",
          i.line.buf);
      FIO___CLI_EACH_ALIAS(i, al) {
        if (!first_alias.buf)
          help = fio___cli_write2line(help,
                                      FIO_BUF_INFO1((char *)"  \x1B[1m"),
                                      help_org_state.buf == help.buf);
        else
          help = fio___cli_write2line(help,
                                      FIO_BUF_INFO1((char *)"\x1B[0m, \x1B[1m"),
                                      help_org_state.buf == help.buf);
        first_alias = al;
        help = fio___cli_write2line(help,
                                    FIO_STR2BUF_INFO(al),
                                    help_org_state.buf == help.buf);
      }
      help = fio___cli_write2line(help,
                                  argument_type_txt,
                                  help_org_state.buf == help.buf);
      if (def.len) {
        help = fio___cli_write2line(help,
                                    FIO_BUF_INFO1((char *)", defaults to: "),
                                    help_org_state.buf == help.buf);
        help = fio___cli_write2line(help,
                                    FIO_STR2BUF_INFO(def),
                                    help_org_state.buf == help.buf);
      }
      help = fio___cli_write2line(help,
                                  FIO_BUF_INFO1((char *)"\x1B[0m\n"),
                                  help_org_state.buf == help.buf);
      FIO___CLI_EACH_DESC(i, desc) {
        help = fio___cli_write2line(help,
                                    FIO_BUF_INFO1((char *)"\t"),
                                    help_org_state.buf == help.buf);
        help = fio___cli_write2line(help,
                                    FIO_STR2BUF_INFO(desc),
                                    help_org_state.buf == help.buf);
        help = fio___cli_write2line(help,
                                    FIO_BUF_INFO1((char *)"\n"),
                                    help_org_state.buf == help.buf);
      }
      continue;
    case FIO___CLI_PRINT:
      help = fio___cli_write2line(help,
                                  FIO_BUF_INFO1((char *)"\t"),
                                  help_org_state.buf ==
                                      help.buf); /* fall through */
    case FIO___CLI_PRINT_LINE:
      help = fio___cli_write2line(help,
                                  FIO_STR2BUF_INFO(i.line),
                                  help_org_state.buf == help.buf);
      help = fio___cli_write2line(help,
                                  FIO_BUF_INFO1((char *)"\n"),
                                  help_org_state.buf == help.buf);
      continue;
    case FIO___CLI_PRINT_HEADER:
      help = fio___cli_write2line(help,
                                  FIO_BUF_INFO1((char *)"\n\x1B[4m"),
                                  help_org_state.buf == help.buf);
      help = fio___cli_write2line(help,
                                  FIO_STR2BUF_INFO(i.line),
                                  help_org_state.buf == help.buf);
      help = fio___cli_write2line(help,
                                  FIO_BUF_INFO1((char *)"\x1B[0m\n"),
                                  help_org_state.buf == help.buf);
      continue;
    }
  }
  help = fio___cli_write2line(
      help,
      FIO_BUF_INFO1((char *)"\nUse any of the following input formats:\n"
                            "\t-arg <value>\t-arg=<value>\t-arg<value>\n"
                            "\n"
                            "Use \x1B[1m-h\x1B[0m , \x1B[1m-help\x1B[0m or "
                            "\x1B[1m-?\x1B[0m "
                            "to get this information again.\n"
                            "\n"),
      help_org_state.buf == help.buf);
  help = fio___cli_write2line_finalize(help,
                                       app_name,
                                       help_org_state.buf == help.buf);
  fwrite(help.buf, 1, help.len, stdout);
  if (help_org_state.buf != help.buf) {
    FIO_MEM_FREE_(help.buf, help.capa);
    FIO___LEAK_COUNTER_ON_FREE(fio_cli_help_writer);
  }
  fio_cli_end();
  exit(0);
}
/* *****************************************************************************
CLI - cleanup
***************************************************************************** */
#undef FIO___CLI_ON_ALLOC
#undef FIO___CLI_ON_FREE
#endif /* FIO_EXTERN_COMPLETE*/
#endif /* FIO_CLI */
#undef FIO_CLI
