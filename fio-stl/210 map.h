/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MAP_NAME map       /* Development inclusion - ignore line */
#define FIO_MAP_TEST           /* Development inclusion - ignore line */
#define FIO_MAP_KEY  size_t    /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  Unordered/Ordered Map Implementation



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MAP_NAME)
/* *****************************************************************************
Map Settings - Sets have only keys (value == key) - Hash Maps have values
***************************************************************************** */

/* if FIO_MAP_KEY_KSTR is defined, use fio_keystr_s keys */
#ifdef FIO_MAP_KEY_KSTR
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         fio_keystr_s
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_keystr_info(&(k))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio_keystr_copy((src), FIO_NAME(FIO_MAP_NAME, __key_alloc))
#define FIO_MAP_KEY_CMP(a, b) fio_keystr_is_eq2((a), (b))
#define FIO_MAP_KEY_DESTROY(key)                                               \
  fio_keystr_destroy(&(key), FIO_NAME(FIO_MAP_NAME, __key_free))
#define FIO_MAP_KEY_DISCARD(key)
FIO_SFUNC void *FIO_NAME(FIO_MAP_NAME, __key_alloc)(size_t len) {
  return FIO_MEM_REALLOC_(NULL, 0, len, 0);
}
FIO_SFUNC void FIO_NAME(FIO_MAP_NAME, __key_free)(void *ptr, size_t len) {
  FIO_MEM_FREE_(ptr, len);
  (void)len; /* if unused */
}
#undef FIO_MAP_KEY_KSTR

/* if FIO_MAP_KEY is undefined, assume String keys (using `fio_bstr`). */
#elif !defined(FIO_MAP_KEY) || defined(FIO_MAP_KEY_BSTR)
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         char *
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio_bstr_write(NULL, (src).buf, (src).len)
#define FIO_MAP_KEY_CMP(a, b)    fio_bstr_is_eq2info((a), (b))
#define FIO_MAP_KEY_DESTROY(key) fio_bstr_free((key))
#define FIO_MAP_KEY_DISCARD(key)
#endif
#undef FIO_MAP_KEY_BSTR

#ifndef FIO_MAP_KEY_INTERNAL
#define FIO_MAP_KEY_INTERNAL FIO_MAP_KEY
#endif

#ifndef FIO_MAP_KEY_FROM_INTERNAL
#define FIO_MAP_KEY_FROM_INTERNAL(o) o
#endif

#ifndef FIO_MAP_KEY_COPY
#define FIO_MAP_KEY_COPY(dest, src) ((dest) = (src))
#endif

#ifndef FIO_MAP_KEY_CMP
#define FIO_MAP_KEY_CMP(a, b) ((a) == (b))
#endif

#ifndef FIO_MAP_KEY_DESTROY
#define FIO_MAP_KEY_DESTROY(o)
#define FIO_MAP_KEY_DESTROY_SIMPLE 1
#endif

#ifndef FIO_MAP_KEY_DISCARD
#define FIO_MAP_KEY_DISCARD(o)
#endif

/* FIO_MAP_HASH_FN(key) - used instead of providing a hash value. */
#ifndef FIO_MAP_HASH_FN
#undef FIO_MAP_RECALC_HASH
#endif

/* FIO_MAP_RECALC_HASH - if true, hash values won't be cached. */
#ifndef FIO_MAP_RECALC_HASH
#define FIO_MAP_RECALC_HASH 0
#endif

#ifdef FIO_MAP_VALUE_BSTR
#define FIO_MAP_VALUE                  fio_str_info_s
#define FIO_MAP_VALUE_INTERNAL         char *
#define FIO_MAP_VALUE_FROM_INTERNAL(v) fio_bstr_info((v))
#define FIO_MAP_VALUE_COPY(dest, src)                                          \
  (dest) = fio_bstr_write(NULL, (src).buf, (src).len)
#define FIO_MAP_VALUE_DESTROY(v) fio_bstr_free((v))
#define FIO_MAP_VALUE_DISCARD(v)
#endif

#ifdef FIO_MAP_VALUE
#define FIO_MAP_GET_T FIO_MAP_VALUE
#else
#define FIO_MAP_GET_T FIO_MAP_KEY
#endif

#ifndef FIO_MAP_VALUE_INTERNAL
#define FIO_MAP_VALUE_INTERNAL FIO_MAP_VALUE
#endif

#ifndef FIO_MAP_VALUE_FROM_INTERNAL
#ifdef FIO_MAP_VALUE
#define FIO_MAP_VALUE_FROM_INTERNAL(o) o
#else
#define FIO_MAP_VALUE_FROM_INTERNAL(o)
#endif
#endif

#ifndef FIO_MAP_VALUE_COPY
#ifdef FIO_MAP_VALUE
#define FIO_MAP_VALUE_COPY(dest, src) (dest) = (src)
#else
#define FIO_MAP_VALUE_COPY(dest, src)
#endif
#endif

#ifndef FIO_MAP_VALUE_DESTROY
#define FIO_MAP_VALUE_DESTROY(o)
#define FIO_MAP_VALUE_DESTROY_SIMPLE 1
#endif

#ifndef FIO_MAP_VALUE_DISCARD
#define FIO_MAP_VALUE_DISCARD(o)
#endif

#ifdef FIO_MAP_LRU
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1 /* required for least recently used order */
#endif

/* test if FIO_MAP_ORDERED was defined as an empty macro */
#if defined(FIO_MAP_ORDERED) && ((0 - FIO_MAP_ORDERED - 1) == 1)
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1 /* assume developer's intention */
#endif

#ifndef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 0
#endif

/* *****************************************************************************
Pointer Tagging Support
***************************************************************************** */

#ifdef FIO_PTR_TAG_TYPE
#define FIO_MAP_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, s) *
#endif
#define FIO_MAP_T FIO_NAME(FIO_MAP_NAME, s)

/* *****************************************************************************
Map Types
***************************************************************************** */

/** internal object data representation */
typedef struct FIO_NAME(FIO_MAP_NAME, node_s) FIO_NAME(FIO_MAP_NAME, node_s);

/** A Hash Map / Set type */
typedef struct FIO_NAME(FIO_MAP_NAME, s) {
  uint32_t bits;
  uint32_t count;
  FIO_NAME(FIO_MAP_NAME, node_s) * map;
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST32_HEAD head;
#endif
} FIO_NAME(FIO_MAP_NAME, s);

/** internal object data representation */
struct FIO_NAME(FIO_MAP_NAME, node_s) {
#if !FIO_MAP_RECALC_HASH
  uint64_t hash;
#endif
  FIO_MAP_KEY_INTERNAL key;
#ifdef FIO_MAP_VALUE
  FIO_MAP_VALUE_INTERNAL value;
#endif
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST32_NODE node;
#endif
};

/** Map iterator type */
typedef struct {
  /** the node in the internal map */
  FIO_NAME(FIO_MAP_NAME, node_s) * node;
  /** the key in the current position */
  FIO_MAP_KEY key;
#ifdef FIO_MAP_VALUE
  /** the value in the current position */
  FIO_MAP_VALUE value;
#endif
#if !FIO_MAP_RECALC_HASH
  /** the hash for the current position */
  uint64_t hash;
#endif
  struct {                   /* internal usage, do not access */
    uint32_t index;          /* the index in the internal map */
    uint32_t pos;            /* the position in the ordering scheme */
    uintptr_t map_validator; /* map mutation guard */
  } private_;
} FIO_NAME(FIO_MAP_NAME, iterator_s);

#ifndef FIO_MAP_INIT
/* Initialization macro. */
#define FIO_MAP_INIT                                                           \
  { 0 }
#define FIO_MAP_INIT                                                           \
  { 0 }
#endif

/* *****************************************************************************
Construction / Deconstruction
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new object on the heap and initializes it's memory. */
SFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void);

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR map);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, reinitializing its container. */
SFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR map);

/* *****************************************************************************
Map State
***************************************************************************** */

/** Theoretical map capacity. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR map);

/** The number of objects in the map capacity. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR map);

/** Reserves at minimum the capacity requested. */
SFUNC void FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_PTR map, size_t capa);

