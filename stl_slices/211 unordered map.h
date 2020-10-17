/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_UMAP_NAME umap          /* Development inclusion - ignore line */
#define FIO_UMAP_TEST               /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                  Unordered Map - an Unordered Hash Map / Set










***************************************************************************** */
#ifdef FIO_UMAP_NAME

/* *****************************************************************************
Unordered Map Settings

The following macros are used to customize the unordered map.
***************************************************************************** */

/**
 * Normally, FIO_UMAP uses 32bit internal indexing and types.
 *
 * This limits the map to approximately 2 billion items (2,147,483,648).
 * Depending on possible 32 bit hash collisions, more items may be inserted.
 *
 * If FIO_UMAP_BIG is be defined, 64 bit addressing is used, increasing the
 * maximum number of items to... hmm... a lot (1 << 63).
 */
#ifdef FIO_UMAP_BIG
#define FIO_UMAP_SIZE_TYPE      uint64_t
#define FIO_UMAP_INDEX_USED_BIT ((uint64_t)1 << 63)
#else
#define FIO_UMAP_SIZE_TYPE      uint32_t
#define FIO_UMAP_INDEX_USED_BIT ((uint32_t)1 << 31)
#endif /* FIO_UMAP_BIG */

#ifndef FIO_UMAP_TYPE
/** The type for the elements in the map */
#define FIO_UMAP_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_UMAP_TYPE_INVALID NULL
#else
#ifndef FIO_UMAP_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_UMAP_TYPE_INVALID ((FIO_UMAP_TYPE){0})
#endif /* FIO_UMAP_TYPE_INVALID */
#endif /* FIO_UMAP_TYPE */

#ifndef FIO_UMAP_TYPE_COPY
/** Handles a copy operation for an value. */
#define FIO_UMAP_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - do not set */
#define FIO_UMAP_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_UMAP_TYPE_DESTROY
/** Handles a destroy / free operation for a map's value. */
#define FIO_UMAP_TYPE_DESTROY(obj)
/** internal flag - set only if the object desctructor is optional */
#define FIO_UMAP_TYPE_DESTROY_SIMPLE 1
#else
#ifndef FIO_UMAP_TYPE_DESTROY_SIMPLE
#define FIO_UMAP_TYPE_DESTROY_SIMPLE 0
#endif
#endif

#ifndef FIO_UMAP_TYPE_DISCARD
/** Handles discarded value data (i.e., insert without overwrite). */
#define FIO_UMAP_TYPE_DISCARD(obj)
#endif

#ifndef FIO_UMAP_TYPE_CMP
/** Handles a comparison operation for a map's value. */
#define FIO_UMAP_TYPE_CMP(a, b) 1
#endif

/**
 * The FIO_UMAP_DESTROY_AFTER_COPY macro should be set if FIO_UMAP_TYPE_DESTROY
 * should be called after FIO_UMAP_TYPE_COPY when an object is removed from the
 * array after being copied to an external container (an `old` pointer)
 */
#ifndef FIO_UMAP_DESTROY_AFTER_COPY
#if !FIO_UMAP_TYPE_DESTROY_SIMPLE && !FIO_UMAP_TYPE_COPY_SIMPLE
#define FIO_UMAP_DESTROY_AFTER_COPY 1
#else
#define FIO_UMAP_DESTROY_AFTER_COPY 0
#endif
#endif /* FIO_UMAP_DESTROY_AFTER_COPY */

/* *****************************************************************************
Unordered Hash Map - a Hash Map is basically a Set of couplets
***************************************************************************** */
/* Defining a key makes a Hash Map instead of a Set */
#ifdef FIO_UMAP_KEY

#ifndef FIO_UMAP_KEY_INVALID
/** An invalid value for the hash map key type (if any). */
#define FIO_UMAP_KEY_INVALID ((FIO_UMAP_KEY){0})
#endif

#ifndef FIO_UMAP_KEY_COPY
/** Handles a copy operation for a hash maps key. */
#define FIO_UMAP_KEY_COPY(dest, src) (dest) = (src)
#endif

#ifndef FIO_UMAP_KEY_DESTROY
/** Handles a destroy / free operation for a hash maps key. */
#define FIO_UMAP_KEY_DESTROY(obj)
/** internal flag - set only if the object desctructor is optional */
#define FIO_UMAP_KEY_DESTROY_SIMPLE 1
#else
#ifndef FIO_UMAP_KEY_DESTROY_SIMPLE
#define FIO_UMAP_KEY_DESTROY_SIMPLE 0
#endif
#endif

#ifndef FIO_UMAP_KEY_DISCARD
/** Handles discarded element data (i.e., when overwriting only the value). */
#define FIO_UMAP_KEY_DISCARD(obj)
#endif

#ifndef FIO_UMAP_KEY_CMP
/** Handles a comparison operation for a hash maps key. */
#define FIO_UMAP_KEY_CMP(a, b) 1
#endif

typedef struct {
  FIO_UMAP_KEY key;
  FIO_UMAP_TYPE value;
} FIO_NAME(FIO_UMAP_NAME, couplet_s);

FIO_IFUNC void FIO_NAME(FIO_UMAP_NAME, __couplet_copy)(
    FIO_NAME(FIO_UMAP_NAME, couplet_s) * dest,
    FIO_NAME(FIO_UMAP_NAME, couplet_s) * src) {
  FIO_UMAP_KEY_COPY((dest->key), (src->key));
  FIO_UMAP_TYPE_COPY((dest->value), (src->value));
}

FIO_IFUNC void FIO_NAME(FIO_UMAP_NAME,
                        __couplet_destroy)(FIO_NAME(FIO_UMAP_NAME, couplet_s) *
                                           c) {
  FIO_UMAP_KEY_DESTROY(c->key);
  FIO_UMAP_TYPE_DESTROY(c->value);
  (void)c; /* in case where macros do nothing */
}

/** FIO_UMAP_OBJ is either a couplet (for hash maps) or the objet (for sets) */
#define FIO_UMAP_OBJ FIO_NAME(FIO_UMAP_NAME, couplet_s)

/** FIO_UMAP_OBJ_KEY is FIO_UMAP_KEY for hash maps or FIO_UMAP_TYPE for sets */
#define FIO_UMAP_OBJ_KEY FIO_UMAP_KEY

