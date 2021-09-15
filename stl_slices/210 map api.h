/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_UMAP_NAME umap          /* Development inclusion - ignore line */
#define FIO_MAP_TEST                /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                  Common Map Settings (ordered / unordered)




***************************************************************************** */
#if defined(FIO_UMAP_NAME)
#ifndef FIO_MAP_NAME
#define FIO_MAP_NAME FIO_UMAP_NAME
#endif
#ifndef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 0
#endif
#elif defined(FIO_OMAP_NAME)
#ifndef FIO_MAP_NAME
#define FIO_MAP_NAME FIO_OMAP_NAME
#endif
#ifndef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1
#endif
#else
#ifndef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1
#endif
#endif

#ifdef FIO_MAP_NAME
/* *****************************************************************************
The following macros are used to customize the map.
***************************************************************************** */

#ifndef FIO_MAP_TYPE
/** The type for the elements in the map */
#define FIO_MAP_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_MAP_TYPE_INVALID NULL
#else
#ifndef FIO_MAP_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_MAP_TYPE_INVALID ((FIO_MAP_TYPE){0})
#endif /* FIO_MAP_TYPE_INVALID */
#endif /* FIO_MAP_TYPE */

#ifndef FIO_MAP_TYPE_COPY
/** Handles a copy operation for an value. */
#define FIO_MAP_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - do not set */
#define FIO_MAP_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_MAP_TYPE_DESTROY
/** Handles a destroy / free operation for a map's value. */
#define FIO_MAP_TYPE_DESTROY(obj)
/** internal flag - set only if the object desctructor is optional */
#define FIO_MAP_TYPE_DESTROY_SIMPLE 1
#else
#ifndef FIO_MAP_TYPE_DESTROY_SIMPLE
#define FIO_MAP_TYPE_DESTROY_SIMPLE 0
#endif
#endif

#ifndef FIO_MAP_TYPE_DISCARD
/** Handles discarded value data (i.e., insert without overwrite). */
#define FIO_MAP_TYPE_DISCARD(obj)
#endif

#ifndef FIO_MAP_TYPE_CMP
/** Handles a comparison operation for a map's value. */
#define FIO_MAP_TYPE_CMP(a, b) 1
/* internal flag - do not set */
#define FIO_MAP_TYPE_CMP_SIMPLE 1
#else
/* internal flag - do not set */
#define FIO_MAP_TYPE_CMP_SIMPLE 0
#endif

/**
 * The FIO_MAP_DESTROY_AFTER_COPY macro should be set if FIO_MAP_TYPE_DESTROY
 * should be called after FIO_MAP_TYPE_COPY when an object is removed from the
 * array after being copied to an external container (an `old` pointer)
 */
#ifndef FIO_MAP_DESTROY_AFTER_COPY
#if !FIO_MAP_TYPE_DESTROY_SIMPLE && !FIO_MAP_TYPE_COPY_SIMPLE
#define FIO_MAP_DESTROY_AFTER_COPY 1
#else
#define FIO_MAP_DESTROY_AFTER_COPY 0
#endif
#endif /* FIO_MAP_DESTROY_AFTER_COPY */

/* *****************************************************************************
Dictionary / Hash Map - a Hash Map is basically a Set of couplets
***************************************************************************** */
/* Defining a key makes a Hash Map instead of a Set */
#ifdef FIO_MAP_KEY

#ifndef FIO_MAP_KEY_INVALID
/** An invalid value for the hash map key type (if any). */
#define FIO_MAP_KEY_INVALID ((FIO_MAP_KEY){0})
#endif

#ifndef FIO_MAP_KEY_COPY
/** Handles a copy operation for a hash maps key. */
#define FIO_MAP_KEY_COPY(dest, src) (dest) = (src)
#endif

#ifndef FIO_MAP_KEY_DESTROY
/** Handles a destroy / free operation for a hash maps key. */
#define FIO_MAP_KEY_DESTROY(obj)
/** internal flag - set only if the object desctructor is optional */
#define FIO_MAP_KEY_DESTROY_SIMPLE 1
#else
#ifndef FIO_MAP_KEY_DESTROY_SIMPLE
#define FIO_MAP_KEY_DESTROY_SIMPLE 0
#endif
#endif