/** Returns the key value associated with the node's pointer (see set_ptr). */
FIO_IFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME,
                               node2key)(FIO_NAME(FIO_MAP_NAME, node_s) * node);

/** Returns the hash value associated with the node's pointer (see set_ptr). */
FIO_IFUNC uint64_t FIO_NAME(FIO_MAP_NAME,
                            node2hash)(FIO_NAME(FIO_MAP_NAME, node_s) * node);

#ifdef FIO_MAP_VALUE
/** Returns the value associated with the node's pointer (see set_ptr). */
FIO_IFUNC FIO_MAP_VALUE FIO_NAME(FIO_MAP_NAME,
                                 node2val)(FIO_NAME(FIO_MAP_NAME, node_s) *
                                           node);
#endif

/** Returns the key value associated with the node's pointer (see set_ptr). */
FIO_IFUNC FIO_MAP_KEY_INTERNAL *FIO_NAME(FIO_MAP_NAME, node2key_ptr)(
    FIO_NAME(FIO_MAP_NAME, node_s) * node);

#ifdef FIO_MAP_VALUE
/** Returns the value associated with the node's pointer (see set_ptr). */
FIO_IFUNC FIO_MAP_VALUE_INTERNAL *FIO_NAME(FIO_MAP_NAME, node2val_ptr)(
    FIO_NAME(FIO_MAP_NAME, node_s) * node);
#endif

/* *****************************************************************************
Adding / Removing Elements from the Map
***************************************************************************** */

/** Removes an object in the map, returning a pointer to the map data. */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                         uint64_t hash,
#endif
                                         FIO_MAP_KEY key,
#ifdef FIO_MAP_VALUE
                                         FIO_MAP_VALUE_INTERNAL *old
#else
                                         FIO_MAP_KEY_INTERNAL *old
#endif
);

/** Evicts elements in order least recently used (LRU), FIFO or undefined. */
SFUNC void FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR map,
                                         size_t number_of_elements);

/** Removes all objects from the map, without releasing the map's resources. */
SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map);

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR map);

/** Gets a value from the map, if exists. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                    uint64_t hash,
#endif
                                                    FIO_MAP_KEY key);

/** Sets a value in the map, hash maps will overwrite existing data if any. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                    uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                                                    FIO_MAP_KEY key,
                                                    FIO_MAP_VALUE obj,
                                                    FIO_MAP_VALUE_INTERNAL *old
#else
                                                    FIO_MAP_KEY key
#endif
);

/** Sets a value in the map if not set previously. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, set_if_missing)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                               uint64_t hash,
#endif
                                                               FIO_MAP_KEY key
#ifdef FIO_MAP_VALUE
                                                               ,
                                                               FIO_MAP_VALUE obj
#endif
);

/**
 * The core set function.
 *
 * This function returns `NULL` on error (errors are logged).
 *
 * If the map is a hash map, overwriting the value (while keeping the key) is
 * possible. In this case the `old` pointer is optional, and if set than the old
 * data will be copied to over during an overwrite.
 *
 * NOTE: the function returns a pointer to the map's internal storage.
 */
SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, set_ptr)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                    uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                                    FIO_MAP_KEY key,
                                    FIO_MAP_VALUE val,
                                    FIO_MAP_VALUE_INTERNAL *old,
                                    int overwrite
#else
                                    FIO_MAP_KEY key
#endif
    );

/**
 * The core get function. This function returns NULL if item is missing.
 *
 * NOTE: the function returns a pointer to the map's internal storage.
 */
SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                    uint64_t hash,
#endif
                                    FIO_MAP_KEY key);
/* *****************************************************************************
Map Iteration and Traversal
***************************************************************************** */

/**
 * Returns the next iterator object after `current_pos` or the first if `NULL`.
 *
 * Note that adding objects to the map or rehashing between iterations could
 * incur performance penalties when re-setting and re-seeking the previous
 * iterator position.
 *
 * Adding objects to, or rehashing, an unordered maps could invalidate the
 * iterator object completely as the ordering may have changed and so the "next"
 * object might be any object in the map.
 */
SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME,
             get_next)(FIO_MAP_PTR map,
                       FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos);

/**
 * Returns the next iterator object after `current_pos` or the last if `NULL`.
 *
 * See notes in `get_next`.
 */
SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME,
             get_prev)(FIO_MAP_PTR map,
                       FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos);

/** Returns 1 if the iterator is out of bounds, otherwise returns 0. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       iterator_is_valid)(FIO_NAME(FIO_MAP_NAME, iterator_s) *
                                          iterator);

/** Returns a pointer to the node object in the internal map. */
FIO_IFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME,
             iterator2node)(FIO_MAP_PTR map,
                            FIO_NAME(FIO_MAP_NAME, iterator_s) * iterator);

#ifndef FIO_MAP_EACH
/** Iterates through the map using an iterator object. */
#define FIO_MAP_EACH(map_name, map_ptr, i)                                     \
  for (FIO_NAME(map_name, iterator_s)                                          \
           i = FIO_NAME(map_name, get_next)(map_ptr, NULL);                    \
       FIO_NAME(map_name, iterator_is_valid)(&i);                              \
       i = FIO_NAME(map_name, get_next)(map_ptr, &i))
/** Iterates through the map using an iterator object. */
#define FIO_MAP_EACH_REVERSED(map_name, map_ptr, i)                            \
  for (FIO_NAME(map_name, iterator_s)                                          \
           i = FIO_NAME(map_name, get_prev)(map_ptr, NULL);                    \
       FIO_NAME(map_name, iterator_is_valid)(&i);                              \
       i = FIO_NAME(map_name, get_prev)(map_ptr, &i))
#endif

/** Iteration information structure passed to the callback. */
typedef struct FIO_NAME(FIO_MAP_NAME, each_s) {
  /** The being iterated. Once set, cannot be safely changed. */
  FIO_MAP_PTR const parent;
  /** The current object's index */
  uint64_t index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(struct FIO_NAME(FIO_MAP_NAME, each_s) * info);
  /** Opaque user data. */
  void *udata;
#ifdef FIO_MAP_VALUE
  /** The object's value at the current index. */
  FIO_MAP_VALUE value;
#endif
  /** The object's key the current index. */
  FIO_MAP_KEY key;
} FIO_NAME(FIO_MAP_NAME, each_s);

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an each_s pointer, see above.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
SFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                        each)(FIO_MAP_PTR map,
                              int (*task)(FIO_NAME(FIO_MAP_NAME, each_s) *),
                              void *udata,
                              ssize_t start_at);

/* *****************************************************************************
Optional Sorting Support - TODO? (convert to array, sort, rehash)
***************************************************************************** */

#if defined(FIO_MAP_KEY_IS_GREATER_THAN) && !defined(FIO_SORT_TYPE) &&         \
    FIO_MAP_ORDERED
#undef FIO_SORT_NAME
#endif

/* *****************************************************************************
Map Implementation - inlined static functions
***************************************************************************** */

#ifndef FIO_MAP_CAPA_BITS_LIMIT
/* Note: cannot be more than 31 bits unless some of the code is rewritten. */
#define FIO_MAP_CAPA_BITS_LIMIT 31
#endif

/* Theoretical map capacity. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  FIO_MAP_T *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (o->map)
    return (uint32_t)((size_t)1ULL << o->bits);
  return 0;
}

/* The number of objects in the map capacity. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  return ((FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map))->count;
}

/** Returns 1 if the iterator points to a valid object, otherwise returns 0. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       iterator_is_valid)(FIO_NAME(FIO_MAP_NAME, iterator_s) *
                                          iterator) {
  return (iterator && iterator->private_.map_validator);
}

/** Returns the key value associated with the node's pointer. */
FIO_IFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME,
                               node2key)(FIO_NAME(FIO_MAP_NAME, node_s) *
                                         node) {
  FIO_MAP_KEY r = (FIO_MAP_KEY){0};
  if (!node)
    return r;
  return FIO_MAP_KEY_FROM_INTERNAL(node->key);
}