#define FIO_UMAP_OBJ_INVALID                                                   \
  ((FIO_NAME(FIO_UMAP_NAME, couplet_s)){.key = FIO_UMAP_KEY_INVALID,           \
                                        .value = FIO_UMAP_TYPE_INVALID})

#define FIO_UMAP_OBJ_COPY(dest, src)                                           \
  FIO_NAME(FIO_UMAP_NAME, __couplet_copy)(&(dest), &(src))

#define FIO_UMAP_OBJ_DESTROY(obj)                                              \
  FIO_NAME(FIO_UMAP_NAME, __couplet_destroy)(&(obj))

#define FIO_UMAP_OBJ_CMP(a, b)        FIO_UMAP_KEY_CMP((a).key, (b).key)
#define FIO_UMAP_OBJ_KEY_CMP(a, key_) FIO_UMAP_KEY_CMP((a).key, (key_))
#define FIO_UMAP_OBJ2KEY(o)           (o).key
#define FIO_UMAP_OBJ2TYPE(o)          (o).value

#define FIO_UMAP_OBJ_DISCARD(o)                                                \
  do {                                                                         \
    FIO_UMAP_TYPE_DISCARD(((o).value));                                        \
    FIO_UMAP_KEY_DISCARD(((o).key));                                           \
  } while (0);

#if FIO_UMAP_DESTROY_AFTER_COPY
#define FIO_UMAP_OBJ_DESTROY_AFTER FIO_UMAP_OBJ_DESTROY
#else
#define FIO_UMAP_OBJ_DESTROY_AFTER(obj) FIO_UMAP_KEY_DESTROY((obj).key);
#endif /* FIO_UMAP_DESTROY_AFTER_COPY */

/* *****************************************************************************
Unordered Map - Set
***************************************************************************** */
#else /* FIO_UMAP_KEY */
/** FIO_UMAP_OBJ is either a couplet (for hash maps) or the objet (for sets) */
#define FIO_UMAP_OBJ         FIO_UMAP_TYPE
/** FIO_UMAP_OBJ_KEY is FIO_UMAP_KEY for hash maps or FIO_UMAP_TYPE for sets */
#define FIO_UMAP_OBJ_KEY     FIO_UMAP_TYPE
#define FIO_UMAP_OBJ_INVALID FIO_UMAP_TYPE_INVALID
#define FIO_UMAP_OBJ_COPY    FIO_UMAP_TYPE_COPY
#define FIO_UMAP_OBJ_DESTROY FIO_UMAP_TYPE_DESTROY
#define FIO_UMAP_OBJ_CMP     FIO_UMAP_TYPE_CMP
#define FIO_UMAP_OBJ_KEY_CMP FIO_UMAP_TYPE_CMP
#define FIO_UMAP_OBJ2KEY(o)  (o)
#define FIO_UMAP_OBJ2TYPE(o) (o)
#define FIO_UMAP_OBJ_DISCARD FIO_UMAP_TYPE_DISCARD
#define FIO_UMAP_KEY_DISCARD(_ignore)
#define FIO_UMAP_KEY_COPY(_ignore, _ignore2)
#if FIO_UMAP_DESTROY_AFTER_COPY
#define FIO_UMAP_OBJ_DESTROY_AFTER FIO_UMAP_TYPE_DESTROY
#else
#define FIO_UMAP_OBJ_DESTROY_AFTER(obj)
#endif /* FIO_UMAP_DESTROY_AFTER_COPY */

#endif /* FIO_UMAP_KEY */

/* *****************************************************************************
Misc Settings (eviction policy, load-factor attempts, etc')
***************************************************************************** */

#ifndef FIO_UMAP_MAX_SEEK /* LIMITED to 255 */
/* The maximum number of bins to rotate when (partial/full) collisions occure */
#define FIO_UMAP_MAX_SEEK (4U)
#endif

#ifndef FIO_UMAP_MAX_FULL_COLLISIONS /* LIMITED to 255 */
/* The maximum number of full hash collisions that can be consumed */
#define FIO_UMAP_MAX_FULL_COLLISIONS (22U)
#endif

#ifndef FIO_UMAP_CUCKOO_STEPS
/* Prime numbers are better */
#define FIO_UMAP_CUCKOO_STEPS (0x43F82D0B) /* should be a high prime */
#endif

#ifndef FIO_UMAP_EVICT_LRU
/** Set the `evict` method to evict based on the Least Recently Used object. */
#define FIO_UMAP_EVICT_LRU 1
#endif

/* *****************************************************************************
Pointer Tagging Support
***************************************************************************** */

#ifdef FIO_PTR_TAG_TYPE
#define FIO_UMAP_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_UMAP_PTR FIO_NAME(FIO_UMAP_NAME, s) *
#endif

/* *****************************************************************************





Unordered Map API





***************************************************************************** */

/* *****************************************************************************
Types
***************************************************************************** */

/** The type for each member in the unordered map */
typedef struct {
  /** the data being stored in the Map. */
  FIO_UMAP_OBJ obj;
  /** a copy of the hash value. */
  uintptr_t hash;
#if FIO_UMAP_EVICT_LRU
  /** LRU evicion monitoring - do not access directly */
  struct {
    FIO_UMAP_SIZE_TYPE next;
    FIO_UMAP_SIZE_TYPE prev;
  } node;
#endif /* FIO_UMAP_EVICT_LRU */
} FIO_NAME(FIO_UMAP_NAME, each_s);

/** An Unordered Map Type */
typedef struct {
  /** Internal map / memory - do not access directly */
  FIO_NAME(FIO_UMAP_NAME, each_s) * map;
  /** Object count - do not access directly */
  FIO_UMAP_SIZE_TYPE count;
#if FIO_UMAP_EVICT_LRU
  /** LRU evicion monitoring - do not access directly */
  FIO_UMAP_SIZE_TYPE last_used;
#endif /* FIO_UMAP_EVICT_LRU */
  uint8_t bits;
  uint8_t under_attack;
} FIO_NAME(FIO_UMAP_NAME, s);

