/* *****************************************************************************



                              Multi-Inclusion Macros



***************************************************************************** */

/* *****************************************************************************
Tests Inclusion (everything + MEMALT)
***************************************************************************** */
#if (defined(FIO_TEST_ALL) || defined(FIO___TEST_MACRO_SUSPENDED)) &&          \
    !defined(H___FIO_TESTS_INC_FINISHED___H) &&                                \
    !defined(FIO___RECURSIVE_INCLUDE)

/* Inclusion cycle three - facil.io memory allocator for all else. */
#if !defined(H___FIO_TESTS_INC_FINISHED___H) &&                                \
    defined(H___FIO_EVERYTHING_FINISHED___H)
#define H___FIO_TESTS_INC_FINISHED___H
#undef FIO___TEST_MACRO_SUSPENDED
#define FIO_TEST_ALL
#elif !defined(H___FIO_BASIC1___H)
#undef FIO_TEST_ALL
#define FIO___TEST_MACRO_SUSPENDED
#undef FIO_LEAK_COUNTER
#define FIO_LEAK_COUNTER 1
#define FIO_EVERYTHING
#endif

#endif /* FIO_TEST_ALL */

/* *****************************************************************************
Special `extern` support FIO_BASIC, FIO_EVERYTHING, etc'
***************************************************************************** */
#if defined(FIO_EXTERN) && !defined(FIO___RECURSIVE_INCLUDE) &&                \
    (defined(FIO_TEST_ALL) || defined(FIO_EVERYTHING) || defined(FIO_BASIC))
#if defined(FIO_EXTERN) && ((FIO_EXTERN + 1) < 3)
#undef FIO_EXTERN
#define FIO_EXTERN                     2
#define FIO_EVERYTHING___REMOVE_EXTERN 1
#endif
#if defined(FIO_EXTERN_COMPLETE) && ((FIO_EXTERN_COMPLETE + 1) < 3)
#undef FIO_EXTERN_COMPLETE
#define FIO_EXTERN_COMPLETE                     2
#define FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE 1
#endif
#endif

/* *****************************************************************************
Everything Inclusion
***************************************************************************** */
#if defined(FIO_EVERYTHING) && !defined(FIO___RECURSIVE_INCLUDE)

#if !defined(H___FIO_EVERYTHING_FINISHED___H) &&                               \
    defined(H___FIO_EVERYTHING2___H)
/* Inclusion cycle three - facil.io memory allocator for all else. */
#define H___FIO_EVERYTHING_FINISHED___H
#undef FIO_MEMALT
#define FIO_MEMALT
#define FIO___INCLUDE_AGAIN
#elif !defined(H___FIO_EVERYTHING2___H) && defined(H___FIO_BASIC_FINISHED___H)
/* Inclusion cycle two - import server modules. */
#define H___FIO_EVERYTHING2___H
#define FIO_SERVER
#define FIO_PUBSUB
#define FIO_HTTP
#define FIO___INCLUDE_AGAIN
#elif !defined(H___FIO_EVERYTHING_FINISHED___H)
/* Inclusion cycle one - import FIO_BASIC. */
#undef FIO_SERVER
#undef FIO_PUBSUB
#undef FIO_HTTP
#undef FIO_BASIC
#undef FIO_SIGNAL
#undef FIO_SOCK
#define FIO_BASIC
#define FIO_SIGNAL
#define FIO_SOCK
#else
#undef FIO_EVERYTHING /* final cycle, allows extension  */

#endif /* H___FIO_EVERYTHING___H */
#endif /* FIO_EVERYTHING */
/* *****************************************************************************
Basics Inclusion
***************************************************************************** */
#if defined(FIO_BASIC) && !defined(FIO___RECURSIVE_INCLUDE)

#if !defined(H___FIO_BASIC_FINISHED___H) && defined(H___FIO_BASIC2___H)
/* Inclusion cycle three - facil.io memory allocator for all else. */
#define H___FIO_BASIC_FINISHED___H
#define FIO_MALLOC
#define FIO___INCLUDE_AGAIN

#elif !defined(H___FIO_BASIC_FINISHED___H) && defined(H___FIO_BASIC1___H)
/* Inclusion cycle two - FIOBJ & its dedicated memory allocator. */
#define H___FIO_BASIC2___H
#define FIO_FIOBJ
#define FIO_MUSTACHE
#define FIOBJ_MALLOC
#define FIO___INCLUDE_AGAIN

