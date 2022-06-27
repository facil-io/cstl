/* ************************************************************************* */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#define FIO_IMAP_CORE                 /* Development inclusion - ignore line */
#include "100 mem.h"                  /* Development inclusion - ignore line */
#endif                                /* Development inclusion - ignore line */
/* *****************************************************************************




            Index Maps (mapping a partial hash to an Array object)
        Maps a Log 2 sized index map to a position in a Log 2 sized Array



Copyright: Boaz Segev, 2019-2021; License: ISC / MIT (choose your license)
***************************************************************************** */
#if defined(FIO_IMAP_CORE) && !defined(H___FIO_IMAP_CORE___H)
#define H___FIO_IMAP_CORE___H

/* *****************************************************************************
iMap Creation Macro
***************************************************************************** */
/**
 * This MACRO defines the type and functions needed for an indexed array.
 *
 * This is used internally and documentation is poor.
 *
 * An indexed array is simple ordered array who's objects are indexed using an
 * almost-hash map, allowing for easy seeking while also enjoying an array's
 * advantages.
 *
 * The index map uses one `imap_type` (i.e., `uint64_t`) to store both the index
 * in array and any leftover hash data (the first half being tested during the
 * random access and the leftover during comparison). The reserved value `0`
 * indicates a free slot. The reserved value `~0` indicates a freed item (a free
 * slot that was previously used).
 *
 * - `array_name_s` will be the main array container (.ary is the array itself)
 * - `array_name_seeker_s` is a seeker type that finds objects.
 * - `array_name_seek` is the seeking function finds an object or its future pos
 * - `array_name_set` writes or overwrites data to the array.
 * - `array_name_get` returns a pointer to the object within the array.
 * - `array_name_remove` removes an object and resets its memory to zero.
 */
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,                                     \
                               array_type,                                     \
                               imap_type,                                      \
                               hash_fn,                                        \
                               cmp_fn,                                         \
                               is_valid_fn)                                    \
  typedef struct {                                                             \
    array_type *ary;                                                           \
    imap_type count;                                                           \
    imap_type w;                                                               \
    imap_type capa_bits;                                                       \
  } FIO_NAME(array_name, s);                                                   \
  typedef struct {                                                             \
    imap_type pos;                                                             \
    imap_type ipos;                                                            \
    imap_type set_val;                                                         \
  } FIO_NAME(array_name, seeker_s);                                            \
  /** Returns the theoretical capacity for the indexed array. */               \
  FIO_IFUNC int FIO_NAME(array_name, is_valid)(array_type * pobj) {            \
    return !!is_valid_fn(pobj);                                                \
  }                                                                            \
  /** Returns the theoretical capacity for the indexed array. */               \
  FIO_IFUNC imap_type FIO_NAME(array_name,                                     \
                               capa)(FIO_NAME(array_name, s) * a) {            \
    if (!a || !a->capa_bits)                                                   \
      return 0;                                                                \
    return ((imap_type)1ULL << a->capa_bits);                                  \
  }                                                                            \
  /** Returns a pointer to the index map. */                                   \
  FIO_IFUNC imap_type *FIO_NAME(array_name,                                    \
                                imap)(FIO_NAME(array_name, s) * a) {           \
    return (imap_type *)(a->ary + ((imap_type)1ULL << a->capa_bits));          \
  }                                                                            \
  /** Deallocates dynamic memory. */                                           \
  FIO_IFUNC void FIO_NAME(array_name, destroy)(FIO_NAME(array_name, s) * a) {  \
    size_t capa = FIO_NAME(array_name, capa)(a);                               \
    FIO_MEM_FREE(a->ary,                                                       \
                 (capa * (sizeof(*a->ary)) + (capa * (sizeof(imap_type)))));   \
    *a = (FIO_NAME(array_name, s)){0};                                         \
    (void)capa; /* if unused */                                                \
  }                                                                            \
  /** Allocates dynamic memory. */                                             \
  FIO_IFUNC int FIO_NAME(array_name, __alloc)(FIO_NAME(array_name, s) * a,     \
                                              size_t bits) {                   \
    if (!bits || bits > ((sizeof(imap_type) << 3) - 2))                        \
      return -1;                                                               \
    size_t capa = 1ULL << bits;                                                \
    size_t old_capa = FIO_NAME(array_name, capa)(a);                           \
    array_type *tmp = (array_type *)FIO_MEM_REALLOC(                           \
        a->ary,                                                                \
        (a->bits ? (old_capa * (sizeof(array_type)) +                          \
                    (old_capa * (sizeof(imap_type))))                          \
                 : 0),                                                         \
        (capa * (sizeof(array_type)) + (capa * (sizeof(imap_type)))),          \
        (a->w * (sizeof(array_type))));                                        \
    (void)old_capa; /* if unused */                                            \
    if (!tmp)                                                                  \
      return -1;                                                               \
    a->capa_bits = bits;                                                       \
    a->ary = tmp;                                                              \
    if (!FIO_MEM_REALLOC_IS_SAFE)                                              \
      FIO_MEMSET((tmp + capa), 0, (capa * (sizeof(imap_type))));               \
    return 0;                                                                  \
  }                                                                            \
  /** Returns the index map position and array position of a value, if any. */ \
  FIO_SFUNC FIO_NAME(array_name, seeker_s)                                     \
      FIO_NAME(array_name, seek)(FIO_NAME(array_name, s) * a,                  \
                                 array_type * pobj) {                          \
    FIO_NAME(array_name, seeker_s) r = {0, (~(imap_type)0), (~(imap_type)0)};  \
    if (!a || ((!a->capa_bits) | (!a->ary)))                                   \
      return r;                                                                \
    r.pos = a->w;                                                              \
    imap_type capa = (imap_type)1UL << a->capa_bits;                           \
    imap_type *imap = (imap_type *)(a->ary + capa);                            \
    const imap_type pos_mask = capa - 1;                                       \
    const imap_type hash_mask = ~pos_mask;                                     \
    const imap_type hash = hash_fn(pobj);                                      \
    imap_type tester = hash & hash_mask;                                       \
    tester += (!tester) << a->capa_bits;                                       \
    tester -= (hash_mask == tester) << a->capa_bits;                           \
    size_t attempts = 11;                                                      \
    imap_type pos = hash;                                                      \
    for (;;) {                                                                 \
      pos &= pos_mask;                                                         \
      /* test up to 3 groups of 4 bytes (uint32_t) within a 64 byte group */   \
      for (int mini_steps = 0;;) {                                             \
        const uint32_t pos_hash = imap[pos] & hash_mask;                       \
        const uint32_t pos_index = imap[pos] & pos_mask;                       \
        if ((pos_hash == tester) && cmp_fn((a->ary + pos_index), pobj)) {      \
          r.ipos = pos;                                                        \
          r.pos = imap[pos] & pos_mask;                                        \
          r.set_val = tester | r.pos;                                          \
          return r;                                                            \
        }                                                                      \
        if (!pos_hash) {                                                       \
          r.ipos = pos;                                                        \
          r.set_val = tester | r.pos;                                          \
          return r;                                                            \
        }                                                                      \
        if (imap[pos] == (imap_type)(~(imap_type)0)) {                         \
          r.ipos = pos;                                                        \
          r.set_val = tester | r.pos;                                          \
        }                                                                      \
        if (!((--attempts)))                                                   \
          goto done;                                                           \
        if (mini_steps == 2)                                                   \
          break;                                                               \
        pos += 3 + mini_steps; /* 0, 3, 7 =  max of 56 byte distance */        \
        pos &= pos_mask;                                                       \
        ++mini_steps;                                                          \
      }                                                                        \
      pos += 0x43F82D0BUL; /* big step */                                      \
    };                                                                         \
  done:                                                                        \
    return r;                                                                  \
  }                                                                            \
  /** fills an empty imap with the info about existing elements. */            \
  FIO_SFUNC int FIO_NAME(array_name,                                           \
                         __fill_imap)(FIO_NAME(array_name, s) * a) {           \
    imap_type *imap = FIO_NAME(array_name, imap)(a);                           \
    if (a->count != a->w) {                                                    \
      a->count = 0;                                                            \
      for (size_t i = 0; i < a->w; ++i) {                                      \
        if (!is_valid_fn(a->ary + i))                                          \
          continue;                                                            \
        if (a->count != i)                                                     \
          a->ary[a->count] = a->ary[i];                                        \
        ++a->count;                                                            \
      }                                                                        \
    }                                                                          \
    for (size_t i = 0; i < a->count; ++i) {                                    \
      a->w = i;                                                                \
      FIO_NAME(array_name, seeker_s)                                           \
      s = FIO_NAME(array_name, seek)(a, a->ary + i);                           \
      if (s.pos != i) {                                                        \
        a->w = a->count;                                                       \
        return -1; /* destination not big enough to contain collisions! */     \
      }                                                                        \
      imap[s.ipos] = s.set_val;                                                \
    }                                                                          \
    a->w = a->count;                                                           \
    return 0;                                                                  \
  }                                                                            \
  /** expands the existing array & imap storage capacity. */                   \
  FIO_IFUNC int FIO_NAME(array_name, __expand)(FIO_NAME(array_name, s) * a) {  \
    for (;;) {                                                                 \
      if (FIO_NAME(array_name, __alloc)(a,                                     \
                                        a->capa_bits + 1 + (!a->capa_bits)))   \
        return -1;                                                             \
      if (!FIO_NAME(array_name, __fill_imap)(a))                               \
        return 0;                                                              \
    }                                                                          \
  }                                                                            \
  /** Sets an object in the Array. Optionally overwrites existing data if any. \
   */                                                                          \
  FIO_IFUNC array_type *FIO_NAME(array_name, set)(FIO_NAME(array_name, s) * a, \
                                                  array_type obj,              \
                                                  int overwrite) {             \
    if (!a || !is_valid_fn(&obj))                                              \
      return NULL;                                                             \
    {                                                                          \
      size_t capa = FIO_NAME(array_name, capa)(a);                             \
      if (a->w == capa)                                                        \
        FIO_NAME(array_name, __expand)(a);                                     \
      else if (a->count != a->w &&                                             \
               (a->w + (a->w >> 1)) > FIO_NAME(array_name, capa)(a)) {         \
        FIO_MEMSET((a->ary + capa), 0, (capa * (sizeof(imap_type))));          \
        FIO_NAME(array_name, __fill_imap)(a);                                  \
      }                                                                        \
    }                                                                          \
    for (;;) {                                                                 \
      FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);  \
      if (s.ipos == (imap_type)(~(imap_type)0)) { /* no room in the imap */    \
        FIO_NAME(array_name, __expand)(a);                                     \
        continue;                                                              \
      }                                                                        \
      if (s.pos == a->w) { /* new object */                                    \
        a->ary[a->w] = obj;                                                    \
        ++a->w;                                                                \
        ++a->count;                                                            \
        FIO_NAME(array_name, imap)(a)[s.ipos] = s.set_val;                     \
        return a->ary + s.pos;                                                 \
      }                                                                        \
      FIO_ASSERT_DEBUG(s.pos < a->w && s.ipos < FIO_NAME(array_name, capa)(a), \
                       "WTF?");                                                \
      if (!overwrite)                                                          \
        return a->ary + s.pos;                                                 \
      a->ary[s.pos] = obj;                                                     \
      return a->ary + s.pos;                                                   \
    }                                                                          \
  }                                                                            \
  /** Finds an object in the Array using the index map. */                     \
  FIO_IFUNC array_type *FIO_NAME(array_name, get)(FIO_NAME(array_name, s) * a, \
                                                  array_type obj) {            \
    if (!a || !is_valid_fn(&obj))                                              \
      return NULL;                                                             \
    FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);    \
    if (s.pos >= a->w)                                                         \
      return NULL;                                                             \
    return a->ary + s.pos;                                                     \
  }                                                                            \
  /** Removes an object in the Array's index map, zeroing out its memory. */   \
  FIO_IFUNC int FIO_NAME(array_name, remove)(FIO_NAME(array_name, s) * a,      \
                                             array_type obj) {                 \
    if (!a || !is_valid_fn(&obj))                                              \
      return -1;                                                               \
    FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);    \
    if (s.pos >= a->w)                                                         \
      return -1;                                                               \
    a->ary[s.pos] = (array_type){0};                                           \
    FIO_NAME(array_name, imap)(a)[s.ipos] = (~(imap_type)0);                   \
    --a->count;                                                                \
    while (a->w && !is_valid_fn(a->ary + a->w - 1))                            \
      --a->w;                                                                  \
    return 0;                                                                  \
  }