#ifndef FIO_UMAP_INIT
/* Initialization macro. */
#define FIO_UMAP_INIT                                                          \
  { 0 }
#endif

/* *****************************************************************************
Contruction API
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_UMAP_PTR FIO_NAME(FIO_UMAP_NAME, new)(void);

/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_UMAP_NAME, free)(FIO_UMAP_PTR map);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the object, reinitializing its container. */
SFUNC void FIO_NAME(FIO_UMAP_NAME, destroy)(FIO_UMAP_PTR map);

/* *****************************************************************************
Get / Set / Remove
***************************************************************************** */

/** Gets a value from the map, returning a temporary pointer. */
SFUNC FIO_UMAP_TYPE *FIO_NAME(FIO_UMAP_NAME, get_ptr)(FIO_UMAP_PTR map,
                                                      uintptr_t hash,
                                                      FIO_UMAP_OBJ_KEY key);

/** Sets a value in the map, returning a temporary pointer. */
SFUNC FIO_UMAP_TYPE *FIO_NAME(FIO_UMAP_NAME, set_ptr)(FIO_UMAP_PTR map,
                                                      uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                      FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                      FIO_UMAP_TYPE obj,
                                                      FIO_UMAP_TYPE *old,
                                                      uint8_t overwrite);

/** Gets a value from the map, if exists. */
SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, get)(FIO_UMAP_PTR map,
                                                 uintptr_t hash,
                                                 FIO_UMAP_OBJ_KEY key);

/** Sets a value in the map, overwriting existing data if any. */
SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, set)(FIO_UMAP_PTR map,
                                                 uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                 FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                 FIO_UMAP_TYPE obj,
                                                 FIO_UMAP_TYPE *old);

/** Removes a value from the map. */
SFUNC int FIO_NAME(FIO_UMAP_NAME, remove)(FIO_UMAP_PTR map,
                                          uintptr_t hash,
                                          FIO_UMAP_OBJ_KEY key,
                                          FIO_UMAP_TYPE *old);

/** Sets the object only if missing. Otherwise keeps existing value. */
SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, set_if_missing)(FIO_UMAP_PTR map,
                                                            uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                            FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                            FIO_UMAP_TYPE obj);

/** Removes all objects from the map. */
SFUNC void FIO_NAME(FIO_UMAP_NAME, clear)(FIO_UMAP_PTR map);

/**
 * If `FIO_UMAP_EVICT_LRU` is defined, evicts `number_of_elements` least
 * recently accessed.
 *
 * Otherwise, eviction is somewhat random and undefined.
 */
SFUNC int FIO_NAME(FIO_UMAP_NAME, evict)(FIO_UMAP_PTR map,
                                         size_t number_of_elements);

/* *****************************************************************************
Object state information
***************************************************************************** */

/** Returns the maps current object count. */
FIO_IFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, count)(FIO_UMAP_PTR map);

/** Returns the maps current theoretical capacity. */
FIO_IFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, capa)(FIO_UMAP_PTR map);

/** Reservse enough space for a theoretical capacity of `capa` objects. */
SFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, reserve)(FIO_UMAP_PTR map,
                                                 FIO_UMAP_SIZE_TYPE capa);

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_UMAP_NAME, compact)(FIO_UMAP_PTR map);

/** Rehashes the map. No need to call this, rehashing is automatic. */
SFUNC int FIO_NAME(FIO_UMAP_NAME, rehash)(FIO_UMAP_PTR map);

/* *****************************************************************************
Iteration
***************************************************************************** */

SFUNC FIO_NAME(FIO_UMAP_NAME, each_s) *
    FIO_NAME(FIO_UMAP_NAME, each_next)(FIO_UMAP_PTR map,
                                       FIO_NAME(FIO_UMAP_NAME, each_s) * *first,
                                       FIO_NAME(FIO_UMAP_NAME, each_s) * pos);

SFUNC FIO_UMAP_SIZE_TYPE FIO_NAME(FIO_UMAP_NAME,
                                  each)(FIO_UMAP_PTR map,
                                        FIO_UMAP_SIZE_TYPE start_at,
                                        int (*task)(FIO_UMAP_TYPE obj,
                                                    void *arg),
                                        void *arg);

#ifdef FIO_UMAP_KEY
SFUNC FIO_UMAP_KEY FIO_NAME(FIO_UMAP_NAME, each_get_key)(void);
#endif
/* *****************************************************************************
Unordered Map Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size) fio_free((ptr))

*/

#ifndef FIO_UMAP_BITS2CAPA
#define FIO_UMAP_BITS2CAPA(bits) ((uintptr_t)1ULL << (bits))
#endif

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_UMAP_PTR FIO_NAME(FIO_UMAP_NAME, new)(void) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*m), 0);
  if (!m)
    return (FIO_UMAP_PTR)NULL;
  *m = (FIO_NAME(FIO_UMAP_NAME, s))FIO_UMAP_INIT;
  return (FIO_UMAP_PTR)FIO_PTR_TAG(m);
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_UMAP_NAME, free)(FIO_UMAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  FIO_NAME(FIO_UMAP_NAME, destroy)(map);
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  FIO_MEM_FREE_(m, sizeof(*m));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

FIO_IFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, count)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  return m->count;
}

FIO_IFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, capa)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  return FIO_UMAP_BITS2CAPA(m->bits);
}

/* *****************************************************************************
Unordered Map Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

#ifndef FIO_UMAP_MEMORY_SIZE
#define FIO_UMAP_MEMORY_SIZE(bits)                                             \
  ((sizeof(FIO_NAME(FIO_UMAP_NAME, each_s)) + sizeof(uint8_t)) *               \
   FIO_UMAP_BITS2CAPA(bits))
#endif
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size) fio_free((ptr))

*/

/* *****************************************************************************
Unordered Map Implementation - helper functions.
***************************************************************************** */

#ifndef FIO_UMAP___IMAP_DELETED
#define FIO_UMAP___IMAP_DELETED 255
#endif
#ifndef FIO_UMAP___IMAP_FREE
#define FIO_UMAP___IMAP_FREE 0
#endif

#ifndef FIO_UMAP___NORMALIZE_HASH
#define FIO_UMAP___NORMALIZE_HASH(hash) ((hash) ? (hash) : -2)
#endif

