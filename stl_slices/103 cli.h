/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_CLI                     /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "005 riskyhash.h"          /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "210 map api.h"            /* Development inclusion - ignore line */
#include "211 ordered map.h"        /* Development inclusion - ignore line */
#include "211 unordered map.h"      /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                  CLI helpers - command line interface parsing



***************************************************************************** */
#if defined(FIO_CLI) && !defined(H___FIO_CLI___H) && !defined(FIO_STL_KEEP__)
#define H___FIO_CLI___H 1

/* *****************************************************************************
Internal Macro Implementation
***************************************************************************** */

/** Used internally. */
#define FIO_CLI_STRING__TYPE_I       0x1
#define FIO_CLI_BOOL__TYPE_I         0x2
#define FIO_CLI_INT__TYPE_I          0x3
#define FIO_CLI_PRINT__TYPE_I        0x4
#define FIO_CLI_PRINT_LINE__TYPE_I   0x5
#define FIO_CLI_PRINT_HEADER__TYPE_I 0x6

/** Indicates the CLI argument should be a String (default). */
#define FIO_CLI_STRING(line) (line), ((char *)FIO_CLI_STRING__TYPE_I)
/** Indicates the CLI argument is a Boolean value. */
#define FIO_CLI_BOOL(line) (line), ((char *)FIO_CLI_BOOL__TYPE_I)
/** Indicates the CLI argument should be an Integer (numerical). */
#define FIO_CLI_INT(line) (line), ((char *)FIO_CLI_INT__TYPE_I)
/** Indicates the CLI string should be printed as is with proper offset. */
#define FIO_CLI_PRINT(line) (line), ((char *)FIO_CLI_PRINT__TYPE_I)
/** Indicates the CLI string should be printed as is with no offset. */
#define FIO_CLI_PRINT_LINE(line) (line), ((char *)FIO_CLI_PRINT_LINE__TYPE_I)
/** Indicates the CLI string should be printed as a header. */
#define FIO_CLI_PRINT_HEADER(line)                                             \
  (line), ((char *)FIO_CLI_PRINT_HEADER__TYPE_I)

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
 *                  FIO_CLI_PRINT_HREADER("Concurrency:"),
 *                  FIO_CLI_INT("-t -thread number of threads to run."),
 *                  FIO_CLI_INT("-w -workers number of workers to run."),
 *                  FIO_CLI_PRINT_HREADER("Address Binding:"),
 *                  "-b, -address the address to bind to.",
 *                  FIO_CLI_INT("-p,-port the port to bind to."),
 *                  FIO_CLI_PRINT("\t\tset port to zero (0) for Unix s."),
 *                  FIO_CLI_PRINT_HREADER("Logging:"),
 *                  FIO_CLI_BOOL("-v -log enable logging."));
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
                (char const *[]){__VA_ARGS__, (char const *)NULL})
/**
 * Never use the function directly, always use the MACRO, because the macro
 * attaches a NULL marker at the end of the `names` argument collection.
 */
SFUNC void fio_cli_start FIO_NOOP(int argc,
                                  char const *argv[],
                                  int unnamed_min,
                                  int unnamed_max,
                                  char const *description,
                                  char const **names);
/**
 * Clears the memory used by the CLI dictionary, removing all parsed data.
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_end(void);

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name);

/** Returns the argument's value as an integer. */
SFUNC int fio_cli_get_i(char const *name);

/** This MACRO returns the argument's value as a boolean. */
#define fio_cli_get_bool(name) (fio_cli_get((name)) != NULL)

/** Returns the number of unnamed argument. */
SFUNC unsigned int fio_cli_unnamed_count(void);

/** Returns the unnamed argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index);

/**
 * Sets the argument's value as a NUL terminated C String (no copy!).
 *
 * CAREFUL: This does not automatically detect aliases or type violations! it
 * will only effect the specific name given, even if invalid. i.e.:
 *
 *     fio_cli_start(argc, argv,
 *                  "this is example accepts the following options:",
 *                  "-p -port the port to bind to", FIO_CLI_INT;
 *
 *     fio_cli_set("-p", "hello"); // fio_cli_get("-p") != fio_cli_get("-port");
 *
 * Note: this does NOT copy the C strings to memory. Memory should be kept alive
 *       until `fio_cli_end` is called.
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set(char const *name, char const *value);

/**
 * This MACRO is the same as:
 *
 *     if(!fio_cli_get(name)) {
 *       fio_cli_set(name, value)
 *     }
 *
 * See fio_cli_set for notes and restrictions.
 */
