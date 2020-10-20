/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_UMAP_NAME map           /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "210 map api.h"            /* Development inclusion - ignore line */
#define FIO_MAP_TEST                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                  Unordered Map - an Unordered Hash Map / Set










***************************************************************************** */
#if defined(FIO_MAP_NAME) && !FIO_MAP_ORDERED

/* *****************************************************************************



Unordered Map Types - Implementation



***************************************************************************** */

/** An Unordered Map Type */
struct FIO_NAME(FIO_MAP_NAME, s) {
  /** Internal map / memory - do not access directly */
  FIO_NAME(FIO_MAP_NAME, each_s) * map;
  /** Object count - do not access directly */
  FIO_MAP_SIZE_TYPE count;
#if FIO_MAP_EVICT_LRU
  /** LRU evicion monitoring - do not access directly */
  FIO_MAP_SIZE_TYPE last_used;
#endif /* FIO_MAP_EVICT_LRU */
  uint8_t bits;
  uint8_t under_attack;
};

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

#ifndef FIO_MAP_CAPA
#define FIO_MAP_CAPA(bits) ((uintptr_t)1ULL << (bits))
#endif

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC FIO_MAP_PTR FIO_NAME(FIO_MAP_NAME, new)(void) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*m), 0);
  if (!m)
    return (FIO_MAP_PTR)NULL;
  *m = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
  return (FIO_MAP_PTR)FIO_PTR_TAG(m);
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_PTR map) {
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  FIO_NAME(FIO_MAP_NAME, destroy)(map);
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  FIO_MEM_FREE_(m, sizeof(*m));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** IInternal helper - do not access */
FIO_IFUNC uint8_t *FIO_NAME(FIO_MAP_NAME,
                            __imap)(FIO_NAME(FIO_MAP_NAME, s) * m) {
  return (uint8_t *)(m->map + FIO_MAP_CAPA(m->bits));
}

FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  return m->count;
}

FIO_IFUNC size_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  return FIO_MAP_CAPA(m->bits);
}

FIO_IFUNC FIO_NAME(FIO_MAP_NAME, each_s) *
    FIO_NAME(FIO_MAP_NAME, each_next)(FIO_MAP_PTR map,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * *first,
                                      FIO_NAME(FIO_MAP_NAME, each_s) * pos) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !first)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  if (!m->count || !m->map)
    return NULL;
  size_t i;
#if FIO_MAP_EVICT_LRU
  intptr_t next;
  if (!pos) {
    i = m->last_used;
    *first = m->map;
    return m->map + i;
  }
  i = pos - *first;
  *first = m->map; /* was it updated? */
  next = m->map[i].node.next;
  if (next == m->last_used)
    return NULL;
  return m->map + next;

#else  /*FIO_MAP_EVICT_LRU*/
  if (!pos || !(*first)) {
    i = -1;
  } else {
    i = pos - *first;
  }
  ++i;
  *first = m->map;
  while (i + 8 < FIO_MAP_CAPA(m->bits)) {
    uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
    /* test only groups with valid values (test for all bytes == 0 || 255 */
    register const uint64_t row = fio_buf2u64_local(imap + i);
    if ((fio_has_full_byte64(row) | fio_has_zero_byte64(row)) !=
        UINT64_C(0x8080808080808080)) {
      for (int j = 0; j < 8; ++j) {
        if (m->map[i + j].hash)
          return m->map + i + j;
      }
    }
    i += 8;
  }
  while (i < FIO_MAP_CAPA(m->bits)) {
    if (m->map[i].hash)
      return m->map + i;
    ++i;
  }
  return NULL;