FIO_IFUNC uint8_t *FIO_NAME(FIO_UMAP_NAME,
                            __imap)(FIO_NAME(FIO_UMAP_NAME, s) * m) {
  return (uint8_t *)(m->map + FIO_UMAP_BITS2CAPA(m->bits));
}

FIO_IFUNC FIO_UMAP_SIZE_TYPE FIO_NAME(FIO_UMAP_NAME,
                                      __hash2imap)(uintptr_t hash,
                                                   uint8_t bits) {
  FIO_UMAP_SIZE_TYPE r = (((hash >> bits) ^ hash) & 255);
  if (!r || r == 255)
    r ^= 1;
  return r;
}

FIO_SFUNC FIO_UMAP_SIZE_TYPE FIO_NAME(FIO_UMAP_NAME,
                                      __index)(FIO_NAME(FIO_UMAP_NAME, s) * m,
                                               const uintptr_t hash,
                                               FIO_UMAP_OBJ_KEY key) {
  FIO_UMAP_SIZE_TYPE pos = (FIO_UMAP_SIZE_TYPE)-1LL;
  FIO_UMAP_SIZE_TYPE free_slot = (FIO_UMAP_SIZE_TYPE)-1LL;
  size_t total_collisions = 0;
  if (!m->map)
    return pos;
  const uint8_t *imap = FIO_NAME(FIO_UMAP_NAME, __imap)(m);
  /* note: hash MUST be normalized by this point */
  const uint64_t simd_base =
      FIO_NAME(FIO_UMAP_NAME, __hash2imap)(hash, m->bits) *
      UINT64_C(0x0101010101010101);
  const uint64_t pos_mask = FIO_UMAP_BITS2CAPA(m->bits) - 1;
  const int max_attempts =
      ((FIO_UMAP_BITS2CAPA(m->bits) >> 3) >= FIO_UMAP_MAX_SEEK
           ? (int)FIO_UMAP_MAX_SEEK
           : (FIO_UMAP_BITS2CAPA(m->bits) >> 3)
                 ? (int)(FIO_UMAP_BITS2CAPA(m->bits) >> 3)
                 : (int)1);
  for (int attempts = 0; attempts < max_attempts; ++attempts) {
    pos = (hash + (FIO_UMAP_CUCKOO_STEPS * attempts)) & pos_mask;
    uint64_t simd_result =
        simd_base ^ ((uint64_t)imap[pos & pos_mask] |
                     ((uint64_t)imap[(pos + 1) & pos_mask] << (1 * 8)) |
                     ((uint64_t)imap[(pos + 2) & pos_mask] << (2 * 8)) |
                     ((uint64_t)imap[(pos + 3) & pos_mask] << (3 * 8)) |
                     ((uint64_t)imap[(pos + 4) & pos_mask] << (4 * 8)) |
                     ((uint64_t)imap[(pos + 5) & pos_mask] << (5 * 8)) |
                     ((uint64_t)imap[(pos + 6) & pos_mask] << (6 * 8)) |
                     ((uint64_t)imap[(pos + 7) & pos_mask] << (7 * 8)));

    /* test for exact match in each of the bytes in the 8 byte group */
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if ((simd_result - UINT64_C(0x0101010101010101)) &
        ((~simd_result) & UINT64_C(0x8080808080808080)))
      for (size_t byte = 0; byte < 8; ++byte) {
        /* test cache friendly 8bit match */
        if (!(simd_result & (UINT64_C(0xFF) << (byte << 3)))) {
          /* test full hash */
          if (m->map[(pos + byte) & pos_mask].hash == hash) {
            /* test full collisions (attack) / match */
            if (m->under_attack ||
                FIO_UMAP_OBJ_KEY_CMP(m->map[(pos + byte) & pos_mask].obj,
                                     key)) {
              pos = (pos + byte) & pos_mask;
              return pos;
            } else if (++total_collisions >= FIO_UMAP_MAX_FULL_COLLISIONS) {
              m->under_attack = 1;
              FIO_LOG_SECURITY("Unordered map under attack?");
            }
          }
        }
      }
    /* test if there's an available slot in the group */
    if (free_slot == (FIO_UMAP_SIZE_TYPE)-1LL) {
      for (int byte = 0; byte < 8; ++byte) {
        if (imap[(pos + byte) & pos_mask] == 255 ||
            !imap[(pos + byte) & pos_mask]) {
          free_slot = (pos + byte) & pos_mask;
          break;
        }
      }
    }
    /* test if there's a free slot in the group (never used => stop seeking) */
    simd_result ^= simd_base;
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if ((simd_result - UINT64_C(0x0101010101010101)) &
        ((~simd_result) & UINT64_C(0x8080808080808080)))
      break;
  }

  pos = free_slot;
  return pos;
  (void)key; /* if unused */
}

