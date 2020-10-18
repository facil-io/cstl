/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_MAP_NAME fio_map        /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "210 map settings.h"       /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************









                            Hash Maps / Sets




Hash Maps inherently cause memory cache misses, making them slow when they grow
beyond the CPU cache size.

This Map implementation attempts to minimize some of the re-hashing costs
associated with CPU cache misses by keeping the data and the hash values in an
array.


Example - string based map which automatically copies and frees string data:

```c
#define FIO_RISKY_HASH 1 // for hash value computation
#define FIO_ATOL 1       // for string <=> number conversion
#define FIO_MALLOC 1     // using the custom memory allocator
#include "fio-stl.h"

#define FIO_MAP_NAME mstr

#define FIO_MAP_TYPE char *
#define FIO_MAP_TYPE_DESTROY(s) fio_free(s)
#define FIO_MAP_TYPE_COPY(dest, src)                                           \
  do {                                                                         \
    size_t l = sizeof(char) * (strlen(src) + 1);                               \
    dest = fio_malloc(l);                                                      \
    memcpy(dest, src, l);                                                      \
  } while (0)

#define FIO_MAP_KEY char *
#define FIO_MAP_KEY_CMP(a, b) (!strcmp((a), (b)))
#define FIO_MAP_KEY_DESTROY(s) fio_free(s)
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  do {                                                                         \
    size_t l = sizeof(char) * (strlen(src) + 1);                               \
    dest = fio_malloc(l);                                                      \
    memcpy(dest, src, l);                                                      \
  } while (0)

#include "fio-stl.h"

void main(void) {
  mstr_s map = FIO_MAP_INIT;
  for (size_t i = 0; i < 16; ++i) {
    /. create and insert keys
    char key_buf[48];
    char val_buf[48];
    size_t key_len = fio_ltoa(key_buf, i, 2);
    key_buf[key_len] = 0;
    val_buf[fio_ltoa(val_buf, i, 16)] = 0;
    mstr_set(&map, fio_risky_hash(key_buf, key_len, 0), key_buf, val_buf,
                NULL);
  }
  fprintf(stderr, "Mapping binary representation strings to hex:\n");
  FIO_MAP_EACH2(mstr, &map, pos) {
    // print keys in insertion order
    fprintf(stderr, "%s => %s\n", pos->obj.key, pos->obj.value);
  }
  for (size_t i = 15; i < 16; --i) {
    // search keys out of order
    char key_buf[48];
    size_t key_len = fio_ltoa(key_buf, i, 2);
    key_buf[key_len] = 0;
    char *val = mstr_get(&map, fio_risky_hash(key_buf, key_len, 0), key_buf);
    fprintf(stderr, "found %s => %s\n", key_buf, val);
  }
  mstr_destroy(&map); // will automatically free strings
}
```

***************************************************************************** */
#if defined(FIO_MAP_NAME) && !defined(FIO_MAP_UNORDERED)

/* *****************************************************************************
Hash Map / Set - types
***************************************************************************** */

typedef struct {
  FIO_MAP_HASH hash;
  FIO_MAP_OBJ obj;
#if FIO_MAP_EVICT_LRU
  struct {
    FIO_MAP_SIZE_TYPE next;
    FIO_MAP_SIZE_TYPE prev;
  } node;
#endif /* FIO_MAP_EVICT_LRU */
} FIO_NAME(FIO_MAP_NAME, each_s);

typedef struct {
  FIO_NAME(FIO_MAP_NAME, each_s) * map;
  FIO_MAP_SIZE_TYPE count;
  FIO_MAP_SIZE_TYPE w; /* writing position */
#if FIO_MAP_EVICT_LRU
  FIO_MAP_SIZE_TYPE recent;
#endif
  uint8_t bits;
  uint8_t under_attack;
} FIO_NAME(FIO_MAP_NAME, s);

#define FIO_MAP_S FIO_NAME(FIO_MAP_NAME, s)

#ifdef FIO_PTR_TAG_TYPE
#define FIO_MAP_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, s) *
#endif

/* *****************************************************************************
Hash Map / Set - API (initialization)
***************************************************************************** */

#ifndef FIO_MAP_INIT
/* Initialization macro. */
#define FIO_MAP_INIT                                                           \
  { 0 }
#endif

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/**
 * Allocates a new map on the heap.
 */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void);

/**
 * Frees a map that was allocated on the heap.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR m);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Empties the Map, keeping the current resources and memory for future use.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR m);

/**
 * Destroys the map's internal data and re-initializes it.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR m);

/* *****************************************************************************
Hash Map / Set - API (set/get)
***************************************************************************** */

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR m,
                                                   FIO_MAP_HASH hash,
                                                   FIO_MAP_OBJ_KEY key);

/** Returns a pointer to the object in the hash map (if any) or NULL. */
FIO_IFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR m,
                                                        FIO_MAP_HASH hash,
                                                        FIO_MAP_OBJ_KEY key);
/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR m,
                                                   FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                   FIO_MAP_OBJ_KEY key,
#endif /* FIO_MAP_KEY */
                                                   FIO_MAP_TYPE obj,
                                                   FIO_MAP_TYPE *old);

/**
 * Inserts an object to the hash map, returning the existing or new object.
 */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME,
                                set_if_missing)(FIO_MAP_PTR m,
                                                FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                FIO_MAP_OBJ_KEY key,
#endif /* FIO_MAP_KEY */
                                                FIO_MAP_TYPE obj);