#ifndef FIO_MAP_KEY_DISCARD
/** Handles discarded element data (i.e., when overwriting only the value). */
#define FIO_MAP_KEY_DISCARD(obj)
#endif

#ifndef FIO_MAP_KEY_CMP
/** Handles a comparison operation for a hash maps key. */
#define FIO_MAP_KEY_CMP(a, b) 1
#endif

typedef struct {
  FIO_MAP_KEY key;
  FIO_MAP_TYPE value;
} FIO_NAME(FIO_MAP_NAME, couplet_s);

FIO_IFUNC void FIO_NAME(FIO_MAP_NAME, __couplet_copy)(
    FIO_NAME(FIO_MAP_NAME, couplet_s) * dest,
    FIO_NAME(FIO_MAP_NAME, couplet_s) * src) {
  FIO_MAP_KEY_COPY((dest->key), (src->key));
  FIO_MAP_TYPE_COPY((dest->value), (src->value));
}

FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __couplet_destroy)(FIO_NAME(FIO_MAP_NAME, couplet_s) *
                                           c) {
  FIO_MAP_KEY_DESTROY(c->key);
  FIO_MAP_TYPE_DESTROY(c->value);
  (void)c; /* in case where macros do nothing */
}

/** FIO_MAP_OBJ is either a couplet (for hash maps) or the objet (for sets) */
#define FIO_MAP_OBJ FIO_NAME(FIO_MAP_NAME, couplet_s)

/** FIO_MAP_OBJ_KEY is FIO_MAP_KEY for hash maps or FIO_MAP_TYPE for sets */
#define FIO_MAP_OBJ_KEY FIO_MAP_KEY

#define FIO_MAP_OBJ_INVALID                                                    \
  ((FIO_NAME(FIO_MAP_NAME, couplet_s)){.key = FIO_MAP_KEY_INVALID,             \
                                       .value = FIO_MAP_TYPE_INVALID})

#define FIO_MAP_OBJ_COPY(dest, src)                                            \
  FIO_NAME(FIO_MAP_NAME, __couplet_copy)(&(dest), &(src))

#define FIO_MAP_OBJ_DESTROY(obj)                                               \
  FIO_NAME(FIO_MAP_NAME, __couplet_destroy)(&(obj))

#define FIO_MAP_OBJ_CMP(a, b)        FIO_MAP_KEY_CMP((a).key, (b).key)
#define FIO_MAP_OBJ_KEY_CMP(a, key_) FIO_MAP_KEY_CMP((a).key, (key_))
#define FIO_MAP_OBJ2KEY(o)           (o).key
#define FIO_MAP_OBJ2TYPE(o)          (o).value

#define FIO_MAP_OBJ_DISCARD(o)                                                 \
  do {                                                                         \
    FIO_MAP_TYPE_DISCARD(((o).value));                                         \
    FIO_MAP_KEY_DISCARD(((o).key));                                            \
  } while (0);

#if FIO_MAP_DESTROY_AFTER_COPY
#define FIO_MAP_OBJ_DESTROY_AFTER FIO_MAP_OBJ_DESTROY
#else
#define FIO_MAP_OBJ_DESTROY_AFTER(obj) FIO_MAP_KEY_DESTROY((obj).key);
#endif /* FIO_MAP_DESTROY_AFTER_COPY */