#define fio_cli_set_default(name, value)                                       \
  if (!fio_cli_get((name)))                                                    \
    fio_cli_set(name, value);

/* *****************************************************************************
CLI Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
CLI Data Stores
***************************************************************************** */

typedef struct {
  const char *buf;
  size_t len;
} fio___cli_cstr_s;

#define FIO_RISKY_HASH
#define FIO_MAP_TYPE const char *
#define FIO_MAP_KEY  fio___cli_cstr_s
#define FIO_MAP_KEY_CMP(o1, o2)                                                \
  (o1.len == o2.len &&                                                         \
   (!o1.len || o1.buf == o2.buf ||                                             \
    (o1.buf && o2.buf && !memcmp(o1.buf, o2.buf, o1.len))))
#define FIO_UMAP_NAME fio___cli_hash
#define FIO_STL_KEEP__
#include __FILE__
#undef FIO_STL_KEEP__

static fio___cli_hash_s fio___cli_aliases = FIO_MAP_INIT;
static fio___cli_hash_s fio___cli_values = FIO_MAP_INIT;
static size_t fio___cli_unnamed_count = 0;

typedef struct {
  int unnamed_min;
  int unnamed_max;
  int pos;
  int unnamed_count;
  int argc;
  char const **argv;
  char const *description;
  char const **names;
} fio_cli_parser_data_s;

#define FIO_CLI_HASH_VAL(s)                                                    \
  fio_risky_hash((s).buf, (s).len, (uint64_t)(uintptr_t)fio_cli_start)

/* *****************************************************************************
Default parameter storage
***************************************************************************** */

typedef struct {
  FIO_LIST_NODE node;
  size_t len;
  char buf[];
} fio___cli_def_str_s;

/* A linked list linking default values */
static FIO_LIST_HEAD fio___cli_default_values;

/** extracts the "default" marker from a string's line */
FIO_SFUNC fio___cli_cstr_s fio___cli_map_line2default(char const *line) {
  fio___cli_cstr_s n = {.buf = line};
  /* skip aliases */
  while (n.buf[n.len] == '-') {
    while (n.buf[n.len] && n.buf[n.len] != ' ' && n.buf[n.len] != ',')
      ++n.len;
    while (n.buf[n.len] && (n.buf[n.len] == ' ' || n.buf[n.len] == ',')) {
      ++n.len;
    }
    n.buf += n.len;
    n.len = 0;
  }
  /* a default is made with (value) or ("value"), both escapable with '\\' */
  if (n.buf[0] != '(')
    goto no_default;
  ++n.buf;
  if (n.buf[0] == '"') {
    ++n.buf;
    /* seek default value end with `")` */
    while (n.buf[n.len] && !(n.buf[n.len] == '"' && n.buf[n.len + 1] == ')'))
      ++n.len;
    if ((n.buf[n.len] != '"' || n.buf[n.len + 1] != ')'))
      goto no_default;
  } else {
    /* seek default value end with `)` */
    while (n.buf[n.len] && n.buf[n.len] != ')')
      ++n.len;
    if (n.buf[n.len] != ')')
      goto no_default;
  }

  return n;
no_default:
  n.buf = NULL;
  n.len = 0;
  return n;
}

FIO_IFUNC fio___cli_cstr_s fio___cli_map_store_default(fio___cli_cstr_s d) {
  fio___cli_cstr_s val = {.buf = NULL, .len = 0};
  if (!d.len || !d.buf)
    return val;
  fio___cli_def_str_s *str =
      FIO_MEM_REALLOC_(NULL, 0, (sizeof(*str) + d.len + 1), 0);
  FIO_ASSERT_ALLOC(str);
  FIO_LIST_PUSH(&fio___cli_default_values, &str->node);
  val.buf = str->buf;
  str->len = val.len = d.len;
  str->buf[str->len] = 0;
  FIO_MEMCPY((char *)val.buf, d.buf, val.len);
  FIO_LOG_DEBUG("CLI stored a string: %s (%zu bytes)", str->buf, str->len);
  return val;
}

/* *****************************************************************************
CLI Parsing
***************************************************************************** */