/** Returns the hash value associated with the node's pointer. */
FIO_IFUNC uint64_t FIO_NAME(FIO_MAP_NAME,
                            node2hash)(FIO_NAME(FIO_MAP_NAME, node_s) * node) {
  uint32_t r = (uint32_t){0};
  if (!node)
    return r;
#if FIO_MAP_RECALC_HASH
  FIO_MAP_KEY k = FIO_MAP_KEY_FROM_INTERNAL(node->key);
  uint64_t hash = FIO_MAP_HASH_FN(k);
  hash += !hash;
  return hash;
#else
  return node->hash;
#endif
}

#ifdef FIO_MAP_VALUE
/** Returns the value associated with the node's pointer. */
FIO_IFUNC FIO_MAP_VALUE FIO_NAME(FIO_MAP_NAME,
                                 node2val)(FIO_NAME(FIO_MAP_NAME, node_s) *
                                           node) {
  FIO_MAP_VALUE r = (FIO_MAP_VALUE){0};
  if (!node)
    return r;
  return FIO_MAP_VALUE_FROM_INTERNAL(node->value);
}
#else
/* If called for a node without a value, returns the key (simplifies stuff). */
FIO_IFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME,
                               node2val)(FIO_NAME(FIO_MAP_NAME, node_s) *
                                         node) {
  return FIO_NAME(FIO_MAP_NAME, node2key)(node);
}
#endif

/** Returns the key value associated with the node's pointer. */
FIO_IFUNC FIO_MAP_KEY_INTERNAL *FIO_NAME(FIO_MAP_NAME, node2key_ptr)(
    FIO_NAME(FIO_MAP_NAME, node_s) * node) {
  if (!node)
    return NULL;
  return &(node->key);
}

#ifdef FIO_MAP_VALUE
/** Returns the value associated with the node's pointer. */
FIO_IFUNC FIO_MAP_VALUE_INTERNAL *FIO_NAME(FIO_MAP_NAME, node2val_ptr)(
    FIO_NAME(FIO_MAP_NAME, node_s) * node) {
  if (!node)
    return NULL;
  return &(node->value);
}
#else
/* If called for a node without a value, returns the key (simplifies stuff). */
FIO_IFUNC FIO_MAP_KEY_INTERNAL *FIO_NAME(FIO_MAP_NAME, node2val_ptr)(
    FIO_NAME(FIO_MAP_NAME, node_s) * node) {
  return FIO_NAME(FIO_MAP_NAME, node2key_ptr)(node);
}
#endif

/** Gets a value from the map, if exists. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                    uint64_t hash,
#endif
                                                    FIO_MAP_KEY key) {
  return FIO_NAME(FIO_MAP_NAME, node2val)(FIO_NAME(FIO_MAP_NAME, get_ptr)(map,
#if !defined(FIO_MAP_HASH_FN)
                                                                          hash,
#endif
                                                                          key));
}

/** Sets a value in the map, hash maps will overwrite existing data if any. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                    uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                                                    FIO_MAP_KEY key,
                                                    FIO_MAP_VALUE obj,
                                                    FIO_MAP_VALUE_INTERNAL *old
#else
                                                    FIO_MAP_KEY key
#endif
) {
  return FIO_NAME(FIO_MAP_NAME, node2val)(FIO_NAME(FIO_MAP_NAME, set_ptr)(map,
#if !defined(FIO_MAP_HASH_FN)
                                                                          hash,
#endif
                                                                          key
#ifdef FIO_MAP_VALUE
                                                                          ,
                                                                          obj,
                                                                          old,
                                                                          1
#endif
                                                                          ));
}

/** Sets a value in the map if not set previously. */
FIO_IFUNC FIO_MAP_GET_T FIO_NAME(FIO_MAP_NAME, set_if_missing)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                                               uint64_t hash,
#endif
                                                               FIO_MAP_KEY key
#ifdef FIO_MAP_VALUE
                                                               ,
                                                               FIO_MAP_VALUE obj
#endif
) {
  return FIO_NAME(FIO_MAP_NAME, node2val)(FIO_NAME(FIO_MAP_NAME, set_ptr)(map,
#if !defined(FIO_MAP_HASH_FN)
                                                                          hash,
#endif
                                                                          key
#ifdef FIO_MAP_VALUE
                                                                          ,
                                                                          obj,
                                                                          NULL,
                                                                          0
#endif
                                                                          ));
}

/** Returns a pointer to the node object in the internal map. */
FIO_IFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME,
             iterator2node)(FIO_MAP_PTR map,
                            FIO_NAME(FIO_MAP_NAME, iterator_s) * iterator) {
  FIO_NAME(FIO_MAP_NAME, node_s) *node = NULL;
  if (!iterator || !iterator->private_.map_validator)
    return node;
  FIO_PTR_TAG_VALID_OR_RETURN(map, node);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  node = o->map + iterator->private_.index;
  return node;
}

/* *****************************************************************************
Map Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_MAP_NAME, s))
FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_MAP_NAME, destroy))
/* *****************************************************************************
Constructors
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void) {
  FIO_NAME(FIO_MAP_NAME, s) *o =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*o), 0);
  if (!o)
    return (FIO_MAP_PTR)NULL;
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MAP_NAME, s));
  *o = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
  return (FIO_MAP_PTR)FIO_PTR_TAG(o);
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, destroy)(map);
  FIO_NAME(FIO_MAP_NAME, s) *o =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_MAP_NAME, s), map);
  FIO_MEM_FREE_(o, sizeof(*o));
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MAP_NAME, s));
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* *****************************************************************************
Internal Helpers
***************************************************************************** */

#ifndef FIO_MAP_ATTACK_LIMIT
#define FIO_MAP_ATTACK_LIMIT 16
#endif
#ifndef FIO_MAP_CUCKOO_STEPS
/* Prime numbers are better */
#define FIO_MAP_CUCKOO_STEPS (0x43F82D0BUL) /* a big high prime */
#endif
#ifndef FIO_MAP_SEEK_LIMIT
#define FIO_MAP_SEEK_LIMIT 13U
#endif
#ifndef FIO_MAP_ARRAY_LOG_LIMIT
#define FIO_MAP_ARRAY_LOG_LIMIT 3
#endif
#ifndef FIO_MAP_CAPA
#define FIO_MAP_CAPA(bits) ((size_t)1ULL << bits)
#endif

#ifndef FIO_MAP_IS_SPARSE
#define FIO_MAP_IS_SPARSE(map)                                                 \
  (o->bits > FIO_MAP_ARRAY_LOG_LIMIT && ((capa >> 2) > o->count))
#endif

/* The number of objects in the map capacity. */
FIO_IFUNC uint8_t *FIO_NAME(FIO_MAP_NAME,
                            __imap)(FIO_NAME(FIO_MAP_NAME, s) * o) {
  // FIO_ASSERT(o && o->map, "shouldn't have been called.");
  return (uint8_t *)(o->map + FIO_MAP_CAPA(o->bits));
}

FIO_IFUNC uint64_t FIO_NAME(FIO_MAP_NAME,
                            __byte_hash)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                         uint64_t hash) {
  hash = (hash >> o->bits);
  hash &= 0xFF;
  hash += !(hash);
  hash -= (hash == 255);
  return hash;
}

FIO_IFUNC uint64_t FIO_NAME(FIO_MAP_NAME,
                            __is_eq_hash)(FIO_NAME(FIO_MAP_NAME, node_s) * o,
                                          uint64_t hash) {
#if FIO_MAP_RECALC_HASH && defined(FIO_MAP_HASH_FN)
  uint64_t khash = FIO_MAP_HASH_FN(FIO_MAP_KEY_FROM_INTERNAL(o->key));
  khash += !khash;
#else
  const uint64_t khash = o->hash;
#endif
  return (khash == hash);
}