/* *****************************************************************************
Set Map
***************************************************************************** */
#else /* FIO_MAP_KEY */
#define FIO_MAP_KEY_DESTROY_SIMPLE 1
/** FIO_MAP_OBJ is either a couplet (for hash maps) or the objet (for sets) */
#define FIO_MAP_OBJ                FIO_MAP_TYPE
/** FIO_MAP_OBJ_KEY is FIO_MAP_KEY for hash maps or FIO_MAP_TYPE for sets */
#define FIO_MAP_OBJ_KEY            FIO_MAP_TYPE
#define FIO_MAP_OBJ_INVALID        FIO_MAP_TYPE_INVALID
#define FIO_MAP_OBJ_COPY           FIO_MAP_TYPE_COPY
#define FIO_MAP_OBJ_DESTROY        FIO_MAP_TYPE_DESTROY
#define FIO_MAP_OBJ_CMP            FIO_MAP_TYPE_CMP
#define FIO_MAP_OBJ_KEY_CMP        FIO_MAP_TYPE_CMP
#define FIO_MAP_OBJ2KEY(o)         (o)
#define FIO_MAP_OBJ2TYPE(o)        (o)
#define FIO_MAP_OBJ_DISCARD        FIO_MAP_TYPE_DISCARD
#define FIO_MAP_KEY_DISCARD(_ignore)
#define FIO_MAP_KEY_COPY(_ignore, _ignore2)
#if FIO_MAP_DESTROY_AFTER_COPY
#define FIO_MAP_OBJ_DESTROY_AFTER FIO_MAP_TYPE_DESTROY
#else
#define FIO_MAP_OBJ_DESTROY_AFTER(obj)
#endif /* FIO_MAP_DESTROY_AFTER_COPY */

#endif /* FIO_MAP_KEY */

/* *****************************************************************************
Misc Settings (eviction policy, load-factor attempts, etc')
***************************************************************************** */

#ifndef FIO_MAP_MAX_SEEK /* LIMITED to 255 */
#if FIO_MAP_ORDERED
/* The maximum number of bins to rotate when (partial/full) collisions occure */
#define FIO_MAP_MAX_SEEK (13U)
#else
#define FIO_MAP_MAX_SEEK (7U)
#endif
#endif

#ifndef FIO_MAP_MAX_FULL_COLLISIONS /* LIMITED to 255 */
/* The maximum number of full hash collisions that can be consumed */
#define FIO_MAP_MAX_FULL_COLLISIONS (22U)
#endif

#ifndef FIO_MAP_CUCKOO_STEPS
/* Prime numbers are better */
#define FIO_MAP_CUCKOO_STEPS (0x43F82D0BUL) /* should be a high prime */
#endif

#ifndef FIO_MAP_EVICT_LRU
/** Set the `evict` method to evict based on the Least Recently Used object. */
#define FIO_MAP_EVICT_LRU 0
#endif

#ifndef FIO_MAP_SHOULD_OVERWRITE
/** Tests if `older` should be replaced with `newer`. */
#define FIO_MAP_SHOULD_OVERWRITE(older, newer) 1
#endif

#ifndef FIO_MAP_MAX_ELEMENTS
/** The maximum number of elements allowed before removing old data (FIFO) */
#define FIO_MAP_MAX_ELEMENTS 0
#endif

#ifndef FIO_MAP_HASH
/** The type for map hash value (an X bit integer) */
#define FIO_MAP_HASH uint64_t
#endif

#undef FIO_MAP_HASH_FIXED
/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIXED ((FIO_MAP_HASH)-2LL)

#undef FIO_MAP_HASH_FIX
/** Validates the hash value and returns the valid value. */
#define FIO_MAP_HASH_FIX(h) (!h ? FIO_MAP_HASH_FIXED : (h))

/**
 * Unordered maps don't have to cache an object's hash.
 *
 * If the hash is cheap to calculate, it could be recalculated on the fly.
 */
#if defined(FIO_MAP_HASH_FN) && !FIO_MAP_ORDERED
FIO_IFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, __get_hash)(FIO_MAP_OBJ_KEY k) {
  FIO_MAP_HASH h = FIO_MAP_HASH_FN(k);
  h = FIO_MAP_HASH_FIX(h);
  return h;
}
#define FIO_MAP_HASH_CACHED 0
#define FIO_MAP_HASH_GET_HASH(map_ptr, index)                                  \
  FIO_NAME(FIO_MAP_NAME, __get_hash)                                           \
  (FIO_MAP_OBJ2KEY((map_ptr)->map[(index)].obj))
#else
#define FIO_MAP_HASH_GET_HASH(map_ptr, index) (map_ptr)->map[(index)].hash
#define FIO_MAP_HASH_CACHED                   1
#endif

