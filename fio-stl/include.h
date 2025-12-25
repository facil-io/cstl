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
#ifdef FIO_RESP3
#include "004 resp3.h"
#endif
#ifdef FIO_URL_ENCODED
#include "004 urlencoded.h"
#endif
#ifdef FIO_MULTIPART
#include "004 multipart.h"
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

#ifdef FIO_CRYPTO_CORE
#include "150 crypto core.h"
#endif

#ifdef FIO_SHA1
#include "152 sha1.h"
#endif
#ifdef FIO_SHA2
#include "152 sha2.h"
#endif
#ifdef FIO_HKDF
#include "152 sha2z hkdf.h"
#endif
#ifdef FIO_BLAKE2
#include "152 blake2.h"
#endif
#ifdef FIO_SHA3
#include "152 sha3.h"
#endif
#ifdef FIO_CHACHA
#include "152 chacha20poly1305.h"
#endif
#ifdef FIO_AES
#include "153 aes.h"
#endif

#ifdef FIO_ED25519
#include "154 ed25519.h"
#endif

#ifdef FIO_P256
#include "154 p256.h"
#endif

#ifdef FIO_ASN1
#include "155 asn1.h"
#endif

#ifdef FIO_RSA
#include "155 rsa.h"
#endif

#ifdef FIO_X509
#include "155 x509.h"
#endif

#ifdef FIO_OTP
#include "160 otp.h"
#endif
#ifdef FIO_SECRET
#include "160 secret.h"
#endif

#ifdef FIO_TLS13
#include "190 tls13.h"
#endif

#ifdef FIO_PEM
#include "301 pem.h"
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

#if defined(FIO_MAP2_NAME)
#include "210 map2.h"
#endif

#include "249 reference counter.h" /* required: pointer tagging cleanup is here */

#if defined(FIO_FIOBJ) && !defined(FIO___RECURSIVE_INCLUDE)
#include "250 fiobj.h"
#endif

#if defined(FIO_IO) && !defined(FIO___RECURSIVE_INCLUDE)
#include "400 io api.h"
#include "401 io types.h"
#include "402 io reactor.h"
#if defined(HAVE_OPENSSL)
#include "411 openssl.h"
#endif
#include "412 tls13.h"
#endif /* FIO_IO */

#if defined(FIO_PUBSUB) && !defined(FIO___RECURSIVE_INCLUDE)
#include "420 pubsub.h"
#endif

#if defined(FIO_REDIS) && !defined(FIO___RECURSIVE_INCLUDE)
#include "422 redis.h"
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

#ifndef FIO___DEV___
#include "700 cleanup.h"
#endif

#if 0 && defined(FIO_TEST_ALL) && !defined(H___FIO_TESTS_START___H)
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
#include "902 io.h"
#include "902 math.h"
#include "902 memalt.h"
#include "902 mustache.h"
#include "902 poll.h"
#include "902 pubsub.h"
#include "902 queue.h"
#include "902 random.h"
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
