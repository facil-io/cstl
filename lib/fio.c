/*
 * Allows the use of the facil.io C STL as a library.
 *
 * This C file will include all function definitions and can be compiled either
 * with the project or separately to be used as a dynamic / static library.
 */
#ifndef FIO_EXTERN_COMPLETE
#define FIO_EXTERN_COMPLETE 99 /* a value above 1 == "non single-use" */
#endif
#include "fio.h"

size_t fio_version_major(void) { return FIO_VERSION_MAJOR; }
size_t fio_version_minor(void) { return FIO_VERSION_MINOR; }
size_t fio_version_patch(void) { return FIO_VERSION_PATCH; }
char *fio_version_string(void) { return FIO_VERSION_STRING; }
