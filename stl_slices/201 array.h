/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                            Dynamic Arrays








Example:

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_CMP(a,b) (a.i == b.i && a.f == b.f)
#include "fio_cstl.h"

void example(void) {
  ary_s a = FIO_ARRAY_INIT;
  foo_s *p = ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(&a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - ary2ptr(&a)), pos->i);
  }
  ary_destroy(&a);
}
```

***************************************************************************** */

#ifdef FIO_ARRAY_NAME

#ifndef FIO_ARRAY_TYPE
/** The type for array elements (an array of FIO_ARRAY_TYPE) */
#define FIO_ARRAY_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID NULL
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#else
#ifndef FIO_ARRAY_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID ((FIO_ARRAY_TYPE){0})
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#endif
#endif

#ifndef FIO_ARRAY_TYPE_COPY
/** Handles a copy operation for an array's element. */
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_DESTROY
/** Handles a destroy / free operation for an array's element. */
#define FIO_ARRAY_TYPE_DESTROY(obj)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_DESTROY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_CMP
/** Handles a comparison operation for an array's element. */
#define FIO_ARRAY_TYPE_CMP(a, b) (a) == (b)
/* internal flag - do not set */
#define FIO_ARRAY_TYPE_CMP_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_CONCAT_COPY
#define FIO_ARRAY_TYPE_CONCAT_COPY FIO_ARRAY_TYPE_COPY
#define FIO_ARRAY_TYPE_CONCAT_SIMPLE FIO_ARRAY_TYPE_COPY_SIMPLE
#endif
/**
 * The FIO_ARRAY_DESTROY_AFTER_COPY macro should be set if
 * FIO_ARRAY_TYPE_DESTROY should be called after FIO_ARRAY_TYPE_COPY when an
 * object is removed from the array after being copied to an external container
 * (an `old` pointer)
 */
#ifndef FIO_ARRAY_DESTROY_AFTER_COPY
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE && !FIO_ARRAY_TYPE_COPY_SIMPLE
#define FIO_ARRAY_DESTROY_AFTER_COPY 1
#else
#define FIO_ARRAY_DESTROY_AFTER_COPY 0
#endif
#endif

/* Extra empty slots when allocating memory. */
#ifndef FIO_ARRAY_PADDING
#define FIO_ARRAY_PADDING 4
#endif

/* Sets memory growth to exponentially increase. Consumes more memory. */
#ifndef FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_EXPONENTIAL 0
#endif

#undef FIO_ARRAY_SIZE2WORDS
#define FIO_ARRAY_SIZE2WORDS(size)                                             \
  ((sizeof(FIO_ARRAY_TYPE) & 1)                                                \
       ? (((size) & (~15)) + 16)                                               \
       : (sizeof(FIO_ARRAY_TYPE) & 2)                                          \
             ? (((size) & (~7)) + 8)                                           \
             : (sizeof(FIO_ARRAY_TYPE) & 4)                                    \
                   ? (((size) & (~3)) + 4)                                     \
                   : (sizeof(FIO_ARRAY_TYPE) & 8) ? (((size) & (~1)) + 2)      \
                                                  : (size))

/* *****************************************************************************
Dynamic Arrays - type
***************************************************************************** */

typedef struct {
  FIO_ARRAY_TYPE *ary;
  uint32_t capa;
  uint32_t start;
  uint32_t end;
} FIO_NAME(FIO_ARRAY_NAME, s);

#ifdef FIO_PTR_TAG_TYPE
#define FIO_ARRAY_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, s) *
#endif

/* *****************************************************************************
Dynamic Arrays - API
***************************************************************************** */

#ifndef FIO_ARRAY_INIT
/* Initialization macro. */
#define FIO_ARRAY_INIT                                                         \
  { 0 }
#endif

#ifndef FIO_REF_CONSTRUCTOR_ONLY

/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void);

/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary);

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary);

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary);

/**
 * Reserves a minimal capacity for the array.
 *
 * If `capa` is negative, new memory will be allocated at the beginning of the
 * array rather then it's end.
 *
 * Returns the array's new capacity.
 *
 * Note: the reserved capacity includes existing data. If the requested reserved
 * capacity is equal (or less) then the existing capacity, nothing will be done.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary,
                                                 int32_t capa);

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Always returns the destination array (`dest`).
 */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_PTR dest,
                                                     FIO_ARRAY_PTR src);

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old);

/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary,
                                                   int32_t index);

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary,
                                             FIO_ARRAY_TYPE data,
                                             int32_t start_at);

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary,
                                           int32_t index,
                                           FIO_ARRAY_TYPE *old);

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary,
                                                 FIO_ARRAY_TYPE data);

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary);

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary);

/**
 * Pushes an object to the end of the Array. Returns a pointer to the new object
 * or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary,
                                                     FIO_ARRAY_TYPE data);

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE *old);

/**
 * Unshifts an object to the beginning of the Array. Returns a pointer to the
 * new object or NULL on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary,
                                                        FIO_ARRAY_TYPE data);

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary,
                                          FIO_ARRAY_TYPE *old);

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an the entry data as well as an opaque
 * user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_PTR ary,
                              int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg);

#ifndef FIO_ARRAY_EACH
/**
 * Iterates through the list using a `for` loop.
 *
 * Access the object with the pointer `pos`. The `pos` variable can be named
 * however you please.
 *
 * Avoid editing the array during a FOR loop, although I hope it's possible, I
 * wouldn't count on it.
 *
 * **Note**: doesn't support automatic pointer tagging / untagging.
 */
#define FIO_ARRAY_EACH(array, pos)                                             \
  if ((array)->ary)                                                            \
    for (__typeof__((array)->ary) start__tmp__ = (array)->ary,                 \
                                  pos = ((array)->ary + (array)->start);       \
         pos < (array)->ary + (array)->end;                                    \
         (pos = (array)->ary + (pos - start__tmp__) + 1),                      \
                                  (start__tmp__ = (array)->ary))
#endif

#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
Dynamic Arrays - internal helpers
***************************************************************************** */

#define FIO_ARRAY_POS2ABS(ary, pos)                                            \
  (pos > 0 ? (ary->start + pos) : (ary->end - pos))

#define FIO_ARRAY_AB_CT(cond, a, b) ((b) ^ ((0 - ((cond)&1)) & ((a) ^ (b))))

/* *****************************************************************************
Dynamic Arrays - implementation
***************************************************************************** */

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, new)(void) {
  FIO_NAME(FIO_ARRAY_NAME, s) *a =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)FIO_MEM_CALLOC_(sizeof(*a), 1);
  return (FIO_ARRAY_PTR)FIO_PTR_TAG(a);
}

/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  FIO_NAME(FIO_ARRAY_NAME, destroy)(ary_);
  FIO_MEM_FREE_(ary, sizeof(*ary));
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  /* move array data to a temp var., protecting against recursive nesting */
  FIO_NAME(FIO_ARRAY_NAME, s) tmp = *ary;
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
  for (size_t i = tmp.start; i < tmp.end; ++i) {
    FIO_ARRAY_TYPE_DESTROY(tmp.ary[i]);
  }
#endif
  FIO_MEM_FREE_(tmp.ary, tmp.capa * sizeof(*tmp.ary));
}

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  return (ary->end - ary->start);
}

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  return ary->capa;
}
/** Reserves a minimal capacity for the array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_PTR ary_,
                                                 int32_t capa) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary)
    return 0;
  const uint32_t s = ary->start;
  const uint32_t e = ary->end;
  if (capa > 0) {
    if (ary->capa >= (uint32_t)capa)
      return ary->capa;
    FIO_NAME(FIO_ARRAY_NAME, set)(ary_, capa - 1, FIO_ARRAY_TYPE_INVALID, NULL);
    ary->end = ary->start + (e - s);
  } else {
    if (ary->capa >= (uint32_t)(0 - capa))
      return ary->capa;
    FIO_NAME(FIO_ARRAY_NAME, set)(ary_, capa, FIO_ARRAY_TYPE_INVALID, NULL);
    ary->start = ary->end - (e - s);
  }
  return ary->capa;
}

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Returns `dest` on success or NULL on error (i.e., no memory).
 */
SFUNC FIO_ARRAY_PTR FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_PTR dest_,
                                                     FIO_ARRAY_PTR src_) {
  FIO_PTR_TAG_VALID_OR_RETURN(dest_, (FIO_ARRAY_PTR)NULL);
  FIO_PTR_TAG_VALID_OR_RETURN(src_, (FIO_ARRAY_PTR)NULL);
  FIO_NAME(FIO_ARRAY_NAME, s) *dest =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(dest_));
  FIO_NAME(FIO_ARRAY_NAME, s) *src =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(src_));
  if (!dest || !src || !src->end || src->end - src->start == 0)
    return dest_;
  /* avoid '-' in (dest->capa < dest->end + src->end - src->start) */
  if (dest->capa + src->start < src->end + dest->end) {
    /* insufficiant memory, (re)allocate */
    uint32_t new_capa = dest->end + (src->end - src->start);
    FIO_ARRAY_TYPE *tmp =
        (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(dest->ary,
                                           dest->capa * sizeof(*tmp),
                                           new_capa * sizeof(*tmp),
                                           dest->end * sizeof(*tmp));
    if (!tmp)
      return (FIO_ARRAY_PTR)(NULL);
    dest->ary = tmp;
    dest->capa = new_capa;
  }
  /* copy data */
#if FIO_ARRAY_TYPE_COPY_SIMPLE && FIO_ARRAY_TYPE_CONCAT_SIMPLE
  memcpy(dest->ary + dest->end, src->ary + src->start, src->end - src->start);
#else
  for (size_t i = 0; i + src->start < src->end; ++i) {
    FIO_ARRAY_TYPE_CONCAT_COPY((dest->ary + dest->end + i)[0],
                               (src->ary + i + src->start)[0]);
  }
#endif
  /* update dest */
  dest->end += src->end - src->start;
  return dest_;
}

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_PTR ary_,
                                                    int32_t index,
                                                    FIO_ARRAY_TYPE data,
                                                    FIO_ARRAY_TYPE *old) {
#if FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_ADD2CAPA ary->capa + FIO_ARRAY_PADDING
#else
#define FIO_ARRAY_ADD2CAPA FIO_ARRAY_PADDING
#endif

  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid_ptr);
  {
    FIO_NAME(FIO_ARRAY_NAME, s) *ary =
        (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
    uint8_t pre_existing = 1;
    if (index >= 0) {
      /* zero based (look forward) */
      index = index + ary->start;
      if ((uint32_t)index >= ary->capa) {
        /* we need more memory */
        uint32_t new_capa =
            FIO_ARRAY_SIZE2WORDS(((uint32_t)index + FIO_ARRAY_ADD2CAPA));
        FIO_ARRAY_TYPE *tmp =
            (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(ary->ary,
                                               ary->capa * sizeof(*tmp),
                                               new_capa * sizeof(*tmp),
                                               ary->end * sizeof(*tmp));
        if (!tmp)
          return NULL;
        ary->ary = tmp;
        ary->capa = new_capa;
      }
      ary->ary[ary->end++] = FIO_ARRAY_TYPE_INVALID;
      if ((uint32_t)index >= ary->end) {
        /* we to initialize memory between ary->end and index + ary->start */
        pre_existing = 0;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
        memset(ary->ary + ary->end, 0, (index - ary->end) * sizeof(*ary->ary));
#else
        for (size_t i = ary->end; i <= (size_t)index; ++i) {
          FIO_ARRAY_TYPE_COPY(ary->ary[i], FIO_ARRAY_TYPE_INVALID);
        }
#endif
        ary->end = index + 1;
      }
    } else {
      /* -1 based (look backwards) */
      index += ary->end;
      if (index < 0) {
        /* we need more memory at the HEAD (requires copying) */
        const uint32_t new_capa = FIO_ARRAY_SIZE2WORDS(
            ((uint32_t)ary->capa + FIO_ARRAY_ADD2CAPA + ((uint32_t)0 - index)));
        const uint32_t valid_data = ary->end - ary->start;
        index -= ary->end; /* return to previous state */
        FIO_ARRAY_TYPE *tmp =
            (FIO_ARRAY_TYPE *)FIO_MEM_CALLOC_(new_capa, sizeof(*tmp));
        if (!tmp)
          return NULL;
        if (valid_data)
          memcpy(tmp + new_capa - valid_data,
                 ary->ary + ary->start,
                 valid_data * sizeof(*tmp));
        FIO_MEM_FREE_(ary->ary, sizeof(*ary->ary) * ary->capa);
        ary->end = ary->capa = new_capa;
        index += new_capa;
        ary->ary = tmp;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
        ary->start = index;
#else
        ary->start = new_capa - valid_data;
#endif
      }
      if ((uint32_t)index < ary->start) {
        /* initialize memory between `index` and `ary->start-1` */
        pre_existing = 0;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
        memset(ary->ary + index, 0, (ary->start - index) * sizeof(*ary->ary));
        ary->start = index;
#else
        while ((uint32_t)index < ary->start) {
          --ary->start;
          ary->ary[ary->start] = FIO_ARRAY_TYPE_INVALID;
          // FIO_ARRAY_TYPE_COPY(ary->ary[ary->start], FIO_ARRAY_TYPE_INVALID);
        }
#endif
      }
    }
    /* copy / clear object */
    if (old) {
      FIO_ARRAY_TYPE_COPY((*old), ary->ary[index]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
      if (pre_existing) {
        FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
      }
#endif
    } else if (pre_existing) {
      FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
    }

    ary->ary[index] = FIO_ARRAY_TYPE_INVALID;
    FIO_ARRAY_TYPE_COPY(ary->ary[index], data);
    return ary->ary + index;
  }

invalid_ptr:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;
}
#undef FIO_ARRAY_ADD2CAPA
/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_PTR ary_,
                                                   int32_t index) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, FIO_ARRAY_TYPE_INVALID);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  index += FIO_ARRAY_AB_CT(index >= 0, ary->start, ary->end);
  if (index < 0 || (uint32_t)index >= ary->end)
    return FIO_ARRAY_TYPE_INVALID;
  return ary->ary[index];
}

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_PTR ary_,
                                             FIO_ARRAY_TYPE data,
                                             int32_t start_at) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (start_at >= 0) {
    /* seek forwards */
    while ((uint32_t)start_at < ary->end) {
      if (FIO_ARRAY_TYPE_CMP(ary->ary[start_at], data))
        return start_at;
      ++start_at;
    }
  } else {
    /* seek backwards */
    start_at = start_at + ary->end;
    if (start_at >= (int32_t)ary->end)
      start_at = ary->end - 1;
    while (start_at > (int32_t)ary->start) {
      if (FIO_ARRAY_TYPE_CMP(ary->ary[start_at], data))
        return start_at;
      --start_at;
    }
  }
  return -1;
}

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_PTR ary_,
                                           int32_t index,
                                           FIO_ARRAY_TYPE *old) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  index += FIO_ARRAY_AB_CT(index >= 0, ary->start, ary->end);
  if (!ary || (uint32_t)index >= ary->end || index < (int32_t)ary->start) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[index]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
    FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
#endif
  } else {
    FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
  }
  if ((uint32_t)index == ary->start) {
    /* unshift */
    ++ary->start;
  } else {
    /* pop? */
    --ary->end;
    if (ary->end != (uint32_t)index) {
      memmove(ary->ary + index,
              ary->ary + index + 1,
              (ary->end - index) * sizeof(*old));
    }
  }
  return 0;
}

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_PTR ary_,
                                                 FIO_ARRAY_TYPE data) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  size_t c = 0;
  size_t i = ary->start;
  while (i < ary->end) {
    if (!(FIO_ARRAY_TYPE_CMP(ary->ary[i + c], data))) {
      ary->ary[i] = ary->ary[i + c];
      ++i;
      continue;
    }
    FIO_ARRAY_TYPE_DESTROY(ary->ary[i + c]);
    --ary->end;
    ++c;
  }
  return c;
}

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(ary_);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  FIO_ARRAY_TYPE *tmp = NULL;
  if (!(ary->end - ary->start))
    goto finish;
  tmp = (FIO_ARRAY_TYPE *)FIO_MEM_CALLOC((ary->end - ary->start), sizeof(*tmp));
  if (!tmp)
    return;
  memcpy(
      tmp, ary->ary + ary->start, (ary->end - ary->start) * sizeof(*ary->ary));
finish:
  if (ary->ary) {
    FIO_MEM_FREE_(ary->ary, ary->capa * sizeof(*ary->ary));
  }
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
      .ary = tmp,
      .capa = (ary->end - ary->start),
      .start = 0,
      .end = (ary->end - ary->start),
  };
}

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME2(FIO_ARRAY_NAME, ptr)(FIO_ARRAY_PTR ary_) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, (FIO_ARRAY_TYPE *)NULL);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  return ary->ary + ary->start;
}

/**
 * Pushes an object to the end of the Array. Returns -1 on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_PTR ary_,
                                                     FIO_ARRAY_TYPE data) {
  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid_ptr);
  {
    FIO_NAME(FIO_ARRAY_NAME, s) *ary =
        (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
    FIO_ARRAY_TYPE *pos;
    if (ary->end >= ary->capa)
      goto needs_memory;
    pos = ary->ary + ary->end;
    *pos = FIO_ARRAY_TYPE_INVALID;
    ++ary->end;
    FIO_ARRAY_TYPE_COPY(*pos, data);
    return pos;
  needs_memory:
    return FIO_NAME(FIO_ARRAY_NAME, set)(ary_, ary->end, data, NULL);
  }
invalid_ptr:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;
}

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_PTR ary_,
                                        FIO_ARRAY_TYPE *old) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary || ary->start == ary->end) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  --ary->end;
  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->end]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
    FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->end]);
#endif
  } else {
    FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->end]);
  }
  return 0;
}

/**
 * Unshifts an object to the beginning of the Array. Returns -1 on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_PTR ary_,
                                                        FIO_ARRAY_TYPE data) {
  FIO_PTR_TAG_VALID_OR_GOTO(ary_, invalid_ptr);

  {
    FIO_NAME(FIO_ARRAY_NAME, s) *ary =
        (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
    if (ary->start) {
      --ary->start;
      FIO_ARRAY_TYPE *pos = ary->ary + ary->start;
      *pos = FIO_ARRAY_TYPE_INVALID;
      FIO_ARRAY_TYPE_COPY(*pos, data);
      return pos;
    }
    return FIO_NAME(FIO_ARRAY_NAME, set)(ary_, -1 - ary->end, data, NULL);
  }
invalid_ptr:
  FIO_ARRAY_TYPE_DESTROY(data);
  return NULL;
}

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_PTR ary_,
                                          FIO_ARRAY_TYPE *old) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, -1);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary || ary->start == ary->end) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  if (old) {
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->start]);
#if FIO_ARRAY_DESTROY_AFTER_COPY
    FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->start]);
#endif
  } else {
    FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->start]);
  }
  ++ary->start;
  return 0;
}

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an the entry data as well as an opaque
 * user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_PTR ary_,
                              int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg) {
  FIO_PTR_TAG_VALID_OR_RETURN(ary_, 0);
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_PTR_UNTAG(ary_));
  if (!ary || !task)
    return start_at;
  if (start_at < 0)
    start_at += ary->end - ary->start;
  if (start_at < 0)
    start_at = 0;
  for (size_t i = ary->start + start_at; i < ary->end; ++i) {
    if (task(ary->ary[i], arg) == -1) {
      return (uint32_t)((i + 1) - ary->start);
    }
  }
  return ary->end - ary->start;
}

/* *****************************************************************************
Dynamic Arrays - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_ARRAY_NAME
#undef FIO_ARRAY_TYPE
#undef FIO_ARRAY_TYPE_INVALID
#undef FIO_ARRAY_TYPE_INVALID_SIMPLE
#undef FIO_ARRAY_TYPE_COPY
#undef FIO_ARRAY_TYPE_COPY_SIMPLE
#undef FIO_ARRAY_TYPE_DESTROY
#undef FIO_ARRAY_TYPE_DESTROY_SIMPLE
#undef FIO_ARRAY_DESTROY_AFTER_COPY
#undef FIO_ARRAY_TYPE_CMP
#undef FIO_ARRAY_TYPE_CMP_SIMPLE
#undef FIO_ARRAY_TYPE_CONCAT_SIMPLE
#undef FIO_ARRAY_TYPE_CONCAT_COPY
#undef FIO_ARRAY_PADDING
#undef FIO_ARRAY_SIZE2WORDS
#undef FIO_ARRAY_POS2ABS
#undef FIO_ARRAY_AB_CT
#undef FIO_ARRAY_PTR
#undef FIO_ARRAY_EXPONENTIAL
#endif /* FIO_ARRAY_NAME */
