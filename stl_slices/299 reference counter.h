/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_REF_NAME long_ref       /* Development inclusion - ignore line */
#define FIO_REF_TYPE long           /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                      Reference Counting / Wrapper
                   (must be placed after all type macros)









***************************************************************************** */

#ifdef FIO_REF_NAME

#ifndef fio_atomic_add
#error FIO_REF_NAME requires enabling the FIO_ATOMIC extension.
#endif

#ifndef FIO_REF_TYPE
#define FIO_REF_TYPE FIO_NAME(FIO_REF_NAME, s)
#endif

#ifndef FIO_REF_INIT
#define FIO_REF_INIT(obj)                                                      \
  do {                                                                         \
    (obj) = (FIO_REF_TYPE){0};                                                 \
  } while (0)
#endif

#ifndef FIO_REF_DESTROY
#define FIO_REF_DESTROY(obj)
#endif

#ifndef FIO_REF_METADATA_INIT
#ifdef FIO_REF_METADATA
#define FIO_REF_METADATA_INIT(meta)                                            \
  do {                                                                         \
    (meta) = (FIO_REF_METADATA){0};                                            \
  } while (0)
#else
#define FIO_REF_METADATA_INIT(meta)
#endif
#endif

#ifndef FIO_REF_METADATA_DESTROY
#define FIO_REF_METADATA_DESTROY(meta)
#endif

/**
 * FIO_REF_CONSTRUCTOR_ONLY allows the reference counter constructor (TYPE_new)
 * to be the only constructor function.
 *
 * When set, the reference counting functions will use `X_new` and `X_free`.
 * Otherwise (assuming `X_new` and `X_free` are already defined), the reference
 * counter will define `X_new2` and `X_free2` instead.
 */
#ifdef FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_CONSTRUCTOR new
#define FIO_REF_DESTRUCTOR  free
#define FIO_REF_DUPNAME     dup
#else
#define FIO_REF_CONSTRUCTOR new2
#define FIO_REF_DESTRUCTOR  free2
#define FIO_REF_DUPNAME     dup2
#endif

typedef struct {
  volatile uint32_t ref;
#ifdef FIO_REF_METADATA
  FIO_REF_METADATA metadata;
#endif
  FIO_REF_TYPE wrapped;
} FIO_NAME(FIO_REF_NAME, _wrapper_s);

#ifdef FIO_PTR_TAG_TYPE
#define FIO_REF_TYPE_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_REF_TYPE_PTR FIO_REF_TYPE *
#endif

/* *****************************************************************************
Reference Counter (Wrapper) API
***************************************************************************** */

/** Allocates a reference counted object. */
#ifdef FIO_REF_FLEX_TYPE
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_CONSTRUCTOR)(size_t members);
#else
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME, FIO_REF_CONSTRUCTOR)(void);
#endif /* FIO_REF_FLEX_TYPE */
/** Increases the reference count. */
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_DUPNAME)(FIO_REF_TYPE_PTR wrapped);

/**
 * Frees a reference counted object (or decreases the reference count).
 *
 * Returns 1 if the object was actually freed, returns 0 otherwise.
 */
IFUNC int FIO_NAME(FIO_REF_NAME, FIO_REF_DESTRUCTOR)(FIO_REF_TYPE_PTR wrapped);

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped);
#endif

/* *****************************************************************************
Reference Counter (Wrapper) Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/** Allocates a reference counted object. */
#ifdef FIO_REF_FLEX_TYPE
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_CONSTRUCTOR)(size_t members) {
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      (FIO_NAME(FIO_REF_NAME, _wrapper_s) *)FIO_MEM_REALLOC_(
          NULL,
          0,
          sizeof(*o) + (sizeof(FIO_REF_FLEX_TYPE) * members),
          0);
#else
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME, FIO_REF_CONSTRUCTOR)(void) {
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      (FIO_NAME(FIO_REF_NAME,
                _wrapper_s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*o), 0);
#endif /* FIO_REF_FLEX_TYPE */
  if (!o)
    return (FIO_REF_TYPE_PTR)(FIO_PTR_TAG((FIO_REF_TYPE *)o));
  o->ref = 1;
  FIO_REF_METADATA_INIT((o->metadata));
  FIO_REF_INIT(o->wrapped);
  FIO_REF_TYPE *ret = &o->wrapped;
  return (FIO_REF_TYPE_PTR)(FIO_PTR_TAG(ret));
}

/** Increases the reference count. */
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_DUPNAME)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      FIO_PTR_FROM_FIELD(FIO_NAME(FIO_REF_NAME, _wrapper_s), wrapped, wrapped);
  fio_atomic_add(&o->ref, 1);
  return wrapped_;
}

/** Frees a reference counted object (or decreases the reference count). */
IFUNC int FIO_NAME(FIO_REF_NAME,
                   FIO_REF_DESTRUCTOR)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  if (!wrapped || !wrapped_)
    return -1;
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      FIO_PTR_FROM_FIELD(FIO_NAME(FIO_REF_NAME, _wrapper_s), wrapped, wrapped);
  if (!o)
    return -1;
  if (fio_atomic_sub_fetch(&o->ref, 1))
    return 0;
  FIO_REF_DESTROY(o->wrapped);
  FIO_REF_METADATA_DESTROY((o->metadata));
  FIO_MEM_FREE_(o, sizeof(*o));
  return 1;
}

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      FIO_PTR_FROM_FIELD(FIO_NAME(FIO_REF_NAME, _wrapper_s), wrapped, wrapped);
  return &o->metadata;
}
#endif

/* *****************************************************************************
Reference Counter (Wrapper) Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_REF_NAME
#undef FIO_REF_FLEX_TYPE
#undef FIO_REF_TYPE
#undef FIO_REF_INIT
#undef FIO_REF_DESTROY
#undef FIO_REF_METADATA
#undef FIO_REF_METADATA_INIT
#undef FIO_REF_METADATA_DESTROY
#undef FIO_REF_TYPE_PTR
#undef FIO_REF_CONSTRUCTOR_ONLY
#undef FIO_REF_CONSTRUCTOR
#undef FIO_REF_DUPNAME
#undef FIO_REF_DESTRUCTOR
#endif
