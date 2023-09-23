/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MODULE_NAME module /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  A Template for New Types / Modules




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MODULE_NAME) /* && !defined(FIO___RECURSIVE_INCLUDE) */

/* *****************************************************************************
Module Settings

At this point, define any MACROs and customizable settings available to the
developer.
***************************************************************************** */

/* *****************************************************************************
Pointer Tagging Support: !!! valid only for dynamic types, filename 2xx XXX.h
***************************************************************************** */

#ifdef FIO_PTR_TAG_TYPE
#define FIO_MODULE_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, s) *
#endif

#define FIO___UNTAG_T FIO_NAME(FIO_MODULE_NAME, s)

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
SFUNC FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, new)(void);

/* Frees any internal data AND the object's container! */
SFUNC int FIO_NAME(FIO_MODULE_NAME, free)(FIO_MODULE_PTR obj);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, reinitializing its container. */
SFUNC void FIO_NAME(FIO_MODULE_NAME, destroy)(FIO_MODULE_PTR obj);

/* *****************************************************************************
Module Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All short term / type memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

All long-term / system memory allocations should use:
* FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE(ptr, size)

Module and File Names:
======================

00# XXX.h - the module is a core module, independent or doesn't define a type
1## XXX.h - the module doesn't define a type, but requires memory allocations
2## XXX.h - the module defines a type
3## XXX.h - hashes / crypto.
4## XXX.h - server related modules
5## XXX.h - FIOBJ related modules
9## XXX.h - testing (usually use 902 XXX.h unless tests depend on other tests)

When
*/

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(FIO_MODULE_NAME)

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
SFUNC FIO_MODULE_PTR FIO_NAME(FIO_MODULE_NAME, new)(void) {
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      (FIO_NAME(FIO_MODULE_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*o), 0);
  if (!o)
    return (FIO_MODULE_PTR)NULL;
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_MODULE_NAME);
  *o = (FIO_NAME(FIO_MODULE_NAME, s))FIO_MODULE_INIT;
  return (FIO_MODULE_PTR)FIO_PTR_TAG(o);
}
/* Frees any internal data AND the object's container! */
SFUNC int FIO_NAME(FIO_MODULE_NAME, free)(FIO_MODULE_PTR obj) {
  FIO_PTR_TAG_VALID_OR_RETURN(obj, 0);
  FIO_NAME(FIO_MODULE_NAME, destroy)(obj);
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      FIO_PTR_TAG_GET_UNTAGGED(FIO___UNTAG_T, obj);
  FIO___LEAK_COUNTER_ON_FREE(FIO_MODULE_NAME);
  FIO_MEM_FREE_(o, sizeof(*o));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_MODULE_NAME, destroy)(FIO_MODULE_PTR obj) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(obj);
  FIO_NAME(FIO_MODULE_NAME, s) *o =
      FIO_PTR_TAG_GET_UNTAGGED(FIO___UNTAG_T, obj);
  /* TODO: add destruction logic */

  *o = (FIO_NAME(FIO_MODULE_NAME, s))FIO_MODULE_INIT;
  return;
}

/* *****************************************************************************
Module Testing - Please place testing in a dedicated testing file if possible.
***************************************************************************** */
#if 0
#ifdef FIO_TEST_ALL

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_ALL */
#endif /* 0 */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MODULE_PTR
#undef FIO_MODULE_NAME
#undef FIO___UNTAG_T
#endif /* FIO_MODULE_NAME */
