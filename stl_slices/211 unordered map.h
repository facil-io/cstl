/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
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



TODO?
Benchmark: https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html
***************************************************************************** */

/** An Unordered Map Type */
struct FIO_NAME(FIO_MAP_NAME, s) {
  /** Internal map / memory - do not access directly */
  FIO_NAME(FIO_MAP_NAME, node_s) * map;
  /** Object count - do not access directly */
  FIO_MAP_SIZE_TYPE count;
#if FIO_MAP_EVICT_LRU
  /** LRU eviction monitoring - do not access directly */
  FIO_MAP_SIZE_TYPE last_used;
#endif /* FIO_MAP_EVICT_LRU */
  uint8_t bits;
  uint8_t under_attack;
};

/* *****************************************************************************
Unordered Map Implementation - inlined static functions
***************************************************************************** */

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

/** Internal helper - do not access */
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
  return (m->bits ? FIO_MAP_CAPA(m->bits) : 0);
}

/* *****************************************************************************
Unordered Map Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

#ifndef FIO_MAP_MEMORY_SIZE
#define FIO_MAP_MEMORY_SIZE(bits)                                              \
  ((sizeof(FIO_NAME(FIO_MAP_NAME, node_s)) + sizeof(uint8_t)) *                \
   FIO_MAP_CAPA(bits))
#endif

/* *****************************************************************************
Unordered Map Implementation - helper functions.
***************************************************************************** */

#ifndef FIO_MAP___IMAP_DELETED
#define FIO_MAP___IMAP_DELETED 255
#endif
#ifndef FIO_MAP___IMAP_FREE
#define FIO_MAP___IMAP_FREE 0
#endif

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
  if (m->bits <= FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT)
    goto seek_as_array;
  /* we perrform X attempts using large cuckoo steps */
  pos = hash;
  for (int attempts = 0; attempts < max_attempts;
       (++attempts), (pos += FIO_MAP_CUCKOO_STEPS)) {
    /* each attempt test a group of 8 slots spaced by a few bytes (comb) */
    const uint8_t offsets[] = {0, 3, 7, 12, 18, 25, 33, 41};
    const uint64_t comb =
        (uint64_t)imap[(pos + offsets[0]) & pos_mask] |
        ((uint64_t)imap[(pos + offsets[1]) & pos_mask] << (1 * 8)) |
        ((uint64_t)imap[(pos + offsets[2]) & pos_mask] << (2 * 8)) |
        ((uint64_t)imap[(pos + offsets[3]) & pos_mask] << (3 * 8)) |
        ((uint64_t)imap[(pos + offsets[4]) & pos_mask] << (4 * 8)) |
        ((uint64_t)imap[(pos + offsets[5]) & pos_mask] << (5 * 8)) |
        ((uint64_t)imap[(pos + offsets[6]) & pos_mask] << (6 * 8)) |
        ((uint64_t)imap[(pos + offsets[7]) & pos_mask] << (7 * 8));
    uint64_t simd_result = simd_base ^ comb;
    simd_result = fio_has_zero_byte64(simd_result);

    /* test for exact match in each of the bytes in the 8 byte group */
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if (simd_result) {
      for (int i = 0; simd_result; ++i) {
        /* test cache friendly 8bit match */
        if ((simd_result & UINT64_C(0x80))) {
          /* test full hash */
          register FIO_MAP_HASH obj_hash =
              FIO_MAP_HASH_GET_HASH(m, ((pos + offsets[i]) & pos_mask));
          if (obj_hash == hash) {
            /* test full collisions (attack) / match */
            if (m->under_attack ||
                FIO_MAP_OBJ_KEY_CMP(m->map[(pos + offsets[i]) & pos_mask].obj,
                                    key)) {
              pos = (pos + offsets[i]) & pos_mask;
              return pos;
            } else if (++total_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
              m->under_attack = 1;
              FIO_LOG_SECURITY("Unordered map under attack?");
            }
          }
        }
        simd_result >>= 8;
      }
    }
    /* test if there's an available slot in the group */
    if (free_slot == (FIO_MAP_SIZE_TYPE)-1LL &&
        (simd_result =
             (fio_has_zero_byte64(comb) | fio_has_full_byte64(comb)))) {
      for (int i = 0; simd_result; ++i) {
        if (simd_result & UINT64_C(0x80)) {
          free_slot = (pos + offsets[i]) & pos_mask;
          break;
        }
        simd_result >>= 8;
      }
    }
    /* test if there's a free slot in the group (never used => stop seeking) */
    /* note: the MSB is 1 for both (x-1) and (~x) only if x == 0. */
    if (fio_has_zero_byte64(comb))
      break;
  }

  pos = free_slot;
  return pos;