#endif /* FIO_MAP_EVICT_LRU */
}
/* *****************************************************************************
Unordered Map Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

#ifndef FIO_MAP_MEMORY_SIZE
#define FIO_MAP_MEMORY_SIZE(bits)                                              \
  ((sizeof(FIO_NAME(FIO_MAP_NAME, each_s)) + sizeof(uint8_t)) *                \
   FIO_MAP_CAPA(bits))
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

#ifndef FIO_MAP___IMAP_DELETED
#define FIO_MAP___IMAP_DELETED 255
#endif
#ifndef FIO_MAP___IMAP_FREE
#define FIO_MAP___IMAP_FREE 0
#endif

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIXED ((FIO_MAP_HASH)-2LL)

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIX(h) (!h ? FIO_MAP_HASH_FIXED : (h))

FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     __hash2imap)(FIO_MAP_HASH hash,
                                                  uint8_t bits) {
  FIO_MAP_SIZE_TYPE r = (((hash >> bits) ^ hash) & 255);
  if (!r || r == 255)
    r ^= 1;
  return r;
}

FIO_SFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     __index)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                              const FIO_MAP_HASH hash,
                                              FIO_MAP_OBJ_KEY key) {
  FIO_MAP_SIZE_TYPE pos = (FIO_MAP_SIZE_TYPE)-1LL;
  FIO_MAP_SIZE_TYPE free_slot = (FIO_MAP_SIZE_TYPE)-1LL;
  size_t total_collisions = 0;
  if (!m->map)
    return pos;
  const uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  /* note: hash MUST be normalized by this point */
  const uint64_t simd_base =
      FIO_NAME(FIO_MAP_NAME, __hash2imap)(hash, m->bits) *
      UINT64_C(0x0101010101010101);
  const FIO_MAP_SIZE_TYPE pos_mask = FIO_MAP_CAPA(m->bits) - 1;
  const int max_attempts = (FIO_MAP_CAPA(m->bits) >> 3) >= FIO_MAP_MAX_SEEK
                               ? (int)FIO_MAP_MAX_SEEK
                               : (FIO_MAP_CAPA(m->bits) >> 3);
  /* we perrform X attempts using large cuckoo steps */
  pos = hash;
  for (int attempts = 0; attempts < max_attempts;
       (++attempts), (pos += FIO_MAP_CUCKOO_STEPS)) {
    /* each attempt test a group of 8 slots spaced by 7 bytes (comb) */
    const uint64_t comb = (uint64_t)imap[pos & pos_mask] |
                          ((uint64_t)imap[(pos + 7) & pos_mask] << (1 * 8)) |
                          ((uint64_t)imap[(pos + 14) & pos_mask] << (2 * 8)) |
                          ((uint64_t)imap[(pos + 21) & pos_mask] << (3 * 8)) |
                          ((uint64_t)imap[(pos + 28) & pos_mask] << (4 * 8)) |
                          ((uint64_t)imap[(pos + 35) & pos_mask] << (5 * 8)) |
                          ((uint64_t)imap[(pos + 42) & pos_mask] << (6 * 8)) |
                          ((uint64_t)imap[(pos + 49) & pos_mask] << (7 * 8));
    uint64_t simd_result = simd_base ^ comb;
    simd_result = fio_has_zero_byte64(simd_result);

    /* test for exact match in each of the bytes in the 8 byte group */
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if (simd_result) {
      for (int byte = 0, offset = 0; byte < 8; (++byte), (offset += 7)) {
        /* test cache friendly 8bit match */
        if ((simd_result & (UINT64_C(0xFF) << (byte << 3)))) {
          /* test full hash */
          if (m->map[(pos + offset) & pos_mask].hash == hash) {
            /* test full collisions (attack) / match */
            if (m->under_attack ||
                FIO_MAP_OBJ_KEY_CMP(m->map[(pos + offset) & pos_mask].obj,
                                    key)) {
              pos = (pos + offset) & pos_mask;
              return pos;
            } else if (++total_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
              m->under_attack = 1;
              FIO_LOG_SECURITY("Unordered map under attack?");
            }
          }
        }
      }
    }
    /* test if there's an available slot in the group */
    if (free_slot == (FIO_MAP_SIZE_TYPE)-1LL &&
        (fio_has_zero_byte64(comb) || fio_has_full_byte64(comb))) {
      for (int byte = 0, offset = 0; byte < 8; (++byte), (offset += 7)) {
        if (imap[(pos + offset) & pos_mask] == 255 ||
            !imap[(pos + offset) & pos_mask]) {
          free_slot = (pos + offset) & pos_mask;
          break;
        }
      }
    }
    /* test if there's a free slot in the group (never used => stop seeking) */
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if (fio_has_zero_byte64(comb))
      break;
  }

  pos = free_slot;
  return pos;
  (void)key; /* if unused */
}

FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, __realloc)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                                size_t bits) {
  if (!m || bits >= (sizeof(FIO_MAP_SIZE_TYPE) * 8))
    return -1;
  if (bits < 3)
    bits = 3;
  FIO_NAME(FIO_MAP_NAME, each_s) *tmp = (FIO_NAME(FIO_MAP_NAME, each_s) *)
      FIO_MEM_REALLOC_(NULL, 0, FIO_MAP_MEMORY_SIZE(bits), 0);
  if (!tmp)
    return -1;
  if (!FIO_MEM_REALLOC_IS_SAFE_)
    memset(tmp, 0, FIO_MAP_MEMORY_SIZE(bits));
  /* rehash the map */
  FIO_NAME(FIO_MAP_NAME, s) m2;
  m2 = (FIO_NAME(FIO_MAP_NAME, s)){
      .map = tmp,
      .bits = (uint8_t)bits,
  };
  if (m->count) {
#if FIO_MAP_EVICT_LRU
    /* use eviction list to re-insert data. */
    FIO_MAP_SIZE_TYPE last = 0;
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, i) {
      /* place old values in new hash */
      FIO_MAP_SIZE_TYPE pos = FIO_NAME(
          FIO_MAP_NAME,
          __index)(&m2, m->map[i].hash, FIO_MAP_OBJ2KEY(m->map[i].obj));
      if (pos == (FIO_MAP_SIZE_TYPE)-1)
        goto error;
      FIO_NAME(FIO_MAP_NAME, __imap)
      (&m2)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(m->map[i].hash, m2.bits);
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
#else /* FIO_MAP_EVICT_LRU */
    /* scan map for used slots to re-insert data */
    uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
    for (FIO_MAP_SIZE_TYPE i = 0;
         m2.count < m->count && i < FIO_MAP_CAPA(m->bits);
         i += 8) {
      /* skip empty groups (test for all bytes == 0) (can we test == 255?) */
      if ((fio_has_zero_byte64(imap64[(i >> 3)]) |
           fio_has_full_byte64(imap64[(i >> 3)])) ==
          UINT64_C(0x8080808080808080))
        continue;
      for (int j = 0; j < 8; ++j) {
        const FIO_MAP_SIZE_TYPE n = i + j;
        if (m->map[n].hash) {
          /* place in new hash */
          FIO_MAP_SIZE_TYPE pos = FIO_NAME(
              FIO_MAP_NAME,
              __index)(&m2, m->map[n].hash, FIO_MAP_OBJ2KEY(m->map[n].obj));
          if (pos == (FIO_MAP_SIZE_TYPE)-1)
            goto error;
          FIO_NAME(FIO_MAP_NAME, __imap)
          (&m2)[pos] =
              FIO_NAME(FIO_MAP_NAME, __hash2imap)(m->map[n].hash, m2.bits);
          m2.map[pos] = m->map[n];
#if FIO_MAP_EVICT_LRU
          if (!m2.count) {
            m2.last_used = pos;
            m2.map[pos].node.prev = m2.map[pos].node.next = pos;
          }
          FIO_INDEXED_LIST_PUSH(m2.map, node, m2.last_used, pos);
          if (m->last_used == n)
            m2.last_used = pos;
#endif /* FIO_MAP_EVICT_LRU */
          ++m2.count;
        }
      }
    }
#endif /* FIO_MAP_EVICT_LRU */
  }

  FIO_MEM_FREE_(m->map, FIO_MAP_MEMORY_SIZE(m->bits));
  *m = m2;
  return 0;
error:
  FIO_MEM_FREE_(tmp, FIO_MAP_MEMORY_SIZE(bits));
  return -1;
}

/* *****************************************************************************
Unordered Map Implementation - API implementation
***************************************************************************** */

/* Frees any internal data AND the object's container! */
SFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, clear)(map);
  FIO_MEM_FREE_(m->map, FIO_MAP_MEMORY_SIZE(m->bits));
  *m = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
  return;
}

/* *****************************************************************************
Get / Set / Remove
***************************************************************************** */

SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR map,
                                                    FIO_MAP_HASH hash,
                                                    FIO_MAP_OBJ_KEY key) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_MAP_HASH_FIX(hash);
  FIO_MAP_SIZE_TYPE pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key);
  if (pos == (FIO_MAP_SIZE_TYPE)(-1) ||
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] == 255 ||
      !FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] || !m->map[pos].hash)
    return NULL;
#if FIO_MAP_EVICT_LRU
  if (m->last_used != pos) {
    FIO_INDEXED_LIST_REMOVE(m->map, node, pos);
    FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
    m->last_used = pos;
  }
#endif /* FIO_MAP_EVICT_LRU */
  return &FIO_MAP_OBJ2TYPE(m->map[pos].obj);
}

