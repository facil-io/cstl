/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#include "000 header.h"               /* Development inclusion - ignore line */
#endif                                /* Development inclusion - ignore line */
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

***************************************************************************** */

/**
 * Enables logging macros that avoid heap memory allocations
 */
#if !defined(H___FIO_LOG___H) && (defined(FIO_LOG) || defined(FIO_LEAK_COUNTER))
#define H___FIO_LOG___H

#if FIO_LOG_LENGTH_LIMIT > 128
#define FIO_LOG____LENGTH_ON_STACK FIO_LOG_LENGTH_LIMIT
#define FIO_LOG____LENGTH_BORDER   (FIO_LOG_LENGTH_LIMIT - 34)
#else
#define FIO_LOG____LENGTH_ON_STACK (FIO_LOG_LENGTH_LIMIT + 34)
#define FIO_LOG____LENGTH_BORDER   FIO_LOG_LENGTH_LIMIT
#endif

#undef FIO_LOG2STDERR

__attribute__((format(FIO___PRINTF_STYLE, 1, 0), weak)) void FIO_LOG2STDERR(
    const char *format,
    ...) {
  va_list argv;
  char tmp___log[FIO_LOG____LENGTH_ON_STACK + 32];
  va_start(argv, format);
  int len___log = vsnprintf(tmp___log, FIO_LOG_LENGTH_LIMIT - 2, format, argv);
  va_end(argv);
  if (len___log > 0) {
    if (len___log >= FIO_LOG_LENGTH_LIMIT - 2) {
      FIO_MEMCPY(tmp___log + FIO_LOG____LENGTH_BORDER,
                 "...\n\t\x1B[2mWARNING:\x1B[0m TRUNCATED!",
                 32);
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
}
#undef FIO_LOG____LENGTH_ON_STACK
#undef FIO_LOG____LENGTH_BORDER

// clang-format off
#undef FIO_LOG2STDERR2
#define FIO_LOG2STDERR2(...) FIO_LOG2STDERR("(" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)
// clang-format on

/** Logging level of zero (no logging). */
#define FIO_LOG_LEVEL_NONE 0
/** Log fatal errors. */
#define FIO_LOG_LEVEL_FATAL 1
/** Log errors and fatal errors. */
#define FIO_LOG_LEVEL_ERROR 2
/** Log warnings, errors and fatal errors. */
#define FIO_LOG_LEVEL_WARNING 3
/** Log every message (info, warnings, errors and fatal errors). */
#define FIO_LOG_LEVEL_INFO 4
/** Log everything, including debug messages. */
#define FIO_LOG_LEVEL_DEBUG 5

/** The logging level */
#ifndef FIO_LOG_LEVEL_DEFAULT
#if DEBUG
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_DEBUG
#else
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_INFO
#endif
#endif
int __attribute__((weak)) FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEFAULT;

#undef FIO_LOG_PRINT__
#define FIO_LOG_PRINT__(level, ...)                                            \
  do {                                                                         \
    if (level <= FIO_LOG_LEVEL)                                                \
      FIO_LOG2STDERR(__VA_ARGS__);                                             \
  } while (0)

// clang-format off
#undef FIO_LOG_FATAL
#define FIO_LOG_FATAL(...)    FIO_LOG_PRINT__(FIO_LOG_LEVEL_FATAL, "\x1B[1m\x1B[7mFATAL:\x1B[0m    " __VA_ARGS__)
#undef FIO_LOG_ERROR
#define FIO_LOG_ERROR(...)    FIO_LOG_PRINT__(FIO_LOG_LEVEL_ERROR, "\x1B[1mERROR:\x1B[0m    " __VA_ARGS__)
#undef FIO_LOG_SECURITY
#define FIO_LOG_SECURITY(...) FIO_LOG_PRINT__(FIO_LOG_LEVEL_ERROR, "\x1B[1mSECURITY:\x1B[0m " __VA_ARGS__)
#undef FIO_LOG_WARNING
#define FIO_LOG_WARNING(...)  FIO_LOG_PRINT__(FIO_LOG_LEVEL_WARNING, "\x1B[2mWARNING:\x1B[0m  " __VA_ARGS__)
#undef FIO_LOG_INFO
#define FIO_LOG_INFO(...)     FIO_LOG_PRINT__(FIO_LOG_LEVEL_INFO, "INFO:     " __VA_ARGS__)
#undef FIO_LOG_DEBUG
#define FIO_LOG_DEBUG(...)    FIO_LOG_PRINT__(FIO_LOG_LEVEL_DEBUG,"DEBUG:    (" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__)
#undef FIO_LOG_DEBUG2
#define FIO_LOG_DEBUG2(...)   FIO_LOG_PRINT__(FIO_LOG_LEVEL_DEBUG, "DEBUG:    " __VA_ARGS__)
// clang-format on

#endif /* FIO_LOG */
#undef FIO_LOG
