/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_MAP_NAME map            /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "210 map api.h"            /* Development inclusion - ignore line */
#define FIO_MAP_TEST                /* Development inclusion - ignore line */
#define FIO_MAP_V2                  /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                  Unordered Map - an Unordered Hash Map / Set




***************************************************************************** */
#if defined(FIO_MAP_NAME) && FIO_MAP_ORDERED

/* *****************************************************************************





Ordered Map Types - Implementation





***************************************************************************** */

/** An Ordered Map Type */
struct FIO_NAME(FIO_MAP_NAME, s) {
  /** Internal map / memory - do not access directly */
  FIO_NAME(FIO_MAP_NAME, node_s) * map;
  /** Object count - do not access directly */
  FIO_MAP_SIZE_TYPE count;
  /** Writing position - do not access directly */
  FIO_MAP_SIZE_TYPE w;
#if FIO_MAP_EVICT_LRU
  /** LRU evicion monitoring - do not access directly */
  FIO_MAP_SIZE_TYPE last_used;
#endif /* FIO_MAP_EVICT_LRU */
  uint8_t bits;
  uint8_t under_attack;
};

/* *****************************************************************************
Ordered Map Implementation - inlined static functions
***************************************************************************** */

#ifndef FIO_MAP_CAPA
#define FIO_MAP_CAPA(bits) (((uintptr_t)1ULL << (bits)) - 1)
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

FIO_IFUNC FIO_NAME(FIO_MAP_NAME, node_s) *
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
  intptr_t i;
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
  if (!pos) {
    i = -1;
  } else {
    i = (intptr_t)(pos - *first);
  }
  ++i;
  *first = m->map;
  while ((uintptr_t)i < (uintptr_t)m->w) {
    if (m->map[i].hash)
      return m->map + i;
    ++i;
  }
  return NULL;
#endif /* FIO_MAP_EVICT_LRU */
}

/* *****************************************************************************
Ordered Map Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

#ifndef FIO_MAP_MEMORY_SIZE
#define FIO_MAP_MEMORY_SIZE(bits)                                              \
  ((sizeof(FIO_NAME(FIO_MAP_NAME, node_s)) + sizeof(FIO_MAP_SIZE_TYPE)) *      \
   FIO_MAP_CAPA(bits))
#endif

/* *****************************************************************************
Ordered Map Implementation - helper functions.
***************************************************************************** */

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIXED ((FIO_MAP_HASH)-2LL)

/** the value to be used when the hash is a reserved value. */
#define FIO_MAP_HASH_FIX(h) (!h ? FIO_MAP_HASH_FIXED : (h))

FIO_IFUNC FIO_MAP_SIZE_TYPE *FIO_NAME(FIO_MAP_NAME,
                                      __imap)(FIO_NAME(FIO_MAP_NAME, s) * m) {
  return (FIO_MAP_SIZE_TYPE *)(m->map + FIO_MAP_CAPA(m->bits));
}

FIO_IFUNC FIO_MAP_SIZE_TYPE FIO_NAME(FIO_MAP_NAME,
                                     __hash2imap)(FIO_MAP_HASH hash,
                                                  uint8_t bits) {
  FIO_MAP_SIZE_TYPE r = hash & ((~(FIO_MAP_SIZE_TYPE)0) << bits);
  return r ? r
           : (((~(FIO_MAP_SIZE_TYPE)0) << bits) << 1); /* must never be zero */
}

typedef struct {
  /* index in the index map */
  FIO_MAP_SIZE_TYPE i;
  /* index in the data array */
  FIO_MAP_SIZE_TYPE a;
} FIO_NAME(FIO_MAP_NAME, __pos_s);