FIO_IFUNC int FIO_NAME(FIO_UMAP_NAME, __realloc)(FIO_NAME(FIO_UMAP_NAME, s) * m,
                                                 size_t bits) {
  if (!m || bits >= (sizeof(FIO_UMAP_SIZE_TYPE) * 8))
    return -1;
  if (bits < 3)
    bits = 3;
  FIO_NAME(FIO_UMAP_NAME, each_s) *tmp =
      FIO_MEM_REALLOC_(NULL, 0, FIO_UMAP_MEMORY_SIZE(bits), 0);
  if (!tmp)
    return -1;
  if (!FIO_MEM_REALLOC_IS_SAFE_)
    memset(tmp, 0, FIO_UMAP_MEMORY_SIZE(bits));
  /* rehash the map */
  FIO_NAME(FIO_UMAP_NAME, s) m2;
  m2 = (FIO_NAME(FIO_UMAP_NAME, s)){
      .map = tmp,
      .bits = bits,
  };
  if (m->count) {
#if FIO_UMAP_EVICT_LRU
    /* use eviction list to re-insert data. */
    FIO_UMAP_SIZE_TYPE last = 0;
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, i) {
      /* place old values in new hash */
      FIO_UMAP_SIZE_TYPE pos = FIO_NAME(
          FIO_UMAP_NAME,
          __index)(&m2, m->map[i].hash, FIO_UMAP_OBJ2KEY(m->map[i].obj));
      if (pos == (FIO_UMAP_SIZE_TYPE)-1)
        goto error;
      FIO_NAME(FIO_UMAP_NAME, __imap)
      (&m2)[pos] =
          FIO_NAME(FIO_UMAP_NAME, __hash2imap)(m->map[i].hash, m2.bits);
      m2.map[pos].hash = m->map[i].hash;
      m2.map[pos].obj = m->map[i].obj;
      if (m2.count) {
        FIO_INDEXED_LIST_PUSH(m2.map, node, last, pos);
      } else {
        m2.map[pos].node.prev = m2.map[pos].node.next = pos;
        m2.last_used = pos;
      }
      last = pos;
      ++m2.count;
    }
#else  /* FIO_UMAP_EVICT_LRU */
    /* scan map for used slots to re-insert data */
    uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_UMAP_NAME, __imap)(m);
    for (FIO_UMAP_SIZE_TYPE i = 0;
         m2.count < m->count && i < FIO_UMAP_BITS2CAPA(m->bits);
         i += 8) {
      /* skip empty groups (test for all bytes == 0 || 255) */
      {
        register const size_t i8 = i >> 3;
        if (((((imap64[i8] - UINT64_C(0x0101010101010101)) & (~(imap64[i8]))) |
              (imap64[i8] & (~(imap64[i8] + UINT64_C(0x0101010101010101))))) &
             UINT64_C(0x8080808080808080)) == UINT64_C(0x8080808080808080))
          continue;
      }
      for (int j = 0; j < 8; ++j) {
        const FIO_UMAP_SIZE_TYPE n = i + j;
        if (m->map[n].hash) {
          /* place in new hash */
          FIO_UMAP_SIZE_TYPE pos = FIO_NAME(FIO_UMAP_NAME, __index)(
              &m2,
              m->map[n].hash,
              FIO_UMAP_OBJ2KEY(m->map[i + j].obj));
          if (pos == (FIO_UMAP_SIZE_TYPE)-1)
            goto error;
          FIO_NAME(FIO_UMAP_NAME, __imap)
          (&m2)[pos] =
              FIO_NAME(FIO_UMAP_NAME, __hash2imap)(m->map[n].hash, m2.bits);
          m2.map[pos] = m->map[n];
          ++m2.count;
        }
      }
    }
#endif /* FIO_UMAP_EVICT_LRU */
  }

  FIO_MEM_FREE_(m->map, FIO_UMAP_MEMORY_SIZE(m->bits));
  *m = m2;
  return 0;
error:
  FIO_MEM_FREE_(tmp, FIO_UMAP_MEMORY_SIZE(bits));
  return -1;
}

/* *****************************************************************************
Unordered Map Implementation - API implementation
***************************************************************************** */

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_UMAP_NAME, destroy)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
/* add destruction logic */
#if !FIO_UMAP_TYPE_DESTROY_SIMPLE
  FIO_NAME(FIO_UMAP_NAME, clear)(map);
#endif
  FIO_MEM_FREE_(m->map, FIO_UMAP_MEMORY_SIZE(m->bits));
  *m = (FIO_NAME(FIO_UMAP_NAME, s))FIO_UMAP_INIT;
  return;
}

/* *****************************************************************************
Get / Set / Remove
***************************************************************************** */

SFUNC FIO_UMAP_TYPE *FIO_NAME(FIO_UMAP_NAME, get_ptr)(FIO_UMAP_PTR map,
                                                      uintptr_t hash,
                                                      FIO_UMAP_OBJ_KEY key) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_UMAP___NORMALIZE_HASH(hash);
  FIO_UMAP_SIZE_TYPE pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, key);
  if (pos == (FIO_UMAP_SIZE_TYPE)(-1) ||
      FIO_NAME(FIO_UMAP_NAME, __imap)(m)[pos] == 255 ||
      !FIO_NAME(FIO_UMAP_NAME, __imap)(m)[pos] || !m->map[pos].hash)
    return NULL;
#if FIO_UMAP_EVICT_LRU
  if (m->last_used != pos) {
    FIO_INDEXED_LIST_REMOVE(m->map, node, pos);
    FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
    m->last_used = pos;
  }
#endif /* FIO_UMAP_EVICT_LRU */
  return &FIO_UMAP_OBJ2TYPE(m->map[pos].obj);
}

SFUNC FIO_UMAP_TYPE *FIO_NAME(FIO_UMAP_NAME, set_ptr)(FIO_UMAP_PTR map,
                                                      uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                      FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                      FIO_UMAP_TYPE obj,
                                                      FIO_UMAP_TYPE *old,
                                                      uint8_t overwrite) {
  if (old)
    *old = FIO_UMAP_TYPE_INVALID;
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_UMAP___NORMALIZE_HASH(hash);
#ifdef FIO_UMAP_KEY
  FIO_UMAP_SIZE_TYPE pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, key);
#else
  FIO_UMAP_SIZE_TYPE pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, obj);
#endif /* FIO_UMAP_KEY */

  for (int i = 0; pos == (FIO_UMAP_SIZE_TYPE)-1 && i < 2; ++i) {
    if (FIO_NAME(FIO_UMAP_NAME, __realloc)(m, m->bits + 1))
      goto error;
#ifdef FIO_UMAP_KEY
    pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, key);
#else
    pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, obj);