FIO_SFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                            __index)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                     FIO_MAP_KEY key,
                                     uint64_t hash) {
  uint32_t r = (uint32_t)-1;
  if (!o->map)
    return r;
  static int guard_print = 0;
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  size_t capa = FIO_MAP_CAPA(o->bits);
  size_t bhash = FIO_NAME(FIO_MAP_NAME, __byte_hash)(o, hash);
  size_t guard = FIO_MAP_ATTACK_LIMIT + 1;
  if (o->bits > FIO_MAP_ARRAY_LOG_LIMIT) { /* treat as map */
    uint64_t bhash64 = bhash | (bhash << 8);
    bhash64 |= bhash64 << 16;
    bhash64 |= bhash64 << 32;
    bhash64 = ~bhash64;
    const uintptr_t pos_mask = capa - 1;
    const uint_fast8_t offsets[8] = {0, 3, 8, 17, 28, 41, 58, 60};
    for (uintptr_t pos = hash, c = 0; c < FIO_MAP_SEEK_LIMIT;
         (pos += FIO_MAP_CUCKOO_STEPS), ++c) {
      uint64_t comb = imap[(pos + offsets[0]) & pos_mask];
      comb |= ((uint64_t)imap[(pos + offsets[1]) & pos_mask]) << (1 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[2]) & pos_mask]) << (2 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[3]) & pos_mask]) << (3 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[4]) & pos_mask]) << (4 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[5]) & pos_mask]) << (5 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[6]) & pos_mask]) << (6 * 8);
      comb |= ((uint64_t)imap[(pos + offsets[7]) & pos_mask]) << (7 * 8);
      const uint64_t has_possible_match =
          (((comb ^ bhash64) & 0x7F7F7F7F7F7F7F7FULL) + 0x0101010101010101ULL) &
          0x8080808080808080ULL;
      if (has_possible_match) {
        /* there was a 7 bit match in one of the bytes in this 8 byte group */
        for (size_t i = 0; i < 8; ++i) {
          const uint32_t tmp = (uint32_t)((pos + offsets[i]) & pos_mask);
          if (imap[tmp] != bhash)
            continue;
          /* test key and hash equality */
          if (FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + tmp, hash)) {
            if (FIO_MAP_KEY_CMP(o->map[tmp].key, key)) {
              guard_print = 0;
              return (r = tmp);
            }
            if (!(--guard)) {
              if (!guard_print)
                FIO_LOG_SECURITY("hash map " FIO_MACRO2STR(
                    FIO_NAME(FIO_MAP_NAME, s)) " under attack?");
              guard_print = 1;
              return (r = tmp);
            }
          }
        }
      }
      const uint64_t has_possible_full_byte =
          (((comb)&0x7F7F7F7F7F7F7F7FULL) + 0x0101010101010101ULL) &
          0x8080808080808080ULL;
      const uint64_t has_possible_empty_byte =
          (((~comb) & 0x7F7F7F7F7F7F7F7FULL) + 0x0101010101010101ULL) &
          0x8080808080808080ULL;
      if (!(has_possible_full_byte | has_possible_empty_byte))
        continue;
      /* there was a 7 bit match for a possible free space in this group */
      for (int i = 0; i < 8; ++i) {
        const uint32_t tmp = (uint32_t)((pos + offsets[i]) & pos_mask);
        if (!imap[tmp])
          return (r = tmp); /* empty slot always ends search */
        if (r > pos_mask && imap[tmp] == 255)
          r = tmp; /* mark hole to be filled */
      }
    }
    return r;
  } /* treat as array */
  for (size_t i = 0; i < capa; ++i) {
    if (!imap[i])
      return (r = (uint32_t)i);
    if (imap[i] == bhash) {
      /* test key and hash equality */
      if (FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + i, hash)) {
        if (FIO_MAP_KEY_CMP(o->map[i].key, key)) {
          guard_print = 0;
          return (r = (uint32_t)i);
        }
        if (!(--guard)) {
          if (!guard_print)
            FIO_LOG_SECURITY("hash map " FIO_MACRO2STR(
                FIO_NAME(FIO_MAP_NAME, s)) " under attack?");
          guard_print = 1;
          return (r = (uint32_t)i);
        }
      }
    }
    if (imap[i] == 0xFF)
      r = (uint32_t)i; /* a free spot is available*/
  }
  return r;
}
/* deallocate the map's memory. */
FIO_SFUNC void FIO_NAME(FIO_MAP_NAME,
                        __dealloc_map)(FIO_NAME(FIO_MAP_NAME, s) * o) {
  if (!o->map)
    return;
  const size_t capa = FIO_MAP_CAPA(o->bits);
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MAP_NAME, destroy));
  FIO_MEM_FREE_(o->map, (capa * sizeof(*o->map)) + capa);
  (void)capa;
}

/** duplicates an objects between two maps. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       __copy_obj)(FIO_NAME(FIO_MAP_NAME, s) * dest,
                                   FIO_NAME(FIO_MAP_NAME, node_s) * o,
                                   uint32_t internal) {
  FIO_MAP_KEY key = FIO_MAP_KEY_FROM_INTERNAL(o->key);
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(dest);
#if FIO_MAP_RECALC_HASH
  uint64_t ohash = FIO_MAP_HASH_FN(key);
  ohash += !ohash;
#else
  const uint64_t ohash = o->hash;
#endif
  uint32_t i = FIO_NAME(FIO_MAP_NAME, __index)(dest, key, ohash);
  if (i == (uint32_t)-1 || (imap[i] + 1) > 1)
    return -1;
  if (internal) {
    dest->map[i] = *o;
    imap[i] = FIO_NAME(FIO_MAP_NAME, __byte_hash)(dest, ohash);
#if FIO_MAP_ORDERED
    if (dest->count) { /* update ordering */
      FIO_INDEXED_LIST_PUSH(dest->map, node, dest->head, i);
    } else { /* set first order */
      dest->map[i].node.next = dest->map[i].node.prev = i;
      dest->head = i;
    }
#endif
    ++dest->count;
    return 0;
  }
  imap[i] = FIO_NAME(FIO_MAP_NAME, __byte_hash)(dest, ohash);
  FIO_MAP_KEY_COPY(dest->map[i].key, FIO_MAP_KEY_FROM_INTERNAL(o->key));
  FIO_MAP_VALUE_COPY(dest->map[i].value, FIO_MAP_VALUE_FROM_INTERNAL(o->value));
#if !FIO_MAP_RECALC_HASH
  dest->map[i].hash = o->hash;
#endif
#if FIO_MAP_ORDERED
  if (dest->count) { /* update ordering */
    FIO_INDEXED_LIST_PUSH(dest->map, node, dest->head, i);
  } else { /* set first order */
    dest->map[i].node.next = dest->map[i].node.prev = i;
    dest->head = i;
  }
#endif
  ++dest->count;
  return 0;
}

/** duplicates a map to a new copy (usually for rehashing / reserving space). */
FIO_IFUNC FIO_NAME(FIO_MAP_NAME, s)
    FIO_NAME(FIO_MAP_NAME, __duplicate)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                        uint32_t bits,
                                        uint32_t internal) {
  FIO_NAME(FIO_MAP_NAME, s) cpy = {0};
  if (bits > FIO_MAP_CAPA_BITS_LIMIT)
    return cpy;
  size_t capa = FIO_MAP_CAPA(bits);
  cpy.map = (FIO_NAME(FIO_MAP_NAME, node_s) *)
      FIO_MEM_REALLOC_(NULL, 0, ((capa * sizeof(*cpy.map)) + capa), 0);
  if (!cpy.map)
    return cpy;
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MAP_NAME, destroy));
  if (!FIO_MEM_REALLOC_IS_SAFE_) {
    /* set only the imap, the rest can be junk data */
    FIO_MEMSET((cpy.map + capa), 0, capa);
  }
  cpy.bits = bits;
  if (!o->count)
    return cpy;
#if FIO_MAP_ORDERED
  /* copy objects in order */
  FIO_INDEXED_LIST_EACH(o->map, node, o->head, i) {
    if (FIO_NAME(FIO_MAP_NAME, __copy_obj)(&cpy, o->map + i, internal))
      goto error;
  }
