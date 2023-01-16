/* *****************************************************************************




***************************************************************************** */

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif

#if !defined(FIO_STL_KEEP__)
/* *****************************************************************************
Everything, and the Kitchen Sink
***************************************************************************** */
#if defined(FIO_EVERYTHING) && !defined(H___FIO_EVERYTHING___H)
#undef FIO_EVERYTHING
#define H___FIO_EVERYTHING___H

#define FIO_BASIC
#define FIO_CORE
#define FIO_CRYPT
#define FIO_SERVER_COMPLETE

#endif /* FIO_EVERYTHING */

#ifdef FIO_BASIC
#define FIO_SERVER_COMPLETE
#endif

/* *****************************************************************************
Basic Elements
***************************************************************************** */
#if defined(FIO_BASIC)
#undef FIO_BASIC

#define FIO_CLI
#define FIO_LOG
#include FIO_INCLUDE_FILE

#define FIO_FIOBJ
#define FIOBJ_MALLOC
#include FIO_INCLUDE_FILE

#define FIO_CORE
#define FIO_MALLOC
#define FIO_THREADS

#endif /* FIO_BASIC */

/* *****************************************************************************
Core Elements
***************************************************************************** */
#if defined(FIO_CORE)
#undef FIO_CORE

#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_BITMAP
#define FIO_BITWISE
#define FIO_FILES
#define FIO_GLOB_MATCH
#define FIO_LOG
#define FIO_MATH
#define FIO_RAND
#define FIO_STATE
#define FIO_STR
#define FIO_TIME
#define FIO_URL

#include FIO_INCLUDE_FILE

#endif /* FIO_CORE */

/* *****************************************************************************
Core Elements
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
#if defined(FIO_SERVER_COMPLETE)
#undef FIO_SERVER_COMPLETE

// #define FIO_HTTP1_PARSER
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

#endif /* FIO_STL_KEEP__ */
