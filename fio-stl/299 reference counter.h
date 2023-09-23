/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_REF_NAME long_ref  /* Development inclusion - ignore line */
#define FIO_REF_TYPE long      /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      Reference Counting / Wrapper
                   (must be placed after all type macros)


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#ifdef FIO_REF_NAME

#ifndef FIO_REF_TYPE
#define FIO_REF_TYPE FIO_NAME(FIO_REF_NAME, s)
#endif

#ifndef FIO_REF_INIT
#define FIO_REF_INIT(obj)                                                      \
  do {                                                                         \
    if (!FIO_MEM_REALLOC_IS_SAFE_)                                             \
      (obj) = (FIO_REF_TYPE){0};                                               \
  } while (0)
#endif

#ifndef FIO_REF_DESTROY
#define FIO_REF_DESTROY(obj)
#endif

#ifndef FIO_REF_METADATA_INIT
#ifdef FIO_REF_METADATA
#define FIO_REF_METADATA_INIT(meta)                                            \
  do {                                                                         \
    if (!FIO_MEM_REALLOC_IS_SAFE_)                                             \
      (meta) = (FIO_REF_METADATA){0};                                          \
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
  volatile size_t ref;
#ifdef FIO_REF_METADATA
  FIO_REF_METADATA metadata;
#endif
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
FIO_IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                    FIO_REF_DUPNAME)(FIO_REF_TYPE_PTR wrapped);

/** Frees a reference counted object (or decreases the reference count). */
IFUNC void FIO_NAME(FIO_REF_NAME, FIO_REF_DESTRUCTOR)(FIO_REF_TYPE_PTR wrapped);

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped);
#endif

/* *****************************************************************************
Inline Implementation
***************************************************************************** */
/** Increases the reference count. */
FIO_IFUNC FIO_REF_TYPE_PTR
FIO_NAME(FIO_REF_NAME, FIO_REF_DUPNAME)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      ((FIO_NAME(FIO_REF_NAME, _wrapper_s) *)wrapped) - 1;
  if (!o)
    return wrapped_;
  fio_atomic_add(&o->ref, 1);
  return wrapped_;
}

/** Debugging helper, do not use for data, as returned value is unstable. */
FIO_IFUNC size_t FIO_NAME(FIO_REF_NAME, references)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      ((FIO_NAME(FIO_REF_NAME, _wrapper_s) *)wrapped) - 1;
  if (!o)
    return 0;
  return o->ref;
}

/* *****************************************************************************
Reference Counter (Wrapper) Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(FIO_REF_NAME)

/** Allocates a reference counted object. */
#ifdef FIO_REF_FLEX_TYPE
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_CONSTRUCTOR)(size_t members) {
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      (FIO_NAME(FIO_REF_NAME, _wrapper_s) *)FIO_MEM_REALLOC_(
          NULL,
          0,
          sizeof(*o) + sizeof(FIO_REF_TYPE) +
              (sizeof(FIO_REF_FLEX_TYPE) * members),
          0);
#else
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME, FIO_REF_CONSTRUCTOR)(void) {
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o = (FIO_NAME(FIO_REF_NAME, _wrapper_s) *)
      FIO_MEM_REALLOC_(NULL, 0, sizeof(*o) + sizeof(FIO_REF_TYPE), 0);
#endif /* FIO_REF_FLEX_TYPE */
  if (!o)
    return (FIO_REF_TYPE_PTR)(o);
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_REF_NAME);
  o->ref = 1;
  FIO_REF_METADATA_INIT((o->metadata));
  FIO_REF_TYPE *ret = (FIO_REF_TYPE *)(o + 1);
  FIO_REF_INIT((ret[0]));
  return (FIO_REF_TYPE_PTR)(FIO_PTR_TAG(ret));
  (void)FIO_NAME(FIO_REF_NAME, references);
}

/** Frees a reference counted object (or decreases the reference count). */
IFUNC void FIO_NAME(FIO_REF_NAME,
                    FIO_REF_DESTRUCTOR)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  if (!wrapped || !wrapped_)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(wrapped_);
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      ((FIO_NAME(FIO_REF_NAME, _wrapper_s) *)wrapped) - 1;
  if (!o)
    return;
  if (fio_atomic_sub_fetch(&o->ref, 1))
    return;
  FIO_REF_DESTROY((wrapped[0]));
  FIO_REF_METADATA_DESTROY((o->metadata));
  FIO___LEAK_COUNTER_ON_FREE(FIO_REF_NAME);
  FIO_MEM_FREE_(o, sizeof(*o) + sizeof(FIO_REF_TYPE));
}

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, _wrapper_s) *o =
      ((FIO_NAME(FIO_REF_NAME, _wrapper_s) *)wrapped) - 1;
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
/* *****************************************************************************
Pointer Tagging Cleanup
***************************************************************************** */
#ifndef FIO___DEV___
#undef FIO_PTR_TAG
#undef FIO_PTR_UNTAG
#undef FIO_PTR_TAG_TYPE
#undef FIO_PTR_TAG_VALIDATE
#undef FIO_PTR_TAG_VALID_OR_RETURN
#undef FIO_PTR_TAG_VALID_OR_RETURN_VOID
#undef FIO_PTR_TAG_VALID_OR_GOTO
#endif