/* locat an objects index in the index map and its array position */
FIO_SFUNC FIO_NAME(FIO_MAP_NAME, __pos_s)
    FIO_NAME(FIO_MAP_NAME, __index)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                    const FIO_MAP_HASH hash,
                                    FIO_MAP_OBJ_KEY key,
                                    FIO_MAP_SIZE_TYPE set_hash) {
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  i = {
      .i = (FIO_MAP_SIZE_TYPE)-1LL,
      .a = (FIO_MAP_SIZE_TYPE)-1LL,
  };
  size_t total_collisions = 0;
  if (!m->map)
    return i;
  FIO_MAP_SIZE_TYPE *const imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
  /* note: hash MUST be normalized by this point */
  const FIO_MAP_SIZE_TYPE pos_mask = FIO_MAP_CAPA(m->bits);
  const FIO_MAP_SIZE_TYPE hashed_mask =
      ((size_t)(m->bits + 1) < (size_t)(sizeof(FIO_MAP_SIZE_TYPE) * 8))
          ? ((~(FIO_MAP_SIZE_TYPE)0) << m->bits)
          : 0;
  const int max_attempts = (FIO_MAP_CAPA(m->bits)) >= FIO_MAP_MAX_SEEK
                               ? (int)FIO_MAP_MAX_SEEK
                               : (FIO_MAP_CAPA(m->bits));
  /* we perform X attempts using large cuckoo steps */
  FIO_MAP_SIZE_TYPE pos = hash;
  if (m->bits <= FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT)
    goto seek_as_array;
  for (int attempts = 0; attempts < max_attempts;
       (++attempts), (pos += FIO_MAP_CUCKOO_STEPS)) {
    const FIO_MAP_SIZE_TYPE desired_hash =
        FIO_NAME(FIO_MAP_NAME, __hash2imap)(pos, m->bits);
    /* each attempt tests a group of 5 slots with high cache locality */
    for (int byte = 0, offset = 0; byte < 5; (++byte), (offset += byte)) {
      const FIO_MAP_SIZE_TYPE index = (pos + offset) & pos_mask;
      /* the last slot is reserved for marking deleted items, not allocated. */
      if (index == pos_mask) {
        continue;
      }
      /* return if there's an available slot (no need to look further) */
      if (!imap[index]) {
        i.i = index;
        if (set_hash)
          imap[index] = desired_hash;
        return i;
      }
      /* test cache friendly partial match */
      if ((imap[index] & hashed_mask) == desired_hash || !hashed_mask) {
        /* test full hash */
        FIO_MAP_SIZE_TYPE a_index = imap[index] & pos_mask;
        if (a_index != pos_mask) {
          if (m->map[a_index].hash == hash) {
            /* test full collisions (attack) / match */
            if (m->under_attack ||
                FIO_MAP_OBJ_KEY_CMP(m->map[a_index].obj, key)) {
              i.i = index;
              i.a = a_index;
              return i;
            } else if (++total_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
              m->under_attack = 1;
              FIO_LOG_SECURITY("Ordered map under attack?");
            }
          }
        }
      } else if (i.i == (FIO_MAP_SIZE_TYPE)-1LL &&
                 (imap[index] & pos_mask) == pos_mask) {
        /* (recycling) mark first available slot in the group */
        i.i = index;
        set_hash *= desired_hash;
      }
    }
  }

  if (set_hash && i.i != (FIO_MAP_SIZE_TYPE)-1LL)
    imap[i.i] = set_hash;

  return i;

seek_as_array:
  pos = 0;
  if (m->w < FIO_MAP_CAPA(m->bits))
    i.i = m->w;
  while (pos < m->w) {
    if (m->map[pos].hash == hash) {
      /* test full collisions (attack) / match */
      if (m->under_attack || FIO_MAP_OBJ_KEY_CMP(m->map[pos].obj, key)) {
        i.i = pos;
        i.a = pos;
        return i;
      } else if (++total_collisions >= FIO_MAP_MAX_FULL_COLLISIONS) {
        m->under_attack = 1;
        FIO_LOG_SECURITY("Ordered map under attack?");
      }
    } else if (!m->map[pos].hash && i.i > pos) {
      i.i = pos;
    }
    ++pos;
  }
  if (set_hash && i.i != (FIO_MAP_SIZE_TYPE)-1LL)
    imap[i.i] = FIO_NAME(FIO_MAP_NAME, __hash2imap)(hash, m->bits);
  return i;

  (void)key; /* if unused */
}