#endif /* FIO_UMAP_KEY */
  }
  if (pos == (FIO_UMAP_SIZE_TYPE)-1)
    goto error;
  if (!m->map[pos].hash) {
    /* new */
    FIO_NAME(FIO_UMAP_NAME, __imap)
    (m)[pos] = FIO_NAME(FIO_UMAP_NAME, __hash2imap)(hash, m->bits);
    m->map[pos].hash = hash;
    FIO_UMAP_TYPE_COPY(FIO_UMAP_OBJ2TYPE(m->map[pos].obj), obj);
    FIO_UMAP_KEY_COPY(FIO_UMAP_OBJ2KEY(m->map[pos].obj), key);
#if FIO_UMAP_EVICT_LRU
    if (m->count) {
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
    } else {
      m->map[pos].node.prev = m->map[pos].node.next = pos;
    }
    m->last_used = pos;
#endif /* FIO_UMAP_EVICT_LRU */
    ++m->count;
  } else if (overwrite) {
    /* overwrite existing */
    FIO_UMAP_KEY_DISCARD(key);
    if (old) {
      FIO_UMAP_TYPE_COPY(old[0], FIO_UMAP_OBJ2TYPE(m->map[pos].obj));
      if (FIO_UMAP_DESTROY_AFTER_COPY) {
        FIO_UMAP_TYPE_DESTROY(FIO_UMAP_OBJ2TYPE(m->map[pos].obj));
      }
    } else {
      FIO_UMAP_TYPE_DESTROY(FIO_UMAP_OBJ2TYPE(m->map[pos].obj));
    }
    FIO_UMAP_TYPE_COPY(FIO_UMAP_OBJ2TYPE(m->map[pos].obj), obj);
#if FIO_UMAP_EVICT_LRU
    if (m->last_used != pos) {
      FIO_INDEXED_LIST_REMOVE(m->map, node, pos);
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
      m->last_used = pos;
    }
#endif /* FIO_UMAP_EVICT_LRU */
  } else {
    FIO_UMAP_TYPE_DISCARD(obj);
    FIO_UMAP_KEY_DISCARD(key);
  }
  return &FIO_UMAP_OBJ2TYPE(m->map[pos].obj);

error:
  FIO_UMAP_TYPE_DISCARD(obj);
  FIO_UMAP_KEY_DISCARD(key);
  return NULL;
}

SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, get)(FIO_UMAP_PTR map,
                                                 uintptr_t hash,
                                                 FIO_UMAP_OBJ_KEY key) {
  FIO_UMAP_TYPE *r = FIO_NAME(FIO_UMAP_NAME, get_ptr)(map, hash, key);
  if (!r)
    return FIO_UMAP_TYPE_INVALID;
  return *r;
}

SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, set)(FIO_UMAP_PTR map,
                                                 uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                 FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                 FIO_UMAP_TYPE obj,
                                                 FIO_UMAP_TYPE *old) {
  FIO_UMAP_TYPE *r = FIO_NAME(FIO_UMAP_NAME, set_ptr)(map,
                                                      hash,
#ifdef FIO_UMAP_KEY
                                                      key,
#endif /* FIO_UMAP_KEY */
                                                      obj,
                                                      old,
                                                      1);
  if (!r)
    return FIO_UMAP_TYPE_INVALID;
  return *r;
}

SFUNC int FIO_NAME(FIO_UMAP_NAME, remove)(FIO_UMAP_PTR map,
                                          uintptr_t hash,
                                          FIO_UMAP_OBJ_KEY key,
                                          FIO_UMAP_TYPE *old) {
  if (old)
    *old = FIO_UMAP_TYPE_INVALID;
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !m->count)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_UMAP___NORMALIZE_HASH(hash);
  FIO_UMAP_SIZE_TYPE pos = FIO_NAME(FIO_UMAP_NAME, __index)(m, hash, key);
  if (pos == (FIO_UMAP_SIZE_TYPE)(-1) ||
      FIO_NAME(FIO_UMAP_NAME, __imap)(m)[pos] == 255 ||
      !FIO_NAME(FIO_UMAP_NAME, __imap)(m)[pos] || !m->map[pos].hash)
    return -1;
  FIO_NAME(FIO_UMAP_NAME, __imap)(m)[pos] = 255;
  m->map[pos].hash = 0;
  --m->count;
  if (old) {
    FIO_UMAP_TYPE_COPY(*old, FIO_UMAP_OBJ2TYPE(m->map[pos].obj));
    FIO_UMAP_OBJ_DESTROY_AFTER(m->map[pos].obj)
  } else {
    FIO_UMAP_OBJ_DESTROY(m->map[pos].obj);
  }
  return 0;
}

SFUNC FIO_UMAP_TYPE FIO_NAME(FIO_UMAP_NAME, set_if_missing)(FIO_UMAP_PTR map,
                                                            uintptr_t hash,
#ifdef FIO_UMAP_KEY
                                                            FIO_UMAP_KEY key,
#endif /* FIO_UMAP_KEY */
                                                            FIO_UMAP_TYPE obj) {
  FIO_UMAP_TYPE *r = FIO_NAME(FIO_UMAP_NAME, set_ptr)(map,
                                                      hash,
#ifdef FIO_UMAP_KEY
                                                      key,
#endif /* FIO_UMAP_KEY */
                                                      obj,
                                                      NULL,
                                                      0);
  if (!r)
    return FIO_UMAP_TYPE_INVALID;
  return *r;
}

SFUNC void FIO_NAME(FIO_UMAP_NAME, clear)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
#if FIO_UMAP_EVICT_LRU
  /* use LRU list to iterate and clear data. */

#else  /* FIO_UMAP_EVICT_LRU */
  /* scan map to clear data. */
  uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_UMAP_NAME, __imap)(m);
  for (FIO_UMAP_SIZE_TYPE i = 0; m->count && i < FIO_UMAP_BITS2CAPA(m->bits);
       i += 8) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    register const size_t i8 = i >> 3;
    if (((((imap64[i8] - UINT64_C(0x0101010101010101)) & (~imap64[i8])) |
          (imap64[i8] & (~(imap64[i8] + UINT64_C(0x0101010101010101))))) &
         UINT64_C(0x8080808080808080)) == UINT64_C(0x8080808080808080)) {
      imap64[i8] = 0;
      continue;
    }
    imap64[i8] = 0;
    for (int j = 0; j < 8; ++j) {
      if (m->map[i + j].hash) {
        FIO_UMAP_OBJ_DESTROY(m->map[i + j].obj);
        m->map[i + j].hash = 0;
        --m->count; /* stop seeking if no more elements */
      }
    }
  }
  FIO_ASSERT_DEBUG(!m->count, "logic error @ unordered map clear.");
#endif /* FIO_UMAP_EVICT_LRU */
}

SFUNC int FIO_NAME(FIO_UMAP_NAME, evict)(FIO_UMAP_PTR map,
                                         size_t number_of_elements) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  if (!m->count)
    return -1;
  if (number_of_elements >= m->count) {
    FIO_NAME(FIO_UMAP_NAME, clear)(map);
    return -1;
  }
