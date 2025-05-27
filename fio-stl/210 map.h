/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MAP_NAME map       /* Development inclusion - ignore line */
#define FIO_MAP_KEY  size_t    /* Development inclusion - ignore line */
// #define FIO_MAP_VALUE_BSTR     /* Development inclusion - ignore line */
// #define FIO_MAP_ORDERED        /* Development inclusion - ignore line */
#define FIO_MAP_TEST   /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
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
  (dest) = fio_keystr_init((src), FIO_NAME(FIO_MAP_NAME, __key_alloc))
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

#ifndef FIO_MAP_INIT
/* Initialization macro. */
#define FIO_MAP_INIT                                                           \
  { 0 }
#endif

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

/** Destroys the object, re-initializing its container. */
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
#if defined(FIO_MAP_VALUE)
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

#endif /* FIO_MAP_EACH */

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
  return FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map)->count;
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
/* If called for a node without a
 * value, returns the key (simplifies
 * stuff). */
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
/* If called for a node without a
 * value, returns the key (simplifies
 * stuff). */
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

FIO_LEAK_COUNTER_DEF(FIO_NAME(FIO_MAP_NAME, destroy))
/* *****************************************************************************
Constructors
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
FIO_LEAK_COUNTER_DEF(FIO_NAME(FIO_MAP_NAME, s))
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void) {
  FIO_NAME(FIO_MAP_NAME, s) *o =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*o), 0);
  if (!o)
    return (FIO_MAP_PTR)NULL;
  FIO_LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MAP_NAME, s));
  *o = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
  return (FIO_MAP_PTR)FIO_PTR_TAG(o);
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, destroy)(map);
  FIO_NAME(FIO_MAP_NAME, s) *o =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_MAP_NAME, s), map);
  FIO_LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MAP_NAME, s));
  FIO_MEM_FREE_(o, sizeof(*o));
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* *****************************************************************************




Internal Helpers (Core)




***************************************************************************** */

/** internal object data representation */
struct FIO_NAME(FIO_MAP_NAME, __imap_s) {
  uint8_t h[64];
};

#ifndef FIO_MAP_ATTACK_LIMIT
#define FIO_MAP_ATTACK_LIMIT 16
#endif
#ifndef FIO_MAP_MINIMAL_BITS
#define FIO_MAP_MINIMAL_BITS 1
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
#define FIO_MAP_CAPA(bits) ((size_t)1ULL << (bits))
#endif

#ifndef FIO_MAP_IS_SPARSE
#define FIO_MAP_IS_SPARSE(map)                                                 \
  (o->bits > FIO_MAP_ARRAY_LOG_LIMIT && ((capa >> 2) > o->count))
#endif

/* Allocates resources for a new (clean) map. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       __allocate_map)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                       uint32_t bits) {
  if (bits < FIO_MAP_MINIMAL_BITS)
    bits = FIO_MAP_MINIMAL_BITS;
  if (bits > FIO_MAP_CAPA_BITS_LIMIT)
    return -1;
  size_t s = (sizeof(o->map[0]) + 1) << bits;
  FIO_NAME(FIO_MAP_NAME, node_s) *n =
      (FIO_NAME(FIO_MAP_NAME, node_s) *)FIO_MEM_REALLOC_(NULL, 0, s, 0);
  if (!n)
    return -1;
  if (!FIO_MEM_REALLOC_IS_SAFE_) /* set only imap to zero */
    FIO_MEMSET((n + (1ULL << bits)), 0, (1ULL << bits));
  *o = (FIO_NAME(FIO_MAP_NAME, s)){.map = n, .bits = bits};
  FIO_LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MAP_NAME, destroy));
  return 0;
}

/* The number of objects in the map capacity. */
FIO_IFUNC uint8_t *FIO_NAME(FIO_MAP_NAME,
                            __imap)(FIO_NAME(FIO_MAP_NAME, s) const *o) {
  return (uint8_t *)(o->map + FIO_MAP_CAPA(o->bits));
}