seek_as_array:

  if (m->count < FIO_MAP_CAPA(m->bits))
    free_slot = m->count;
  pos = 0;
  while (pos < m->count) {
    switch (imap[pos]) {
    case 0: return pos;
    case 255:
      if (free_slot > pos)
        free_slot = pos;
      break;
    default:
      if (imap[pos] == (uint8_t)(simd_base & 0xFF)) {
        FIO_MAP_HASH obj_hash = FIO_MAP_HASH_GET_HASH(m, pos);
        if (obj_hash == hash) {
          /* test full collisions (attack) / match */
          if (m->under_attack || FIO_MAP_OBJ_KEY_CMP(m->map[pos].obj, key)) {
            return pos;
          } else if (++total_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
            m->under_attack = 1;
            FIO_LOG_SECURITY("Unordered map under attack?");
          }
        }
      }
    }
    ++pos;
  }
  pos = free_slot;
  return pos;

  (void)key; /* if unused */
}

FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, __realloc)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                                size_t bits) {
  if (!m || bits >= (sizeof(FIO_MAP_SIZE_TYPE) * 8))
    return -1;
  FIO_NAME(FIO_MAP_NAME, node_s) *tmp = (FIO_NAME(FIO_MAP_NAME, node_s) *)
      FIO_MEM_REALLOC_(NULL, 0, FIO_MAP_MEMORY_SIZE(bits), 0);
  if (!tmp)
    return -1;
  if (!FIO_MEM_REALLOC_IS_SAFE_)
    FIO_MEMSET(tmp, 0, FIO_MAP_MEMORY_SIZE(bits));
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
      FIO_MAP_HASH obj_hash = FIO_MAP_HASH_GET_HASH(m, i);
      FIO_MAP_SIZE_TYPE pos =
          FIO_NAME(FIO_MAP_NAME,
                   __index)(&m2, obj_hash, FIO_MAP_OBJ2KEY(m->map[i].obj));
      if (pos == (FIO_MAP_SIZE_TYPE)-1)
        goto error;
      FIO_NAME(FIO_MAP_NAME, __imap)
      (&m2)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(obj_hash, m2.bits);
#if FIO_MAP_HASH_CACHED
      m2.map[pos].hash = obj_hash;
#endif /* FIO_MAP_HASH_CACHED */
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
    if (FIO_MAP_CAPA(m->bits) > 8) {
      uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
      for (FIO_MAP_SIZE_TYPE i = 0;
           m2.count < m->count && i < FIO_MAP_CAPA(m->bits);
           i += 8) {
        /* skip empty groups (test for all bytes == 0) (can we test == 255?) */
        uint64_t result = (fio_has_zero_byte64(imap64[(i >> 3)]) |
                           fio_has_full_byte64(imap64[(i >> 3)]));
        if (result == UINT64_C(0x8080808080808080))
          continue;
        result ^= UINT64_C(0x8080808080808080);
        for (int j = 0; j < 8 && result; ++j) {
          const FIO_MAP_SIZE_TYPE n = i + j;
          if ((result & UINT64_C(0x80))) {
            /* place in new hash */
            FIO_MAP_HASH obj_hash = FIO_MAP_HASH_GET_HASH(m, n);
            FIO_MAP_SIZE_TYPE pos = FIO_NAME(
                FIO_MAP_NAME,
                __index)(&m2, obj_hash, FIO_MAP_OBJ2KEY(m->map[n].obj));
            if (pos == (FIO_MAP_SIZE_TYPE)-1)
              goto error;
            FIO_NAME(FIO_MAP_NAME, __imap)
            (&m2)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(obj_hash, m2.bits);
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
          result >>= 8;
        }
      }
    } else {
      for (FIO_MAP_SIZE_TYPE i = 0; m->count && i < FIO_MAP_CAPA(m->bits);
           ++i) {
        if (FIO_NAME(FIO_MAP_NAME, __imap)(m)[i] &&
            FIO_NAME(FIO_MAP_NAME, __imap)(m)[i] != 255) {
          FIO_MAP_HASH obj_hash = FIO_MAP_HASH_GET_HASH(m, i);
          FIO_MAP_SIZE_TYPE pos =
              FIO_NAME(FIO_MAP_NAME,
                       __index)(&m2, obj_hash, FIO_MAP_OBJ2KEY(m->map[i].obj));
          if (pos == (FIO_MAP_SIZE_TYPE)-1)
            goto error;
          FIO_NAME(FIO_MAP_NAME, __imap)
          (&m2)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(obj_hash, m2.bits);
          m2.map[pos] = m->map[i];
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
*****************************************************************************
*/

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
*****************************************************************************
*/

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
      !FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos])
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
  if (!FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] ||
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] == 255) {
    /* new */
    FIO_NAME(FIO_MAP_NAME, __imap)
    (m)[pos] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(hash, m->bits);
#if FIO_MAP_HASH_CACHED
    m->map[pos].hash = hash;
#endif
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
  } else if (overwrite &&
             FIO_MAP_SHOULD_OVERWRITE(FIO_MAP_OBJ2TYPE(m->map[pos].obj), obj)) {
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
  FIO_PTR_TAG_VALID_OR_RETURN(map, -1);
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !m->count)
    return -1;
  hash = FIO_MAP_HASH_FIX(hash);
  FIO_MAP_SIZE_TYPE pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key);
  if (pos == (FIO_MAP_SIZE_TYPE)(-1) ||
      FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] == 255 ||
      !FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos])
    return -1;
  FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos] = 255;