SFUNC FIO_MAP_TYPE *FIO_NAME(FIO_MAP_NAME, set_ptr)(FIO_MAP_PTR map,
                                                    FIO_MAP_HASH hash,
#ifdef FIO_MAP_KEY
                                                    FIO_MAP_KEY key,
#endif /* FIO_MAP_KEY */
                                                    FIO_MAP_TYPE obj,
                                                    FIO_MAP_TYPE *old,
                                                    uint8_t overwrite) {
  if (old)
    *old = FIO_MAP_TYPE_INVALID;
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_MAP_HASH_FIX(hash);
#ifdef FIO_MAP_KEY
  FIO_MAP_SIZE_TYPE pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key);
#else
  FIO_MAP_SIZE_TYPE pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, obj);
#endif /* FIO_MAP_KEY */

  for (int i = 0; pos == (FIO_MAP_SIZE_TYPE)-1 && i < 2; ++i) {
    if (FIO_NAME(FIO_MAP_NAME, __realloc)(m, m->bits + 1))
      goto error;
#ifdef FIO_MAP_KEY
    pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key);
#else
    pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, obj);
#endif /* FIO_MAP_KEY */
  }
  if (pos == (FIO_MAP_SIZE_TYPE)-1)
    goto error;
  if (!m->map[pos].hash) {
    /* new */
    FIO_NAME(FIO_MAP_NAME, __imap)
    (m)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(hash, m->bits);
    m->map[pos].hash = hash;
    FIO_MAP_TYPE_COPY(FIO_MAP_OBJ2TYPE(m->map[pos].obj), obj);
    FIO_MAP_KEY_COPY(FIO_MAP_OBJ2KEY(m->map[pos].obj), key);
#if FIO_MAP_EVICT_LRU
    if (m->count) {
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
    } else {
      m->map[pos].node.prev = m->map[pos].node.next = pos;
    }
    m->last_used = pos;
#endif /* FIO_MAP_EVICT_LRU */
    ++m->count;
  } else if (overwrite) {
    /* overwrite existing */
    FIO_MAP_KEY_DISCARD(key);
    if (old) {
      FIO_MAP_TYPE_COPY(old[0], FIO_MAP_OBJ2TYPE(m->map[pos].obj));
      if (FIO_MAP_DESTROY_AFTER_COPY) {
        FIO_MAP_TYPE_DESTROY(FIO_MAP_OBJ2TYPE(m->map[pos].obj));
      }
    } else {
      FIO_MAP_TYPE_DESTROY(FIO_MAP_OBJ2TYPE(m->map[pos].obj));
    }
    FIO_MAP_TYPE_COPY(FIO_MAP_OBJ2TYPE(m->map[pos].obj), obj);
#if FIO_MAP_EVICT_LRU
    if (m->last_used != pos) {
      FIO_INDEXED_LIST_REMOVE(m->map, node, pos);
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos);
      m->last_used = pos;
    }
#endif /* FIO_MAP_EVICT_LRU */
  } else {
    FIO_MAP_TYPE_DISCARD(obj);
    FIO_MAP_KEY_DISCARD(key);
  }
  return &FIO_MAP_OBJ2TYPE(m->map[pos].obj);

error:
  FIO_MAP_TYPE_DISCARD(obj);
  FIO_MAP_KEY_DISCARD(key);
  return NULL;
}

SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_PTR map,
                                         FIO_MAP_HASH hash,
                                         FIO_MAP_OBJ_KEY key,
                                         FIO_MAP_TYPE *old) {
  if (old)
    *old = FIO_MAP_TYPE_INVALID;
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !m->count)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  hash = FIO_MAP_HASH_FIX(hash);
  FIO_MAP_SIZE_TYPE pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key);
  if (pos == (FIO_MAP_SIZE_TYPE)(-1) ||
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] == 255 ||
      !FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] || !m->map[pos].hash)
    return -1;
  FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] = 255;
  m->map[pos].hash = 0;
  --m->count;
  if (old) {
    FIO_MAP_TYPE_COPY(*old, FIO_MAP_OBJ2TYPE(m->map[pos].obj));
    FIO_MAP_OBJ_DESTROY_AFTER(m->map[pos].obj)
  } else {
    FIO_MAP_OBJ_DESTROY(m->map[pos].obj);
  }