/**
 * Removes an object from the hash map.
 *
 * If `old` is given, existing data will be copied to that location.
 *
 * Returns 0 on success or -1 if the object couldn't be found.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR m,
                                         FIO_MAP_HASH hash,
                                         FIO_MAP_OBJ_KEY key,
                                         FIO_MAP_TYPE *old);

/**
 * Evicts the number of `elements` specified.
 *
 * Eviction is FIFO (first in first out) unless FIO_MAP_EVICT_LRU is defined, in
 * which case the least recently used element will be evicted.
 *
 * Returns 0 on success or -1 if less than `elements` were evicted.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR m, size_t elements);

/* *****************************************************************************
Hash Map / Set - API (misc)
***************************************************************************** */

/** Returns the number of objects in the map. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR m);

/** Returns the current map's theoretical capacity. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR m);

/** Reserves a minimal capacity for the hash map, might reserve more. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     reserve)(FIO_MAP_PTR m,
                                              FIO_MAP_SIZE_TYPE capa);

/** Rehashes the Hash Map / Set. Usually this is performed automatically. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_PTR m);

/** Attempts to lower the map's memory consumption. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR m);

/* *****************************************************************************
Hash Map / Set - API (iterration)
***************************************************************************** */

/**
 * Returns a pointer to the (next) object's information in the map.
 *
 * To access the object information, use:
 *
 *    MAP_each_s * pos = MAP_each_next(map, NULL);
 *
 * - `i->hash` to access the hash value.
 *
 * - `i->obj` to access the object's data.
 *
 *    For Hash Maps, use `i->obj.key` and `i->obj.value`.
 *
 * Returns the first object if `pos == NULL` and there are objects in the map.
 * The first object's address should be used for any future call as the `first`
 * address.
 *
 * Returns the next object if both `first` and `pos` are valid
 *
 * Returns NULL if `pos` was the last object or no object exist.
 *
 */
FIO_IFUNC FIO_NAME(FIO_MAP_NAME, each_s) *
    FIO_NAME(FIO_MAP_NAME, each_next)(FIO_MAP_PTR m,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * *first,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * pos);

#ifndef FIO_MAP_EACH
/**
 * A macro for a `for` loop that iterates over all the Map's objects (in
 * order).
 *
 * Use this macro for small Hash Maps / Sets.
 *
 * - `map_type` is the Map's type name/function prefix, same as FIO_MAP_NAME.
 *
 * - `map_p` is a pointer to the Hash Map / Set variable.
 *
 * - `pos` is a temporary variable name to be created for iteration. This
 *    variable may SHADOW external variables, be aware.
 *
 * To access the object information, use:
 *
 * - `pos->hash` to access the hash value.
 *
 * - `pos->obj` to access the object's data.
 *
 *    For Hash Maps, use `pos->obj.key` and `pos->obj.value`.
 */
#define FIO_MAP_EACH(map_type, map_p, pos)                                     \
  for (FIO_NAME(map_type,                                                      \
                each_s) *first___ = NULL,                                      \
                        *pos = FIO_NAME(map_type,                              \
                                        each_next)(map_p, &first___, NULL);    \
       pos;                                                                    \
       pos = FIO_NAME(map_type, each_next)(map_p, &first___, pos))
#endif

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an element variable as well as an
 * opaque user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
SFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                 each)(FIO_MAP_PTR m,
                                       ssize_t start_at,
                                       int (*task)(FIO_MAP_TYPE obj, void *arg),
                                       void *arg);

#ifdef FIO_MAP_KEY
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME, each_get_key)(void);
#else
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, each_get_key)(void);
#endif

/* *****************************************************************************





Hash Map / Set - Implementation - INLINE





***************************************************************************** */

/* *****************************************************************************
Helper macros
***************************************************************************** */

/** An invalid hash value (all bits are zero). */
#define FIO_MAP_HASH_INVALID ((FIO_MAP_HASH)0)

/** tests if the hash value is valid (not reserved). */
#define FIO_MAP_HASH_IS_INVALID(h) ((h) == FIO_MAP_HASH_INVALID)

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIXED ((FIO_MAP_HASH)-1LL)

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIX(h)                                                    \
  (FIO_MAP_HASH_IS_INVALID(h) ? FIO_MAP_HASH_FIXED : (h))

/* the last (-1) index is always reserved, it will make "holes" */
#define FIO_MAP_INDEX_INVALID ((FIO_MAP_SIZE_TYPE)-1)

/* all bytes == 0 means the index was never used */
#define FIO_MAP_INDEX_UNUSED ((FIO_MAP_SIZE_TYPE)0)

#define FIO_MAP_INDEX_CALC(index, hash, index_mask)                            \
  (((hash) & (~(index_mask))) | ((index) & (index_mask)) |                     \
   FIO_MAP_INDEX_USED_BIT)

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers)
***************************************************************************** */

/* INTERNAL helper: computes a map's capacity according to the bits. */
#define FIO_MAP_CAPA(bits) (((FIO_MAP_SIZE_TYPE)1 << bits) - 1)

/* INTERNAL returns the required memory size in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, __byte_size)(uint8_t bits);

/* INTERNAL reduces the hash's bit-width . */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     __hash2index)(FIO_MAP_HASH hash,
                                                   uint8_t bits);

