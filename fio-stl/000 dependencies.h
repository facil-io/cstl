/* *****************************************************************************



                              Multi-Inclusion Macros



***************************************************************************** */

/* *****************************************************************************
Tests Inclusion (everything + MEMALT)
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) &&                                       \
    (defined(FIO_TEST_ALL) || defined(FIO___TEST_MACRO_SUSPENDED)) &&          \
    !defined(H___FIO_TESTS_INC_FINISHED___H)

/* Inclusion cycle three - facil.io memory allocator for all else. */
#if !defined(H___FIO_EVERYTHING___H) /* include everything first, then test */
#undef FIO_TEST_ALL
#define FIO___TEST_MACRO_SUSPENDED
#undef FIO_LEAK_COUNTER
#define FIO_LEAK_COUNTER 1
#define FIO_EVERYTHING
#else /* define test inclusion */
#define H___FIO_TESTS_INC_FINISHED___H
#undef FIO___TEST_MACRO_SUSPENDED
#define FIO_TEST_ALL
#endif

#endif /* FIO_TEST_ALL */

/* *****************************************************************************
Special `extern` support FIO_BASIC, FIO_EVERYTHING, etc'
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO_EXTERN) &&                \
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
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO_EVERYTHING) &&            \
    !defined(H___FIO_EVERYTHING___H)

#if !defined(H___FIO_EVERYTHING1___H)
#define H___FIO_EVERYTHING1___H
#undef FIO_FIOBJ
#undef FIO_HTTP
#undef FIO_MALLOC
#undef FIO_MUSTACHE
#undef FIO_PUBSUB
#undef FIO_IO
#undef FIOBJ_MALLOC
#define FIO_CLI
#define FIO_CORE
#define FIO_CRYPT
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_THREADS

#else
#undef H___FIO_EVERYTHING1___H
#undef FIO_EVERYTHING
#define H___FIO_EVERYTHING___H
#undef FIO_MEMALT
#define FIO_FIOBJ
#define FIO_HTTP
#define FIO_IO
#define FIO_MALLOC
#define FIO_MUSTACHE
#define FIO_PUBSUB
#define FIO_MEMALT

#endif

#define FIO___INCLUDE_AGAIN
#endif /* FIO_EVERYTHING */

/* *****************************************************************************
FIO_BASIC                   Basic Kitchen Sink Inclusion
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO_BASIC) &&                 \
    !defined(H___FIO_BASIC___H)

#if !defined(H___FIO_BASIC_ROUND1___H)
#define H___FIO_BASIC_ROUND1___H
#undef FIO_CLI
#undef FIO_CORE
#undef FIO_CRYPT
#undef FIO_FIOBJ
#undef FIO_MALLOC
#undef FIO_MUSTACHE
#undef FIO_THREADS
#undef FIOBJ_MALLOC
#define FIO_CLI
#define FIO_CORE
#define FIO_CRYPT
#define FIO_THREADS

#elif !defined(H___FIO_BASIC_ROUND2___H)
#define H___FIO_BASIC_ROUND2___H
#define FIO_FIOBJ
#define FIO_MUSTACHE
#define FIOBJ_MALLOC
#define FIO_OTP

#else
#define H___FIO_BASIC___H
#undef H___FIO_BASIC_ROUND1___H
#undef H___FIO_BASIC_ROUND2___H
#undef FIO_BASIC
#define FIO_MALLOC
#endif

#define FIO___INCLUDE_AGAIN
#endif /* FIO_BASIC */
/* *****************************************************************************
FIO_CRYPT             Poor-man's Cryptographic Elements
***************************************************************************** */
#if defined(FIO_CRYPT) || defined(FIO_CRYPTO) || defined(FIO_TLS13)
#undef FIO_CRYPT
#undef FIO_CRYPTO
#undef FIO_AES
#undef FIO_ASN1
#undef FIO_CHACHA
#undef FIO_CRYPTO_CORE
#undef FIO_ED25519
#undef FIO_OTP
#undef FIO_P256
#undef FIO_RSA
#undef FIO_SECRET
#undef FIO_SHA1
#undef FIO_SHA2
#undef FIO_TLS13
#undef FIO_X509
#define FIO_CRYPTO_CORE
#define FIO_SHA1
#define FIO_SHA2
#define FIO_SHA3
#define FIO_BLAKE2
#define FIO_CHACHA
#define FIO_HKDF
#define FIO_AES
#define FIO_ED25519
#define FIO_P256
#define FIO_ASN1
#define FIO_RSA
#define FIO_X509
#define FIO_OTP
#define FIO_SECRET
#define FIO_TLS13
#endif /* FIO_CRYPT || defined(FIO_CRYPTO) */

/* *****************************************************************************
FIO_CORE                        Core Inclusion
***************************************************************************** */
#if defined(FIO_CORE)
#undef FIO_ATOL
#undef FIO_FILES
#undef FIO_GLOB_MATCH
#undef FIO_LOG
#undef FIO_MATH
#undef FIO_RAND
#undef FIO_STATE
#undef FIO_TIME
#undef FIO_URL
#undef FIO_CORE
#define FIO_ATOL
#define FIO_FILES
#define FIO_GLOB_MATCH
#define FIO_LOG
#define FIO_MATH
#define FIO_RAND
#define FIO_STATE
#define FIO_TIME
#define FIO_URL
#endif