FIO_IFUNC uint64_t FIO_NAME(FIO_MAP_NAME, __byte_hash)(uint64_t hash) {
  hash = (hash >> 48) ^ (hash >> 56);
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

typedef struct FIO_NAME(FIO_MAP_NAME, __each_node_s) {
  FIO_NAME(FIO_MAP_NAME, s) * map;
  FIO_NAME(FIO_MAP_NAME, node_s) * node;
  int (*fn)(struct FIO_NAME(FIO_MAP_NAME, __each_node_s) *);
  void *udata;
} FIO_NAME(FIO_MAP_NAME, __each_node_s);

void fio___map___each_node___(void); /* IDE Marker */
/* perform task for each node. */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       __each_node)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                    int (*fn)(FIO_NAME(FIO_MAP_NAME,
                                                       __each_node_s) *),
                                    void *udata) {
  FIO_NAME(FIO_MAP_NAME, __each_node_s)
  each = {.map = o, .fn = fn, .udata = udata};
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  size_t counter = o->count;
  if (!counter)
    return 0;
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST_EACH(o->map, node, o->head, pos) {
    each.node = o->map + pos;
    if (each.fn(&each))
      return -1;
    --counter;
    if (FIO_UNLIKELY(imap != FIO_NAME(FIO_MAP_NAME, __imap)(o)))
      return -1;
  }
#else
  const size_t len = FIO_MAP_CAPA(o->bits);
  if (FIO_UNLIKELY(o->bits > 5 && (FIO_MAP_CAPA(o->bits) >> 2) > o->count))
    goto sparse_map;
  for (size_t i = 0; counter; ++i) {
    if (!imap[i] || imap[i] == 255)
      continue;
    each.node = o->map + i;
    if (FIO_UNLIKELY(each.fn(&each)))
      return -1;
    --counter;
    if (FIO_UNLIKELY(imap != FIO_NAME(FIO_MAP_NAME, __imap)(o)))
      return -1;
  }
  FIO_ASSERT_DEBUG(
      !counter,
      "detected error while looping over all elements in map (%zu/%zu)",
      counter,
      (size_t)FIO_MAP_CAPA(o->bits));
  return 0;

sparse_map:
  for (size_t i = 0; counter && i < len; i += 64) {
    uint64_t bitmap = 0;
    for (size_t j = 0; j < 64; j += 8) {
      uint64_t tmp = *((uint64_t *)(imap + i + j));
      uint64_t inv = ~tmp;
      tmp = FIO_HAS_FULL_BYTE64(tmp);
      inv = FIO_HAS_FULL_BYTE64(inv);
      tmp |= inv;
      FIO_HAS_BYTE2BITMAP(tmp, 7);
      bitmap |= (tmp << j);
    }
    bitmap = ~bitmap; /* where 1 was a free slot, now it's an occupied one */
    for (size_t j = 0; bitmap; ++j) {
      if ((bitmap & 1)) {
        each.node = o->map + i + j;
        if (each.fn(&each))
          return -1;
        --counter;
        if (imap != FIO_NAME(FIO_MAP_NAME, __imap)(o))
          return -1;
      }
      bitmap >>= 1;
    }
  }
#endif
  FIO_ASSERT_DEBUG(
      !counter,
      "detected error while looping over all elements in map (%zu/%zu)",
      counter,
      (size_t)FIO_MAP_CAPA(o->bits));
  return 0;
}

#if !FIO_MAP_KEY_DESTROY_SIMPLE || !FIO_MAP_VALUE_DESTROY_SIMPLE
static int FIO_NAME(FIO_MAP_NAME,
                    __destroy_map_task)(FIO_NAME(FIO_MAP_NAME, __each_node_s) *
                                        e) {
  FIO_MAP_KEY_DESTROY(e->node->key);
  FIO_MAP_VALUE_DESTROY(e->node->value);
  return 0;
}
#endif

/* Destroys and exsiting map. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __destroy_map)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                       _Bool should_zero) {
#if !FIO_MAP_KEY_DESTROY_SIMPLE || !FIO_MAP_VALUE_DESTROY_SIMPLE
  FIO_NAME(FIO_MAP_NAME, __each_node)
  (o, FIO_NAME(FIO_MAP_NAME, __destroy_map_task), NULL);
#endif
  if (should_zero) /* set only imap to zero */
    FIO_MEMSET((o->map + (1ULL << o->bits)), 0, (1ULL << o->bits));
  o->count = 0;
}

/* Destroys and exsiting map. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __free_map)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                                  _Bool should_destroy) {
  if (!o->map)
    return;
  if (should_destroy)
    FIO_NAME(FIO_MAP_NAME, __destroy_map)(o, 0);
  FIO_MEM_FREE_(o->map, ((sizeof(o->map[0]) + 1) << o->bits));
  FIO_LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MAP_NAME, destroy));
  *o = (FIO_NAME(FIO_MAP_NAME, s)){0};
}

#ifndef H___FIO_MAP_INDEX_TYPE___H
#define H___FIO_MAP_INDEX_TYPE___H
typedef struct {
  uint32_t home;
  uint32_t act;
  uint32_t alt;
  uint32_t bhash;
} fio___map_node_info_s;
#endif

/** internal object data representation */
typedef struct FIO_NAME(FIO_MAP_NAME, __o_node_s) {
  uint64_t hash;
  FIO_MAP_KEY key;
#ifdef FIO_MAP_VALUE
  FIO_MAP_VALUE value;
#endif
} FIO_NAME(FIO_MAP_NAME, __o_node_s);

/* seek a node for very small collections 8 item capacity at most */
FIO_SFUNC fio___map_node_info_s FIO_NAME(FIO_MAP_NAME, __node_info_mini)(
    FIO_NAME(FIO_MAP_NAME, s) * o,
    FIO_NAME(FIO_MAP_NAME, __o_node_s) * node) {
  // FIO_LOG_INFO("seek as linear array for h %llu", node->hash);
  fio___map_node_info_s r = {
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)FIO_NAME(FIO_MAP_NAME, __byte_hash)(node->hash)};
  if (!o->bits)
    return r;
  const uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  const uint32_t capa = (uint32_t)FIO_MAP_CAPA(o->bits);
  const uint32_t mask = capa - 1;
  size_t pos = node->hash & 0xFF;
  for (uint32_t i = 0; i < capa; ++i) {
    pos = (pos + i) & mask;
    if (imap[pos] == r.bhash &&
        FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + pos, node->hash) &&
        FIO_MAP_KEY_CMP(o->map[pos].key, node->key)) {
      r.act = (uint32_t)pos;
      return r;
    } else if (!imap[pos]) {
      r.alt = r.home = (uint32_t)pos;
      return r;
    } else if (imap[pos] == 255U) { /* "home" has been occupied before */
      r.alt = r.home = (uint32_t)pos;
    }
  }
  return r;
}