#ifndef FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT
/* Hash to Array optimization limit in log2. MUST be less then 8. */
#define FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT 3
#endif

/**
 * Normally, FIO_MAP uses 32bit internal indexing and types.
 *
 * This limits the map to approximately 2 billion items (2,147,483,648).
 * Depending on possible 32 bit hash collisions, more items may be inserted.
 *
 * If FIO_MAP_BIG is be defined, 64 bit addressing is used, increasing the
 * maximum number of items to... hmm... a lot (1 << 63).
 */
#ifdef FIO_MAP_BIG
#define FIO_MAP_SIZE_TYPE      uint64_t
#define FIO_MAP_INDEX_USED_BIT ((uint64_t)1 << 63)
#else
#define FIO_MAP_SIZE_TYPE      uint32_t
#define FIO_MAP_INDEX_USED_BIT ((uint32_t)1 << 31)
#endif /* FIO_MAP_BIG */
/* *****************************************************************************
Pointer Tagging Support
***************************************************************************** */

#ifdef FIO_PTR_TAG_TYPE
#define FIO_MAP_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, s) *
#endif

/* *****************************************************************************





Map API





***************************************************************************** */

/* *****************************************************************************
Types
***************************************************************************** */

/** The type for each node in the map. */
typedef struct FIO_NAME(FIO_MAP_NAME, node_s) FIO_NAME(FIO_MAP_NAME, node_s);
/** The Map Type (container) itself. */
typedef struct FIO_NAME(FIO_MAP_NAME, s) FIO_NAME(FIO_MAP_NAME, s);

#ifndef FIO_MAP_INIT
/* Initialization macro. */
#define FIO_MAP_INIT                                                           \
  { 0 }
#endif

struct FIO_NAME(FIO_MAP_NAME, node_s) {
  /** the data being stored in the Map / key-value pair: obj.key obj.value. */
  FIO_MAP_OBJ obj;
#if FIO_MAP_HASH_CACHED
  /** a copy of the hash value. */
  FIO_MAP_HASH hash;
#endif
#if FIO_MAP_EVICT_LRU
  /** LRU evicion monitoring - do not access directly */
  struct {
    FIO_MAP_SIZE_TYPE next;
    FIO_MAP_SIZE_TYPE prev;
  } node;
#endif /* FIO_MAP_EVICT_LRU */
};

/* *****************************************************************************
Contruction API
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void);

/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR map);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, reinitializing its container. */
SFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR map);

/* *****************************************************************************
Get / Set / Remove
***************************************************************************** */

/** Gets a value from the map, returning a temporary pointer. */
SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR map,
                                                    FIO_MAP_HASH hash,
                                                    FIO_MAP_OBJ_KEY key);

/** Sets a value in the map, returning a temporary pointer. */
SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, set_ptr)(FIO_MAP_PTR map,
                                                    FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                    FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                    FIO_MAP_TYPE obj,
                                                    FIO_MAP_TYPE *old,
                                                    uint8_t overwrite);

/** Gets a value from the map, if exists. */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR map,
                                                   FIO_MAP_HASH hash,
                                                   FIO_MAP_OBJ_KEY key);

/** Sets a value in the map, overwriting existing data if any. */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR map,
                                                   FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                   FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                   FIO_MAP_TYPE obj,
                                                   FIO_MAP_TYPE *old);

/** Removes a value from the map. */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR map,
                                         FIO_MAP_HASH hash,
                                         FIO_MAP_OBJ_KEY key,
                                         FIO_MAP_TYPE *old);

/** Sets the object only if missing. Otherwise keeps existing value. */
FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, set_if_missing)(FIO_MAP_PTR map,
                                                              FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                              FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                              FIO_MAP_TYPE obj);

/** Removes all objects from the map. */
SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map);

/**
 * If `FIO_MAP_EVICT_LRU` is defined, evicts `number_of_elements` least
 * recently accessed.
 *
 * Otherwise, eviction is somewhat random and undefined.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR map,
                                        size_t number_of_elements);

/* *****************************************************************************
Object state information
***************************************************************************** */

/** Returns the maps current object count. */
FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR map);

/** Returns the maps current theoretical capacity. */
FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR map);