#else
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  capa = FIO_MAP_CAPA(o->bits);
  if (FIO_MAP_IS_SPARSE(o)) { /* sparsely populated */
    for (size_t i = 0; i < capa; i += 8) {
      uint64_t comb = *((uint64_t *)(imap + i));
      if (!comb || comb == 0xFFFFFFFFFFFFFFFFULL)
        continue;
      for (size_t j = 0; j < 8; ++j) {
        const size_t tmp = j + i;
        if (!imap[tmp] || imap[tmp] == 0xFF)
          continue;
        if (FIO_NAME(FIO_MAP_NAME, __copy_obj)(&cpy, o->map + tmp, internal))
          goto error;
      }
    }
    return cpy;
  } /* review as array */
  for (size_t i = 0; i < capa; ++i) {
    if (!imap[i] || imap[i] == 0xFF)
      continue;
    if (FIO_NAME(FIO_MAP_NAME, __copy_obj)(&cpy, o->map + i, internal))
      goto error;
  }
#endif
  return cpy;
error:
  FIO_NAME(FIO_MAP_NAME, __dealloc_map)(&cpy);
  cpy = (FIO_NAME(FIO_MAP_NAME, s)){0};
  return cpy;
}

/* destroys all objects in the map, without(!) resetting the `imap`. */
FIO_SFUNC void FIO_NAME(FIO_MAP_NAME,
                        __destroy_objects)(FIO_NAME(FIO_MAP_NAME, s) * o) {
#if FIO_MAP_VALUE_DESTROY_SIMPLE && FIO_MAP_KEY_DESTROY_SIMPLE
  (void)o;
  return;
#else
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  const size_t capa = FIO_MAP_CAPA(o->bits);
  if (FIO_MAP_IS_SPARSE(o)) {
    for (size_t i = 0; i < capa; i += 8) {
      uint64_t comb = *((uint64_t *)(imap + i));
      if (!comb || comb == 0xFFFFFFFFFFFFFFFFULL)
        continue;
      for (size_t j = i; j < i + 8; ++j) {
        FIO_MAP_KEY_DESTROY(o->map[j].key);
        FIO_MAP_VALUE_DESTROY(o->map[j].value);
      }
    }
  } else { /* review as array */
    for (size_t i = 0; i < capa; ++i) {
      if (!imap[i] || imap[i] == 0xFF)
        continue;
      FIO_MAP_KEY_DESTROY(o->map[i].key);
      FIO_MAP_VALUE_DESTROY(o->map[i].value);
    }
  }
#endif /* FIO_MAP_VALUE_DESTROY_SIMPLE */
}

/* *****************************************************************************
API implementation
***************************************************************************** */

/** Reserves at minimum the capacity requested. */
SFUNC void FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_PTR map, size_t capa) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  capa += o->count;
  if (FIO_MAP_CAPA(o->bits) >= capa || (capa >> FIO_MAP_CAPA_BITS_LIMIT))
    return;
  uint_fast8_t bits = o->bits + 1;
  while (FIO_MAP_CAPA(bits) < capa)
    ++bits;
  FIO_NAME(FIO_MAP_NAME, s)
  cpy = FIO_NAME(FIO_MAP_NAME, __duplicate)(o, bits, 1);
  if (!cpy.map)
    return;
  FIO_NAME(FIO_MAP_NAME, __dealloc_map)(o);
  *o = cpy;
}

/* Removes all objects from the map, without releasing the map's resources. */
SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->map || !o->count)
    return;
  FIO_NAME(FIO_MAP_NAME, __destroy_objects)(o);
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  const size_t capa = FIO_MAP_CAPA(o->bits);
  FIO_MEMSET(imap, 0, capa);
  o->count = 0;
#if FIO_MAP_ORDERED
  o->head = 0;
#endif
}

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->map || !o->count)
    return;
  uint32_t bits = o->bits;
  while ((bits >> 1) > o->count)
    bits >>= 1;
  for (;;) {
    if (bits == o->bits)
      return;
    FIO_NAME(FIO_MAP_NAME, s)
    cpy = FIO_NAME(FIO_MAP_NAME, __duplicate)(o, bits, 1);
    if (!cpy.map) {
      ++bits;
      continue;
    }
    FIO_NAME(FIO_MAP_NAME, __dealloc_map)(o);
    *o = cpy;
    return;
  }
}

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (o->map && o->count)
    FIO_NAME(FIO_MAP_NAME, __destroy_objects)(o);
  FIO_NAME(FIO_MAP_NAME, __dealloc_map)(o);
  *o = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
  return;
}

/** Evicts elements least recently used (LRU), FIFO or undefined. */
SFUNC void FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR map,
                                         size_t number_of_elements) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->count)
    return;
  if (number_of_elements >= o->count) {
    FIO_NAME(FIO_MAP_NAME, clear)(map);
    return;
  }
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
#ifdef FIO_MAP_LRU /* remove last X elements from the list */
  FIO_INDEXED_LIST_EACH_REVERSED(o->map, node, o->head, i) {
    FIO_MAP_KEY_DESTROY(o->map[i].key);
    FIO_MAP_VALUE_DESTROY(o->map[i].value);
    FIO_INDEXED_LIST_REMOVE(o->map, node, i);
    imap[i] = 0xFF;
    --o->count;
    if (!(--number_of_elements))
      return;
  }
#elif FIO_MAP_ORDERED /* remove first X elements from the list */
  FIO_INDEXED_LIST_EACH(o->map, node, o->head, i) {
    FIO_MAP_KEY_DESTROY(o->map[i].key);
    FIO_MAP_VALUE_DESTROY(o->map[i].value);
    FIO_INDEXED_LIST_REMOVE(o->map, node, i);
    imap[i] = 0xFF;
    --o->count;
    if (!(--number_of_elements)) {
      o->head = o->map[i].node.next;
      return;
    }
  }
#else                 /* remove whatever... */
  if (o->bits > FIO_MAP_ARRAY_LOG_LIMIT) {
    /* map is scattered */
    uint32_t pos_mask = (uint32_t)(FIO_MAP_CAPA(o->bits) - 1);
    uint32_t pos = *(uint32_t *)o->map;
    for (int i = 0; i < 3; ++i) {
      struct timespec t = {0};
      clock_gettime(CLOCK_MONOTONIC, &t);
      pos *= t.tv_nsec ^ t.tv_sec ^ (uintptr_t)imap;
      pos ^= pos >> 7;
    }
    for (;;) { /* a bit of non-random randomness... */
      uint32_t offset = ((pos << 3)) & pos_mask;
      for (uint_fast8_t i = 0; i < 8; ++i) { /* ordering bias? vs performance */
        const uint32_t tmp = offset + i;
        if (!imap[tmp] || imap[tmp] == 0xFF)
          continue;
        FIO_MAP_KEY_DESTROY(o->map[tmp].key);
        FIO_MAP_VALUE_DESTROY(o->map[tmp].value);
        imap[tmp] = 0xFF;
        --o->count;
        if (!(--number_of_elements))
          return;
      }
      pos += FIO_MAP_CUCKOO_STEPS;
    }
  }
  /* map is a simple array */
  while (number_of_elements--) {
    FIO_MAP_KEY_DESTROY(o->map[number_of_elements].key);
    FIO_MAP_VALUE_DESTROY(o->map[number_of_elements].value);
    imap[number_of_elements] = 0xFF;
  }
#endif                /* FIO_MAP_LRU / FIO_MAP_ORDERED */
}

/* *****************************************************************************
The Map set/get functions
***************************************************************************** */

/**
 * The core get function. This function returns NULL if item is missing.
 *
 * NOTE: the function returns the internal representation of objects.
 */
SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                    uint64_t hash,
#endif
                                    FIO_MAP_KEY key) {
  FIO_NAME(FIO_MAP_NAME, node_s) *r = NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->count)
    return r;
#if defined(FIO_MAP_HASH_FN)
  uint64_t hash = FIO_MAP_HASH_FN(key);
#endif
  hash += !hash;
  uint32_t pos = FIO_NAME(FIO_MAP_NAME, __index)(o, key, hash);
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  if (pos == (uint32_t)-1 || !imap[pos] || imap[pos] == 0xFF)
    return r;
#ifdef FIO_MAP_LRU
  if (o->head != pos) {
    FIO_INDEXED_LIST_REMOVE(o->map, node, pos);
    FIO_INDEXED_LIST_PUSH(o->map, node, o->head, pos);
    o->head = pos;
  }
