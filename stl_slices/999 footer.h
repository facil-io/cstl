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
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_STATE
#define FIO_THREADS
#define FIO_TIME
#define FIO_URL

#include FIO__FILE__

#define FIO_MALLOC
#define FIO_QUEUE
#define FIO_STR_CORE

#include FIO__FILE__

#define FIO_MEMORY_NAME fio__srv_mem
#define FIO_PUBSUB
#define FIO_SERVER
#define FIO_STREAM

#include FIO__FILE__

#define FIO_FIOBJ
#ifdef FIO_EXTERN
#ifndef FIOBJ_EXTERN
#define FIOBJ_EXTERN
#endif
#ifndef FIOBJ_MALLOC
#define FIOBJ_MALLOC
#endif
#endif
#ifdef FIO_EXTERN_COMPLETE
#ifndef FIOBJ_EXTERN_COMPLETE
#define FIOBJ_EXTERN_COMPLETE
#endif
#define FIOBJ_EXTERN_COMPLETE
#endif

#include FIO__FILE__
#endif
