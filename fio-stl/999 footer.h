/* *****************************************************************************




***************************************************************************** */

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif

#if !defined(FIO___STL_KEEP) && !defined(FIO___STL_SHORTCUTS)
#define FIO___STL_SHORTCUTS
/* *****************************************************************************
Everything, and the Kitchen Sink
***************************************************************************** */
#if defined(FIO_EVERYTHING) && !defined(H___FIO_EVERYTHING___H)
#define H___FIO_EVERYTHING___H

#define FIO_BASIC
#define FIO_CORE
#define FIO_CRYPT
#define FIO_SERVER_COMPLETE

#endif /* FIO_EVERYTHING */

#if defined(FIO_BASIC) && !defined(H___FIO_BASIC___H)
#define H___FIO_BASIC___H
#define FIO_BASIC___PRE
#define FIO_BASIC___POST
#define FIO_CORE
#endif

/* *****************************************************************************
Basic Elements - Pre Allocator
***************************************************************************** */
#if defined(FIO_BASIC___PRE)
#undef FIO_BASIC___PRE

#define FIO_CLI
#define FIO_FILES
#define FIO_LOG
#define FIO_STATE
#define FIO_THREADS
#include FIO_INCLUDE_FILE

#define FIO_MALLOC
#endif /* FIO_BASIC */

/* *****************************************************************************
Core Elements
***************************************************************************** */
#if defined(FIO_CORE) && !defined(H___FIO_CORE___H)
#define H___FIO_CORE___H

#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_BITMAP
#define FIO_BITWISE
#define FIO_GLOB_MATCH
#define FIO_LOG
#define FIO_MATH
#define FIO_RAND
#define FIO_STR
#define FIO_TIME
#define FIO_URL

#include FIO_INCLUDE_FILE

#endif /* FIO_CORE */

/* *****************************************************************************
Basic Elements - Post Allocator
***************************************************************************** */
#if defined(FIO_BASIC___POST)
#undef FIO_BASIC___POST

#define FIO_FIOBJ
#define FIOBJ_MALLOC
#include FIO_INCLUDE_FILE

#endif /* FIO_BASIC */

/* *****************************************************************************
Poor-man's Cryptographic Elements
***************************************************************************** */
#if defined(FIO_CRYPT)
#undef FIO_CRYPT
#define FIO_CHACHA
#define FIO_ED25519
#define FIO_SHA1
#define FIO_SHA2

#include FIO_INCLUDE_FILE

#endif /* FIO_CRYPT */
/* *****************************************************************************
Server Elements
***************************************************************************** */
#if defined(FIO_SERVER_COMPLETE) && !defined(H___FIO_SERVER_COMPLETE___H)
#define H___FIO_SERVER_COMPLETE___H

#define FIO_MEMORY_NAME        fio___server_mem
#define FIO_MEMORY_ARENA_COUNT 4

#define FIO_HTTP
#define FIO_PUBSUB
#define FIO_QUEUE
#define FIO_SERVER
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_STREAM

#include FIO_INCLUDE_FILE
#endif /* FIO_SERVER_COMPLETE */

/* *****************************************************************************
Cleanup
***************************************************************************** */
#ifdef FIO_EVERYTHING___REMOVE_EXTERN
#undef FIO_EXTERN
#undef FIO_EVERYTHING___REMOVE_EXTERN
#endif
#ifdef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#undef FIO_EXTERN_COMPLETE
#undef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#endif

#undef FIO_EVERYTHING
#undef FIO_BASIC
#undef FIO_CORE
#undef FIO_CRYPT
#undef FIO_SERVER_COMPLETE
#undef FIO___STL_SHORTCUTS
#endif /* FIO___STL_KEEP */
