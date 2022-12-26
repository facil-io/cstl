#ifndef H___INCLUDE_FIO_EVERYTHING___H
/*
 * Allows the use of the facil.io C STL as a library.
 *
 * This header includes all the macros and static functions, the compiled C file
 * will include all published functions.
 */
#define FIO_EVERYTHING
#define FIO_EXTERN

// #define FIO_MEMORY_DISABLE 1
// #define FIO_USE_THREAD_MUTEX 1
// #define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL

#ifdef DEBUG
#define FIO_LEAK_COUNTER 1
#endif

#include "fio-stl/include.h"

/** Returns the major version number. */
size_t fio_version_major(void);
/** Returns the minor version number. */
size_t fio_version_minor(void);
/** Returns the patch version number. */
size_t fio_version_patch(void);
/** Returns the build version string. */
const char *fio_version_build(void);
/** Returns the version number as a string. */
char *fio_version_string(void);
/** Returns the complete version number in 32 bit format. */
size_t fio_version(void);

FIO_SFUNC void fio_version_validate(void) {
  FIO_ASSERT(fio_version_major() == FIO_VERSION_MAJOR &&
                 FIO_VERSION_MINOR >= fio_version_minor(),
             "facil.io C STL library version mismatch! (%d.x.x != %d.x.x)"
             "\n\tplease recompile to manage breaking changes.",
             fio_version_major(),
             FIO_VERSION_MAJOR);
}

#define FIO_VERSION_NUMBER                                                     \
  (((size_t)FIO_VERSION_MAJOR << 20) | ((size_t)FIO_VERSION_MINOR << 10) |     \
   ((size_t)FIO_VERSION_PATCH))

#endif /* H___INCLUDE_FIO_EVERYTHING___H */