#if FIO_MAP_HASH_CACHED
  m->map[pos].hash = 0;
#endif
  --m->count;
  if (old) {
    FIO_MAP_TYPE_COPY(*old, FIO_MAP_OBJ2TYPE(m->map[pos].obj));
    FIO_MAP_OBJ_DESTROY_AFTER(m->map[pos].obj);
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
  if (m->bits > 3) {
    uint64_t *imap64 = (uint64_t *)FIO_NAME(FIO_MAP_NAME, __imap)(m);
    for (FIO_MAP_SIZE_TYPE i = 0; m->count && i < FIO_MAP_CAPA(m->bits);
         i += 8) {
      /* skip empty groups (test for all bytes == 0 || 255 */
      register uint64_t row = imap64[i >> 3];
      row = (fio_has_full_byte64(row) | fio_has_zero_byte64(row));
      if (row == UINT64_C(0x8080808080808080)) {
        imap64[i >> 3] = 0;
        continue;
      }
      imap64[i >> 3] = 0;
      row ^= UINT64_C(0x8080808080808080);
      for (int j = 0; j < 8; ++j) {
        if ((row & UINT64_C(0x80))) {
          FIO_MAP_OBJ_DESTROY(m->map[i + j].obj);
#if FIO_MAP_HASH_CACHED
          m->map[i + j].hash = 0;
#endif
          --m->count; /* stop seeking if no more elements */
        }
        row >>= 8;
      }
    }
  } else {
    for (FIO_MAP_SIZE_TYPE i = 0; m->count && i < FIO_MAP_CAPA(m->bits); ++i) {
      if (FIO_NAME(FIO_MAP_NAME, __imap)(m)[i] &&
          FIO_NAME(FIO_MAP_NAME, __imap)(m)[i] != 255) {
        FIO_MAP_OBJ_DESTROY(m->map[i].obj);
        --m->count;
      }
    }
  }
  FIO_ASSERT_DEBUG(!m->count,
                   "logic error @ unordered map clear (count == %zd != 0.",
                   (ssize_t)m->count);
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
  for (FIO_MAP_SIZE_TYPE i = 0;
       number_of_elements && (i + 7) < FIO_MAP_CAPA(m->bits);
       i += 8) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    register uint64_t row = imap64[i >> 3];
    row = (fio_has_full_byte64(row) | fio_has_zero_byte64(row));
    if (row == UINT64_C(0x8080808080808080))
      continue;
    row ^= UINT64_C(0x8080808080808080);
    for (int j = 0; number_of_elements && j < 8; ++j) {
      if ((row & UINT64_C(0x80))) {
        FIO_MAP_OBJ_DESTROY(m->map[i + j].obj);
#if FIO_MAP_HASH_CACHED
        m->map[i + j].hash = 0;
#endif
        FIO_NAME(FIO_MAP_NAME, __imap)(m)[i + j] = 255;
        --m->count;
        --number_of_elements; /* stop evicting? */
      }
      row >>= 8;
    }
  }

#endif /* FIO_MAP_EVICT_LRU */
  return -1;
}

/* *****************************************************************************
Object state information
*****************************************************************************
*/

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
SFUNC FIO_MAP_SIZE_TYPE
FIO_NAME(FIO_MAP_NAME, each)(FIO_MAP_PTR map,
                             int (*task)(FIO_NAME(FIO_MAP_NAME, each_s) *),
                             void *udata,
                             ssize_t start_at) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return 0;
  FIO_PTR_TAG_VALID_OR_RETURN(map, (FIO_MAP_SIZE_TYPE)-1);
  if (start_at < 0) {
    start_at = m->count - start_at;
    if (start_at < 0)
      start_at = 0;
  }
  if ((FIO_MAP_SIZE_TYPE)start_at >= m->count)
    return m->count;

  FIO_NAME(FIO_MAP_NAME, each_s)
  e = {
      .parent = map,
      .index = (uint64_t)start_at,
      .task = task,
      .udata = udata,
  };