FIO_IFUNC int FIO_NAME(FIO_MAP_NAME, __realloc)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                                size_t bits) {
  if (!m || bits > (sizeof(FIO_MAP_SIZE_TYPE) * 8))
    return -1;
  // if (bits < 3)
  //   bits = 3;
  if (bits != m->bits) {
    FIO_NAME(FIO_MAP_NAME, node_s) *tmp =
        (FIO_NAME(FIO_MAP_NAME, node_s) *)FIO_MEM_REALLOC_(
            m->map,
            FIO_MAP_MEMORY_SIZE(m->bits),
            FIO_MAP_MEMORY_SIZE(bits),
            (m->w * sizeof(*m->map)));
    if (!tmp)
      return -1;
    m->map = tmp;
    m->bits = (uint8_t)bits;
  }
  if (!FIO_MEM_REALLOC_IS_SAFE_ || bits == m->bits)
    memset(FIO_NAME(FIO_MAP_NAME, __imap)(m),
           0,
           sizeof(FIO_MAP_SIZE_TYPE) * FIO_MAP_CAPA(bits));
  /* rehash the map */
  if (m->count) {
    register FIO_MAP_SIZE_TYPE *const imap = FIO_NAME(FIO_MAP_NAME, __imap)(m);
    /* scan map for used slots to re-insert data */
    register const FIO_MAP_SIZE_TYPE end = m->w;
    if (m->w == m->count) {
      /* no holes, we can quickly run through the array and reindex */
      FIO_MAP_SIZE_TYPE i = 0;
      do {
        if (m->map[i].hash) {
          FIO_NAME(FIO_MAP_NAME, __pos_s)
          pos = FIO_NAME(
              FIO_MAP_NAME,
              __index)(m, m->map[i].hash, FIO_MAP_OBJ2KEY(m->map[i].obj), 1);
          if (pos.i == (FIO_MAP_SIZE_TYPE)-1LL)
            goto error;
          imap[pos.i] |= i;
        }
        i++;
      } while (i < end);
    } else {
      /* the array has holes -o compact the array while reindexing */
      FIO_MAP_SIZE_TYPE r = 0, w = 0;
      do {
#if FIO_MAP_EVICT_LRU
        if (w != r) {
          FIO_MAP_SIZE_TYPE head = m->map[r].node.next;
          m->map[w++] = m->map[r];
          if (m->last_used == r)
            m->last_used = w;
          FIO_INDEXED_LIST_REMOVE(m->map, node, r);
          FIO_INDEXED_LIST_PUSH(m->map, node, head, w);
        }
#else
        m->map[w++] = m->map[r];
#endif /* FIO_MAP_EVICT_LRU */
        if (m->map[r].hash) {
          FIO_NAME(FIO_MAP_NAME, __pos_s)
          pos = FIO_NAME(
              FIO_MAP_NAME,
              __index)(m, m->map[r].hash, FIO_MAP_OBJ2KEY(m->map[r].obj), 1);
          if (pos.i == (FIO_MAP_SIZE_TYPE)-1)
            goto error;
          imap[pos.i] |= r;
        }
        r++;
      } while (r < end);
      FIO_ASSERT_DEBUG(w == m->count, "rehashing logic error @ ordered map");
    }
  }
  return 0;
error:
  return -1;
}

FIO_IFUNC void FIO_NAME(FIO_MAP_NAME,
                        __destroy_all_objects)(FIO_NAME(FIO_MAP_NAME, s) * m) {
#if !FIO_MAP_TYPE_DESTROY_SIMPLE || !FIO_MAP_KEY_DESTROY_SIMPLE
  for (FIO_MAP_SIZE_TYPE i = 0; i < m->w; ++i) {
    if (!m->map[i].hash)
      continue;
    FIO_MAP_OBJ_DESTROY(m->map[i].obj);
#if DEBUG
    --m->count;
#endif
  }
  FIO_ASSERT_DEBUG(!m->count, "logic error @ ordered map clear.");
#else
  (void)m; /* no-op*/
#endif
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
  /* add destruction logic */
  FIO_NAME(FIO_MAP_NAME, __destroy_all_objects)(m);
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
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key, 0);
  if (pos.a == (FIO_MAP_SIZE_TYPE)(-1) || !m->map[pos.a].hash)
    return NULL;
#if FIO_MAP_EVICT_LRU
  if (m->last_used != pos.a) {
    FIO_INDEXED_LIST_REMOVE(m->map, node, pos.a);
    FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos.a);
    m->last_used = pos.a;
  }
