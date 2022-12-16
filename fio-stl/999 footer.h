/* *****************************************************************************




***************************************************************************** */

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif

/* *****************************************************************************
Everything, and the Kitchen Sink
***************************************************************************** */
#if defined(FIO_EVERYTHING) && !defined(H___FIO_EVERYTHING___H)
#undef FIO_EVERYTHING
#define H___FIO_EVERYTHING___H

#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_BITMAP
#define FIO_BITWISE
#define FIO_CHACHA
#define FIO_CLI
#define FIO_FILES
#define FIO_GLOB_MATCH
#define FIO_LOG
#define FIO_MATH
#define FIO_RAND
#define FIO_RISKY_HASH
#define FIO_SHA1
#define FIO_SHA2
// #define FIO_ED25519
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_STATE
#define FIO_THREADS
#define FIO_TIME
#define FIO_URL

#include FIO___INCLUDE_FILE

#define FIO_MALLOC
#define FIO_PUBSUB
#define FIO_QUEUE
#define FIO_SERVER
#define FIO_STR
#define FIO_STREAM

#include FIO___INCLUDE_FILE

#define FIOBJ_MALLOC
#define FIO_FIOBJ
#include FIO___INCLUDE_FILE

#ifdef FIO_EVERYTHING___REMOVE_EXTERN
#undef FIO_EXTERN
#undef FIO_EVERYTHING___REMOVE_EXTERN
#endif
#ifdef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#undef FIO_EXTERN_COMPLETE
#undef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#endif

#endif /* FIO_EVERYTHING */