#if FIO_MAP_EVICT_LRU
  if (start_at) {
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, pos) {
      if (start_at) {
        --start_at;
        continue;
      }
      e.value = FIO_MAP_OBJ2TYPE(m->map[pos].obj);
#ifdef FIO_MAP_KEY
      e.key = FIO_MAP_OBJ2KEY(m->map[pos].obj);
#endif
      int r = e.task(&e);
      ++e.index;
      if (r == -1)
        goto finish;
    }
  } else {
    FIO_INDEXED_LIST_EACH(m->map, node, m->last_used, pos) {
      e.value = FIO_MAP_OBJ2TYPE(m->map[pos].obj);
#ifdef FIO_MAP_KEY
      e.key = FIO_MAP_OBJ2KEY(m->map[pos].obj);
#endif
      int r = e.task(&e);
      ++e.index;
      if (r == -1)
        goto finish;
    }
  }

#else /* FIO_MAP_EVICT_LRU */

  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  FIO_MAP_SIZE_TYPE pos = 0;
  if (start_at) {
    uint64_t *imap64 = (uint64_t *)imap;
    /* scan map to arrive at starting point. */
    for (FIO_MAP_SIZE_TYPE i = 0; start_at && i < FIO_MAP_CAPA(m->bits);
         i += 8) {
      /* skip empty groups (test for all bytes == 0 || 255 */
      register uint64_t row = imap64[i >> 3];
      row = (fio_has_full_byte64(row) | fio_has_zero_byte64(row));
      if (row == UINT64_C(0x8080808080808080))
        continue;
      row ^= UINT64_C(0x8080808080808080);
      for (int j = 0; start_at && j < 8; ++j) {
        if ((row & UINT64_C(0x80))) {
          pos = i + j;
          --start_at;
        }
        row >>= 8;
      }
    }
  }
  while (pos + 8 < FIO_MAP_CAPA(m->bits)) {
    /* test only groups with valid values (test for all bytes == 0 || 255 */
    register uint64_t row = fio_buf2u64_local(imap + pos);
    row = (fio_has_full_byte64(row) | fio_has_zero_byte64(row));
    if (row != UINT64_C(0x8080808080808080)) {
      row ^= UINT64_C(0x8080808080808080);
      for (int j = 0; j < 8; ++j) {
        if ((row & UINT64_C(0xFF))) {
          e.value = FIO_MAP_OBJ2TYPE(m->map[pos + j].obj);
#ifdef FIO_MAP_KEY
          e.key = FIO_MAP_OBJ2KEY(m->map[pos + j].obj);
#endif
          int r = e.task(&e);
          ++e.index;
          if (r == -1)
            goto finish;
        }
        row >>= 8;
      }
    }
    pos += 8;
  }
  /* scan leftover (not 8 byte aligned) byte-map */
  while (pos < FIO_MAP_CAPA(m->bits)) {
    if (imap[pos] && imap[pos] != 255) {
      e.value = FIO_MAP_OBJ2TYPE(m->map[pos].obj);
#ifdef FIO_MAP_KEY
      e.key = FIO_MAP_OBJ2KEY(m->map[pos].obj);
#endif
      int r = e.task(&e);
      ++e.index;
      if (r == -1)
        goto finish;
    }
    ++pos;
  }
#endif /* FIO_MAP_EVICT_LRU */
finish:
  return (FIO_MAP_SIZE_TYPE)e.index;
}

SFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
    FIO_NAME(FIO_MAP_NAME, each_next)(FIO_MAP_PTR map,
                                      FIO_NAME(FIO_MAP_NAME, node_s) * *first,
                                      FIO_NAME(FIO_MAP_NAME, node_s) * pos) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m || !first)
    return NULL;
  FIO_PTR_TAG_VALID_OR_RETURN(map, NULL);
  if (!m->count || !m->map)
    return NULL;
  size_t i;
#if FIO_MAP_EVICT_LRU
  FIO_MAP_SIZE_TYPE next;
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

#else  /* FIO_MAP_EVICT_LRU */
  if (!pos || !(*first)) {
    i = -1;
  } else {
    i = pos - *first;
  }
  ++i;
  *first = m->map;
  uint8_t *imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  while (i + 8 < FIO_MAP_CAPA(m->bits)) {
    /* test only groups with valid values (test for all bytes == 0 || 255 */
    register uint64_t row = fio_buf2u64_local(imap + i);
    row = (fio_has_full_byte64(row) | fio_has_zero_byte64(row));
    if (row != UINT64_C(0x8080808080808080)) {
      row ^= UINT64_C(0x8080808080808080);
      for (int j = 0; j < 8; ++j) {
        if ((row & UINT64_C(0x80))) {
          return m->map + i + j;
        }
        row >>= 8;
      }
    }
    i += 8;
  }
  while (i < FIO_MAP_CAPA(m->bits)) {
    if (imap[i] && imap[i] != 255)
      return m->map + i;
    ++i;
  }
  return NULL;
#endif /* FIO_MAP_EVICT_LRU */
}

/* *****************************************************************************
Unordered Map Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MAP_NAME */
