/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H)
/* *****************************************************************************
            Including facil.io modules for multi-file header option
***************************************************************************** */
#ifndef FIO_INCLUDE_FILE
#define FIO_INCLUDE_FILE "fio-stl/include.h"
#include "000 core.h"
#include "001 patches.h"
#endif

#include "000 dependencies.h"

#include "001 header.h"
#ifdef FIO_LOG
#include "001 logging.h"
#endif
#ifdef FIO_MEMALT
#include "001 memalt.h"
#endif

#ifdef FIO_ATOMIC
#include "002 atomics.h"
#endif
#ifdef FIO_ATOL
#include "002 atol.h"
#endif
#ifdef FIO_GLOB_MATCH
#include "002 glob matching.h"
#endif
#ifdef FIO_IMAP_CORE
#include "002 imap.h"
#endif
#ifdef FIO_MATH
#include "002 math.h"
#endif
#ifdef FIO_RAND
#include "002 random.h"
#endif
#ifdef FIO_SIGNAL
#include "002 signals.h"
#endif
#ifdef FIO_SORT_NAME
#include "002 sort.h"
#endif
#ifdef FIO_THREADS
#include "002 threads.h"
#endif
#if defined(FIO_URL) || defined(FIO_URI)
#include "002 url.h"
#endif

#ifdef FIO_FILES
#include "004 files.h"
#endif
#ifdef FIO_JSON
#include "004 json.h"
#endif
#ifdef FIO_SOCK
#include "004 sock.h"
#endif
#if defined(FIO_STATE) && !defined(FIO___RECURSIVE_INCLUDE)
#include "004 state callbacks.h"
#endif
#ifdef FIO_TIME
#include "004 time.h"
#endif

#if defined(FIO_CLI) && !defined(FIO___RECURSIVE_INCLUDE)
#include "005 cli.h"
#endif

#if defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) || defined(FIOBJ_MALLOC)
#include "010 mem.h"
#endif

#if defined(FIO_POLL) && !defined(FIO___RECURSIVE_INCLUDE)
#include "102 poll api.h"
#include "102 poll epoll.h"
#include "102 poll kqueue.h"
#include "102 poll poll.h"
#endif
#ifdef FIO_STR
#include "102 string core.h"
#endif
#ifdef FIO_STREAM
#include "102 stream.h"
#endif
#ifdef FIO_QUEUE
#include "102 queue.h"
#endif

#ifdef FIO_MUSTACHE
#include "104 mustache.h"
#endif

#if defined(FIO_STR_SMALL) || defined(FIO_STR_NAME)
#include "200 string.h"
#endif
#ifdef FIO_ARRAY_NAME
#include "201 array.h"
#endif
#if defined(FIO_UMAP_NAME) || defined(FIO_OMAP_NAME) || defined(FIO_MAP_NAME)
#include "210 map.h"
#endif

#include "299 reference counter.h" /* required: pointer tagging cleanup is here */

#ifdef FIO_SHA1
#include "302 sha1.h"
#endif
#ifdef FIO_SHA2
#include "302 sha2.h"
#endif
#ifdef FIO_CHACHA
#include "302 chacha20poly1305.h"
#endif

#ifdef FIO_ED25519
#include "304 ed25519.h"
#endif

#if defined(FIO_SERVER) && !defined(FIO___RECURSIVE_INCLUDE)
#include "400 server.h"
#if defined(HAVE_OPENSSL)
#include "402 openssl.h"
#endif
#endif /* FIO_SERVER */

#if defined(FIO_PUBSUB) && !defined(FIO___RECURSIVE_INCLUDE)
#include "420 pubsub.h"
#endif

#ifdef FIO_HTTP1_PARSER
#include "431 http1 parser.h"
#endif
#ifdef FIO_WEBSOCKET_PARSER
#include "431 websocket parser.h"
#endif

#if defined(FIO_HTTP_HANDLE) && !defined(FIO___RECURSIVE_INCLUDE)
#include "431 http handle.h"
#endif

#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE)
#include "439 http.h"
#endif

#if defined(FIO_FIOBJ) && !defined(FIO___RECURSIVE_INCLUDE)
#include "500 fiobj.h"
#endif

#ifndef FIO___DEV___
#include "700 cleanup.h"
#endif

#if defined(FIO_TEST_ALL) && !defined(H___FIO_TESTS_START___H)
#include "900 tests start.h"
#include "902 atol.h"
#include "902 atomics.h"
#include "902 cli.h"
#include "902 core.h"
#include "902 files.h"
#include "902 fiobj.h"
#include "902 glob matching.h"
#include "902 http handle.h"
#include "902 imap.h"
#include "902 math.h"
#include "902 memalt.h"
#include "902 mustache.h"
#include "902 poll.h"
#include "902 pubsub.h"
#include "902 queue.h"
#include "902 random.h"
#include "902 server.h"
#include "902 sock.h"
#include "902 sort.h"
#include "902 state callbacks.h"
#include "902 stream.h"
#include "902 string core.h"
#include "902 time.h"
#include "902 url.h"
#include "903 chacha.h"
#include "903 sha.h"
#include "998 tests finish.h"
#endif

#endif /* !H___FIO_CSTL_COMBINED___H */
/* ************************************************************************* */
