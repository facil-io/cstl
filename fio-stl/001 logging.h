/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_LOG                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                  Logging





Use:

```c
FIO_LOG2STDERR("message.") // => message.
FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING; // set dynamic logging level
FIO_LOG_INFO("message"); // => [no output, exceeds logging level]
int i = 3;
FIO_LOG_WARNING("number invalid: %d", i); // => WARNING: number invalid: 3
```

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */

/**
 * Enables logging macros that avoid heap memory allocations
 */
#if !defined(FIO_NO_LOG) && !defined(H___FIO_LOG___H) &&                       \
    (defined(FIO_LOG) || defined(FIO_LEAK_COUNTER))
#define H___FIO_LOG___H

#undef FIO_LOG2STDERR

FIO_SFUNC FIO___PRINTF_STYLE(1, 0) void FIO_LOG2STDERR(const char *format,
                                                       ...) {
#if FIO_LOG_LENGTH_LIMIT > 128
#define FIO_LOG____LENGTH_ON_STACK FIO_LOG_LENGTH_LIMIT
#define FIO_LOG____LENGTH_BORDER   (FIO_LOG_LENGTH_LIMIT - 34)
#else
#define FIO_LOG____LENGTH_ON_STACK (FIO_LOG_LENGTH_LIMIT + 34)
#define FIO_LOG____LENGTH_BORDER   FIO_LOG_LENGTH_LIMIT
#endif
  va_list argv;
  char tmp___log[FIO_LOG____LENGTH_ON_STACK + 32];
  va_start(argv, format);
  int len___log = vsnprintf(tmp___log, FIO_LOG_LENGTH_LIMIT - 2, format, argv);
  va_end(argv);
  if (len___log > 0) {
    if (len___log >= FIO_LOG_LENGTH_LIMIT - 2) {
      fio_memcpy32(tmp___log + FIO_LOG____LENGTH_BORDER,
                   "...\n\t\x1B[2mWARNING:\x1B[0m TRUNCATED!");
      len___log = FIO_LOG____LENGTH_BORDER + 32;
    }
    tmp___log[len___log++] = '\n';
    tmp___log[len___log] = '0';
    fwrite(tmp___log, 1, len___log, stderr);
    return;
  }
  fwrite("\x1B[1mERROR:\x1B[0m log output error (can't write).\n",
         1,
         47,
         stderr);
#undef FIO_LOG____LENGTH_ON_STACK
#undef FIO_LOG____LENGTH_BORDER
}

/** The logging level */
#ifndef FIO_LOG_LEVEL_DEFAULT
#if defined(DEBUG) && DEBUG
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_DEBUG
#else
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_INFO
#endif
#endif
int FIO_WEAK_VAR FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEFAULT;
FIO_IFUNC int fio___log_level_set(int i) { return (FIO_LOG_LEVEL = i); }
FIO_IFUNC int fio___log_level(void) { return FIO_LOG_LEVEL; }

#undef FIO_LOG_LEVEL_GET
#undef FIO_LOG_LEVEL_SET

/** Sets the Logging Level. */
#define FIO_LOG_LEVEL_SET(new_level) fio___log_level_set(new_level)
/** Returns the Logging Level. */
#define FIO_LOG_LEVEL_GET() ((fio___log_level()))

#endif /* FIO_LOG */
#undef FIO_LOG