#endif
  r = o->map + pos;
  return r;
}

/** sets / removes an object in the map, returning a pointer to the map data. */
SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, set_ptr)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                    uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                                    FIO_MAP_KEY key,
                                    FIO_MAP_VALUE val,
                                    FIO_MAP_VALUE_INTERNAL *old,
                                    int overwrite
#else
                                    FIO_MAP_KEY key
#endif
    ) {
  FIO_NAME(FIO_MAP_NAME, node_s) *r = NULL;
#ifdef FIO_MAP_VALUE
  if (old)
    *old = (FIO_MAP_VALUE_INTERNAL){0};
#endif
  FIO_NAME(FIO_MAP_NAME, s) * o;
#if defined(FIO_MAP_HASH_FN)
  uint64_t hash;
#endif
  uint32_t pos;
  uint8_t *imap = NULL;

  FIO_PTR_TAG_VALID_OR_GOTO(map, relinquish_attempt);
  o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
#if defined(FIO_MAP_HASH_FN)
  hash = FIO_MAP_HASH_FN(key);
#endif
  hash += !hash; /* hash is never zero */
  /* find the object's (potential) position in the array */
  for (int i = 0;;) {
    pos = FIO_NAME(FIO_MAP_NAME, __index)(o, key, hash);
    if (pos != (uint32_t)-1)
      break;
    if (i == 2)
      goto internal_error;
    FIO_NAME(FIO_MAP_NAME, s)
    tmp = FIO_NAME(FIO_MAP_NAME, __duplicate)(o, o->bits + (++i), 1);
    if (!tmp.map) /* no memory? something bad? */
      goto internal_error;
    FIO_NAME(FIO_MAP_NAME, __dealloc_map)(o);
    *o = tmp;
  }
  /* imap may have been reallocated, collect info now. */
  imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  /* set return value */
  r = o->map + pos;

  if (!imap[pos] || imap[pos] == 0xFF) {
    /* insert new object */
    imap[pos] = FIO_NAME(FIO_MAP_NAME, __byte_hash)(o, hash);
#if !FIO_MAP_RECALC_HASH
    r->hash = hash;
#endif
    FIO_MAP_KEY_COPY(r->key, key);
    FIO_MAP_VALUE_COPY(r->value, val);
#if FIO_MAP_ORDERED
    if (o->count) { /* update ordering */
      FIO_INDEXED_LIST_PUSH(o->map, node, o->head, pos);
#ifdef FIO_MAP_LRU
      o->head = pos;                 /* update LRU head */
      if (FIO_MAP_LRU == o->count) { /* limit reached - evict 1 LRU element */
        uint32_t to_evict = o->map[pos].node.prev;
        FIO_MAP_KEY_DESTROY(o->map[to_evict].key);
        FIO_MAP_VALUE_DESTROY(o->map[to_evict].value);
        FIO_INDEXED_LIST_REMOVE(o->map, node, to_evict);
        imap[to_evict] = 0xFF;
        --o->count;
      }
#endif       /* FIO_MAP_LRU */
    } else { /* set first order */
      o->map[pos].node.next = o->map[pos].node.prev = pos;
      o->head = pos;
    }
#endif /* FIO_MAP_ORDERED */
    ++o->count;
    return r;
  }

#ifdef FIO_MAP_LRU
  /* update ordering (even if not overwriting) */
  if (o->head != pos) {
    FIO_INDEXED_LIST_REMOVE(o->map, node, pos);
    FIO_INDEXED_LIST_PUSH(o->map, node, o->head, pos);
    o->head = pos;
  }
#endif

#ifdef FIO_MAP_VALUE
  if (overwrite) {
    /* overwrite existing object (only relevant for hash maps) */
    FIO_MAP_KEY_DISCARD(key);
    if (!old) {
      FIO_MAP_VALUE_DESTROY(o->map[pos].value);
      FIO_MAP_VALUE_COPY(o->map[pos].value, val);
      return r;
    }
    *old = o->map[pos].value;
    o->map[pos].value = (FIO_MAP_VALUE_INTERNAL){0};
    FIO_MAP_VALUE_COPY(o->map[pos].value, val);
    return r;
  }
#endif
relinquish_attempt:
  /* discard attempt */
  FIO_MAP_KEY_DISCARD(key);
  FIO_MAP_VALUE_DISCARD(val);
  return r;
internal_error:
  FIO_MAP_KEY_DISCARD(key);
  FIO_MAP_VALUE_DISCARD(val);
  FIO_LOG_ERROR("unknown error occurred trying to add an entry to the map");
  FIO_ASSERT_DEBUG(0, "these errors shouldn't happen");
  return r;
}

/* *****************************************************************************
The Map remove function
***************************************************************************** */

/** Removes an object in the map, returning a pointer to the map data. */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                         uint64_t hash,
#endif
                                         FIO_MAP_KEY key,
#ifdef FIO_MAP_VALUE
                                         FIO_MAP_VALUE_INTERNAL *old
#else
                                         FIO_MAP_KEY_INTERNAL *old
#endif
) {
#ifdef FIO_MAP_VALUE
  if (old)
    *old = (FIO_MAP_VALUE_INTERNAL){0};
#else
  if (old)
    *old = (FIO_MAP_KEY_INTERNAL){0};
#endif

  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
#if defined(FIO_MAP_HASH_FN)
  uint64_t hash = FIO_MAP_HASH_FN(key);
#endif
  hash += !hash; /* hash is never zero */
  uint32_t pos = FIO_NAME(FIO_MAP_NAME, __index)(o, key, hash);
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);

  if (pos == (uint32_t)-1 || !imap[pos] || imap[pos] == 0xFF)
    return -1;

  imap[pos] = 0xFF; /* mark hole and update count */
  --o->count;

#if FIO_MAP_ORDERED
  /* update ordering */
  if (o->head == pos)
    o->head = o->map[pos].node.next;
  if (o->head == pos)
    o->head = 0;
  else {
    FIO_INDEXED_LIST_REMOVE(o->map, node, pos);
  }
#endif

/* destroy data, copy to `old` pointer if necessary. */
#ifdef FIO_MAP_VALUE
  FIO_MAP_KEY_DESTROY(o->map[pos].key);
  o->map[pos].key = (FIO_MAP_KEY_INTERNAL){0};
  if (!old) {
    FIO_MAP_VALUE_DESTROY(o->map[pos].value);
  } else {
    *old = o->map[pos].value;
  }
  o->map[pos].value = (FIO_MAP_VALUE_INTERNAL){0};
#else
  if (!old) {
    FIO_MAP_KEY_DESTROY(o->map[pos].key);
  } else {
    *old = o->map[pos].key;
  }
  o->map[pos].key = (FIO_MAP_KEY_INTERNAL){0};
#endif
#if !FIO_MAP_RECALC_HASH && defined(DEBUG)
  o->map[pos].hash = 0; /* not necessary, but ... good for debugging? */
#endif
  return 0;
}

/* *****************************************************************************
Map Iteration
***************************************************************************** */

/** Returns the next iterator position after `current_pos`, first if `NULL`. */
SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME,
             get_next)(FIO_MAP_PTR map,
                       FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos) {
  FIO_NAME(FIO_MAP_NAME, iterator_s) r = {.private_ = {.pos = 0}};
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->count)
    return r;
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  size_t capa = FIO_MAP_CAPA(o->bits);
  size_t pos_counter = 0;
  if (!current_pos || !current_pos->private_.map_validator) {
    goto find_pos;
  }
  if (current_pos->private_.pos + 1 == o->count)
    return r;
  r.private_.pos = current_pos->private_.pos + 1;
  if (current_pos->private_.map_validator != (uintptr_t)o) {
    goto refind_pos;
  }
  r.private_.index = current_pos->private_.index;

#if !FIO_MAP_RECALC_HASH
#define FIO_MAP___EACH_COPY_HASH() r.hash = o->map[r.private_.index].hash
#else
#define FIO_MAP___EACH_COPY_HASH()
#endif