#endif /* FIO_MAP_EVICT_LRU */
  return &FIO_MAP_OBJ2TYPE(m->map[pos.a].obj);
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
  /* make sure there's room in the value array */
  if (m->w + 1 == FIO_MAP_CAPA(m->bits))
    FIO_NAME(FIO_MAP_NAME, __realloc)(m, m->bits + (m->w == m->count));

#ifdef FIO_MAP_KEY
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key, 1);
#else
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, obj, 1);
#endif /* FIO_MAP_KEY */

  for (int i = 0; pos.i == (FIO_MAP_SIZE_TYPE)-1LL && i < 2; ++i) {
    if (FIO_NAME(FIO_MAP_NAME, __realloc)(m, m->bits + 1))
      continue;
#ifdef FIO_MAP_KEY
    pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key, 1);
#else
    pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, obj, 1);
#endif /* FIO_MAP_KEY */
  }
  if (pos.i == (FIO_MAP_SIZE_TYPE)-1LL)
    goto error;
  if (pos.a == (FIO_MAP_SIZE_TYPE)-1LL || !m->map[pos.a].hash) {
    /* new */
    if (pos.a == (FIO_MAP_SIZE_TYPE)-1LL)
      pos.a = m->w++;
#if FIO_MAP_MAX_ELEMENTS
    if (m->count >= FIO_MAP_MAX_ELEMENTS) {
      FIO_NAME(FIO_MAP_NAME, evict)(map, 1);
    }
#endif
    FIO_NAME(FIO_MAP_NAME, __imap)
    (m)[pos.i] |= pos.a;
    m->map[pos.a].hash = hash;
    FIO_MAP_TYPE_COPY(FIO_MAP_OBJ2TYPE(m->map[pos.a].obj), obj);
    FIO_MAP_KEY_COPY(FIO_MAP_OBJ2KEY(m->map[pos.a].obj), key);
#if FIO_MAP_EVICT_LRU
    if (m->count) {
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos.a);
    } else {
      m->map[pos.a].node.prev = m->map[pos.a].node.next = pos.a;
    }
    m->last_used = pos.a;
#endif /* FIO_MAP_EVICT_LRU */
    ++m->count;
  } else if (overwrite &&
             FIO_MAP_SHOULD_OVERWRITE(FIO_MAP_OBJ2TYPE(m->map[pos.a].obj),
                                      obj)) {
    /* overwrite existing */
    FIO_MAP_KEY_DISCARD(key);
    if (old) {
      FIO_MAP_TYPE_COPY(old[0], FIO_MAP_OBJ2TYPE(m->map[pos.a].obj));
      if (FIO_MAP_DESTROY_AFTER_COPY) {
        FIO_MAP_TYPE_DESTROY(FIO_MAP_OBJ2TYPE(m->map[pos.a].obj));
      }
    } else {
      FIO_MAP_TYPE_DESTROY(FIO_MAP_OBJ2TYPE(m->map[pos.a].obj));
    }
    FIO_MAP_TYPE_COPY(FIO_MAP_OBJ2TYPE(m->map[pos.a].obj), obj);
#if FIO_MAP_EVICT_LRU
    if (m->last_used != pos.a) {
      FIO_INDEXED_LIST_REMOVE(m->map, node, pos.a);
      FIO_INDEXED_LIST_PUSH(m->map, node, m->last_used, pos.a);
      m->last_used = pos.a;
    }
#endif /* FIO_MAP_EVICT_LRU */
  } else {
    FIO_MAP_TYPE_DISCARD(obj);
    FIO_MAP_KEY_DISCARD(key);
  }
  return &FIO_MAP_OBJ2TYPE(m->map[pos.a].obj);

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
  FIO_NAME(FIO_MAP_NAME, __pos_s)
  pos = FIO_NAME(FIO_MAP_NAME, __index)(m, hash, key, 0);
  if (pos.a == (FIO_MAP_SIZE_TYPE)(-1) || pos.i == (FIO_MAP_SIZE_TYPE)(-1) ||
      !m->map[pos.a].hash)
    return -1;
  FIO_NAME(FIO_MAP_NAME, __imap)(m)[pos.i] = ~(FIO_MAP_SIZE_TYPE)0;
  m->map[pos.a].hash = 0;
  --m->count;
  if (old) {
    FIO_MAP_TYPE_COPY(*old, FIO_MAP_OBJ2TYPE(m->map[pos.a].obj));
    FIO_MAP_OBJ_DESTROY_AFTER(m->map[pos.a].obj);
  } else {
    FIO_MAP_OBJ_DESTROY(m->map[pos.a].obj);
  }