#define FIO_IMAP_EACH(array_name, map_ptr, i)                                  \
  for (size_t i = 0; i < map_ptr->w; ++i)                                      \
    if (!FIO_NAME(array_name, is_valid)(map_ptr->ary + i))                     \
      continue;                                                                \
    else

/* *****************************************************************************
iMap Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

#define FIO_IMAP_TESTER_IMAP_HASH(n)   ((n)[0] | ((n)[0] << 9))
#define FIO_IMAP_TESTER_IMAP_CMP(a, b) (*(a) == *(b))
#define FIO_IMAP_TESTER_IMAP_VALID(n)  ((n)[0])
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester,
                       size_t,
                       uint32_t, /* good for up to 65K objects */
                       FIO_IMAP_TESTER_IMAP_HASH,
                       FIO_IMAP_TESTER_IMAP_CMP,
                       FIO_IMAP_TESTER_IMAP_VALID)

#undef FIO_IMAP_TESTER_IMAP_HASH
#undef FIO_IMAP_TESTER_IMAP_CMP
#undef FIO_IMAP_TESTER_IMAP_VALID

FIO_SFUNC void FIO_NAME_TEST(stl, imap_core)(void) {
  fprintf(stderr, "* testing core indexed array type (imap)\n");
  fio_imap_tester_s a = {0};
  for (size_t val = 1; val < 4096; ++val) {
    fio_imap_tester_set(&a, val, 1);
    FIO_ASSERT(a.count == val, "imap array count failed at set %zu!", val);
    fio_imap_tester_set(&a, val, 0);
    fio_imap_tester_set(&a, val, 0);
    fio_imap_tester_set(&a, val, 0);
    FIO_ASSERT(a.count == val, "imap array double-set error %zu!", val);
    FIO_ASSERT(fio_imap_tester_get(&a, val) &&
                   fio_imap_tester_get(&a, val)[0] == val,
               "imap array get failed for %zu!",
               val);
  }
  for (size_t val = 4096; --val;) {
    FIO_ASSERT(fio_imap_tester_get(&a, val) &&
                   fio_imap_tester_get(&a, val)[0] == val,
               "imap array get failed for %zu (2)!",
               val);
    fio_imap_tester_remove(&a, val);
    FIO_ASSERT((size_t)(a.count + 1) == val,
               "imap array count failed at remove %zu!",
               val);
    FIO_ASSERT(!fio_imap_tester_get(&a, val),
               "imap array get should fail after remove for %zu!",
               val);
  }
  fio_imap_tester_destroy(&a);
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
iMap Cleanup
***************************************************************************** */
#endif /* FIO_IMAP_CORE */
#undef FIO_IMAP_CORE
