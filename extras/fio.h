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
#endif /* H___INCLUDE_FIO_EVERYTHING___H */