#ifdef FIO_MAP_VALUE
#define FIO_MAP___EACH_COPY_DATA()                                             \
  FIO_MAP___EACH_COPY_HASH();                                                  \
  r.private_.map_validator = (uintptr_t)o;                                     \
  r.node = o->map + r.private_.index;                                          \
  r.key = FIO_MAP_KEY_FROM_INTERNAL(o->map[r.private_.index].key);             \
  r.value = FIO_MAP_VALUE_FROM_INTERNAL(o->map[r.private_.index].value)
#else
#define FIO_MAP___EACH_COPY_DATA()                                             \
  FIO_MAP___EACH_COPY_HASH();                                                  \
  r.private_.map_validator = (uintptr_t)o;                                     \
  r.node = o->map + r.private_.index;                                          \
  r.key = FIO_MAP_KEY_FROM_INTERNAL(o->map[r.private_.index].key)
#endif

/* start seeking at the position inherited from current_pos */
#if FIO_MAP_ORDERED
  (void)imap; /* unused in ordered maps */
  (void)capa; /* unused in ordered maps */
  r.private_.index = o->map[r.private_.index].node.next;
  if (r.private_.index == o->head)
    goto not_found;
  FIO_MAP___EACH_COPY_DATA();
  return r;
#else
  if (FIO_MAP_IS_SPARSE(o)) { /* sparsely populated */
    while ((++r.private_.index) & 7) {
      if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
        continue;
      FIO_MAP___EACH_COPY_DATA();
      return r;
    }
    while (r.private_.index < capa) {
      uint64_t simd = *(uint64_t *)(imap + r.private_.index);
      if (!simd || simd == 0xFFFFFFFFFFFFFFFFULL) {
        r.private_.index += 8;
        continue;
      }
      for (int i = 0; i < 8; (++i), (++r.private_.index)) {
        if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
          continue;
        FIO_MAP___EACH_COPY_DATA();
        return r;
      }
    }
    goto not_found;
  }
  /* review as array */
  while ((++r.private_.index) < capa) {
    if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
      continue;
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
  goto not_found;
#endif /* FIO_MAP_ORDERED */

refind_pos:
  if (current_pos->private_.index)
    goto not_found;
find_pos:
/* first seek... re-start seeking */
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST_EACH(o->map, node, o->head, i) {
    if (pos_counter != r.private_.pos) {
      ++pos_counter;
      continue;
    }
    r.private_.index = (uint32_t)i;
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
  goto not_found;
#else
  if (FIO_MAP_IS_SPARSE(o)) { /* sparsely populated */
    while (r.private_.index < capa) {
      uint64_t simd = *(uint64_t *)(imap + r.private_.index);
      if (!simd || simd == 0xFFFFFFFFFFFFFFFFULL) {
        r.private_.index += 8;
        continue;
      }
      for (int i = 0; i < 8; (++i), (++r.private_.index)) {
        if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
          continue;
        if (pos_counter != r.private_.pos) {
          ++pos_counter;
          continue;
        }
        FIO_MAP___EACH_COPY_DATA();
        return r;
      }
    }
    goto not_found;
  }
  /* review as array */
  while (r.private_.index < capa) {
    if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF) {
      ++r.private_.index;
      continue;
    }
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
#endif /* FIO_MAP_ORDERED */

not_found:
  return (r = (FIO_NAME(FIO_MAP_NAME, iterator_s)){.private_ = {.pos = 0}});
  FIO_ASSERT_DEBUG(0, "should this happen? ever?");
}

/** Returns the next iterator position after `current_pos`, first if `NULL`. */
SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME,
             get_prev)(FIO_MAP_PTR map,
                       FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos) {
  FIO_NAME(FIO_MAP_NAME, iterator_s) r = {.private_ = {.pos = 0}};
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->count)
    return r;
#if !FIO_MAP_ORDERED
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  size_t capa = FIO_MAP_CAPA(o->bits);
#endif
  size_t pos_counter = o->count;
  if (!current_pos || !current_pos->private_.map_validator) {
    r.private_.map_validator = (uintptr_t)o;
    r.private_.pos = o->count;
    goto find_pos;
  }
  if (!current_pos->private_.pos)
    return r;
  r.private_.pos = current_pos->private_.pos - 1;
  r.private_.map_validator = (uintptr_t)o;
  if (current_pos->private_.map_validator != (uintptr_t)o) {
    goto refind_pos;
  }
  r.private_.index = current_pos->private_.index;

/* start seeking at the position inherited from current_pos */
#if FIO_MAP_ORDERED
  if (r.private_.index == o->head)
    goto not_found;
  r.private_.index = o->map[r.private_.index].node.prev;
  FIO_MAP___EACH_COPY_DATA();
  return r;
#else
  if (FIO_MAP_IS_SPARSE(o)) { /* sparsely populated */
    while ((--r.private_.index) & 7) {
      if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
        continue;
      FIO_MAP___EACH_COPY_DATA();
      return r;
    }
    while (r.private_.index) {
      uint64_t simd = *(uint64_t *)(imap + (r.private_.index - 8));
      if (!simd || simd == 0xFFFFFFFFFFFFFFFFULL) {
        r.private_.index -= 8;
        continue;
      }
      for (int i = 0; i < 8; ++i) {
        --r.private_.index;
        if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
          continue;
        FIO_MAP___EACH_COPY_DATA();
        return r;
      }
    }
    goto not_found;
  }
  /* review as array */
  while (r.private_.index--) {
    if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
      continue;
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
  goto not_found;
#endif /* FIO_MAP_ORDERED */

refind_pos:
  if (current_pos->private_.index)
    goto not_found;
find_pos:
/* first seek... re-start seeking */
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST_EACH_REVERSED(o->map, node, o->head, i) {
    if (pos_counter != r.private_.pos) {
      --pos_counter;
      continue;
    }
    r.private_.index = (uint32_t)i;
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
  goto not_found;
#else
  r.private_.index = (uint32_t)capa;
  if (FIO_MAP_IS_SPARSE(o)) { /* sparsely populated */
    while (r.private_.index) {
      uint64_t simd = *(uint64_t *)(imap + r.private_.index);
      if (!simd || simd == 0xFFFFFFFFFFFFFFFFULL) {
        r.private_.index -= 8;
        continue;
      }
      for (int i = 0; i < 8; (++i), (--r.private_.index)) {
        if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
          continue;
        if (pos_counter != r.private_.pos) {
          ++pos_counter;
          continue;
        }
        FIO_MAP___EACH_COPY_DATA();
        return r;
      }
    }
    goto not_found;
  }
  /* review as array */
  while ((r.private_.index--)) {
    if (!imap[r.private_.index] || imap[r.private_.index] == 0xFF)
      continue;
    FIO_MAP___EACH_COPY_DATA();
    return r;
  }
#endif /* FIO_MAP_ORDERED */

not_found:
  return (r = (FIO_NAME(FIO_MAP_NAME, iterator_s)){.private_ = {.pos = 0}});
  FIO_ASSERT_DEBUG(0, "should this happen? ever?");
}
#undef FIO_MAP___EACH_COPY_HASH
#undef FIO_MAP___EACH_COPY_DATA

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an each_s pointer, see above.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
SFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                        each)(FIO_MAP_PTR map,
                              int (*task)(FIO_NAME(FIO_MAP_NAME, each_s) *),
                              void *udata,
                              ssize_t start_at) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, 1);
  FIO_NAME(FIO_MAP_NAME, each_s)
  e = {
      .parent = map,
      .task = task,
      .udata = udata,
  };
  FIO_NAME(FIO_MAP_NAME, s) *o =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_MAP_NAME, s), map);
  if (start_at < 0) {
    start_at += o->count;
    if (start_at < 0)
      start_at = 0;
  } else if (start_at > o->count)
    return o->count;
  FIO_NAME(FIO_MAP_NAME, iterator_s) i = {.private_ = {.pos = 0}};
  for (;;) {
    i = FIO_NAME(FIO_MAP_NAME, get_next)(map, &i);
    if (!FIO_NAME(FIO_MAP_NAME, iterator_is_valid)(&i))
      return o->count;
    e.index = i.private_.pos;
    e.key = i.key;
#ifdef FIO_MAP_VALUE
    e.value = i.value;
#endif
    if (e.task(&e))
      return (uint32_t)(e.index + 1);
  }
  return o->count;
}

