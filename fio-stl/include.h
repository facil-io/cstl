/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H)
/* *****************************************************************************
                    Including requested facil.io C STL modules
***************************************************************************** */
#ifndef FIO___INCLUDE_FILE
#define FIO___INCLUDE_FILE "./include.h"
#endif
#ifndef FIO___CSTL_NON_COMBINED_INCLUSION
#define FIO___CSTL_NON_COMBINED_INCLUSION
#endif
#include "001 header.h"

#ifdef FIO_LOG
#include "002 logging.h"
#endif
#ifdef FIO_ATOMIC
#include "003 atomics.h"
#endif
#if defined(FIO_BITWISE) || defined(FIO_BITMAP)
#include "004 bitwise.h"
#endif
#ifdef FIO_MATH
#include "005 math.h"
#endif
#ifdef FIO_ATOL
#include "006 atol.h"
#endif
#ifdef FIO_THREADS
#include "007 threads.h"
#endif
#ifdef FIO_RAND
#include "010 random.h"
#endif
#ifdef FIO_SHA1
#include "011 sha1.h"
#endif
#ifdef FIO_SHA2
#include "011 sha2.h"
#endif
#ifdef FIO_CHACHA
#include "012 chacha20poly1305.h"
#endif
#ifdef FIO_ED25519
#include "013 ed25519.h"
#endif
#ifdef FIO_IMAP_CORE
#include "020 imap.h"
#endif
#if defined(FIO_URL) || defined(FIO_URI)
#include "050 url.h"
#endif
#ifdef FIO_JSON
#include "051 json.h"
#endif
#ifdef FIO_STATE
#include "090 state callbacks.h"
#endif

#include "100 mem.h" /* later files rely on macros from here. */

#ifdef FIO_TIME
#include "101 time.h"
#endif
#ifdef FIO_QUEUE
#include "102 queue.h"
#endif
#ifdef FIO_SOCK
#include "104 sock.h"
#endif
#ifdef FIO_STREAM
#include "105 stream.h"
#endif
#ifdef FIO_SIGNAL
#include "106 signals.h"
#endif
#ifdef FIO_GLOB_MATCH
#include "107 glob matching.h"
#endif
#ifdef FIO_FILES
#include "108 files.h"
#endif
#ifdef FIO_STR
#include "199 string core.h"
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
#ifdef FIO_LIST_NAME
#include "220 linked lists.h"
#endif

#include "299 reference counter.h" /* pointer tagging cleanup is here */

#ifdef FIO_SORT_NAME
#include "301 sort.h"
#endif
#if defined(FIO_CLI) && !defined(FIO_STL_KEEP__)
#include "302 cli.h"
#endif
#if defined(FIO_POLL) && !defined(FIO_STL_KEEP__)
#include "330 poll api.h"
#include "331 poll epoll.h"
#include "331 poll kqueue.h"
#include "331 poll poll.h"
#endif
#if defined(FIO_SERVER) && !defined(FIO_STL_KEEP__)
#include "400 server.h"
#if defined(FIO_TEST_CSTL)
#include "409 server test.h"
#endif
#endif
#if defined(FIO_PUBSUB) && !defined(FIO_STL_KEEP__)
#include "420 pubsub.h"
#endif
#if defined(FIO_FIOBJ) && !defined(FIO_STL_KEEP__)
#include "500 fiobj.h"
#endif

#ifndef FIO___DEV___
#include "700 cleanup.h"
#endif

#if defined(FIO_TEST_CSTL) && !defined(FIO_FIO_TEST_CSTL_ONLY_ONCE) &&         \
    !defined(FIO_STL_KEEP__)
#include "998 tests.h"
#endif

#include "999 footer.h"

#ifndef FIO_STL_KEEP__
#undef FIO___CSTL_NON_COMBINED_INCLUSION
#endif

#endif /* !H___FIO_CSTL_COMBINED___H */
/* *************************************************************************
 */