#if FIO_MAP_EVICT_LRU
  if (pos == m->last_used)
    m->last_used = m->map[pos].node.next;
  FIO_INDEXED_LIST_REMOVE(m->map, node, pos);
#endif
  return 0;
}

SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);

  /* scan map to clear data. */
  uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
  for (FIO_MAP_SIZE_TYPE i = 0; m->count && i < FIO_MAP_CAPA(m->bits); i += 8) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    register const uint64_t row = imap64[i >> 3];
    if ((fio_has_full_byte64(row) | fio_has_zero_byte64(row)) ==
        UINT64_C(0x8080808080808080)) {
      imap64[i >> 3] = 0;
      continue;
    }
    imap64[i >> 3] = 0;
    for (int j = 0; j < 8; ++j) {
      if (m->map[i + j].hash) {
        FIO_MAP_OBJ_DESTROY(m->map[i + j].obj);
        m->map[i + j].hash = 0;
        --m->count; /* stop seeking if no more elements */
      }
    }
  }
  FIO_ASSERT_DEBUG(!m->count, "logic error @ unordered map clear.");
}

SFUNC int FIO_NAME(FIO_MAP_NAME, evict)(FIO_MAP_PTR map,
                                        size_t number_of_elements) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  if (!m->count)
    return -1;
  if (number_of_elements >= m->count) {
    FIO_NAME(FIO_MAP_NAME, clear)(map);
    return -1;
  }
#if FIO_MAP_EVICT_LRU
  /* evict by LRU */
  do {
    FIO_MAP_SIZE_TYPE n = m->map[m->last_used].node.prev;
    FIO_INDEXED_LIST_REMOVE(m->map, node, n);
  } while (--number_of_elements);
#else /* FIO_MAP_EVICT_LRU */
  /* scan map and evict semi randomly. */
  uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
  for (FIO_MAP_SIZE_TYPE i = 0; number_of_elements && i < FIO_MAP_CAPA(m->bits);
       i += 8) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    {
      register const uint64_t row = imap64[i >> 3];
      if ((fio_has_full_byte64(row) | fio_has_zero_byte64(row)) ==
          UINT64_C(0x8080808080808080)) {
        continue;
      }
    }
    for (int j = 0; number_of_elements && j < 8; ++j) {
      if (m->map[i + j].hash) {
        FIO_MAP_OBJ_DESTROY(m->map[i + j].obj);
        m->map[i + j].hash = 0;
        FIO_NAME(FIO_MAP_NAME, __imap)(m)[i + j] = 255;
        --m->count;
        --number_of_elements; /* stop evicting? */
      }
    }
  }

#endif /* FIO_MAP_EVICT_LRU */
  return -1;
}

/* *****************************************************************************
Object state information
***************************************************************************** */

/** Reservse enough space for a theoretical capacity of `capa` objects. */
SFUNC size_t FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_PTR map,
                                             FIO_MAP_SIZE_TYPE capa) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  if (FIO_MAP_CAPA(m->bits) < capa) {
    size_t bits = 3;
    while (FIO_MAP_CAPA(bits) < capa)
      ++bits;
    for (int i = 0; FIO_NAME(FIO_MAP_NAME, __realloc)(m, bits + i) && i < 2;
         ++i) {
    }
    if (m->bits < bits)
      return 0;
  }
  return FIO_MAP_CAPA(m->bits);
}

/** Attempts to minimize memory use. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  if (!m->bits)
    return;
  if (!m->count) {
    FIO_NAME(FIO_MAP_NAME, destroy)(map);
    return;
  }
  size_t bits = m->bits;
  size_t count = 0;
  while (bits && FIO_MAP_CAPA((bits - 1)) > m->count) {
    --bits;
    ++count;
  }
  for (size_t i = 0; i < count; ++i) {
    if (!FIO_NAME(FIO_MAP_NAME, __realloc)(m, bits + i))
      return;
  }
}

/** Rehashes the map. No need to call this, rehashing is automatic. */
SFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return -1;
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  return FIO_NAME(FIO_MAP_NAME, __realloc)(m, m->bits);
}

/* *****************************************************************************
Iteration
***************************************************************************** */
FIO_SFUNC __thread FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME, __each_pos) = 0;
FIO_SFUNC __thread FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, __each_map) = NULL;

SFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                 each)(FIO_MAP_PTR map,
                                       ssize_t start_at,
                                       int (*task)(FIO_MAP_TYPE obj, void *arg),
                                       void *arg) {
  FIO_MAP_SIZE_TYPE count = (FIO_MAP_SIZE_TYPE)start_at;
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, 0);
  if (!m->count)
    return 0;

  if (start_at < 0) {
    start_at = m->count + start_at;
    if (start_at < 0)
      start_at = 0;
  }
  if ((FIO_MAP_SIZE_TYPE)start_at >= m->count)
    return m->count;

  FIO_NAME(FIO_MAP_NAME, s) *old_map = FIO_NAME(FIO_MAP_NAME, __each_map);
  FIO_MAP_SIZE_TYPE old_pos = FIO_NAME(FIO_MAP_NAME, __each_pos);
  FIO_NAME(FIO_MAP_NAME, __each_pos) = 0;
  FIO_NAME(FIO_MAP_NAME, __each_map) = m;

#if FIO_MAP_EVICT_LRU
  if (start_at) {
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, pos) {
      ++count;
      if (start_at) {
        --start_at;
        continue;
      }
      FIO_NAME(FIO_MAP_NAME, __each_pos) = pos;
      if (task(FIO_MAP_OBJ2TYPE(m->map[pos].obj), arg) == -1)
        goto finish;
    }
  } else {
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, pos) {
      ++count;
      FIO_NAME(FIO_MAP_NAME, __each_pos) = pos;
      if (task(FIO_MAP_OBJ2TYPE(m->map[pos].obj), arg) == -1)
        goto finish;
    }
  }

#else  /* FIO_MAP_EVICT_LRU */
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  FIO_MAP_SIZE_TYPE pos = 0;
  if (start_at) {
    uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
    /* scan map to arrive at starting point. */
    for (FIO_MAP_SIZE_TYPE i = 0; start_at && i < FIO_MAP_CAPA(m->bits);
         i += 8) {
      /* skip empty groups (test for all bytes == 0 || 255 */
      {
        register const uint64_t row = imap64[i >> 3];
        if ((fio_has_full_byte64(row) | fio_has_zero_byte64(row)) ==
            UINT64_C(0x8080808080808080)) {
          continue;
        }
      }
      for (int j = 0; start_at && j < 8; ++j) {
        if (m->map[i + j].hash) {
          pos = i + j;
          --start_at;
        }
      }
    }
  }
  while (pos + 8 < FIO_MAP_CAPA(m->bits)) {
    /* test only groups with valid values (test for all bytes == 0 || 255 */
    register const uint64_t row = fio_buf2u64_local(imap + pos);
    if ((fio_has_full_byte64(row) | fio_has_zero_byte64(row)) !=
        UINT64_C(0x8080808080808080)) {
      for (int j = 0; j < 8; ++j) {
        if (m->map[pos + j].hash) {
          FIO_NAME(FIO_MAP_NAME, __each_pos) = pos + j;
          ++count;
          if (task(FIO_MAP_OBJ2TYPE(
                       m->map[FIO_NAME(FIO_MAP_NAME, __each_pos)].obj),
                   arg) == -1)
            goto finish;
        }
      }
    }
    pos += 8;
  }
  while (pos < FIO_MAP_CAPA(m->bits)) {
    if (m->map[pos].hash) {
      FIO_NAME(FIO_MAP_NAME, __each_pos) = pos;
      ++count;
      if (task(FIO_MAP_OBJ2TYPE(m->map[FIO_NAME(FIO_MAP_NAME, __each_pos)].obj),
               arg) == -1)
        goto finish;
    }
    ++pos;
  }
#endif /* FIO_MAP_EVICT_LRU */

finish:
  FIO_NAME(FIO_MAP_NAME, __each_pos) = old_pos;
  FIO_NAME(FIO_MAP_NAME, __each_map) = old_map;
  return count;
}

#ifdef FIO_MAP_KEY
SFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_MAP_NAME, __each_map) ||
      !FIO_NAME(FIO_MAP_NAME, __each_map)->count)
    return FIO_MAP_KEY_INVALID;
  return FIO_NAME(FIO_MAP_NAME, __each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, __each_pos)]
      .obj.key;
}
#else

SFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_MAP_NAME, __each_map) ||
      !FIO_NAME(FIO_MAP_NAME, __each_map)->count)
    return 0;
  return FIO_NAME(FIO_MAP_NAME, __each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, __each_pos)]
      .hash;
}
#endif
/* *****************************************************************************
Unordered Map Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MAP_NAME */