FIO_SFUNC void fio___cli_map_line2alias(char const *line) {
  fio___cli_cstr_s n = {.buf = line};
  /* if a line contains a default value, store that value with the aliases. */
  fio___cli_cstr_s def =
      fio___cli_map_store_default(fio___cli_map_line2default(line));
  while (n.buf[0] == '-') {
    while (n.buf[n.len] && n.buf[n.len] != ' ' && n.buf[n.len] != ',') {
      ++n.len;
    }
    const char *old = NULL;
    fio___cli_hash_set(&fio___cli_aliases,
                       FIO_CLI_HASH_VAL(n),
                       n,
                       (char const *)line,
                       &old);
    if (def.buf) {
      fio___cli_hash_set(&fio___cli_values,
                         FIO_CLI_HASH_VAL(n),
                         n,
                         def.buf,
                         NULL);
    }
#ifdef FIO_LOG_ERROR
    if (old) {
      FIO_LOG_ERROR("CLI argument name conflict detected\n"
                    "         The following two directives conflict:\n"
                    "\t%s\n\t%s\n",
                    old,
                    line);
    }
#endif
    while (n.buf[n.len] && (n.buf[n.len] == ' ' || n.buf[n.len] == ',')) {
      ++n.len;
    }
    n.buf += n.len;
    n.len = 0;
  }
}

FIO_SFUNC char const *fio___cli_get_line_type(fio_cli_parser_data_s *parser,
                                              const char *line) {
  if (!line) {
    return NULL;
  }
  char const **pos = parser->names;
  while (*pos) {
    switch ((intptr_t)*pos) {
    case FIO_CLI_STRING__TYPE_I:       /* fall through */
    case FIO_CLI_BOOL__TYPE_I:         /* fall through */
    case FIO_CLI_INT__TYPE_I:          /* fall through */
    case FIO_CLI_PRINT__TYPE_I:        /* fall through */
    case FIO_CLI_PRINT_LINE__TYPE_I:   /* fall through */
    case FIO_CLI_PRINT_HEADER__TYPE_I: /* fall through */
      ++pos;
      continue;
    }
    if (line == *pos) {
      goto found;
    }
    ++pos;
  }
  return NULL;
found:
  switch ((size_t)pos[1]) {
  case FIO_CLI_STRING__TYPE_I:       /* fall through */
  case FIO_CLI_BOOL__TYPE_I:         /* fall through */
  case FIO_CLI_INT__TYPE_I:          /* fall through */
  case FIO_CLI_PRINT__TYPE_I:        /* fall through */
  case FIO_CLI_PRINT_LINE__TYPE_I:   /* fall through */
  case FIO_CLI_PRINT_HEADER__TYPE_I: /* fall through */
    return pos[1];
  }
  return NULL;
}

FIO_SFUNC void fio___cli_print_line(char const *desc, char const *name) {
  char buf[1024];
  size_t pos = 0;
  while (name[0] == '.' || name[0] == '/')
    ++name;
  while (*desc) {
    if (desc[0] == 'N' && desc[1] == 'A' && desc[2] == 'M' && desc[3] == 'E') {
      buf[pos++] = 0;
      desc += 4;
      fprintf(stderr, "%s%s", buf, name);
      pos = 0;
    } else {
      buf[pos++] = *desc;
      ++desc;
      if (pos >= 980) {
        buf[pos++] = 0;
        fwrite(buf, pos, sizeof(*buf), stderr);
        pos = 0;
      }
    }
  }
  if (pos)
    fwrite(buf, pos, sizeof(*buf), stderr);
}