#elif !defined(H___FIO_BASIC_FINISHED___H)
/* Inclusion cycle one - default (system) memory allocator. */
#define H___FIO_BASIC1___H
#undef FIO_CLI
#undef FIO_CORE
#undef FIO_CRYPT
#undef FIO_FIOBJ
#undef FIO_MALLOC
#undef FIO_MUSTACHE
#undef FIO_STATE
#undef FIO_THREADS
#undef FIOBJ_MALLOC
#define FIO_CLI
#define FIO_CORE
#define FIO_CRYPT
#define FIO_STATE
#define FIO_THREADS
#define FIO___INCLUDE_AGAIN

#else
/* Final cycle, does nothing but allows extension from basic to everything. */
#undef FIO_BASIC

#endif /* H___FIO_BASIC___H */
#endif /* FIO_BASIC */
/* *****************************************************************************
Poor-man's Cryptographic Elements
***************************************************************************** */
#if defined(FIO_CRYPT)
#undef FIO_CHACHA
#undef FIO_ED25519
#undef FIO_SHA1
#undef FIO_SHA2
#define FIO_CHACHA
#define FIO_ED25519
#define FIO_SHA1
#define FIO_SHA2
#undef FIO_CRYPT
#endif /* FIO_CRYPT */

/* *****************************************************************************
Core Inclusion
***************************************************************************** */
#if defined(FIO_CORE)
#undef FIO_ATOL
#undef FIO_ATOMIC
#undef FIO_FILES
#undef FIO_GLOB_MATCH
#undef FIO_LOG
#undef FIO_MATH
#undef FIO_RAND
#undef FIO_TIME
#undef FIO_URL
#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_FILES
#define FIO_GLOB_MATCH
#define FIO_LOG
#define FIO_MATH
#define FIO_RAND
#define FIO_TIME
#define FIO_URL
#undef FIO_CORE
#endif

/* *****************************************************************************



                                  Shortcut Macros



***************************************************************************** */

/* *****************************************************************************
Memory Allocation - FIO_MALLOC as a "global" default memory allocator
***************************************************************************** */
/* FIO_MALLOC defines a "global" default memory allocator */
#if defined(FIO_MALLOC) && !defined(H___FIO_MALLOC___H)
#ifndef FIO_MEMORY_NAME
#define FIO_MEMORY_NAME fio
#endif
#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/* for a general allocator, increase system allocation size to 8Mb */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 23
#endif

#ifndef FIO_MEMORY_CACHE_SLOTS
/* for a general allocator, increase cache size */
#define FIO_MEMORY_CACHE_SLOTS 8
#endif

#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
/* set fragmentation cost at 0.25Mb blocks */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 5
#endif

#ifndef FIO_MEMORY_ENABLE_BIG_ALLOC
/* support big allocations using undivided memory chunks */
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
#endif

/* *****************************************************************************
Memory Allocation - FIO_MALLOC defines a FIOBJ dedicated memory allocator
***************************************************************************** */
/* FIOBJ_MALLOC defines a FIOBJ dedicated memory allocator */
#elif defined(FIOBJ_MALLOC) && !defined(H___FIOBJ_MALLOC___H)
#define H___FIOBJ_MALLOC___H
#ifndef FIO_MEMORY_NAME
#define FIO_MEMORY_NAME fiobj_mem
#endif
#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/* 4Mb per system call */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 22
#endif
#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
/* fight fragmentation */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 4
#endif
#ifndef FIO_MEMORY_ALIGN_LOG
/* align on 8 bytes, it's enough for FIOBJ types */
#define FIO_MEMORY_ALIGN_LOG 3
#endif
#ifndef FIO_MEMORY_CACHE_SLOTS
/* cache up to 64Mb */
#define FIO_MEMORY_CACHE_SLOTS 16
#endif
#endif /* FIOBJ_MALLOC */

/* *****************************************************************************
FIO_SORT_NAME naming
***************************************************************************** */

#if defined(FIO_SORT_TYPE) && !defined(FIO_SORT_NAME)
#define FIO_SORT_NAME FIO_NAME(FIO_SORT_TYPE, vec)
#endif

/* *****************************************************************************
FIO_MAP Ordering & Naming Shortcut
***************************************************************************** */
#if defined(FIO_UMAP_NAME)
#define FIO_MAP_NAME FIO_UMAP_NAME
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 0
#elif defined(FIO_OMAP_NAME)
#define FIO_MAP_NAME FIO_OMAP_NAME
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1
#endif

/* *****************************************************************************



                  Higher Level Dependencies (i.e. Server module)



***************************************************************************** */

#if defined(FIO_HTTP)
#undef FIO_HTTP_HANDLE
#define FIO_HTTP_HANDLE
#endif