#if FIO_UMAP_EVICT_LRU
  /* evict by LRU */
  do {
    FIO_UMAP_SIZE_TYPE n = m->map[m->last_used].node.prev;
    FIO_INDEXED_LIST_REMOVE(m->map, node, n);
  } while (--number_of_elements);
#else /* FIO_UMAP_EVICT_LRU */
  /* scan map and evict semi randomly. */
  uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_UMAP_NAME, __imap)(m);
  for (FIO_UMAP_SIZE_TYPE i = 0;
       number_of_elements && i < FIO_UMAP_BITS2CAPA(m->bits);
       i += 8) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    {
      register const size_t i8 = i >> 3;
      if (((((imap64[i8] - UINT64_C(0x0101010101010101)) & (~imap64[i8])) |
            (imap64[i8] & (~(imap64[i8] + UINT64_C(0x0101010101010101))))) &
           UINT64_C(0x8080808080808080)) == UINT64_C(0x8080808080808080)) {
        continue;
      }
    }
    for (int j = 0; number_of_elements && j < 8; ++j) {
      if (m->map[i + j].hash) {
        FIO_UMAP_OBJ_DESTROY(m->map[i + j].obj);
        m->map[i + j].hash = 0;
        FIO_NAME(FIO_UMAP_NAME, __imap)(m)[i + j] = 255;
        --m->count;
        --number_of_elements; /* stop evicting? */
      }
    }
  }

#endif /* FIO_UMAP_EVICT_LRU */
  return -1;
}

/* *****************************************************************************
Object state information
***************************************************************************** */

/** Reservse enough space for a theoretical capacity of `capa` objects. */
SFUNC uintptr_t FIO_NAME(FIO_UMAP_NAME, reserve)(FIO_UMAP_PTR map,
                                                 FIO_UMAP_SIZE_TYPE capa) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  if (FIO_UMAP_BITS2CAPA(m->bits) < capa) {
    size_t bits = 3;
    while (FIO_UMAP_BITS2CAPA(bits) < capa)
      ++bits;
    for (int i = 0; FIO_NAME(FIO_UMAP_NAME, __realloc)(m, bits + i) && i < 2;
         ++i) {
    }
    if (m->bits < bits)
      return 0;
  }
  return FIO_UMAP_BITS2CAPA(m->bits);
}

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_UMAP_NAME, compact)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  if (!m->bits)
    return;
  if (!m->count) {
    FIO_NAME(FIO_UMAP_NAME, clear)(map);
    return;
  }
  size_t bits = m->bits;
  size_t count = 0;
  while (bits && FIO_UMAP_BITS2CAPA((bits - 1)) > m->count) {
    --bits;
    ++count;
  }
  for (size_t i = 0; i < count; ++i) {
    if (!FIO_NAME(FIO_UMAP_NAME, __realloc)(m, bits + i))
      return;
  }
}

/** Rehashes the map. No need to call this, rehashing is automatic. */
SFUNC int FIO_NAME(FIO_UMAP_NAME, rehash)(FIO_UMAP_PTR map) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  return FIO_NAME(FIO_UMAP_NAME, __realloc)(m, m->bits);
}

/* *****************************************************************************
Iteration
***************************************************************************** */