/* INTERNAL helper: destroys each object in the map. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __destroy_each_entry)(FIO_MAP_S *m);

typedef struct {
  FIO_MAP_SIZE_TYPE i;
  FIO_MAP_SIZE_TYPE imap;
} FIO_NAME(FIO_MAP_NAME, __pos_s);

/** INTERNAL: returns position information (potential / existing). */
SFUNC FIO_NAME(FIO_MAP_NAME, __pos_s)
    FIO_NAME(FIO_MAP_NAME, __get_pos)(FIO_MAP_S *m,
                                      FIO_MAP_HASH fixed_hash,
                                      FIO_MAP_SIZE_TYPE index_hash,
                                      FIO_MAP_OBJ_KEY key);

/** INTERNAL: sets an object in the map. */
SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, __set)(FIO_MAP_S *m,
                                                  FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                  FIO_MAP_OBJ_KEY key,
#endif /* FIO_MAP_KEY */
                                                  FIO_MAP_TYPE obj,
                                                  FIO_MAP_TYPE *old,
                                                  uint8_t overwrite);

/** INTERNAL: rehashes a hash map where all the map's bytes are set to zero. */
SFUNC int FIO_NAME(FIO_MAP_NAME, __rehash_router)(FIO_MAP_S *m);

/** INTERNAL: reserves (at least) the requested capacity of objects. */
FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, __reserve)(FIO_MAP_S *m,
                                                    FIO_MAP_SIZE_TYPE capa);

/** INTERNAL: reallocates the map's memory (without rehashing). */
SFUNC int FIO_NAME(FIO_MAP_NAME, __map_realloc)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                                uint8_t bits);

/** Internal: returns the untagged pointer or NULL. */
FIO_IFUNC const FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, __untag)(FIO_MAP_PTR m_);

/** Internal: returns the internal index map. */
FIO_IFUNC FIO_MAP_SIZE_TYPE *FIO_NAME(FIO_MAP_NAME, __imap)(FIO_MAP_S *m);

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers) - INLINE
***************************************************************************** */

/* INTERNAL returns the required memory size in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, __byte_size)(uint8_t bits) {
  size_t r = 1;
  r <<= bits;
  return ((r * sizeof(FIO_MAP_SIZE_TYPE)) +
          ((r - 1) * sizeof(FIO_NAME(FIO_MAP_NAME, each_s))));
}

/* INTERNAL reduces the hash's bit-width . */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     __hash2index)(FIO_MAP_HASH hash,
                                                   uint8_t bits) {
  return (FIO_MAP_SIZE_TYPE)(
      ((hash) ^ (((hash)*FIO_MAP_CUCKOO_STEPS) >> (63 & (bits)))) |
      FIO_MAP_INDEX_USED_BIT);
}

FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __destroy_each_entry)(FIO_MAP_S *m) {
#if !FIO_MAP_TYPE_DESTROY_SIMPLE || !FIO_MAP_KEY_DESTROY_SIMPLE
  if (m->w == m->count) {
    for (FIO_NAME(FIO_MAP_NAME, each_s) *pos = m->map, *end_ = m->map + m->w;
         pos < end_;
         ++pos) {
      FIO_MAP_OBJ_DESTROY(pos->obj);
    }
  } else {
    for (FIO_NAME(FIO_MAP_NAME, each_s) *pos = m->map, *end_ = m->map + m->w;
         pos < end_;
         ++pos) {
      if (!pos->hash)
        continue;
      FIO_MAP_OBJ_DESTROY(pos->obj);
    }
  }
#endif
  (void)m; /* possible NOOP */
}

/** Internal: returns the untagged pointer or NULL. */
FIO_IFUNC const FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, __untag)(FIO_MAP_PTR m_) {
  return (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
}

/** Internal: returns the internal index map. */
FIO_IFUNC FIO_MAP_SIZE_TYPE *FIO_NAME(FIO_MAP_NAME, __imap)(FIO_MAP_S *m) {
  FIO_MAP_SIZE_TYPE *r;
  const FIO_MAP_SIZE_TYPE capa = FIO_MAP_CAPA(m->bits);

  r = (FIO_MAP_SIZE_TYPE *)(m->map + capa);
  return r;
}

/* *****************************************************************************
Hash Map / Set - API (initialization inlined)
***************************************************************************** */

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/**
 * Allocates a new map on the heap.
 */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void) {
  FIO_MAP_PTR r;
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*m), 0);
  if (!FIO_MEM_REALLOC_IS_SAFE_ && m) {
    *m = (FIO_MAP_S)FIO_MAP_INIT;
  }
  // no need to initialize the map object, since all bytes are zero.
  r = (FIO_MAP_PTR)FIO_PTR_TAG(m);
  return r;
}

/**
 * Frees a map that was allocated on the heap.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(m_);
  FIO_NAME(FIO_MAP_NAME, destroy)(m);
  FIO_MEM_FREE_(m, sizeof(*m));
}

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Empties the Map, keeping the current resources and memory for future use.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(m_);
  if (m->map) {
    FIO_NAME(FIO_MAP_NAME, __destroy_each_entry)(m);
    memset(m->map, 0, FIO_NAME(FIO_MAP_NAME, __byte_size)(m->bits));
  }
  *m = (FIO_MAP_S){.map = m->map};
}

/**
 * Destroys the map's internal data and re-initializes it.
 */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return;
  if (m->map) {
    FIO_NAME(FIO_MAP_NAME, __destroy_each_entry)(m);
    FIO_MEM_FREE_(m->map, FIO_NAME(FIO_MAP_NAME, __byte_size)(m->bits));
  }
  *m = (FIO_MAP_S){0};
}