#if defined(FIO_HTTP)
#undef FIO_PUBSUB
#define FIO_PUBSUB
#endif

#if defined(FIO_HTTP) || defined(FIO_PUBSUB)
#undef FIO_SERVER
#define FIO_SERVER
#endif

#if defined(FIO_HTTP) || defined(FIO_SERVER)
#undef FIO_POLL
#define FIO_POLL
#endif

/* *****************************************************************************



                  Mid Level Dependencies (i.e., types / helpers)



***************************************************************************** */

#if defined(FIO_FIOBJ)
#define FIO_MUSTACHE
#endif

#if defined(FIO_HTTP)
#undef FIO_HTTP1_PARSER
#define FIO_HTTP1_PARSER
#endif

#if defined(FIO_HTTP)
#undef FIO_WEBSOCKET_PARSER
#define FIO_WEBSOCKET_PARSER
#endif

#if defined(FIO_POLL) || defined(FIO_SERVER) || defined(FIO_PUBSUB)
#undef FIO_SOCK
#define FIO_SOCK
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_FIOBJ) ||                          \
    defined(FIO_LEAK_COUNTER) || defined(FIO_MEMORY_NAME) || defined(FIO_POLL)
#undef FIO_STATE
#define FIO_STATE
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_STR_NAME) ||                       \
    defined(FIO_STR_SMALL) || defined(FIO_ARRAY_TYPE_STR) ||                   \
    defined(FIO_MAP_KEY_KSTR) || defined(FIO_MAP_KEY_BSTR) ||                  \
    (defined(FIO_MAP_NAME) && !defined(FIO_MAP_KEY)) || defined(FIO_MUSTACHE)
#undef FIO_STR
#define FIO_STR
#endif

#if defined(FIO_SERVER)
#undef FIO_STREAM
#define FIO_STREAM
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_SERVER) || defined(FIO_QUEUE)
#undef FIO_TIME
#define FIO_TIME
#endif

#if defined(FIO_SERVER)
#undef FIO_QUEUE
#define FIO_QUEUE
#endif

/* *****************************************************************************



              Crypto Elements Dependencies (i.e., SHA-1 etc')



***************************************************************************** */
#if defined(FIO_PUBSUB)
#define FIO_CHACHA
#endif

#if defined(FIO_HTTP_HANDLE)
#define FIO_SHA1
#endif

#if defined(FIO_PUBSUB)
#define FIO_SHA2
#endif

/* *****************************************************************************



                  Core Level Dependencies (i.e., atomics, etc')



***************************************************************************** */

#if defined(FIO_STR) || defined(FIO_HTTP) || defined(FIO_STREAM)
#undef FIO_FILES
#define FIO_FILES
#endif

#if defined(FIO_CLI) || defined(FIO_HTTP_HANDLE) ||                            \
    defined(FIO_HTTP1_PARSER) || defined(FIO_JSON) || defined(FIO_STR) ||      \
    defined(FIO_TIME) || defined(FIO_FILES)
#undef FIO_ATOL
#define FIO_ATOL
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_FIOBJ) ||                          \
    defined(FIO_LEAK_COUNTER) || defined(FIO_MEMORY_NAME) ||                   \
    defined(FIO_POLL) || defined(FIO_STATE) || defined(FIO_STR) ||             \
    defined(FIO_QUEUE)
#undef FIO_ATOMIC
#define FIO_ATOMIC
#endif

#if defined(FIO_PUBSUB)
#undef FIO_GLOB_MATCH
#define FIO_GLOB_MATCH
#endif

#if defined(FIO_CLI) || defined(FIO_MEMORY_NAME) || defined(FIO_POLL) ||       \
    defined(FIO_STATE)
#undef FIO_IMAP_CORE
#define FIO_IMAP_CORE
#endif

#if defined(FIO_CHACHA) || defined(FIO_SHA2)
#undef FIO_MATH
#define FIO_MATH
#endif

#if defined(FIO_CLI) || defined(FIO_FILES) || defined(FIO_HTTP_HANDLE) ||      \
    defined(FIO_MEMORY_NAME) || defined(FIO_POLL) || defined(FIO_STATE) ||     \
    defined(FIO_STR)
#undef FIO_RAND
#define FIO_RAND
#endif

#if defined(FIO_SERVER)
#undef FIO_SIGNAL
#define FIO_SIGNAL
#endif

#if defined(FIO_MEMORY_NAME) || defined(FIO_QUEUE)
#undef FIO_THREADS
#define FIO_THREADS
#endif

#if defined(FIO_SOCK)
#undef FIO_URL
#define FIO_URL
#endif