FIO_SFUNC void fio___cli_set_arg(fio___cli_cstr_s arg,
                                 char const *value,
                                 char const *line,
                                 fio_cli_parser_data_s *parser) {
  char const *type = NULL;
  /* handle unnamed argument */
  if (!line || !arg.len) {
    if (!value) {
      goto print_help;
    }
    if (!strcmp(value, "-?") || !strcasecmp(value, "-h") ||
        !strcasecmp(value, "-help") || !strcasecmp(value, "--help")) {
      goto print_help;
    }
    fio___cli_cstr_s n = {.len = (size_t)++parser->unnamed_count};
    fio___cli_hash_set(&fio___cli_values, n.len, n, value, NULL);
    if (parser->unnamed_max >= 0 &&
        parser->unnamed_count > parser->unnamed_max) {
      arg.len = 0;
      goto error;
    }
    FIO_LOG_DEBUG2("(CLI) set an unnamed argument: %s", value);
    FIO_ASSERT_DEBUG(fio___cli_hash_get(&fio___cli_values, n.len, n) == value,
                     "(CLI) set argument failed!");
    return;
  }

  /* validate data types */
  type = fio___cli_get_line_type(parser, line);
  switch ((size_t)type) {
  case FIO_CLI_BOOL__TYPE_I:
    if (value && value != parser->argv[parser->pos + 1]) {
      while (*value) {
        /* support grouped boolean flags with one `-`*/
        char bf[3] = {'-', *value, 0};
        ++value;

        fio___cli_cstr_s a = {.buf = bf, .len = 2};

        const char *l =
            fio___cli_hash_get(&fio___cli_aliases, FIO_CLI_HASH_VAL(a), a);
        if (!l) {
          if (bf[1] == ',')
            continue;
          value = arg.buf + arg.len;
          goto error;
        }
        const char *t = fio___cli_get_line_type(parser, l);
        if (t != (char *)FIO_CLI_BOOL__TYPE_I) {
          value = arg.buf + arg.len;
          goto error;
        }
        fio___cli_set_arg(a, parser->argv[parser->pos + 1], l, parser);
      }
    }
    value = "1";
    break;
  case FIO_CLI_INT__TYPE_I:
    if (value) {
      char const *tmp = value;
      fio_atol((char **)&tmp);
      if (*tmp) {
        goto error;
      }
    }
    /* fall through */
  case FIO_CLI_STRING__TYPE_I:
    if (!value)
      goto error;
    if (!value[0])
      goto finish;
    break;
  }

  /* add values using all aliases possible */
  {
    fio___cli_cstr_s n = {.buf = line};
    while (n.buf[0] == '-') {
      while (n.buf[n.len] && n.buf[n.len] != ' ' && n.buf[n.len] != ',') {
        ++n.len;
      }
      fio___cli_hash_set(&fio___cli_values,
                         FIO_CLI_HASH_VAL(n),
                         n,
                         value,
                         NULL);
      FIO_LOG_DEBUG2("(CLI) set argument %.*s = %s", (int)n.len, n.buf, value);
      FIO_ASSERT_DEBUG(fio___cli_hash_get(&fio___cli_values,
                                          FIO_CLI_HASH_VAL(n),
                                          n) == value,
                       "(CLI) set argument failed!");
      while (n.buf[n.len] && (n.buf[n.len] == ' ' || n.buf[n.len] == ',')) {
        ++n.len;
      }
      n.buf += n.len;
      n.len = 0;
    }
  }

finish:

  /* handle additional argv progress (if value is on separate argv) */
  if (value && parser->pos < parser->argc &&
      value == parser->argv[parser->pos + 1])
    ++parser->pos;
  return;

error: /* handle errors*/
  FIO_LOG_DEBUG2("(CLI) error detected, printing help and exiting.");
  fprintf(stderr,
          "\n\r\x1B[31mError:\x1B[0m invalid argument %.*s %s %s\n\n",
          (int)arg.len,
          arg.buf,
          arg.len ? "with value" : "",
          value ? (value[0] ? value : "(empty)") : "(null)");
print_help:
  if (parser->description) {
    fprintf(stderr, "\n");
    fio___cli_print_line(parser->description, parser->argv[0]);
    fprintf(stderr, "\n");
  } else {
    const char *name_tmp = parser->argv[0];
    while (name_tmp[0] == '.' || name_tmp[0] == '/')
      ++name_tmp;
    fprintf(stderr,
            "\nAvailable command-line options for \x1B[1m%s\x1B[0m:\n",
            name_tmp);
  }
  /* print out each line's arguments */
  char const **pos = parser->names;
  while (*pos) {
    switch ((intptr_t)*pos) {
    case FIO_CLI_STRING__TYPE_I:     /* fall through */
    case FIO_CLI_BOOL__TYPE_I:       /* fall through */
    case FIO_CLI_INT__TYPE_I:        /* fall through */
    case FIO_CLI_PRINT__TYPE_I:      /* fall through */
    case FIO_CLI_PRINT_LINE__TYPE_I: /* fall through */
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      ++pos;
      continue;
    }
    type = (char *)FIO_CLI_STRING__TYPE_I;
    switch ((intptr_t)pos[1]) {
    case FIO_CLI_PRINT__TYPE_I:
      fprintf(stderr, "          \t   ");
      fio___cli_print_line(pos[0], parser->argv[0]);
      fprintf(stderr, "\n");
      pos += 2;
      continue;
    case FIO_CLI_PRINT_LINE__TYPE_I:
      fio___cli_print_line(pos[0], parser->argv[0]);
      fprintf(stderr, "\n");
      pos += 2;
      continue;
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      fprintf(stderr, "\n\x1B[4m");
      fio___cli_print_line(pos[0], parser->argv[0]);
      fprintf(stderr, "\x1B[0m\n");
      pos += 2;
      continue;

    case FIO_CLI_STRING__TYPE_I: /* fall through */
    case FIO_CLI_BOOL__TYPE_I:   /* fall through */
    case FIO_CLI_INT__TYPE_I:    /* fall through */
      type = pos[1];
    }
    /* print line @ pos, starting with main argument name */
    int alias_count = 0;
    int first_len = 0;
    size_t tmp = 0;
    char const *const p = *pos;
    fio___cli_cstr_s def = fio___cli_map_line2default(p);
    while (p[tmp] == '-') {
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        if (!alias_count)
          ++first_len;
        ++tmp;
      }
      ++alias_count;
      while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
        ++tmp;
      }
    }
    if (def.len) {
      tmp = (size_t)((def.buf + def.len + 1) - p);
      tmp += (p[tmp] == ')'); /* in case of `")` */
      while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
        ++tmp;
      }
    }
    switch ((size_t)type) {
    case FIO_CLI_STRING__TYPE_I:
      fprintf(stderr,
              " \x1B[1m%-10.*s\x1B[0m\x1B[2m\t\"\" \x1B[0m%s\n",
              first_len,
              p,
              p + tmp);
      break;
    case FIO_CLI_BOOL__TYPE_I:
      fprintf(stderr, " \x1B[1m%-10.*s\x1B[0m\t   %s\n", first_len, p, p + tmp);
      break;
    case FIO_CLI_INT__TYPE_I:
      fprintf(stderr,
              " \x1B[1m%-10.*s\x1B[0m\x1B[2m\t## \x1B[0m%s\n",
              first_len,
              p,
              p + tmp);
      break;
    }
    /* print alias information */
    tmp = first_len;
    while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
      ++tmp;
    }
    while (p[tmp] == '-') {
      const size_t start = tmp;
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        ++tmp;
      }
      int padding = first_len - (tmp - start);
      if (padding < 0)
        padding = 0;
      switch ((size_t)type) {
      case FIO_CLI_STRING__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%-10.*s\x1B[0m\x1B[2m\t\"\" \x1B[0m%.*s\x1B[2msame as "
                "%.*s\x1B[0m\n",
                (int)(tmp - start),
                p + start,
                padding,
                "",
                first_len,
                p);
        break;
      case FIO_CLI_BOOL__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%-10.*s\x1B[0m\t   %.*s\x1B[2msame as %.*s\x1B[0m\n",
                (int)(tmp - start),
                p + start,
                padding,
                "",
                first_len,
                p);
        break;
      case FIO_CLI_INT__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%-10.*s\x1B[0m\x1B[2m\t## \x1B[0m%.*s\x1B[2msame as "
                "%.*s\x1B[0m\n",
                (int)(tmp - start),
                p + start,
                padding,
                "",
                first_len,
                p);
        break;
      }
    }
    /* print default information */
    if (def.len)
      fprintf(stderr,
              "           \t\x1B[2mdefault value: %.*s\x1B[0m\n",
              (int)def.len,
              def.buf);
    ++pos;
  }
  fprintf(stderr,
          "\nUse any of the following input formats:\n"
          "\t-arg <value>\t-arg=<value>\t-arg<value>\n"
          "\n"
          "Use \x1B[1m-h\x1B[0m , \x1B[1m-help\x1B[0m or "
          "\x1B[1m-?\x1B[0m "
          "to get this information again.\n"
          "\n");
  fio_cli_end();
  exit(0);
}

/* *****************************************************************************
CLI Initialization
***************************************************************************** */

void fio_cli_start___(void); /* sublime text marker */
SFUNC void fio_cli_start FIO_NOOP(int argc,
                                  char const *argv[],
                                  int unnamed_min,
                                  int unnamed_max,
                                  char const *description,
                                  char const **names) {
  if (unnamed_max >= 0 && unnamed_max < unnamed_min)
    unnamed_max = unnamed_min;
  fio_cli_parser_data_s parser = {
      .unnamed_min = unnamed_min,
      .unnamed_max = unnamed_max,
      .pos = 0,
      .argc = argc,
      .argv = argv,
      .description = description,
      .names = names,
  };

  if (fio___cli_hash_count(&fio___cli_values)) {
    fio_cli_end();
  }

  /* initialize the default value linked list */
  fio___cli_default_values = FIO_LIST_INIT(fio___cli_default_values);

  /* prepare aliases hash map */

  char const **line = names;
  while (*line) {
    switch ((intptr_t)*line) {
    case FIO_CLI_STRING__TYPE_I:       /* fall through */
    case FIO_CLI_BOOL__TYPE_I:         /* fall through */
    case FIO_CLI_INT__TYPE_I:          /* fall through */
    case FIO_CLI_PRINT__TYPE_I:        /* fall through */
    case FIO_CLI_PRINT_LINE__TYPE_I:   /* fall through */
    case FIO_CLI_PRINT_HEADER__TYPE_I: /* fall through */
      ++line;
      continue;
    }
    if (line[1] != (char *)FIO_CLI_PRINT__TYPE_I &&
        line[1] != (char *)FIO_CLI_PRINT_LINE__TYPE_I &&
        line[1] != (char *)FIO_CLI_PRINT_HEADER__TYPE_I)
      fio___cli_map_line2alias(*line);
    ++line;
  }

  /* parse existing arguments */

  while ((++parser.pos) < argc) {
    char const *value = NULL;
    fio___cli_cstr_s n = {.buf = argv[parser.pos],
                          .len = strlen(argv[parser.pos])};
    if (parser.pos + 1 < argc) {
      value = argv[parser.pos + 1];
    }
    const char *l = NULL;
    while (
        n.len &&
        !(l = fio___cli_hash_get(&fio___cli_aliases, FIO_CLI_HASH_VAL(n), n))) {
      --n.len;
      value = n.buf + n.len;
    }
    if (n.len && value && value[0] == '=') {
      ++value;
    }
    // fprintf(stderr, "Setting %.*s to %s\n", (int)n.len, n.buf, value);
    fio___cli_set_arg(n, value, l, &parser);
  }

  /* Cleanup and save state for API */
  fio___cli_hash_destroy(&fio___cli_aliases);
  fio___cli_unnamed_count = parser.unnamed_count;
  /* test for required unnamed arguments */
  if (parser.unnamed_count < parser.unnamed_min)
    fio___cli_set_arg((fio___cli_cstr_s){.len = 0}, NULL, NULL, &parser);
}

/* *****************************************************************************
CLI Destruction
***************************************************************************** */

SFUNC void __attribute__((destructor)) fio_cli_end(void) {
  fio___cli_hash_destroy(&fio___cli_values);
  fio___cli_hash_destroy(&fio___cli_aliases);
  fio___cli_unnamed_count = 0;
  if (fio___cli_default_values.next) {
    while (!FIO_LIST_IS_EMPTY(&fio___cli_default_values)) {
      fio___cli_def_str_s *node;
      FIO_LIST_POP(fio___cli_def_str_s, node, node, &fio___cli_default_values);
      FIO_MEM_FREE_(node, sizeof(*node) + node->len + 1);
    }
  }
}
/* *****************************************************************************
CLI Data Access API
***************************************************************************** */

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name) {
  if (!name)
    return NULL;
  fio___cli_cstr_s n = {.buf = name, .len = strlen(name)};
  if (!fio___cli_hash_count(&fio___cli_values)) {
    return NULL;
  }
  char const *val =
      fio___cli_hash_get(&fio___cli_values, FIO_CLI_HASH_VAL(n), n);
  return val;
}