/* *****************************************************************************
Hash Map / Set - API (set/get) inlined
***************************************************************************** */

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR m_,
                                                   FIO_MAP_HASH hash,
                                                   FIO_MAP_OBJ_KEY key) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return FIO_MAP_TYPE_INVALID;
  FIO_PTR_TAG_VALID_OR_RETURN(m_, FIO_MAP_TYPE_INVALID);
  hash = FIO_MAP_HASH_FIX(hash);
  const FIO_MAP_SIZE_TYPE ihash =
      FIO_NAME(FIO_MAP_NAME, __hash2index)(hash, m->bits);
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  i = FIO_NAME(FIO_MAP_NAME, __get_pos)(m, hash, ihash, key);
  if (i.i != FIO_MAP_INDEX_INVALID) {
    return FIO_MAP_OBJ2TYPE(m->map[i.i].obj);
  }
  return FIO_MAP_TYPE_INVALID;
}

/** Returns a pointer to the object in the hash map (if any) or NULL. */
FIO_IFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR m_,
                                                        FIO_MAP_HASH hash,
                                                        FIO_MAP_OBJ_KEY key) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(m_, NULL);
  hash = FIO_MAP_HASH_FIX(hash);
  const FIO_MAP_SIZE_TYPE ihash =
      FIO_NAME(FIO_MAP_NAME, __hash2index)(hash, m->bits);
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  i = FIO_NAME(FIO_MAP_NAME, __get_pos)(m, hash, ihash, key);
  if (i.i != FIO_MAP_INDEX_INVALID) {
    return &FIO_MAP_OBJ2TYPE(m->map[i.i].obj);
  }
  return NULL;
}

/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR m_,
                                                   FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                   FIO_MAP_OBJ_KEY key,
#endif /* FIO_MAP_KEY */
                                                   FIO_MAP_TYPE obj,
                                                   FIO_MAP_TYPE *old) {
  FIO_MAP_TYPE *p;
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    goto error;
  FIO_PTR_TAG_VALID_OR_GOTO(m_, error);

  p = FIO_NAME(FIO_MAP_NAME, __set)(m,
                                    hash,
#ifdef FIO_MAP_KEY
                                    key,
#endif /* FIO_MAP_KEY */
                                    obj,
                                    old,
                                    1);

  if (p)
    return *p;
  return FIO_MAP_TYPE_INVALID;

error:
#ifdef FIO_MAP_KEY
  FIO_MAP_KEY_DISCARD(key);
#endif /* FIO_MAP_KEY */
  FIO_MAP_TYPE_DISCARD(obj);
  return FIO_MAP_TYPE_INVALID;
}

/**
 * Inserts an object to the hash map, returning the existing or new object.
 */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME,
                                set_if_missing)(FIO_MAP_PTR m_,
                                                FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                FIO_MAP_OBJ_KEY key,
#endif /* FIO_MAP_KEY */
                                                FIO_MAP_TYPE obj) {
  FIO_MAP_TYPE *p;
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    goto error;
  FIO_PTR_TAG_VALID_OR_GOTO(m_, error);

  p = FIO_NAME(FIO_MAP_NAME, __set)(m,
                                    hash,
#ifdef FIO_MAP_KEY
                                    key,
#endif /* FIO_MAP_KEY */
                                    obj,
                                    NULL,
                                    0);
  if (p)
    return *p;
  return FIO_MAP_TYPE_INVALID;

error:
#ifdef FIO_MAP_KEY
  FIO_MAP_KEY_DISCARD(key);
#endif /* FIO_MAP_KEY */
  FIO_MAP_TYPE_DISCARD(obj);
  return FIO_MAP_TYPE_INVALID;
}

/* *****************************************************************************
Hash Map / Set - API (inlined)
***************************************************************************** */

/** Returns the number of objects in the map. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(m_, 0);
  return m->count;
}

/** Returns the current map's theoretical capacity. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m->map || !m->bits)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(m_, 0);
  return FIO_MAP_CAPA(m->bits);
}

/** Reserves a minimal capacity for the hash map. */
FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     reserve)(FIO_MAP_PTR m_,
                                              FIO_MAP_SIZE_TYPE capa) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(m_, 0);
  uint8_t bits = 2;
  if (capa == (FIO_MAP_SIZE_TYPE)-1)
    return FIO_MAP_INDEX_INVALID;
  while (((size_t)1 << bits) <= capa)
    ++bits;
  if (FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, bits) ||
      FIO_NAME(FIO_MAP_NAME, __rehash_router)(m))
    return FIO_MAP_INDEX_INVALID;
  return FIO_MAP_CAPA(m->bits);
}

/** Rehashes the Hash Map / Set. Usually this is performed automatically. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_PTR m_) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_ || !m->map || !m->bits)
    return 0;
  return FIO_NAME(FIO_MAP_NAME, __rehash_router)(m);
}

/** Attempts to lower the map's memory consumption. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR m_) {
  int r = 0;
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  FIO_PTR_TAG_VALID_OR_RETURN(m_, r);
  if (!m || !m->map || !m->bits)
    return r;
  uint8_t bits = 1;
  while ((FIO_MAP_SIZE_TYPE)1 << bits <= m->count)
    ++bits;
  r = FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, bits);
  r |= FIO_NAME(FIO_MAP_NAME, __rehash_router)(m);
  return r;
}

/** Returns a pointer to the (next) object's information in the map. */
FIO_IFUNC FIO_NAME(FIO_MAP_NAME, each_s) *
    FIO_NAME(FIO_MAP_NAME, each_next)(FIO_MAP_PTR m_,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * *first,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * pos) {
  FIO_PTR_TAG_VALID_OR_RETURN(m_, NULL);
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  if (!m || !m_ || !m->map || !first)
    return NULL;
  if (!pos || !(*first) || (*first) > pos) {
    pos = m->map - 1;
  } else {
    pos = m->map + (intptr_t)(pos - (*first));
  }
  *first = m->map;
  for (;;) {
    ++pos;
    if (pos >= m->map + m->w)
      return NULL;
    if (pos->hash)
      return pos;
  }
}