SFUNC FIO_NAME(FIO_UMAP_NAME, each_s) *
    FIO_NAME(FIO_UMAP_NAME, each_next)(FIO_UMAP_PTR map,
                                       FIO_NAME(FIO_UMAP_NAME, each_s) * *first,
                                       FIO_NAME(FIO_UMAP_NAME, each_s) * pos) {
  FIO_NAME(FIO_UMAP_NAME, s) *m =
      (FIO_NAME(FIO_UMAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !first || !m->count)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  size_t i;
#if FIO_UMAP_EVICT_LRU
  if (!pos || !(*first)) {
    i = m->last_used;
    *first = (m->map + m->last_used);
  } else {
    i = pos - m->map;
  }
  if (m->map + m->map[i].node.next == *first)
    return NULL;
  return m->map + m->map[i].node.next;

#else  /*FIO_UMAP_EVICT_LRU*/
  if (!pos || !(*first)) {
    i = 0;
  } else {
    i = pos - m->map;
  }
  *first = m->map;
  while (i + 8 < FIO_UMAP_BITS2CAPA(m->bits)) {
    uint8_t *imap = FIO_NAME(FIO_UMAP_NAME, __imap)(m);
    /* test only groups with valid values (test for all bytes == 0 || 255 */
    register const uint64_t grp8 = fio_buf2u64_local(imap + i);
    if (((((grp8 - UINT64_C(0x0101010101010101)) & (~grp8)) |
          (grp8 & (~(grp8 + UINT64_C(0x0101010101010101))))) &
         UINT64_C(0x8080808080808080)) != UINT64_C(0x8080808080808080)) {
      for (int j = 0; j < 8; ++j) {
        if (m->map[i + j].hash)
          return m->map + i + j;
      }
    }
    i += 8;
  }
  while (i < FIO_UMAP_BITS2CAPA(m->bits)) {
    if (m->map[i].hash)
      return m->map + i;
    ++i;
  }
  return NULL;
#endif /*FIO_UMAP_EVICT_LRU*/
}

SFUNC FIO_UMAP_SIZE_TYPE FIO_NAME(FIO_UMAP_NAME,
                                  each)(FIO_UMAP_PTR map,
                                        FIO_UMAP_SIZE_TYPE start_at,
                                        int (*task)(FIO_UMAP_TYPE obj,
                                                    void *arg),
                                        void *arg);

#ifdef FIO_UMAP_KEY
FIO_SFUNC __thread FIO_UMAP_SIZE_TYPE FIO_NAME(FIO_UMAP_NAME, __each_pos) = 0;
FIO_SFUNC __thread FIO_NAME(FIO_UMAP_NAME, s) *
    FIO_NAME(FIO_UMAP_NAME, __each_map) = NULL;

SFUNC FIO_UMAP_KEY FIO_NAME(FIO_UMAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_UMAP_NAME, __each_map) ||
      !FIO_NAME(FIO_UMAP_NAME, __each_map)->count)
    return FIO_UMAP_KEY_INVALID;
  return FIO_NAME(FIO_UMAP_NAME, __each_map)
      ->map[FIO_NAME(FIO_UMAP_NAME, __each_pos)]
      .obj.key;
}
#endif
/* *****************************************************************************
Unordered Map Testing
***************************************************************************** */
#ifdef FIO_UMAP_TEST
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_UMAP_NAME)(void) {
/*
 * test unrodered maps here
 */
#ifdef FIO_UMAP_KEY
  fprintf(
      stderr,
      "* testing unordered map (hash-map)" FIO_MACRO2STR(FIO_UMAP_NAME) "\n");
#else
  fprintf(stderr,
          "* testing unordered map (set)" FIO_MACRO2STR(FIO_UMAP_NAME) "\n");
#endif
  FIO_NAME(FIO_UMAP_NAME, s) m = FIO_UMAP_INIT;
  for (size_t i = 1; i < 4096; ++i) {
    FIO_UMAP_TYPE old = (FIO_UMAP_TYPE)i;
#ifdef FIO_UMAP_KEY
    FIO_ASSERT(
        (FIO_UMAP_TYPE)i ==
            FIO_NAME(
                FIO_UMAP_NAME,
                set)(&m, (uintptr_t)i, (FIO_UMAP_KEY)i, (FIO_UMAP_TYPE)i, &old),
        "insertion failed at %zu",
        i);
#else
    FIO_ASSERT((FIO_UMAP_TYPE)i ==
                   FIO_NAME(FIO_UMAP_NAME,
                            set)(&m, (uintptr_t)i, (FIO_UMAP_TYPE)i, &old),
               "insertion failed at %zu",
               i);
#endif
    FIO_ASSERT(FIO_UMAP_TYPE_CMP(old, FIO_UMAP_TYPE_INVALID),
               "old value should be set to the invalid value");
    FIO_ASSERT(
        FIO_NAME(FIO_UMAP_NAME, get)(&m, (uintptr_t)i, (FIO_UMAP_TYPE)i) ==
            (FIO_UMAP_TYPE)i,
        "set-get roundtrip error for %zu",
        i);
  }

  for (size_t i = 1; i < 4096; ++i) {
    FIO_ASSERT(
        FIO_NAME(FIO_UMAP_NAME, get)(&m, (uintptr_t)i, (FIO_UMAP_TYPE)i) ==
            (FIO_UMAP_TYPE)i,
        "get error for %zu",
        i);
  }
  for (size_t i = 1; i < 4096; ++i) {
    FIO_UMAP_TYPE old = (FIO_UMAP_TYPE)i;
#ifdef FIO_UMAP_KEY
    FIO_ASSERT(
        (FIO_UMAP_TYPE)i ==
            FIO_NAME(
                FIO_UMAP_NAME,
                set)(&m, (uintptr_t)i, (FIO_UMAP_KEY)i, (FIO_UMAP_TYPE)i, &old),
        "re-insertion failed at %zu",
        i);
#else
    FIO_ASSERT((FIO_UMAP_TYPE)i ==
                   FIO_NAME(FIO_UMAP_NAME,
                            set)(&m, (uintptr_t)i, (FIO_UMAP_TYPE)i, &old),
               "re-insertion failed at %zu",
               i);
#endif
    FIO_ASSERT(
        !memcmp(&old, &i, sizeof(old) > sizeof(i) ? sizeof(i) : sizeof(old)),
        "old value should be set to the replaced value");
    FIO_ASSERT(
        FIO_NAME(FIO_UMAP_NAME, get)(&m, (uintptr_t)i, (FIO_UMAP_TYPE)i) ==
            (FIO_UMAP_TYPE)i,
        "set-get overwrite roundtrip error for %zu",
        i);
  }
  FIO_NAME(FIO_UMAP_NAME, destroy)(&m);
}
#undef FIO_UMAP_TEST

#endif /* FIO_UMAP_TEST */
/* *****************************************************************************
Unordered Map Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_UMAP_PTR

#undef FIO_UMAP_BIG
#undef FIO_UMAP_INDEX_USED_BIT
#undef FIO_UMAP_TYPE
#undef FIO_UMAP_TYPE_INVALID
#undef FIO_UMAP_TYPE_COPY
#undef FIO_UMAP_TYPE_COPY_SIMPLE
#undef FIO_UMAP_TYPE_DESTROY
#undef FIO_UMAP_TYPE_DESTROY_SIMPLE
#undef FIO_UMAP_TYPE_DISCARD
#undef FIO_UMAP_TYPE_CMP
#undef FIO_UMAP_DESTROY_AFTER_COPY
#undef FIO_UMAP_KEY
#undef FIO_UMAP_KEY_INVALID
#undef FIO_UMAP_KEY_COPY
#undef FIO_UMAP_KEY_DESTROY
#undef FIO_UMAP_KEY_DESTROY_SIMPLE
#undef FIO_UMAP_KEY_DISCARD
#undef FIO_UMAP_KEY_CMP
#undef FIO_UMAP_OBJ
#undef FIO_UMAP_OBJ_KEY
#undef FIO_UMAP_OBJ_INVALID
#undef FIO_UMAP_OBJ_COPY
#undef FIO_UMAP_OBJ_DESTROY
#undef FIO_UMAP_OBJ_CMP
#undef FIO_UMAP_OBJ_KEY_CMP
#undef FIO_UMAP_OBJ2KEY
#undef FIO_UMAP_OBJ2TYPE
#undef FIO_UMAP_OBJ_DISCARD
#undef FIO_UMAP_DESTROY_AFTER_COPY
#undef FIO_UMAP_OBJ_DESTROY_AFTER
#undef FIO_UMAP_MAX_SEEK
#undef FIO_UMAP_MAX_FULL_COLLISIONS
#undef FIO_UMAP_CUCKOO_STEPS
#undef FIO_UMAP_EVICT_LRU

#undef FIO_UMAP___NORMALIZE_HASH
#undef FIO_UMAP___IMAP_FREE
#undef FIO_UMAP___IMAP_DELETED

#endif /* FIO_UMAP_NAME */
#undef FIO_UMAP_NAME
