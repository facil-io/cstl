#ifndef H___INCLUDE_FIO_EVERYTHING___H
/*
 * To use the facil.io C STL as a static library, we compile can it as a
 * separate unit.
 *
 * This header includes all the macros and static functions, the compiled C file
 * will include all the function definitions to be used as a static library.
 */
#define FIO_EVERYTHING
#define FIO_EXTERN 99 /* a value above 1 == "non single-use" */

#include "fio-stl/include.h"

/** Returns the major version number. */
size_t fio_version_major(void);
/** Returns the minor version number. */
size_t fio_version_minor(void);
/** Returns the patch version number. */
size_t fio_version_patch(void);
/** Returns the version number as a string. */
char *fio_version_string(void);

/** This validates that the facil.io C STL library semantic version matches .*/
FIO_SFUNC void fio_version_validate(void) {
  FIO_ASSERT(fio_version_major() == FIO_VERSION_MAJOR &&
                 FIO_VERSION_MINOR >= fio_version_minor(),
             "facil.io C STL library version mismatch! (%s ! ~= %s)"
             "\n\tplease recompile to manage breaking changes.",
             fio_version_string(),
             FIO_VERSION_STRING);
}

#endif /* H___INCLUDE_FIO_EVERYTHING___H */