/* *****************************************************************************



Hash Map / Set - Implementation



***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers) - Internal Memory Management
***************************************************************************** */

/** Internal: removes "holes" from the internal array. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __compact_forced)(FIO_MAP_S *m,
                                                        uint8_t zero) {
  FIO_MAP_SIZE_TYPE r = 0, w = 0;
  /* consume continuous data */
  while (r < m->w && m->map[r].hash)
    ++r;
  /* fill holes in array */
  for (w = (r++); r < m->w; ++r) {
    if (!m->map[r].hash)
      continue;
    if (r + m->count == m->w + w && r + 4 < m->w) {
      /* filled last hole (optimized?) */
      memmove(m->map + w, m->map + r, sizeof(*m->map) * (m->w - r));
      w += (m->w - r);
      break;
    }
    m->map[w++] = m->map[r];
  }
  m->w = w;
  FIO_ASSERT_DEBUG(m->w == m->count, "implementation error?");

  if (zero) {
    FIO_ASSERT_DEBUG(FIO_NAME(FIO_MAP_NAME, __byte_size)(m->bits) >
                         (sizeof(*m->map) * m->w),
                     "always true");
    memset(m->map + w,
           0,
           FIO_NAME(FIO_MAP_NAME, __byte_size)(m->bits) -
               (sizeof(*m->map) * m->w));
  }
}

/** Internal: reallocates the map's memory, zeroing out as needed. */
SFUNC int FIO_NAME(FIO_MAP_NAME, __map_realloc)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                                uint8_t bits) {
  if (bits >= (sizeof(FIO_MAP_SIZE_TYPE) << 3))
    return -1;
  if (bits < 2)
    bits = 2;
  const size_t old_capa = FIO_MAP_CAPA(m->bits);
  const size_t new_capa = FIO_MAP_CAPA(bits);
  if (new_capa != old_capa) {
    if (new_capa < m->count) {
      /* not enough room for existing items - recall function with valid value
       */
      while (((size_t)1 << bits) <= m->count)
        ++bits;
      return FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, bits);
    }
    if (new_capa <= m->w) {
      /* we need to compact the array (remove holes) before reallocating */
      FIO_NAME(FIO_MAP_NAME, __compact_forced)(m, 0);
    }
    FIO_NAME(FIO_MAP_NAME, each_s) *tmp =
        (FIO_NAME(FIO_MAP_NAME, each_s) *)FIO_MEM_REALLOC_(
            m->map,
            FIO_NAME(FIO_MAP_NAME, __byte_size)(m->bits),
            FIO_NAME(FIO_MAP_NAME, __byte_size)(bits),
            m->w * sizeof(*m->map));
    if (!tmp)
      return -1;
    m->map = tmp;
    m->bits = bits;
  }
  FIO_MAP_SIZE_TYPE *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  if (new_capa > old_capa) {
    if (!FIO_MEM_REALLOC_IS_SAFE_) {
      /* realloc might return recycled (junk filled) memory */
      memset(imap, 0, ((size_t)1 << m->bits) * sizeof(FIO_MAP_SIZE_TYPE));
    }
  } else {
    /* when shrinking (or staying), we might have junk data by design... */
    memset(imap, 0, ((size_t)1 << m->bits) * sizeof(FIO_MAP_SIZE_TYPE));
  }
  return 0;
}

/** Internal: frees the map's memory. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __map_free_map)(FIO_NAME(FIO_MAP_NAME, each_s) * map,
                                        uint8_t bits) {
  const size_t old_size = (1 << bits) - 1;
  FIO_MEM_FREE_(map,
                (old_size * (sizeof(*map)) +
                 (((size_t)old_size + 1) * sizeof(FIO_MAP_SIZE_TYPE))));
  (void)old_size; /* if unused */
}

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers) - Map Positioning
***************************************************************************** */