/* seek a node for medium sized collections, 16-512 item capacity. */
FIO_SFUNC fio___map_node_info_s FIO_NAME(FIO_MAP_NAME, __node_info_med)(
    FIO_NAME(FIO_MAP_NAME, s) * o,
    FIO_NAME(FIO_MAP_NAME, __o_node_s) * node) {
  // FIO_LOG_INFO("seek as linear array for h %llu", node->hash);
  static int guard_print = 0;
  fio___map_node_info_s r = {
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)FIO_NAME(FIO_MAP_NAME, __byte_hash)(node->hash)};
  const uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  const uint32_t mask = (uint32_t)(FIO_MAP_CAPA(o->bits) - 1);
  uint32_t guard = FIO_MAP_ATTACK_LIMIT;
  uint32_t pos = r.home = (node->hash & mask);
  uint32_t step = 2;
  uint32_t attempts = (mask < 511) ? ((mask >> 2) | 8) : 127;
  for (; r.alt == (uint32_t)-1 && attempts; --attempts) {
    if (!imap[pos]) {
      r.alt = pos;
      return r;
    } else if (imap[pos] == 255U) { /* "home" has been occupied before */
      r.alt = pos;
    } else if (imap[pos] == r.bhash &&
               FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + pos, node->hash)) {
      if (FIO_MAP_KEY_CMP(o->map[pos].key, node->key)) {
        r.act = pos;
        return r;
      }
      if (!--guard) {
        r.act = pos;
        goto possible_attack;
      }
    }
    pos = ((pos + (step++)) & mask);
  }
  for (; attempts; --attempts) {
    if (imap[pos] == r.bhash &&
        FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + pos, node->hash)) {
      if (FIO_MAP_KEY_CMP(o->map[pos].key, node->key)) {
        r.act = pos;
        return r;
      }
      if (!--guard) {
        r.act = pos;
        goto possible_attack;
      }
    }
    if (!imap[pos])
      return r;
    pos = ((pos + (step++)) & mask);
  }
  if (r.alt == (uint32_t)-1)
    r.home = r.alt;
  return r;

possible_attack:
  if (!guard_print) {
    guard_print = 1;
    FIO_LOG_SECURITY("hash map " FIO_MACRO2STR(
        FIO_NAME(FIO_MAP_NAME, s)) " under attack? (full collision guard)");
  }
  return r;
}

/* seek a node for larger collections, where 8 byte grouping is meaningless */
FIO_SFUNC fio___map_node_info_s FIO_NAME(FIO_MAP_NAME, __node_info_full)(
    FIO_NAME(FIO_MAP_NAME, s) * o,
    FIO_NAME(FIO_MAP_NAME, __o_node_s) * node) {
  // FIO_LOG_INFO("seek as linear array for h %llu", node->hash);
  static int guard_print = 0;
  fio___map_node_info_s r = {
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)-1,
      (uint32_t)FIO_NAME(FIO_MAP_NAME, __byte_hash)(node->hash)};
  const uint32_t mask = (FIO_MAP_CAPA(o->bits) - 1) & (~(uint32_t)7ULL);
  const uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  const size_t attempt_limit = o->bits + 7;
  const uint64_t mbyte64 = ~(UINT64_C(0x0101010101010101) * (uint64_t)r.bhash);
  uint32_t guard = FIO_MAP_ATTACK_LIMIT;
  uint32_t pos = r.home = (node->hash & mask);
  size_t attempt = 0;
  for (; r.alt == (uint32_t)-1 && attempt < attempt_limit; ++attempt) {
    uint64_t group = fio_buf2u64_le(imap + pos);
    group ^= mbyte64;
    group &= UINT64_C(0x7F7F7F7F7F7F7F7F);
    group += UINT64_C(0x0101010101010101);
    group &= UINT64_C(0x8080808080808080);
    while (group) {
      uint32_t offset = (uint32_t)fio_lsb_index_unsafe(group);
      group ^= (uint64_t)1ULL << offset;
      offset >>= 3;
      offset += pos;
      if (imap[offset] == r.bhash &&
          FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + offset, node->hash)) {
        if (FIO_MAP_KEY_CMP(o->map[offset].key, node->key)) {
          r.act = offset;
          return r;
        }
        if (!--guard) {
          r.act = offset;
          goto possible_attack;
        }
      }
    }
    group = fio_buf2u64_le(imap + pos);
    group &= UINT64_C(0x7F7F7F7F7F7F7F7F);
    group += UINT64_C(0x0101010101010101);
    group &= UINT64_C(0x8080808080808080);
    while (group) {
      uint32_t offset = (uint32_t)fio_lsb_index_unsafe(group);
      group ^= (uint64_t)1ULL << offset;
      offset >>= 3;
      offset += pos;
      if (imap[offset] == 255U) {
        r.alt = offset;
        break;
      }
    }
    group = ~fio_buf2u64_le(imap + pos);
    group &= UINT64_C(0x7F7F7F7F7F7F7F7F);
    group += UINT64_C(0x0101010101010101);
    group &= UINT64_C(0x8080808080808080);
    while (group) {
      uint32_t offset = (uint32_t)fio_lsb_index_unsafe(group);
      group ^= (uint64_t)1ULL << offset;
      offset >>= 3;
      offset += pos;
      if (!imap[offset]) {
        r.alt = offset;
        return r;
      }
    }
    pos += (attempt + 2) << 3;
    pos &= mask;
  }
  for (; attempt < attempt_limit; ++attempt) {
    uint64_t group = fio_buf2u64_le(imap + pos);
    group ^= mbyte64;
    group &= UINT64_C(0x7F7F7F7F7F7F7F7F);
    group += UINT64_C(0x0101010101010101);
    group &= UINT64_C(0x8080808080808080);
    while (group) {
      uint32_t offset = (uint32_t)fio_lsb_index_unsafe(group);
      group ^= (uint64_t)1ULL << offset;
      offset >>= 3;
      offset += pos;
      if (imap[offset] == r.bhash &&
          FIO_NAME(FIO_MAP_NAME, __is_eq_hash)(o->map + offset, node->hash)) {
        if (FIO_MAP_KEY_CMP(o->map[offset].key, node->key)) {
          r.act = offset;
          return r;
        }
        if (!--guard) {
          r.act = offset;
          goto possible_attack;
        }
      }
    }
    group = ~fio_buf2u64_le(imap + pos);
    group &= UINT64_C(0x7F7F7F7F7F7F7F7F);
    group += UINT64_C(0x0101010101010101);
    group &= UINT64_C(0x8080808080808080);
    while (group) {
      uint32_t offset = (uint32_t)fio_lsb_index_unsafe(group);
      group ^= (uint64_t)1ULL << offset;
      offset >>= 3;
      offset += pos;
      if (!imap[offset]) {
        return r;
      }
    }
    pos += (attempt + 2) << 3;
    pos &= mask;
  }
  if (r.alt == (uint32_t)-1)
    r.home = r.alt;
  return r;