/** Returns the argument's value as an integer. */
SFUNC int fio_cli_get_i(char const *name) {
  char const *val = fio_cli_get(name);
  return fio_atol((char **)&val);
}

/** Returns the number of unrecognized argument. */
SFUNC unsigned int fio_cli_unnamed_count(void) {
  return (unsigned int)fio___cli_unnamed_count;
}

/** Returns the unrecognized argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index) {
  if (!fio___cli_hash_count(&fio___cli_values) || !fio___cli_unnamed_count) {
    return NULL;
  }
  fio___cli_cstr_s n = {.buf = NULL, .len = index + 1};
  return fio___cli_hash_get(&fio___cli_values, index + 1, n);
}

/**
 * Sets the argument's value as a NUL terminated C String (no copy!).
 *
 * Note: this does NOT copy the C strings to memory. Memory should be kept
 * alive until `fio_cli_end` is called.
 */
SFUNC void fio_cli_set(char const *name, char const *value) {
  fio___cli_cstr_s n = (fio___cli_cstr_s){.buf = name, .len = strlen(name)};
  fio___cli_hash_set(&fio___cli_values, FIO_CLI_HASH_VAL(n), n, value, NULL);
}

/* *****************************************************************************
CLI - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, cli)(void) {
  const char *argv[] = {
      "appname",
      "-i11",
      "-i2=2",
      "-i3",
      "3",
      "-t,u",
      "-s",
      "test",
      "unnamed",
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);
  fprintf(stderr, "* Testing CLI helpers.\n");
  { /* avoid macro for C++ */
    const char *arguments[] = {
        FIO_CLI_INT("-integer1 -i1 first integer"),
        FIO_CLI_INT("-integer2 -i2 second integer"),
        FIO_CLI_INT("-integer3 -i3 third integer"),
        FIO_CLI_INT("-integer4 -i4 (4) fourth integer"),
        FIO_CLI_INT("-integer5 -i5 (\"5\") fifth integer"),
        FIO_CLI_BOOL("-boolean -t boolean"),
        FIO_CLI_BOOL("-boolean2 -u boolean"),
        FIO_CLI_BOOL("-boolean_false -f boolean"),
        FIO_CLI_STRING("-str -s a string"),
        FIO_CLI_PRINT_HEADER("Printing stuff"),
        FIO_CLI_PRINT_LINE("does nothing, but shouldn't crash either"),
        FIO_CLI_PRINT("does nothing, but shouldn't crash either"),
        NULL,
    };
    fio_cli_start FIO_NOOP(argc, argv, 0, -1, NULL, arguments);
  }
  FIO_ASSERT(fio_cli_get_i("-i2") == 2, "CLI second integer error.");
  FIO_ASSERT(fio_cli_get_i("-i3") == 3, "CLI third integer error.");
  FIO_ASSERT(fio_cli_get_i("-i4") == 4,
             "CLI fourth integer error (%s).",
             fio_cli_get("-i4"));
  FIO_ASSERT(fio_cli_get_i("-i5") == 5,
             "CLI fifth integer error (%s).",
             fio_cli_get("-i5"));
  FIO_ASSERT(fio_cli_get_i("-i1") == 1, "CLI first integer error.");
  FIO_ASSERT(fio_cli_get_i("-i2") == fio_cli_get_i("-integer2"),
             "CLI second integer error.");
  FIO_ASSERT(fio_cli_get_i("-i3") == fio_cli_get_i("-integer3"),
             "CLI third integer error.");
  FIO_ASSERT(fio_cli_get_i("-i1") == fio_cli_get_i("-integer1"),
             "CLI first integer error.");
  FIO_ASSERT(fio_cli_get_i("-t") == 1, "CLI boolean true error.");
  FIO_ASSERT(fio_cli_get_i("-u") == 1, "CLI boolean 2 true error.");
  FIO_ASSERT(fio_cli_get_i("-f") == 0, "CLI boolean false error.");
  FIO_ASSERT(!strcmp(fio_cli_get("-s"), "test"), "CLI string error.");
  FIO_ASSERT(fio_cli_unnamed_count() == 1, "CLI unnamed count error.");
  FIO_ASSERT(!strcmp(fio_cli_unnamed(0), "unnamed"), "CLI unnamed error.");
  fio_cli_set("-manual", "okay");
  FIO_ASSERT(!strcmp(fio_cli_get("-manual"), "okay"), "CLI set/get error.");
  fio_cli_end();
  FIO_ASSERT(fio_cli_get_i("-i1") == 0, "CLI cleanup error.");
}
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
CLI - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE*/
#endif /* FIO_CLI */
#undef FIO_CLI