/** INTERNAL: returns position information (potential / existing). */
SFUNC FIO_NAME(FIO_MAP_NAME, __pos_s)
    FIO_NAME(FIO_MAP_NAME, __get_pos)(FIO_MAP_S *m,
                                      FIO_MAP_HASH hash,
                                      FIO_MAP_SIZE_TYPE ihash,
                                      FIO_MAP_OBJ_KEY key) {
  const size_t imask = ((FIO_MAP_SIZE_TYPE)1 << m->bits) - 1;
  const size_t test_mask = ~imask;
  FIO_MAP_SIZE_TYPE *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  r = {.i = FIO_MAP_INDEX_INVALID, .imap = FIO_MAP_INDEX_INVALID};
  size_t i = ihash;
  if (!m->map)
    return r;
  ihash &= test_mask;
  uint8_t full_collisions = 0;
  unsigned int attempts =
      (unsigned int)((imask < FIO_MAP_MAX_SEEK) ? imask : FIO_MAP_MAX_SEEK);
  if (FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT >= m->bits)
    goto seek_as_array;
  do {
    i &= imask;
    if (!imap[i]) {
      /* unused empty slot */
      /* update returned index only if no previous index exits */
      if (r.imap == FIO_MAP_INDEX_INVALID)
        r.imap = (FIO_MAP_SIZE_TYPE)i;
      return r;
    }
    if (imap[i] == FIO_MAP_INDEX_INVALID) {
      /* known hole, could be filled by `__set` */
      /* update returned index only if no previous index exits */
      if (r.imap == FIO_MAP_INDEX_INVALID)
        r.imap = (FIO_MAP_SIZE_TYPE)i;
    } else if ((imap[i] & test_mask) == ihash &&
               m->map[(imap[i] & imask)].hash == hash) {
      /* potential hit */
      if (m->under_attack ||
          FIO_MAP_OBJ_KEY_CMP(m->map[(imap[i] & imask)].obj, key)) {
        /* object found */
        r = (FIO_NAME(FIO_MAP_NAME, __pos_s)){
            .i = (FIO_MAP_SIZE_TYPE)(imap[i] & imask),
            .imap = (FIO_MAP_SIZE_TYPE)i,
        };
#if FIO_MAP_EVICT_LRU
        if (m->recent != r.i) {
          FIO_INDEXED_LIST_REMOVE(m->map, node, r.i);
          FIO_INDEXED_LIST_PUSH(m->map, node, m->recent, r.i);
          m->recent = r.i;
        }
#endif /* FIO_MAP_EVICT_LRU */
        return r;
      }
      if (++full_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
        m->under_attack = 1;
        FIO_LOG_SECURITY("(core type) Map under attack?"
                         " (multiple full collisions)");
      }
    }
    i += FIO_MAP_CUCKOO_STEPS;
  } while (attempts--);
  return r;

seek_as_array:
  for (i = 0; i < m->w; ++i) {
    if (m->map[i].hash == hash &&
        FIO_MAP_OBJ_KEY_CMP(m->map[(i & imask)].obj, key)) {
      r = (FIO_NAME(FIO_MAP_NAME, __pos_s)){
          .i = (FIO_MAP_SIZE_TYPE)i,
          .imap = (FIO_MAP_SIZE_TYPE)i,
      };
      return r;
    }
  }
  if (m->w <= FIO_MAP_CAPA(m->bits))
    r.imap = m->w;
  return r;

  (void)key; /* just in case it's always true */
}

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers) - Rehashing
***************************************************************************** */

/** Internal: rehashes the map. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, __rehash_no_holes)(FIO_MAP_S *m) {
  size_t pos = 0;
  FIO_MAP_SIZE_TYPE *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  FIO_NAME(FIO_MAP_NAME, each_s) *map = m->map;
  while (pos < m->w) {
    const FIO_MAP_SIZE_TYPE ihash =
        FIO_NAME(FIO_MAP_NAME, __hash2index)(map[pos].hash, m->bits);
    FIO_NAME(FIO_MAP_NAME, __pos_s)
    i = FIO_NAME(
        FIO_MAP_NAME,
        __get_pos)(m, map[pos].hash, ihash, FIO_MAP_OBJ2KEY(map[pos].obj));
    if (i.imap == FIO_MAP_INDEX_INVALID) {
      pos = 0;
      if (FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, m->bits + 1))
        return -1;
      map = m->map;
      imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
      continue;
    }
    imap[i.imap] = (FIO_MAP_SIZE_TYPE)(
        (pos) | (ihash & (~(((FIO_MAP_SIZE_TYPE)1 << m->bits) - 1))));
    ++pos;
  }
  return 0;
}
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, __rehash_has_holes)(FIO_MAP_S *m) {
  FIO_NAME(FIO_MAP_NAME, __compact_forced)(m, 1);
  return FIO_NAME(FIO_MAP_NAME, __rehash_no_holes)(m);
  (void)m;
  return 0;
}

SFUNC int FIO_NAME(FIO_MAP_NAME, __rehash_router)(FIO_MAP_S *m) {
  if (m->count == m->w)
    return FIO_NAME(FIO_MAP_NAME, __rehash_no_holes)(m);
  return FIO_NAME(FIO_MAP_NAME, __rehash_has_holes)(m);
}

/* *****************************************************************************
Hash Map / Set - Internal API (Helpers) - Object Insertion / Removal
***************************************************************************** */

/** INTERNAL: eviction. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __evict)(FIO_MAP_S *m,
                                               FIO_MAP_SIZE_TYPE elements,
                                               uint8_t remove_from_map) {
  FIO_MAP_SIZE_TYPE pos = 0;
  do {
#if FIO_MAP_EVICT_LRU
    pos = m->map[m->recent].node.prev;
    if (!m->map[pos].hash)
      break;
#else
    while (!m->map[pos].hash)
      ++pos;
#endif /* FIO_MAP_EVICT_LRU */
    FIO_ASSERT_DEBUG(pos < m->w, "always true.");
    if (remove_from_map) {
      const FIO_MAP_SIZE_TYPE tmp_ihash =
          FIO_NAME(FIO_MAP_NAME, __hash2index)(m->map[pos].hash, m->bits);
      FIO_NAME(FIO_MAP_NAME, __pos_s)
      i_tmp =
          FIO_NAME(FIO_MAP_NAME, __get_pos)(m,
                                            m->map[pos].hash,
                                            tmp_ihash,
                                            FIO_MAP_OBJ2KEY(m->map[pos].obj));
      FIO_ASSERT_DEBUG(i_tmp.i != FIO_MAP_INDEX_INVALID &&
                           i_tmp.imap != FIO_MAP_INDEX_INVALID,
                       "always true.");
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[i_tmp.imap] = FIO_MAP_INDEX_INVALID;
    }
    FIO_MAP_OBJ_DESTROY(m->map[pos].obj);
    m->map[pos].obj = FIO_MAP_OBJ_INVALID;
    m->map[pos].hash = 0;
    --m->count;
  } while (--elements);
}

