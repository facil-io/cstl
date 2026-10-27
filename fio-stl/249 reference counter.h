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

/* TODOL: reconsider some math and compiler reliance */

/* The header precedes the object. No alignment attribute is used (it may be a
 * no-op, and MSVC rejects non-literal values): instead, the object's offset is
 * the header's size rounded up to the object's alignment. */
typedef struct {
#ifdef FIO_REF_FLEX_TYPE
  volatile uint32_t ref;
  uint32_t flx_size;
#else
  volatile size_t ref;
#endif
#ifdef FIO_REF_METADATA
  FIO_REF_METADATA metadata;
#endif
} FIO_NAME(FIO_REF_NAME, __wrapper_s);

/* Offset of the object from the (allocation's) header. */
#define FIO___REF_OFFSET                                                       \
  ((sizeof(FIO_NAME(FIO_REF_NAME, __wrapper_s)) +                              \
    (size_t)FIO_ALIGNOF(FIO_REF_TYPE) - 1) &                                   \
   ~((size_t)FIO_ALIGNOF(FIO_REF_TYPE) - 1))
/* Allocation alignment, satisfying both the header and the object. */
#define FIO___REF_ALIGN                                                        \
  ((size_t)FIO_ALIGNOF(FIO_REF_TYPE) >                                         \
           (size_t)FIO_ALIGNOF(FIO_NAME(FIO_REF_NAME, __wrapper_s))            \
       ? (size_t)FIO_ALIGNOF(FIO_REF_TYPE)                                     \
       : (size_t)FIO_ALIGNOF(FIO_NAME(FIO_REF_NAME, __wrapper_s)))
/* Header from the object's (untagged) pointer (`char *` math, same block). */
#define FIO___REF_HEADER(ptr)                                                  \
  ((FIO_NAME(FIO_REF_NAME, __wrapper_s) *)((char *)(ptr)-FIO___REF_OFFSET))

#ifdef FIO_REF_FLEX_TYPE
/* A flexible array must start immediately after the object: padding here
 * would move it away from the offset used by the wrapped type's accessors. */
FIO_ASSERT_STATIC(FIO_ALIGNOF(FIO_REF_TYPE) >= FIO_ALIGNOF(FIO_REF_FLEX_TYPE) &&
                      !(sizeof(FIO_REF_TYPE) % FIO_ALIGNOF(FIO_REF_FLEX_TYPE)),
                  "reference-counted flex suffix requires aligned object size");
#endif

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
FIO_IFUNC FIO_REF_TYPE_PTR
    FIO_NAME(FIO_REF_NAME, FIO_REF_DUPNAME)(const FIO_REF_TYPE_PTR wrapped);

/** Frees a reference counted object (or decreases the reference count). */
IFUNC void FIO_NAME(FIO_REF_NAME, FIO_REF_DESTRUCTOR)(FIO_REF_TYPE_PTR wrapped);

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped);
#endif

#ifdef FIO_REF_FLEX_TYPE
/** The allocation length for the flex type */
IFUNC uint32_t FIO_NAME(FIO_REF_NAME,
                        metadata_flex_len)(FIO_REF_TYPE_PTR wrapped);
#endif
/* *****************************************************************************
Inline Implementation
***************************************************************************** */
/** Increases the reference count. */
FIO_IFUNC FIO_REF_TYPE_PTR
FIO_NAME(FIO_REF_NAME, FIO_REF_DUPNAME)(const FIO_REF_TYPE_PTR wrapped_) {
  const FIO_REF_TYPE *wrapped = (const FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  if (!wrapped || !wrapped_)
    return 0;
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = FIO___REF_HEADER(wrapped);
  fio_atomic_add(&o->ref, 1);
  return (FIO_REF_TYPE_PTR)wrapped_;
}

/** Debugging helper, do not use for data, as returned value is unstable. */
FIO_IFUNC size_t FIO_NAME(FIO_REF_NAME, references)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  if (!wrapped || !wrapped_)
    return 0;
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = FIO___REF_HEADER(wrapped);
  return o->ref;
}

#ifdef FIO_REF_FLEX_TYPE
/** The allocation length for the flex type */
IFUNC uint32_t FIO_NAME(FIO_REF_NAME,
                        metadata_flex_len)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  if (!wrapped || !wrapped_)
    return 0;
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = FIO___REF_HEADER(wrapped);
  return o->flx_size;
}
#endif

