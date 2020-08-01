/* *****************************************************************************






                          Internal Dependencies






***************************************************************************** */

/* FIO_LOCK2 dependencies */
#ifdef FIO_LOCK2
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_LOCK2 */

/* FIO_MALLOC dependencies */
#ifdef FIO_MALLOC
#ifndef FIO_LOG
#define FIO_LOG
#endif
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_MALLOC */

/* FIO_BITMAP dependencies */
#ifdef FIO_BITMAP
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_BITMAP */

/* FIO_REF_NAME dependencies */
#ifdef FIO_REF_NAME
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_REF_NAME */

/* FIO_RAND dependencies */
#ifdef FIO_RAND
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#ifndef FIO_RISKY_HASH
#define FIO_RISKY_HASH
#endif
#ifndef FIO_TIME
#define FIO_TIME
#endif
#endif /* FIO_RAND */

/* FIO_STR_NAME / FIO_STR_SMALL dependencies */
#if defined(FIO_STR_NAME) || defined(FIO_STR_SMALL)
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#ifndef FIO_RISKY_HASH
#define FIO_RISKY_HASH
#endif
#endif /* FIO_STR_NAME */

/* FIO_QUEUE dependencies */
#ifdef FIO_QUEUE
#ifndef FIO_TIME
#define FIO_TIME
#endif
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_QUEUE */

/* FIO_TIME dependencies */
#ifdef FIO_TIME
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#endif /* FIO_TIME */

/* FIO_CLI dependencies */
#ifdef FIO_CLI
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#ifndef FIO_RISKY_HASH
#define FIO_RISKY_HASH
#endif
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#endif /* FIO_CLI */

/* FIO_JSON dependencies */
#ifdef FIO_JSON
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#ifndef FIO_BITMAP
#define FIO_BITMAP
#endif
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_JSON */

/* FIO_RISKY_HASH dependencies */
#ifdef FIO_RISKY_HASH
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#endif /* FIO_RISKY_HASH */