/** INTERNAL: sets an object in the map. */
SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, __set)(FIO_MAP_S *m,
                                                  FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                  FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                  FIO_MAP_TYPE obj,
                                                  FIO_MAP_TYPE *old,
                                                  uint8_t overwrite) {

  hash = FIO_MAP_HASH_FIX(hash); // isn't called by caller...
  const FIO_MAP_SIZE_TYPE ihash =
      FIO_NAME(FIO_MAP_NAME, __hash2index)(hash, m->bits);
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  i = FIO_NAME(FIO_MAP_NAME, __get_pos)(m,
                                        hash,
                                        ihash,
#ifdef FIO_MAP_KEY
                                        key
#else
                                        obj
#endif /* FIO_MAP_KEY */
  );
  if (i.i == FIO_MAP_INDEX_INVALID) {
    /* add new object */

    /* grow memory or clean existing memory */
    const FIO_MAP_SIZE_TYPE capa = FIO_MAP_CAPA(m->bits);
    if (m->w >= capa || i.imap == FIO_MAP_INDEX_INVALID) {
      if (m->w > m->count)
        FIO_NAME(FIO_MAP_NAME, __compact_forced)(m, 1);
      else if (FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, m->bits + 1))
        goto error;
      i.imap = FIO_MAP_INDEX_INVALID;
    }

#if FIO_MAP_MAX_ELEMENTS
    /* are we using more elements then we should? */
    if (m->count >= FIO_MAP_MAX_ELEMENTS) {
      FIO_NAME(FIO_MAP_NAME, __evict)(m, 1, i.imap != FIO_MAP_INDEX_INVALID);
    }
#endif

    /* copy information */
#ifdef FIO_MAP_KEY
    FIO_MAP_TYPE_COPY((m->map[m->w].obj.value), obj);
    FIO_MAP_KEY_COPY((m->map[m->w].obj.key), key);
#else
    FIO_MAP_TYPE_COPY((m->map[m->w].obj), obj);
#endif
    m->map[m->w].hash = hash;
#if FIO_MAP_EVICT_LRU
    m->map[i.i].node.next = m->map[i.i].node.prev = 0;
#endif /* FIO_MAP_EVICT_LRU */
    i.i = m->w;
    ++m->w;
    ++m->count;

    if (i.imap == FIO_MAP_INDEX_INVALID) {
      FIO_NAME(FIO_MAP_NAME, __rehash_router)(m);
    } else {
      FIO_MAP_SIZE_TYPE *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
      imap[i.imap] =
          (m->w - 1) | (ihash & (~(((FIO_MAP_SIZE_TYPE)1 << m->bits) - 1)));
    }
    if (old)
      *old = FIO_MAP_TYPE_INVALID;
    return &FIO_MAP_OBJ2TYPE(m->map[i.i].obj);
  }
  /* existing. overwrite? */
  if (overwrite) {
#ifdef FIO_MAP_KEY
    FIO_MAP_TYPE tmp_old = m->map[i.i].obj.value;
    FIO_MAP_TYPE_COPY((m->map[i.i].obj.value), obj);
#else
    FIO_MAP_TYPE tmp_old = m->map[i.i].obj;
    FIO_MAP_TYPE_COPY((m->map[i.i].obj), obj);
#endif
    if (old) {
      FIO_MAP_TYPE_COPY((*old), tmp_old);
#if FIO_MAP_DESTROY_AFTER_COPY
      FIO_MAP_TYPE_DESTROY(tmp_old);
#endif
    } else {
      FIO_MAP_TYPE_DESTROY(tmp_old);
    }
    FIO_MAP_KEY_DISCARD(key);
  } else {
    FIO_MAP_KEY_DISCARD(key);
    FIO_MAP_TYPE_DISCARD(obj);
  }
#if FIO_MAP_EVICT_LRU
  if (m->recent != i.i) {
    FIO_INDEXED_LIST_REMOVE(m->map, node, i.i);
    FIO_INDEXED_LIST_PUSH(m->map, node, m->recent, i.i);
    m->recent = i.i;
  }
#endif /* FIO_MAP_EVICT_LRU */
  return &FIO_MAP_OBJ2TYPE(m->map[i.i].obj);

error:

  FIO_MAP_KEY_DISCARD(key);
  FIO_MAP_TYPE_DISCARD(obj);
  FIO_NAME(FIO_MAP_NAME, __map_realloc)(m, m->bits);
  FIO_NAME(FIO_MAP_NAME, __rehash_router)(m);
  return NULL;
}