/** Reservse enough space for a theoretical capacity of `capa` objects. */
SFUNC size_t FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_PTR map,
                                             FIO_MAP_SIZE_TYPE capa);

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR map);

/** Rehashes the map. No need to call this, rehashing is automatic. */
SFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_PTR map);

/* *****************************************************************************
Iteration
***************************************************************************** */

/** Takes a previous (or NULL) item's position and returns the next. */
SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, each_next)(FIO_MAP_PTR map,
                                      FIO_NAME(FIO_MAP_NAME, node_s) * *first,
                                      FIO_NAME(FIO_MAP_NAME, node_s) * pos);

/** Iteration information structure passed to the callback. */
typedef struct FIO_NAME(FIO_MAP_NAME, each_s) {
  /** The being iterated. Once set, cannot be safely changed. */
  FIO_MAP_PTR const parent;
  /** The current object's index */
  uint64_t index;
  /** Either 1 (set) or 2 (map), and may be used to allow type detection. */
  const int64_t items_at_index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(struct FIO_NAME(FIO_MAP_NAME, each_s) * info);
  /** Opaque user data. */
  void *udata;
  /** The object / value at the current index. */
  FIO_MAP_TYPE value;
#ifdef FIO_MAP_KEY
  /** The key used to access the specific value. */
  FIO_MAP_KEY key;
#endif
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
SFUNC FIO_MAP_SIZE_TYPE
    FIO_NAME(FIO_MAP_NAME, each)(FIO_MAP_PTR map,
                                 int (*task)(FIO_NAME(FIO_MAP_NAME, each_s) *),
                                 void *udata,
                                 ssize_t start_at);

/* *****************************************************************************





Common Map Implementation - inlined static functions





***************************************************************************** */

FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, get)(FIO_MAP_PTR map,
                                                   FIO_MAP_HASH hash,
                                                   FIO_MAP_OBJ_KEY key) {
  FIO_MAP_TYPE *r = FIO_NAME(FIO_MAP_NAME, get_ptr)(map, hash, key);
  if (!r)
    return FIO_MAP_TYPE_INVALID;
  return *r;
}

FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, set)(FIO_MAP_PTR map,
                                                   FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                   FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                   FIO_MAP_TYPE obj,
                                                   FIO_MAP_TYPE *old) {
  FIO_MAP_TYPE *r = FIO_NAME(FIO_MAP_NAME, set_ptr)(map,
                                                    hash,
#ifdef FIO_MAP_KEY
                                                    key,
#endif /* FIO_MAP_KEY */
                                                    obj,
                                                    old,
                                                    1);
  if (!r)
    return FIO_MAP_TYPE_INVALID;
  return *r;
}

FIO_IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME,
                                set_if_missing)(FIO_MAP_PTR map,
                                                FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                FIO_MAP_TYPE obj) {
  FIO_MAP_TYPE *r = FIO_NAME(FIO_MAP_NAME, set_ptr)(map,
                                                    hash,
#ifdef FIO_MAP_KEY
                                                    key,
#endif /* FIO_MAP_KEY */
                                                    obj,
                                                    NULL,
                                                    0);
  if (!r)
    return FIO_MAP_TYPE_INVALID;
  return *r;
}

/* *****************************************************************************
Iteration Macro
***************************************************************************** */
#ifndef FIO_MAP_EACH
/**
 * A macro for a `for` loop that iterates over all the Map's objects (in
 * order).
 *
 * Use this macro for small Hash Maps / Sets.
 *
 * - `map_name` is the Map's type name / function prefix, same as FIO_MAP_NAME.
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
#define FIO_MAP_EACH(map_name, map_p, pos)                                     \
  for (FIO_NAME(map_name,                                                      \
                node_s) *first___ = NULL,                                      \
                        *pos = FIO_NAME(map_name,                              \
                                        each_next)(map_p, &first___, NULL);    \
       pos;                                                                    \
       pos = FIO_NAME(map_name, each_next)(map_p, &first___, pos))
#endif

/* *****************************************************************************
Common Map Settings - Finish
***************************************************************************** */
#endif