possible_attack:
  if (!guard_print) {
    guard_print = 1;
    FIO_LOG_SECURITY("hash map " FIO_MACRO2STR(
        FIO_NAME(FIO_MAP_NAME, s)) " under attack? (full collision guard)");
  }
  return r;
}

FIO_IFUNC fio___map_node_info_s
FIO_NAME(FIO_MAP_NAME, __node_info)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                    FIO_NAME(FIO_MAP_NAME, __o_node_s) * node) {
#if defined(FIO_MAP_HASH_FN)
  if (!node->hash)
    node->hash = FIO_MAP_HASH_FN(node->key);
#endif
  node->hash += !node->hash;
  if (o->bits < 4)
    return FIO_NAME(FIO_MAP_NAME, __node_info_mini)(o, node);
  else if (o->bits < 9)
    return FIO_NAME(FIO_MAP_NAME, __node_info_med)(o, node);
  else
    return FIO_NAME(FIO_MAP_NAME, __node_info_full)(o, node);
}

#if FIO_MAP_ORDERED
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __update_order)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                        uint32_t i) {
  if (o->count == 1) {
    o->head = i;
    o->map[i].node.next = o->map[i].node.prev = i;
  } else {
    FIO_INDEXED_LIST_PUSH(o->map, node, o->head, i);
  }
}
#define FIO___MAP_UPDATE_ORDER(map, at)                                        \
  FIO_NAME(FIO_MAP_NAME, __update_order)(map, at)
#else
#define FIO___MAP_UPDATE_ORDER(map, at)
#endif

static int FIO_NAME(FIO_MAP_NAME,
                    __move2map_task)(FIO_NAME(FIO_MAP_NAME, __each_node_s) *
                                     e) {
  FIO_NAME(FIO_MAP_NAME, s) *dest = (FIO_NAME(FIO_MAP_NAME, s) *)e->udata;
  FIO_NAME(FIO_MAP_NAME, __o_node_s)
  n = {
    .key = FIO_MAP_KEY_FROM_INTERNAL(e->node->key),
#if !FIO_MAP_RECALC_HASH
    .hash = e->node->hash,
#endif
  };
  fio___map_node_info_s i = FIO_NAME(FIO_MAP_NAME, __node_info)(dest, &n);
  if (i.home == (uint32_t)-1) {
    FIO_LOG_ERROR("move2map FAILED (%zu/%zu)!",
                  e->node - e->map->map,
                  FIO_MAP_CAPA(dest->bits));
    return -1;
  }
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(dest);
  ++dest->count;
  /* insert at best position */
  imap[i.alt] = i.bhash;
  dest->map[i.alt] = e->node[0];
  FIO___MAP_UPDATE_ORDER(dest, i.alt);
  return 0;
}

void fio___map___move2map___(void); /* IDE Marker */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME,
                       __move2map)(FIO_NAME(FIO_MAP_NAME, s) * dest,
                                   FIO_NAME(FIO_MAP_NAME, s) * src) {
  return FIO_NAME(
      FIO_MAP_NAME,
      __each_node)(src, FIO_NAME(FIO_MAP_NAME, __move2map_task), dest);
}

