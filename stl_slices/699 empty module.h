/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_MODULE_NAME module      /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                  A Template for New Types / Modules




***************************************************************************** */
#ifdef FIO_MODULE_NAME

/* *****************************************************************************
Module Settings

At this point, define any MACROs and customaizable settings avsailable to the
developer.
***************************************************************************** */

/* *****************************************************************************
Pointer Tagging Support
***************************************************************************** */

#ifdef FIO_PTR_TAG_TYPE
#define FIO_MODULE_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, s) *
#endif

/* *****************************************************************************
Module API
***************************************************************************** */

typedef struct {
  /* module's type(s) if any */
  void *data;
} FIO_NAME(FIO_MODULE_NAME, s);

/* at this point publish (declare only) the public API */

#ifndef FIO_MODULE_INIT
/* Initialization macro. */
#define FIO_MODULE_INIT                                                        \
  { 0 }
#endif

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, new)(void);

/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_MODULE_NAME, free)(FIO_MODULE_PTR obj);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, reinitializing its container. */
SFUNC void FIO_NAME(FIO_MODULE_NAME, destroy)(FIO_MODULE_PTR obj);

/* *****************************************************************************
Module Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size) fio_free((ptr))

*/

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, new)(void) {
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      (FIO_NAME(FIO_MODULE_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*o), 0);
  if (!o)
    return (FIO_MODULE_PTR)NULL;
  *o = (FIO_NAME(FIO_MODULE_NAME, s))FIO_MODULE_INIT;
  return (FIO_MODULE_PTR)FIO_PTR_TAG(o);
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_MODULE_NAME, free)(FIO_MODULE_PTR obj) {
  FIO_PTR_TAG_VALID_OR_RETURN(obj, 0);
  FIO_NAME(FIO_MODULE_NAME, destroy)(obj);
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      (FIO_NAME(FIO_MODULE_NAME, s) *)FIO_PTR_UNTAG(obj);
  FIO_MEM_FREE_(o, sizeof(*o));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size) fio_free((ptr))

*/

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_MODULE_NAME, destroy)(FIO_MODULE_PTR obj) {
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      (FIO_NAME(FIO_MODULE_NAME, s) *)FIO_PTR_UNTAG(obj);
  if (!o)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(obj);
  /* add destruction logic */

  *o = (FIO_NAME(FIO_MODULE_NAME, s))FIO_MODULE_INIT;
  return;
}

/* *****************************************************************************
Module Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {
  /*
   * test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MODULE_PTR
#endif /* FIO_MODULE_NAME */
#undef FIO_MODULE_NAME