/* *****************************************************************************



                                  Shortcut Macros



***************************************************************************** */

/* *****************************************************************************
Memory Allocation - FIO_MALLOC as a "global" default memory allocator
***************************************************************************** */
/* FIO_MALLOC defines a "global" default memory allocator */
#if defined(FIO_MALLOC) && !defined(H___FIO_MALLOC___H)
#define H___FIO_MALLOC___H
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

#ifndef FIO_MEMORY_INITIALIZE_ALLOCATIONS
/* should memory be initialized to zero? */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1
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
#ifndef FIO_MEMORY_CACHE_SLOTS
/* cache up to 64Mb */
#define FIO_MEMORY_CACHE_SLOTS 16
#endif
#endif /* FIOBJ_MALLOC / FIO_MALLOC*/

#undef FIOBJ_MALLOC
#undef FIO_MALLOC
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

#if defined(FIO_HTTP_HANDLE)
#undef FIO_JSON
#define FIO_JSON
#undef FIO_MULTIPART
#define FIO_MULTIPART
#undef FIO_URL_ENCODED
#define FIO_URL_ENCODED
#endif

#if (defined(DEBUG) && defined(FIO_HTTP_HANDLE))
#undef FIO_IO
#define FIO_IO
#endif

#if defined(FIO_HTTP) || defined(FIO_REDIS)
#undef FIO_PUBSUB
#define FIO_PUBSUB
#endif

#if defined(FIO_HTTP) || defined(FIO_PUBSUB)
#undef FIO_IO
#define FIO_IO
#endif

#if defined(FIO_HTTP) || defined(FIO_IO)
#undef FIO_POLL
#define FIO_POLL
#endif

/* *****************************************************************************



                  Mid Level Dependencies (i.e., types / helpers)



***************************************************************************** */

#if defined(FIO_REDIS)
#define FIO_RESP3
#endif

#if defined(FIO_FIOBJ)
#define FIO_MUSTACHE
#define FIO_JSON
#endif

#if defined(FIO_HTTP)
#undef FIO_HTTP1_PARSER
#define FIO_HTTP1_PARSER
#endif

#if defined(FIO_HTTP)
#undef FIO_WEBSOCKET_PARSER
#define FIO_WEBSOCKET_PARSER
#endif

#if defined(FIO_POLL) || defined(FIO_IO) || defined(FIO_PUBSUB)
#undef FIO_SOCK
#define FIO_SOCK
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_QUEUE) || defined(FIO_FIOBJ) ||    \
    defined(FIO_LEAK_COUNTER) || defined(FIO_MEMORY_NAME) || defined(FIO_POLL)
#undef FIO_STATE
#define FIO_STATE
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_STR_NAME) ||                       \
    defined(FIO_STR_SMALL) || defined(FIO_ARRAY_TYPE_STR) ||                   \
    defined(FIO_MAP_KEY_KSTR) || defined(FIO_MAP_KEY_BSTR) ||                  \
    (defined(FIO_MAP_NAME) && !defined(FIO_MAP_KEY)) ||                        \
    defined(FIO_MUSTACHE) || defined(FIO_MAP2_NAME) || defined(FIO_OTP)
#undef FIO_STR
#define FIO_STR
#endif

#if defined(FIO_IO)
#undef FIO_STREAM
#define FIO_STREAM
#endif

#if defined(FIO_IO)
#undef FIO_QUEUE
#define FIO_QUEUE
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_QUEUE) || defined(FIO_OTP) ||      \
    defined(FIO_X509)
#undef FIO_TIME
#define FIO_TIME
#endif

/* *****************************************************************************



              Crypto Elements Dependencies (i.e., SHA-1 etc')



***************************************************************************** */
#if defined(FIO_PUBSUB)
#define FIO_CHACHA
#define FIO_SECRET
#endif

#if defined(FIO_HTTP_HANDLE) || defined(FIO_OTP)
#define FIO_SHA1
#endif

#if defined(FIO_PUBSUB) || defined(FIO_SECRET)
#define FIO_SHA2
#endif

#if defined(FIO_CHACHA) || defined(FIO_SHA1) || defined(FIO_SHA2)
#undef FIO_CRYPTO_CORE
#define FIO_CRYPTO_CORE
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
    defined(FIO_TIME) || defined(FIO_FILES) || defined(FIO_SECRET)
#undef FIO_ATOL
#define FIO_ATOL
#endif

#if defined(FIO_PUBSUB)
#undef FIO_GLOB_MATCH
#define FIO_GLOB_MATCH
#endif

#if defined(FIO_CLI) || defined(FIO_MEMORY_NAME) || defined(FIO_POLL) ||       \
    defined(FIO_STATE) || defined(FIO_HTTP_HANDLE)
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

#if defined(FIO_IO)
#undef FIO_SIGNAL
#define FIO_SIGNAL
#endif

#if defined(FIO_MEMORY_NAME) || defined(FIO_QUEUE) ||                          \
    (defined(DEBUG) && defined(FIO_STATE)) || defined(FIO_HTTP_HANDLE)
#undef FIO_THREADS
#define FIO_THREADS
#endif

#if defined(FIO_SOCK)
#undef FIO_URL
#define FIO_URL
#endif
