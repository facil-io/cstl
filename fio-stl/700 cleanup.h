/* *****************************************************************************




                            Common Cleanup




***************************************************************************** */

/* *****************************************************************************
Common cleanup
***************************************************************************** */
#ifndef FIO___RECURSIVE_INCLUDE

/* undefine FIO_EXTERN only if its value indicates it is temporary. */
#if !defined(FIO_EXTERN) || (FIO_EXTERN + 1) < 3
#undef FIO_EXTERN
#endif
#if !defined(FIO_EXTERN_COMPLETE) || (FIO_EXTERN_COMPLETE + 1) < 3
#undef FIO_EXTERN_COMPLETE
#endif

#undef SFUNC
#undef IFUNC
#undef SFUNC_
#undef IFUNC_

#undef FIO_MALLOC_TMP_USE_SYSTEM
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_
#undef FIO_MEMORY_NAME /* postponed due to possible use in macros */

#undef FIO___LOCK_TYPE
#undef FIO___LOCK_INIT
#undef FIO___LOCK_LOCK
#undef FIO___LOCK_LOCK_TRY
#undef FIO___LOCK_UNLOCK
#undef FIO_USE_THREAD_MUTEX_TMP

#else

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

#endif /* !FIO___RECURSIVE_INCLUDE */

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif

/* *****************************************************************************
Recursive inclusion / cleanup
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO___INCLUDE_AGAIN)
/* recursive include statement */
#undef FIO___INCLUDE_AGAIN
#include FIO_INCLUDE_FILE
#else
#ifdef FIO_EVERYTHING___REMOVE_EXTERN
#undef FIO_EXTERN
#undef FIO_EVERYTHING___REMOVE_EXTERN
#endif
#ifdef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#undef FIO_EXTERN_COMPLETE
#undef FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE
#endif

#endif /* !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO___INCLUDE_AGAIN) */