/**
 * Removes an object from the hash map.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR m_,
                                         FIO_MAP_HASH hash,
                                         FIO_MAP_OBJ_KEY key,
                                         FIO_MAP_TYPE *old) {
  FIO_MAP_S *const m = (FIO_MAP_S *)FIO_PTR_UNTAG(m_);
  FIO_PTR_TAG_VALID_OR_GOTO(m_, not_found);
  if (!m || !m_ || !m->count)
    goto not_found;
  {
    hash = FIO_MAP_HASH_FIX(hash);
    const FIO_MAP_SIZE_TYPE ihash =
        FIO_NAME(FIO_MAP_NAME, __hash2index)(hash, m->bits);
    FIO_NAME(FIO_MAP_NAME, __pos_s)
    i = FIO_NAME(FIO_MAP_NAME, __get_pos)(m, hash, ihash, key);
    if (i.i == FIO_MAP_INDEX_INVALID)
      goto not_found;
    if (old) {
      FIO_MAP_TYPE_COPY((*old), FIO_MAP_OBJ2TYPE(m->map[i.i].obj));
      FIO_MAP_OBJ_DESTROY_AFTER((m->map[i.i].obj));
    } else {
      FIO_MAP_OBJ_DESTROY(m->map[i.i].obj);
    }
    m->map[i.i].obj = FIO_MAP_OBJ_INVALID;
    m->map[i.i].hash = 0;
    if (i.imap != FIO_MAP_INDEX_INVALID) {
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[i.imap] = FIO_MAP_INDEX_INVALID;
    }
    --m->count;
    while (m->w && !m->map[m->w - 1].hash)
      --m->w;
  }
  return 0;
not_found:
  if (old)
    *old = FIO_MAP_TYPE_INVALID;
  return -1;
}

/**
 * Evicts the number of `elements` specified.
 *
 * Eviction is FIFO (first in first out) unless FIO_MAP_EVICT_LRU is defined, in
 * which case the least recently used elemnt will be evicted.
 *
 * Returns 0 on success or -1 if less than `elements` were evicted.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR m_, size_t elements) {
  FIO_PTR_TAG_VALID_OR_RETURN(m_, -1);
  FIO_NAME(FIO_MAP_NAME, s) *const m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(m_);
  if (!m || !m_ || !m->map) {
    return -1;
  }
  if (elements >= m->count) {
    FIO_NAME(FIO_MAP_NAME, clear)(m_);
    return -1;
  }
  FIO_NAME(FIO_MAP_NAME, __evict)(m, elements, 1);
  return 0;
}

/* *****************************************************************************

Hash Map / Set - Iteration

***************************************************************************** */

FIO_SFUNC __thread FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, __each_pos) = 0;
FIO_SFUNC __thread FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, __each_map) = NULL;

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an element variable as well as an
 * opaque user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                 each)(FIO_MAP_PTR m_,
                                       ssize_t start_at,
                                       int (*task)(FIO_MAP_TYPE obj, void *arg),
                                       void *arg) {
  FIO_PTR_TAG_VALID_OR_RETURN(m_, 0);
  FIO_NAME(FIO_MAP_NAME, s) *const m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(m_);
  if (!m || !m_ || !m->map) {
    return 0;
  }

  FIO_NAME(FIO_MAP_NAME, s) *old_map = FIO_NAME(FIO_MAP_NAME, __each_map);
  if (start_at < 0) {
    start_at = m->count + start_at;
    if (start_at < 0)
      start_at = 0;
  }
  if ((FIO_MAP_SIZE_TYPE)start_at >= m->count)
    return m->count;
  FIO_MAP_SIZE_TYPE old_pos = FIO_NAME(FIO_MAP_NAME, __each_pos);
  FIO_MAP_SIZE_TYPE count = (FIO_MAP_SIZE_TYPE)start_at;
  FIO_NAME(FIO_MAP_NAME, __each_pos) = 0;
  FIO_NAME(FIO_MAP_NAME, __each_map) = m;

  if (m->w == m->count) {
    FIO_NAME(FIO_MAP_NAME, __each_pos) = (FIO_MAP_SIZE_TYPE)start_at;
  } else {
    while (start_at) {
      FIO_MAP_SIZE_TYPE tmp = FIO_NAME(FIO_MAP_NAME, __each_pos);
      ++FIO_NAME(FIO_MAP_NAME, __each_pos);
      if (FIO_MAP_HASH_IS_INVALID(m->map[tmp].hash)) {
        continue;
      }
      --start_at;
    }
  }
  while (count < m->count && (++count) &&
         task(FIO_MAP_OBJ2TYPE(m->map[FIO_NAME(FIO_MAP_NAME, __each_pos)].obj),
              arg) != -1)
    ++FIO_NAME(FIO_MAP_NAME, __each_pos);
  FIO_NAME(FIO_MAP_NAME, __each_pos) = old_pos;
  FIO_NAME(FIO_MAP_NAME, __each_map) = old_map;
  return count;
}

#ifdef FIO_MAP_KEY
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_MAP_NAME, __each_map) ||
      !FIO_NAME(FIO_MAP_NAME, __each_map)->map)
    return FIO_MAP_KEY_INVALID;
  return FIO_NAME(FIO_MAP_NAME, __each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, __each_pos)]
      .obj.key;
}
#else
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_MAP_NAME, __each_map) ||
      !FIO_NAME(FIO_MAP_NAME, __each_map)->map)
    return FIO_MAP_HASH_INVALID;
  return FIO_NAME(FIO_MAP_NAME, __each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, __each_pos)]
      .hash;
}
#endif

/* *****************************************************************************
Hash Map / Set - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#endif /* FIO_MAP_NAME */