#if FIO_MAP_EVICT_LRU
  if (pos.a == m->last_used)
    m->last_used = m->map[pos.a].node.next;
  FIO_INDEXED_LIST_REMOVE(m->map, node, pos.a);
#endif
  if (!m->count)
    m->w = 0;
  else if (pos.a + 1 == m->w) {
    --m->w;
    while (m->w && !m->map[m->w - 1].hash)
      --m->w;
  }
  return 0;
}

SFUNC void FIO_NAME(FIO_MAP_NAME, clear)(FIO_MAP_PTR map) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_PTR_UNTAG(map);
  if (!m)
    return;
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(map);
  FIO_NAME(FIO_MAP_NAME, __destroy_all_objects)(m);
  memset(FIO_NAME(FIO_MAP_NAME, __imap)(m), 0, FIO_MAP_CAPA(m->bits));
  m->under_attack = 0;
  m->count = m->w = 0;
#if FIO_MAP_EVICT_LRU
  m->last_used = 0;
#endif
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
    FIO_NAME(FIO_MAP_NAME, remove)
    (map, m->map[n].hash, FIO_MAP_OBJ2KEY(m->map[n].obj), NULL);
  } while (--number_of_elements);
#else  /* FIO_MAP_EVICT_LRU */
  /* scan map and evict FIFO. */
  for (FIO_MAP_SIZE_TYPE i = 0; number_of_elements && i < m->w; ++i) {
    /* skip empty groups (test for all bytes == 0 || 255 */
    if (m->map[i].hash) {
      FIO_NAME(FIO_MAP_NAME, remove)
      (map, m->map[i].hash, FIO_MAP_OBJ2KEY(m->map[i].obj), NULL);
      --number_of_elements; /* stop evicting? */
    }
  }
#endif /* FIO_MAP_EVICT_LRU */
  return 0;
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
  FIO_MAP_SIZE_TYPE count = m->count;
  if (start_at < 0) {
    start_at = count - start_at;
    if (start_at < 0)
      start_at = 0;
  }
  if ((FIO_MAP_SIZE_TYPE)start_at >= count)
    return count;
  FIO_MAP_SIZE_TYPE pos = 0;
  FIO_NAME(FIO_MAP_NAME, each_s)
  e = {
      .parent = map,
      .index = (uint64_t)start_at,
#ifdef FIO_MAP_KEY
      .items_at_index = 2,
#else
      .items_at_index = 1,
#endif
      .task = task,
      .udata = udata,
  };

  if (m->w == m->count) {
    while (e.index < m->count) {
      e.value = FIO_MAP_OBJ2TYPE(m->map[e.index].obj);
#ifdef FIO_MAP_KEY
      e.key = FIO_MAP_OBJ2KEY(m->map[e.index].obj);
#endif
      int r = e.task(&e);
      ++e.index;
      if (r == -1)
        break;
    }
    return (FIO_MAP_SIZE_TYPE)(e.index);
  }

  pos = 0;
  while (start_at && pos < m->w) {
    if (!m->map[pos++].hash) {
      continue;
    }
    --start_at;
  }

  if (start_at)
    return m->count;

  while (e.index < m->count && pos < m->w) {
    if (m->map[pos].hash) {
      e.value = FIO_MAP_OBJ2TYPE(m->map[pos].obj);
#ifdef FIO_MAP_KEY
      e.key = FIO_MAP_OBJ2KEY(m->map[pos].obj);
#endif
      int r = e.task(&e);
      ++e.index;
      if (r == -1)
        break;
    }
    ++pos;
  }
  return e.index;
}

/* *****************************************************************************
Ordered Map Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MAP_NAME */