/* *****************************************************************************
Reference Counter (Wrapper) Implementation

Internal helper types (`FIO___RECURSIVE_INCLUDE` other than 99) are `static`
(see `001 header.h`), so every translation unit needs their implementation,
even a declarations-only `FIO_EXTERN` one (e.g., FIOBJ inline constructors).
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN) ||                    \
    (defined(FIO___RECURSIVE_INCLUDE) && (FIO___RECURSIVE_INCLUDE + 1) != 100)

/* static helpers exist in declarations-only TUs too: report leaks once */
FIO_LEAK_COUNTER_DEF(FIO_REF_NAME)

/** Allocates a reference counted object. */
#ifdef FIO_REF_FLEX_TYPE
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME,
                                FIO_REF_CONSTRUCTOR)(size_t members) {
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = NULL;
  /* flx_size is 32 bits; reject unrepresentable counts and byte sizes before
   * the allocator or initialization hooks can observe a wrapped allocation. */
  const size_t prefix = FIO___REF_OFFSET + sizeof(FIO_REF_TYPE);
  if (members > UINT32_MAX ||
      members > (SIZE_MAX - prefix) / sizeof(FIO_REF_FLEX_TYPE))
    return (FIO_REF_TYPE_PTR)0;
  FIO_MEM_REALLOC_ALIGNED_(o,
                           0,
                           prefix + sizeof(FIO_REF_FLEX_TYPE) * members,
                           0,
                           FIO___REF_ALIGN);
#else
IFUNC FIO_REF_TYPE_PTR FIO_NAME(FIO_REF_NAME, FIO_REF_CONSTRUCTOR)(void) {
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = NULL;
  FIO_MEM_REALLOC_ALIGNED_(o,
                           0,
                           FIO___REF_OFFSET + sizeof(FIO_REF_TYPE),
                           0,
                           FIO___REF_ALIGN);
#endif /* FIO_REF_FLEX_TYPE */
  if (!o)
    return (FIO_REF_TYPE_PTR)(o);
  FIO_LEAK_COUNTER_ON_ALLOC(FIO_REF_NAME);
  o->ref = 1;
#ifdef FIO_REF_FLEX_TYPE
  o->flx_size = (uint32_t)members;
#endif
  FIO_REF_METADATA_INIT((o->metadata));
  FIO_REF_TYPE *ret = (FIO_REF_TYPE *)((char *)o + FIO___REF_OFFSET);
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
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = FIO___REF_HEADER(wrapped);
  if (!o)
    return;
  if (fio_atomic_sub_fetch(&o->ref, 1))
    return;
  FIO_REF_DESTROY((wrapped[0]));
  FIO_REF_METADATA_DESTROY((o->metadata));
  FIO_LEAK_COUNTER_ON_FREE(FIO_REF_NAME);
#ifdef FIO_REF_FLEX_TYPE
  FIO_MEM_FREE_ALIGNED_(o,
                        FIO___REF_OFFSET + sizeof(FIO_REF_TYPE) +
                            ((size_t)o->flx_size * sizeof(FIO_REF_FLEX_TYPE)));
#else
  FIO_MEM_FREE_ALIGNED_(o, FIO___REF_OFFSET + sizeof(FIO_REF_TYPE));
#endif
}

#ifdef FIO_REF_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REF_METADATA *FIO_NAME(FIO_REF_NAME,
                                 metadata)(FIO_REF_TYPE_PTR wrapped_) {
  FIO_REF_TYPE *wrapped = (FIO_REF_TYPE *)(FIO_PTR_UNTAG(wrapped_));
  FIO_NAME(FIO_REF_NAME, __wrapper_s) *o = FIO___REF_HEADER(wrapped);
  return &o->metadata;
}
#endif

/* *****************************************************************************
Reference Counter (Wrapper) Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE || static helper */
#undef FIO_REF_NAME
#undef FIO_REF_FLEX_TYPE
#undef FIO_REF_TYPE
#undef FIO_REF_INIT
#undef FIO_REF_DESTROY
#undef FIO_REF_METADATA
#undef FIO_REF_METADATA_INIT
#undef FIO_REF_METADATA_DESTROY
#undef FIO_REF_TYPE_PTR
#undef FIO___REF_OFFSET
#undef FIO___REF_ALIGN
#undef FIO___REF_HEADER
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