/* Inserts a node to the map. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                            __node_insert)(FIO_NAME(FIO_MAP_NAME, s) * o,
#ifndef FIO_MAP_HASH_FN
                                           uint64_t hash,
#endif
                                           FIO_MAP_KEY key,
#ifdef FIO_MAP_VALUE
                                           FIO_MAP_VALUE value,
#endif
#ifdef FIO_MAP_VALUE
                                           FIO_MAP_VALUE_INTERNAL *old,
#else
                                           FIO_MAP_KEY_INTERNAL *old,
#endif
                                           _Bool overwrite) {
  static FIO_NAME(FIO_MAP_NAME, s) *last_collision = NULL;
  uint32_t r = -1;
  FIO_NAME(FIO_MAP_NAME, s) tmp;
  FIO_NAME(FIO_MAP_NAME, __o_node_s)
  node = {
      .key = key,
#ifdef FIO_MAP_VALUE
      .value = value,
#endif
#ifndef FIO_MAP_HASH_FN
      .hash = hash,
#endif
  };
  fio___map_node_info_s info;
  info = FIO_NAME(FIO_MAP_NAME, __node_info)(o, &node);
  if (info.act != r)
    goto perform_overwrite;
  if (info.home == r)
    goto reallocate_map;

insert:
  ++o->count;
  r = info.alt;
  FIO_NAME(FIO_MAP_NAME, __imap)(o)[r] = info.bhash;
#if !FIO_MAP_RECALC_HASH
  o->map[r].hash = node.hash,
#endif
  FIO_MAP_KEY_COPY(o->map[r].key, node.key);
  FIO_MAP_VALUE_COPY(o->map[r].value, node.value);
  FIO___MAP_UPDATE_ORDER(o, r);
  return r;

perform_overwrite:
  r = info.act;
  FIO_MAP_KEY_DISCARD(node.key);
  if (!overwrite) {
    FIO_MAP_VALUE_DISCARD(value);
    return r;
  }
#ifdef FIO_MAP_VALUE
  if (old)
    *old = o->map[r].value;
  else {
    FIO_MAP_VALUE_DESTROY(o->map[r].value);
  }
  FIO_MAP_VALUE_COPY(o->map[r].value, node.value);
#else
  (void)old;
#endif
#if FIO_MAP_ORDERED
  if (o->head == r)
    o->head = o->map[o->head].node.prev;
  FIO_INDEXED_LIST_REMOVE(o->map, node, r);
  FIO___MAP_UPDATE_ORDER(o, r);
#endif
  return r;

reallocate_map:
  /* reallocate map */
  for (size_t i = 1; i < 3; ++i) {
    if (FIO_NAME(FIO_MAP_NAME, __allocate_map)(&tmp, (uint32_t)(o->bits + i)))
      goto no_memory;
    if (FIO_NAME(FIO_MAP_NAME, __move2map)(&tmp, o)) {
      FIO_NAME(FIO_MAP_NAME, __free_map)(&tmp, 0);
      continue;
    }
    info = FIO_NAME(FIO_MAP_NAME, __node_info)(&tmp, &node);
    if (info.home != r) {
      FIO_NAME(FIO_MAP_NAME, __free_map)(o, 0);
      *o = tmp;
      goto insert;
    }
    FIO_NAME(FIO_MAP_NAME, __free_map)(&tmp, 0);
  }
  goto security_partial;

no_memory:
  FIO_MAP_KEY_DISCARD(key);
  FIO_MAP_VALUE_DISCARD(value);
  FIO_LOG_ERROR(
      "unknown error occurred trying to add an entry to the map (capa: %zu)",
      (size_t)FIO_MAP_CAPA(o->bits));
  FIO_ASSERT_DEBUG(0, "these errors shouldn't happen - no memory?");
  return r;

security_partial:
  if (last_collision != o) {
    FIO_LOG_SECURITY(
        "hash map " FIO_MACRO2STR(FIO_NAME(
            FIO_MAP_NAME,
            s)) " under attack? (partial/full collision guard) - capa: %zu.",
        (size_t)FIO_MAP_CAPA(o->bits));
    last_collision = o;
  }
  return r;
}

/* Inserts a node to the map. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                            __node_find)(FIO_NAME(FIO_MAP_NAME, s) * o,
#ifndef FIO_MAP_HASH_FN
                                         uint64_t hash,
#endif
                                         FIO_MAP_KEY key) {
  uint32_t r = -1;
  fio___map_node_info_s info;
  FIO_NAME(FIO_MAP_NAME, __o_node_s)
  node = {
      .key = key,
#ifndef FIO_MAP_HASH_FN
      .hash = hash,
#endif
  };
  if (!o->map)
    return r;
  info = FIO_NAME(FIO_MAP_NAME, __node_info)(o, &node);
  return (r = info.act);
}

/* Deletes a known node from the map. */
FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __node_delete_at)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                          uint32_t at,
#ifdef FIO_MAP_VALUE
                                          FIO_MAP_VALUE_INTERNAL *old
#else
                                          FIO_MAP_KEY_INTERNAL *old
#endif
) {
  --o->count;
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(o);
  imap[at] = 255U; /* mark as deleted */
  if (old) {
#ifdef FIO_MAP_VALUE
    FIO_MAP_KEY_DESTROY(o->map[at].key);
    *old = o->map[at].value;
#else
    *old = o->map[at].key;
#endif
  } else {
    FIO_MAP_KEY_DESTROY(o->map[at].key);
    FIO_MAP_VALUE_DESTROY(o->map[at].value);
  }

#if FIO_MAP_ORDERED
  if (o->head == at) {
    o->head = (o->count ? o->map[o->head].node.next : 0);
  }
  FIO_INDEXED_LIST_REMOVE(o->map, node, at);
#endif
}

/* Deletes a node from the map. */
FIO_IFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                            __node_delete)(FIO_NAME(FIO_MAP_NAME, s) * o,
                                           FIO_MAP_KEY key,
#ifndef FIO_MAP_HASH_FN
                                           uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                                           FIO_MAP_VALUE_INTERNAL *old
#else
                                           FIO_MAP_KEY_INTERNAL *old
#endif
) {
  uint32_t r = (uint32_t)-1;
  if (!o->map)
    return r;
  fio___map_node_info_s info;
  FIO_NAME(FIO_MAP_NAME, __o_node_s)
  node = {
      .key = key,
#ifndef FIO_MAP_HASH_FN
      .hash = hash,
#endif
  };
  info = FIO_NAME(FIO_MAP_NAME, __node_info)(o, &node);
  if (info.act == r)
    return r;
  r = 0;
  FIO_NAME(FIO_MAP_NAME, __node_delete_at)(o, info.act, old);
  return r;
}

