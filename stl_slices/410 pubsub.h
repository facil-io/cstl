/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_PUBSUB                  /* Development inclusion - ignore line */
#include "400 server.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                Pub/Sub Services for IPC / Server applications




***************************************************************************** */
#if defined(FIO_PUBSUB) && !defined(H___FIO_PUBSUB___H) &&                     \
    !defined(FIO_STL_KEEP__)
#define H___FIO_PUBSUB___H
/* *****************************************************************************
Module Settings

At this point, define any MACROs and customizable settings available to the
developer.
***************************************************************************** */

/* *****************************************************************************
Module API
***************************************************************************** */

/* *****************************************************************************
Module Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
Pub/Sub Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, pubsub)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Pub/Sub Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_PUBSUB
#endif /* FIO_PUBSUB */