/* *****************************************************************************
Speed Testing
***************************************************************************** */

/* *****************************************************************************
Map Testing
***************************************************************************** */
#ifdef FIO_MAP_TEST

#ifdef FIO_MAP_HASH_FN
#define FIO___M_HASH(k)
#else
#define FIO___M_HASH(k) (k),
#endif
#ifdef FIO_MAP_VALUE
#define FIO___M_VAL(v) , (v)
#define FIO___M_OLD    , NULL
#else
#define FIO___M_VAL(v)
#define FIO___M_OLD
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MAP_NAME)(void) {
  /* testing only only works with integer external types */
  fprintf(stderr,
          "* Testing maps with key " FIO_MACRO2STR(
              FIO_MAP_KEY) " (=> " FIO_MACRO2STR(FIO_MAP_VALUE) ").\n");
  { /* test set / get overwrite , FIO_MAP_EACH and evict */
    FIO_NAME(FIO_MAP_NAME, s) map = FIO_MAP_INIT;
    for (size_t i = 1; i < (1UL << (FIO_MAP_ARRAY_LOG_LIMIT + 5)); ++i) {
      FIO_NAME(FIO_MAP_NAME, set)
      (&map, FIO___M_HASH(i) i FIO___M_VAL(i) FIO___M_OLD);
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == i,
                 "map `set` failed? %zu != %zu",
                 (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
                 i);
      for (size_t j = ((i << 2) + 1); j < i; ++j) { /* effects LRU ordering */
        FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, get_ptr)(&map, FIO___M_HASH(j) j) &&
                       FIO_NAME(FIO_MAP_NAME, node2val)(
                           FIO_NAME(FIO_MAP_NAME,
                                    get_ptr)(&map, FIO___M_HASH(j) j)) == j,
                   "map `get` failed? %zu/%zu (%p)",
                   j,
                   i,
                   FIO_NAME(FIO_MAP_NAME, get_ptr)(&map, FIO___M_HASH(j) j));
        FIO_NAME(FIO_MAP_NAME, set)
        (&map, FIO___M_HASH(j) j FIO___M_VAL(j) FIO___M_OLD);
        FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == i,
                   "map `set` added an item that already exists? %zu != %zu",
                   (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
                   i);
      }
    }
    /* test FIO_MAP_EACH and ordering */
    uint32_t count = FIO_NAME(FIO_MAP_NAME, count)(&map);
    uint32_t loop_test = 0;
    FIO_MAP_EACH(FIO_MAP_NAME, &map, i) {
      /* test ordering */
#ifdef FIO_MAP_LRU
      FIO_ASSERT(i.key == (count - loop_test),
                 "map FIO_MAP_EACH LRU ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(count - loop_test));
      ++loop_test;
#elif FIO_MAP_ORDERED
      ++loop_test;
      FIO_ASSERT(i.key == loop_test,
                 "map FIO_MAP_EACH LRU ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(loop_test));
#else
      ++loop_test;
#endif
    }
    FIO_ASSERT(loop_test == count,
               "FIO_MAP_EACH failed to iterate all elements? (%zu != %zu",
               (size_t)loop_test != (size_t)count);
    loop_test = 0;
    FIO_MAP_EACH_REVERSED(FIO_MAP_NAME, &map, i) { ++loop_test; }
    FIO_ASSERT(
        loop_test == count,
        "FIO_MAP_EACH_REVERSED failed to iterate all elements? (%zu != %zu",
        (size_t)loop_test != (size_t)count);
    /* test `evict` while we're here */
    FIO_NAME(FIO_MAP_NAME, evict)(&map, (count >> 1));
    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == (count - (count >> 1)),
               "map `evict` count error %zu != %zu",
               (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
               (size_t)(count - (count >> 1)));
    /* cleanup */
    FIO_NAME(FIO_MAP_NAME, destroy)(&map);
  }
#ifndef FIO_MAP_HASH_FN
  { /* test full collision guard and zero hash*/
    FIO_NAME(FIO_MAP_NAME, s) map = FIO_MAP_INIT;
    fprintf(
        stderr,
        "* Testing full collision guard for " FIO_MACRO2STR(
            FIO_NAME(FIO_MAP_NAME, s)) " - expect SECURITY log messages.\n");
    for (size_t i = 1; i < 4096; ++i) {
      FIO_NAME(FIO_MAP_NAME, set)
      (&map, FIO___M_HASH(0) i FIO___M_VAL(i) FIO___M_OLD);
    }
    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map),
               "zero hash fails insertion?");
    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) <= FIO_MAP_ATTACK_LIMIT,
               "map attack guard failed? %zu != %zu",
               (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
               (size_t)FIO_MAP_ATTACK_LIMIT);
    FIO_NAME(FIO_MAP_NAME, destroy)(&map);
  }
#endif
  { /* test reserve, remove */
    FIO_NAME(FIO_MAP_NAME, s) map = FIO_MAP_INIT;
    FIO_NAME(FIO_MAP_NAME, reserve)(&map, 4096);
    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, capa)(&map) == 4096,
               "map reserve error? %zu != %zu",
               (size_t)FIO_NAME(FIO_MAP_NAME, capa)(&map),
               4096);
    for (size_t i = 1; i < 4096; ++i) {
      FIO_NAME(FIO_MAP_NAME, set)
      (&map, FIO___M_HASH(i) i FIO___M_VAL(i) FIO___M_OLD);
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == i, "insertion failed?");
    }
    for (size_t i = 1; i < 4096; ++i) {
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, get)(&map, FIO___M_HASH(i) i),
                 "key missing?");
      FIO_NAME(FIO_MAP_NAME, remove)
      (&map, FIO___M_HASH(i) i, NULL);
      FIO_ASSERT(!FIO_NAME(FIO_MAP_NAME, get)(&map, FIO___M_HASH(i) i),
                 "map_remove error?");
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == 4095 - i,
                 "map count error after removal? %zu != %zu",
                 (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
                 i);
    }
    FIO_NAME(FIO_MAP_NAME, destroy)(&map);
  }
}
#undef FIO___M_HASH
#undef FIO___M_VAL
#undef FIO___M_OLD

#endif /* FIO_MAP_TEST */
/* *****************************************************************************
Map Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_MAP_ARRAY_LOG_LIMIT
#undef FIO_MAP_ATTACK_LIMIT
#undef FIO_MAP_CAPA
#undef FIO_MAP_CAPA_BITS_LIMIT
#undef FIO_MAP_CUCKOO_STEPS
#undef FIO_MAP_GET_T
#undef FIO_MAP_HASH_FN
#undef FIO_MAP_IS_SPARSE
#undef FIO_MAP_KEY
#undef FIO_MAP_KEY_CMP
#undef FIO_MAP_KEY_COPY
#undef FIO_MAP_KEY_DESTROY
#undef FIO_MAP_KEY_DESTROY_SIMPLE
#undef FIO_MAP_KEY_DISCARD
#undef FIO_MAP_KEY_FROM_INTERNAL
#undef FIO_MAP_KEY_INTERNAL
#undef FIO_MAP_KEY_IS_GREATER_THAN
#undef FIO_MAP_LRU
#undef FIO_MAP_NAME
#undef FIO_MAP_ORDERED
#undef FIO_MAP_PTR
#undef FIO_MAP_RECALC_HASH
#undef FIO_MAP_SEEK_LIMIT
#undef FIO_MAP_T
#undef FIO_MAP_TEST
#undef FIO_MAP_VALUE
#undef FIO_MAP_VALUE_BSTR
#undef FIO_MAP_VALUE_COPY
#undef FIO_MAP_VALUE_DESTROY
#undef FIO_MAP_VALUE_DESTROY_SIMPLE
#undef FIO_MAP_VALUE_DISCARD
#undef FIO_MAP_VALUE_FROM_INTERNAL
#undef FIO_MAP_VALUE_INTERNAL
#undef FIO_OMAP_NAME
#undef FIO_UMAP_NAME

#endif /* FIO_MAP_NAME */