/* *****************************************************************************






Map API






***************************************************************************** */

/** Destroys the object, re-initializing its container. */
SFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR map) {
  // FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  FIO_NAME(FIO_MAP_NAME, __free_map)(m, 1);
  *m = (FIO_NAME(FIO_MAP_NAME, s)){0};
}

/** Reserves at minimum the capacity requested. */
SFUNC void FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_PTR map, size_t capa) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (capa <= FIO_MAP_CAPA(m->bits))
    return;
  uint32_t bits = m->bits;
  for (; capa > FIO_MAP_CAPA(bits); ++bits)
    ;
  FIO_NAME(FIO_MAP_NAME, s) tmp;
  if (FIO_NAME(FIO_MAP_NAME, __allocate_map)(&tmp, bits))
    goto no_memory;
  if (m->count && FIO_NAME(FIO_MAP_NAME, __move2map)(&tmp, m)) {
    FIO_NAME(FIO_MAP_NAME, __free_map)(&tmp, 0);
    goto no_memory;
  }
  FIO_NAME(FIO_MAP_NAME, __free_map)(m, 0);
  *m = tmp;
  return;
no_memory:
  FIO_LOG_ERROR("unknown error occurred trying to rehash the map");
  FIO_ASSERT_DEBUG(0, "these errors shouldn't happen - no memory?");
  return;
}

/** Removes an object in the map, returning a pointer to the map data. */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                         uint64_t hash,
#endif
                                         FIO_MAP_KEY key,
#if defined(FIO_MAP_VALUE)
                                         FIO_MAP_VALUE_INTERNAL *old
#else
                                         FIO_MAP_KEY_INTERNAL *old
#endif
) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!m->count)
    return -1;
  return FIO_NAME(FIO_MAP_NAME, __node_delete)(m,
                                               key,
#if !defined(FIO_MAP_HASH_FN)
                                               hash,
#endif
                                               old);
}

FIO_SFUNC int FIO_NAME(FIO_MAP_NAME,
                       __evict_task)(FIO_NAME(FIO_MAP_NAME, __each_node_s) *
                                     e) {
  size_t *counter = (size_t *)e->udata;
  FIO_NAME(FIO_MAP_NAME, __node_delete_at)
  (e->map, (uint32_t)(e->node - e->map->map), NULL);
  if ((counter[0] -= 1))
    return 0;
  return -1;
}
/** Evicts elements in order least recently used (LRU), FIFO or undefined. */
SFUNC void FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR map,
                                         size_t number_of_elements) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  if (!number_of_elements)
    return;
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (m->count <= number_of_elements) {
    FIO_NAME(FIO_MAP_NAME, __destroy_map)(m, 1);
    return;
  }
  FIO_NAME(FIO_MAP_NAME, __each_node)
  (m, FIO_NAME(FIO_MAP_NAME, __evict_task), &number_of_elements);
}

/** Removes all objects from the map, without releasing the map's resources.
 */
SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (m->map)
    FIO_NAME(FIO_MAP_NAME, __destroy_map)(m, 1);
}

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, s) *o = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (!o->map || !o->count)
    return;
  FIO_NAME(FIO_MAP_NAME, s) cpy = {0};
  uint32_t bits = o->bits;
  while (FIO_MAP_CAPA(bits >> 1) > o->count)
    bits >>= 1;
  ++bits;
  if (bits >= o->bits)
    return;
  for (size_t i = 0; i < 2; ++i) {
    if (FIO_NAME(FIO_MAP_NAME, __allocate_map)(&cpy, bits))
      return;
    if (!FIO_NAME(FIO_MAP_NAME, __move2map)(&cpy, o))
      goto finish;
    FIO_NAME(FIO_MAP_NAME, __free_map)(&cpy, 0);
    ++bits;
  }
  return;

finish:
  FIO_NAME(FIO_MAP_NAME, __free_map)(o, 0);
  o[0] = cpy;
  return;
}

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
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  uint32_t i = FIO_NAME(FIO_MAP_NAME, __node_insert)(m,
#ifndef FIO_MAP_HASH_FN
                                                     hash,
#endif
                                                     key,
#ifdef FIO_MAP_VALUE
                                                     val,
                                                     old,
                                                     overwrite
#else
                                                     NULL,
                                                     1
#endif
  );
  if (i == (uint32_t)-1)
    return NULL;
  return m->map + i;
}

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
                                    FIO_MAP_KEY key) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  uint32_t i = FIO_NAME(FIO_MAP_NAME, __node_find)(m,
#ifndef FIO_MAP_HASH_FN
                                                   hash,
#endif
                                                   key);
  if (i == (uint32_t)-1)
    return NULL;
  return m->map + i;
}

/* *****************************************************************************



Map Iterators



***************************************************************************** */

SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME,
             get_next)(FIO_MAP_PTR map,
                       FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos) {
  FIO_NAME(FIO_MAP_NAME, iterator_s) r = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  if (!m->count)
    return r;
  if (!current_pos)
    goto empty;
  if (current_pos->private_.pos + 1 == m->count)
    return r;
  if (current_pos->private_.map_validator != (uintptr_t)(m->map))
    return r; /* mutation stops iteration */
#if FIO_MAP_ORDERED
  if (current_pos->node->node.next == m->head)
    return r;
  r.node = m->map + current_pos->node->node.next;
#else
  for (size_t i = current_pos->node - m->map + 1; i < FIO_MAP_CAPA(m->bits);
       ++i) {
    if (!imap[i] || imap[i] == 0xFFU)
      continue;
    r.node = m->map + i;
    break;
  }
  if (!r.node)
    return r;
#endif

  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = r.node, .key = FIO_MAP_KEY_FROM_INTERNAL(r.node->key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(r.node->value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = r.node->hash,
#endif
    .private_ = {.index = (uint32_t)(r.node - m->map),
                 .pos = current_pos->private_.pos + 1,
                 .map_validator = (uintptr_t)m->map},
  };
  return r;
empty:

#if FIO_MAP_ORDERED
  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = m->map + m->head,
    .key = FIO_MAP_KEY_FROM_INTERNAL(m->map[m->head].key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(m->map[m->head].value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = m->map[m->head].hash,
#endif
    .private_ = {.index = m->head,
                 .pos = 0,
                 .map_validator = (uintptr_t)m->map},
  };
#else
  for (size_t i = 0; i < FIO_MAP_CAPA(m->bits); ++i) {
    if (!imap[i] || imap[i] == 0xFFU)
      continue;
    r.node = m->map + i;
    break;
  }

  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = r.node, .key = FIO_MAP_KEY_FROM_INTERNAL(r.node->key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(r.node->value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = r.node->hash,
#endif
    .private_ = {.index = (uint32_t)(r.node - m->map),
                 .pos = 0,
                 .map_validator = (uintptr_t)m->map},
  };

#endif
  return r;
  (void)imap; /* if unused */
}

SFUNC FIO_NAME(FIO_MAP_NAME, iterator_s)
    FIO_NAME(FIO_MAP_NAME, get_prev)(FIO_MAP_PTR map,
                                     FIO_NAME(FIO_MAP_NAME, iterator_s) *
                                         current_pos) { // TODO!
  FIO_NAME(FIO_MAP_NAME, iterator_s) r = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
#if FIO_MAP_ORDERED
  uint32_t ipos;
#else
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
#endif
  if (!m->count)
    return r;
  if (!current_pos)
    goto empty;
  if (!current_pos->private_.pos)
    return r;
  if (current_pos->private_.map_validator != (uintptr_t)(m->map))
    return r; /* mutation stops iteration */
#if FIO_MAP_ORDERED
  if (current_pos->private_.index == m->head)
    return r;
  r.node = m->map + current_pos->node->node.prev;
#else
  for (size_t i = current_pos->node - m->map; i;) {
    --i;
    if (!imap[i] || imap[i] == 0xFFU)
      continue;
    r.node = m->map + i;
    break;
  }
#endif

  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = r.node, .key = FIO_MAP_KEY_FROM_INTERNAL(r.node->key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(r.node->value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = r.node->hash,
#endif
    .private_ = {.index = (uint32_t)(r.node - m->map),
                 .pos = current_pos->private_.pos - 1,
                 .map_validator = (uintptr_t)m->map},
  };
  return r;
empty:

#if FIO_MAP_ORDERED

  ipos = m->map[m->head].node.prev;
  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = m->map + ipos, .key = FIO_MAP_KEY_FROM_INTERNAL(m->map[ipos].key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(m->map[ipos].value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = m->map[ipos].hash,
#endif
    .private_ = {.index = ipos,
                 .pos = m->count - 1,
                 .map_validator = (uintptr_t)m->map},
  };

#else
  for (size_t i = FIO_MAP_CAPA(m->bits); i;) {
    --i;
    if (!imap[i] || imap[i] == 0xFFU)
      continue;
    r.node = m->map + i;
    break;
  }

  r = (FIO_NAME(FIO_MAP_NAME, iterator_s)) {
    .node = r.node, .key = FIO_MAP_KEY_FROM_INTERNAL(r.node->key),
#ifdef FIO_MAP_VALUE
    .value = FIO_MAP_VALUE_FROM_INTERNAL(r.node->value),
#endif
#if !FIO_MAP_RECALC_HASH
    .hash = r.node->hash,
#endif
    .private_ = {.index = (uint32_t)(r.node - m->map),
                 .pos = m->count - 1,
                 .map_validator = (uintptr_t)m->map},
  };

#endif

  return r;
}

/* *****************************************************************************



Map Each



***************************************************************************** */

typedef struct {
  FIO_NAME(FIO_MAP_NAME, each_s) each;
  ssize_t start_at;
} FIO_NAME(FIO_MAP_NAME, __each_info_s);

FIO_SFUNC int FIO_NAME(FIO_MAP_NAME,
                       __each_task)(FIO_NAME(FIO_MAP_NAME, __each_node_s) * e) {
  int r;
  FIO_NAME(FIO_MAP_NAME, __each_info_s) *info =
      (FIO_NAME(FIO_MAP_NAME, __each_info_s) *)e->udata;
  info->each.key = FIO_MAP_KEY_FROM_INTERNAL(e->node->key);
#ifdef FIO_MAP_VALUE
  info->each.value = FIO_MAP_VALUE_FROM_INTERNAL(e->node->value);
#endif
  r = info->each.task(&info->each);
  ++info->each.index;
  return r;
}

FIO_SFUNC int FIO_NAME(FIO_MAP_NAME, __each_task_offset)(
    FIO_NAME(FIO_MAP_NAME, __each_node_s) * e) {
  FIO_NAME(FIO_MAP_NAME, __each_info_s) *info =
      (FIO_NAME(FIO_MAP_NAME, __each_info_s) *)e->udata;
  if (FIO_LIKELY(info->each.index < (uint64_t)info->start_at)) {
    ++info->each.index;
    return 0;
  }
  return (e->fn = FIO_NAME(FIO_MAP_NAME, __each_task))(e);
}

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
  uint32_t r = (uint32_t)-1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, r);
  FIO_NAME(FIO_MAP_NAME, s) *m = FIO_PTR_TAG_GET_UNTAGGED(FIO_MAP_T, map);
  if (start_at < 0)
    start_at += m->count;
  if (start_at < 0)
    return m->count;
  FIO_NAME(FIO_MAP_NAME, __each_info_s)
  e = {
      .each =
          {
              .parent = map,
              .index = 0,
              .task = task,
              .udata = udata,
          },
      .start_at = start_at,
  };

  FIO_NAME(FIO_MAP_NAME, __each_node)
  (m,
   !start_at ? FIO_NAME(FIO_MAP_NAME, __each_task)
             : FIO_NAME(FIO_MAP_NAME, __each_task_offset),
   &e);
  return (uint32_t)e.each.index;
}

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
  fprintf(
      stderr,
      "* Testing map " FIO_MACRO2STR(FIO_MAP_NAME) " with key " FIO_MACRO2STR(
          FIO_MAP_KEY) " (=> " FIO_MACRO2STR(FIO_MAP_VALUE) ").\n");
  size_t test_len_limit = (1UL << (FIO_MAP_ARRAY_LOG_LIMIT + 15));
  { /* test set / get overwrite , FIO_MAP_EACH and evict */
    FIO_NAME(FIO_MAP_NAME, s) map = FIO_MAP_INIT;
    for (size_t i = 1; i < test_len_limit; ++i) {
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
      ++loop_test;
#ifdef FIO_MAP_LRU
      FIO_ASSERT(i.key == loop_test,
                 "map FIO_MAP_EACH LRU ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(count - loop_test));
#elif FIO_MAP_ORDERED
      FIO_ASSERT(i.key == loop_test,
                 "map FIO_MAP_EACH ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(loop_test));
#else
      FIO_ASSERT(i.key < test_len_limit,
                 "map FIO_MAP_EACH invalid data? %zu !< %zu",
                 (size_t)(i.key),
                 (size_t)(test_len_limit));
#endif
    }
    FIO_ASSERT(loop_test == count,
               "FIO_MAP_EACH failed to iterate all elements? (%zu != %zu",
               (size_t)loop_test != (size_t)count);
    loop_test = 0;
    FIO_MAP_EACH_REVERSED(FIO_MAP_NAME, &map, i) {
      /* test reversed ordering */
      ++loop_test;
#ifdef FIO_MAP_LRU
      FIO_ASSERT(i.key == (count - (loop_test - 1)),
                 "map FIO_MAP_EACH_REVERSED LRU ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(count - loop_test));
#elif FIO_MAP_ORDERED
      FIO_ASSERT(i.key == (count - (loop_test - 1)),
                 "map FIO_MAP_EACH_REVERSED ordering broken? %zu != %zu",
                 (size_t)(i.key),
                 (size_t)(loop_test));
#endif
    }
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
#if !FIO_MAP_RECALC_HASH
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
    for (size_t i = 1; i < test_len_limit; ++i) {
      FIO_NAME(FIO_MAP_NAME, set)
      (&map, FIO___M_HASH(i) i FIO___M_VAL(i) FIO___M_OLD);
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == i, "insertion failed?");
    }
    for (size_t i = 1; i < test_len_limit; ++i) {
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, get)(&map, FIO___M_HASH(i) i),
                 "key missing?");
      size_t count = FIO_NAME(FIO_MAP_NAME, count)(&map);
      FIO_NAME(FIO_MAP_NAME, remove)
      (&map, FIO___M_HASH(i) i, NULL);
      FIO_ASSERT(!FIO_NAME(FIO_MAP_NAME, get)(&map, FIO___M_HASH(i) i),
                 "map_remove error?");
      FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&map) == count - 1,
                 "map count error after removal? %zu != %zu",
                 (size_t)FIO_NAME(FIO_MAP_NAME, count)(&map),
                 count - 1);
      /* see if removal produces errors while rehashing */
      FIO_NAME(FIO_MAP_NAME, compact)(&map);
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

#undef FIO___MAP_UPDATE_ORDER
#undef FIO_MAP___MAKE_BITMAP
#undef FIO_MAP___STEP_POS
#undef FIO_MAP___TEST_MATCH
#undef FIO_MAP_ARRAY_LOG_LIMIT
#undef FIO_MAP_ATTACK_LIMIT
#undef FIO_MAP_CAPA
#undef FIO_MAP_CAPA_BITS_LIMIT
#undef FIO_MAP_CUCKOO_STEPS
#undef FIO_MAP_GET_T
#undef FIO_MAP_HASH_FN
#undef FIO_MAP_IS_SPARSE
#undef FIO_MAP_KEY
#undef FIO_MAP_KEY_BSTR
#undef FIO_MAP_KEY_CMP
#undef FIO_MAP_KEY_COPY
#undef FIO_MAP_KEY_DESTROY
#undef FIO_MAP_KEY_DESTROY_SIMPLE
#undef FIO_MAP_KEY_DISCARD
#undef FIO_MAP_KEY_FROM_INTERNAL
#undef FIO_MAP_KEY_INTERNAL
#undef FIO_MAP_KEY_IS_GREATER_THAN
#undef FIO_MAP_KEY_KSTR
#undef FIO_MAP_LRU
#undef FIO_MAP_MINIMAL_BITS
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
