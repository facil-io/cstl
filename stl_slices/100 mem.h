/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#define FIO_RAND                    /* Development inclusion - ignore line */
#define FIO_RISKY_HASH              /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "005 riskyhash.h"          /* Development inclusion - ignore line */
#define FIO_MEMORY_NAME fio         /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                      Custom Memory Allocator / Pooling










***************************************************************************** */

/* *****************************************************************************
Memory Allocation - fast setup for a global allocator
***************************************************************************** */
#if defined(FIO_MALLOC) && !defined(H___FIO_MALLOC___H)
#define FIO_MEMORY_NAME fio

#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/* for a general allocator, increase system allocation size to 8Gb */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 23
#endif

#ifndef FIO_MEMORY_CACHE_SLOTS
/* for a general allocator, increase cache size */
#define FIO_MEMORY_CACHE_SLOTS 8
#endif

#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
/* set fragmentation cost at 0.5Mb blocks */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 5
#endif

#ifndef FIO_MEMORY_ENABLE_BIG_ALLOC
/* support big allocations using undivided memory chunks */
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
#endif

#undef FIO_MEM_REALLOC
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))

#undef FIO_MEM_FREE
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))

#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MEM_REALLOC_IS_SAFE fio_realloc_is_safe()

/* prevent double decleration of FIO_MALLOC */
#define H___FIO_MALLOC___H
#endif
#undef FIO_MALLOC

/* *****************************************************************************
Memory Allocation - Setup Alignment Info
***************************************************************************** */
#ifdef FIO_MEMORY_NAME

#undef FIO_MEM_ALIGN
#undef FIO_MEM_ALIGN_NEW

#ifndef FIO_MEMORY_ALIGN_LOG
/** Allocation alignment, MUST be >= 3 and <= 10*/
#define FIO_MEMORY_ALIGN_LOG 4

#elif FIO_MEMORY_ALIGN_LOG < 3 || FIO_MEMORY_ALIGN_LOG > 10
#undef FIO_MEMORY_ALIGN_LOG
#define FIO_MEMORY_ALIGN_LOG 4
#endif

/* Helper macro, don't change this */
#undef FIO_MEMORY_ALIGN_SIZE
/**
 * The maximum allocation size, after which a direct system allocation is used.
 */
#define FIO_MEMORY_ALIGN_SIZE (1UL << FIO_MEMORY_ALIGN_LOG)

/* inform the compiler that the returned value is aligned on 16 byte marker */
#if __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#define FIO_MEM_ALIGN __attribute__((assume_aligned(FIO_MEMORY_ALIGN_SIZE)))
#define FIO_MEM_ALIGN_NEW                                                      \
  __attribute__((malloc, assume_aligned(FIO_MEMORY_ALIGN_SIZE)))
#else
#define FIO_MEM_ALIGN
#define FIO_MEM_ALIGN_NEW
#endif /* (__clang__ || __GNUC__)... */

/* *****************************************************************************
Memory Allocation - API
***************************************************************************** */

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
 * as if `mempool_mmap` was called.
 *
 * `mempool_malloc` promises a best attempt at providing locality between
 * consecutive calls, but locality can't be guaranteed.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, malloc)(size_t size);

/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
 * as if `mempool_mmap` was called.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME,
                                       calloc)(size_t size_per_unit,
                                               size_t unit_count);

/** Frees memory that was allocated using this library. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, free)(void *ptr);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc)(void *ptr,
                                                             size_t new_size);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc2)(void *ptr,
                                                              size_t new_size,
                                                              size_t copy_len);

/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `mempoll_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, mmap)(size_t size);

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_after_fork)(void);

/* *****************************************************************************
Memory Allocation - configuration macros

NOTE: most configuration values should be a power of 2 or a logarithmic value.
***************************************************************************** */

/** FIO_MEMORY_DISABLE diasbles all custom memory allocators. */
#ifdef FIO_MEMORY_DISABLE
#ifndef FIO_MALLOC_TMP_USE_SYSTEM
#define FIO_MALLOC_TMP_USE_SYSTEM 1
#endif
#endif

#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/**
 * The logarithmic size of a single allocatiion "chunk" (16 blocks).
 *
 * Limited to >=17 and <=24.
 *
 * By default 22, which is a ~2Mb allocation per system call, resulting in a
 * maximum allocation size of 131Kb.
 */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 21
#endif

#ifndef FIO_MEMORY_CACHE_SLOTS
/**
 * The number of system allocation "chunks" to cache even if they are not in
 * use.
 */
#define FIO_MEMORY_CACHE_SLOTS 4
#endif

#ifndef FIO_MEMORY_INITIALIZE_ALLOCATIONS
/**
 * Forces the allocator to zero out memory early and often, so allocations
 * return initialized memory (bytes are all zeros.
 *
 * This will make the realloc2 safe for use (all data not copied is zero).
 */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS                                      \
  FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT
#elif FIO_MEMORY_INITIALIZE_ALLOCATIONS
#undef FIO_MEMORY_INITIALIZE_ALLOCATIONS
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1
#else
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#endif

#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
/**
 * The number of blocks per system allocation.
 *
 * More blocks protect against fragmentation, but lower the maximum number that
 * can be allocated without reverting to mmap.
 *
 * Range: 0-4
 * Recommended: depends on object allocation sizes, usually 1 or 2.
 */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 2
#endif

#ifndef FIO_MEMORY_ENABLE_BIG_ALLOC
/**
 * Uses a whole system allocation to support bigger allocations.
 *
 * Could increase fragmentation costs.
 */
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
#endif

#ifndef FIO_MEMORY_ARENA_COUNT
/**
 * Memory arenas mitigate thread contention while using more memory.
 *
 * Note that at some point arenas are statistically irrelevant... except when
 * benchmarking contention in multi-core machines.
 *
 * Negative values will result in dynamic selection based on CPU core count.
 */
#define FIO_MEMORY_ARENA_COUNT -1
#endif

#ifndef FIO_MEMORY_ARENA_COUNT_FALLBACK
/*
 * Used when dynamic arena count calculations fail.
 *
 * NOTE: if FIO_MEMORY_ARENA_COUNT is negative, dynamic arena calculation is
 * performed using CPU core calculation.
 */
#define FIO_MEMORY_ARENA_COUNT_FALLBACK 8
#endif

#ifndef FIO_MEMORY_ARENA_COUNT_MAX
/*
 * Used when dynamic arena count calculations fail.
 *
 * NOTE: if FIO_MEMORY_ARENA_COUNT is negative, dynamic arena calculation is
 * performed using CPU core calculation.
 */
#define FIO_MEMORY_ARENA_COUNT_MAX 32
#endif

#ifndef FIO_MEMORY_WARMUP
#define FIO_MEMORY_WARMUP 0
#endif

#ifndef FIO_MEMORY_USE_PTHREAD_MUTEX
/*
 * If arena count isn't linked to the CPU count, threads might busy-spin.
 * It is better to slow wait than fast busy spin when the work in the lock is
 * longer... and system allocations are performed inside arena locks.
 */
#if FIO_MEMORY_ARENA_COUNT > 0
/** If true, uses a pthread mutex instead of a spinlock. */
#define FIO_MEMORY_USE_PTHREAD_MUTEX 1
#else
/** If true, uses a pthread mutex instead of a spinlock. */
#define FIO_MEMORY_USE_PTHREAD_MUTEX 0
#endif
#endif

#ifndef FIO_MEMORY_USE_FIO_MEMSET
/** If true, uses a facil.io custom implementation. */
#define FIO_MEMORY_USE_FIO_MEMSET 0
#endif

#ifndef FIO_MEMORY_USE_FIO_MEMCOPY
/** If true, uses a facil.io custom implementation. */
#define FIO_MEMORY_USE_FIO_MEMCOPY 0
#endif

#ifndef FIO_MEM_PAGE_SIZE_LOG
#define FIO_MEM_PAGE_SIZE_LOG 12 /* 4096 bytes per page */
#endif

#if !defined(FIO_MEM_PAGE_ALLOC) || !defined(FIO_MEM_PAGE_REALLOC) ||          \
    !defined(FIO_MEM_PAGE_FREE)
/**
 * The following MACROS, when all of them are defined, allow the memory
 * allocator to collect memory from the system using an alternative method.
 *
 * - FIO_MEM_PAGE_ALLOC(pages, alignment_log)
 *
 * - FIO_MEM_PAGE_REALLOC(ptr, old_pages, new_pages, alignment_log)
 *
 * - FIO_MEM_PAGE_FREE(ptr, pages) FIO_MEM_PAGE_FREE_def_func((ptr), (pages))
 *
 * Note that the alignment property for the allocated memory is essential and
 * may me quite large.
 */
#undef FIO_MEM_PAGE_ALLOC
#undef FIO_MEM_PAGE_REALLOC
#undef FIO_MEM_PAGE_FREE
#endif /* undefined FIO_MEM_PAGE_ALLOC... */

/* *****************************************************************************
Memory Allocation - configuration value - results and constants
***************************************************************************** */

/* Helper macros, don't change their values */
#undef FIO_MEMORY_BLOCKS_PER_ALLOCATION
#undef FIO_MEMORY_SYS_ALLOCATION_SIZE
#undef FIO_MEMORY_BLOCK_ALLOC_LIMIT

#if FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG < 0 ||                                \
    FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG > 5
#undef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 3
#endif

/** the number of allocation blocks per system allocation. */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION                                       \
  (1UL << FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG)

/** the total number of bytes consumed per system allocation. */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE                                         \
  (1UL << FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG)

/**
 * The maximum allocation size, after which a big/system allocation is used.
 */
#define FIO_MEMORY_BLOCK_ALLOC_LIMIT                                           \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE >> (FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG + 2))

#if FIO_MEMORY_ENABLE_BIG_ALLOC
/** the limit of a big allocation, if enabled */
#define FIO_MEMORY_BIG_ALLOC_LIMIT                                             \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE >>                                           \
   (FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG > 3                                   \
        ? 3                                                                    \
        : FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG))
#define FIO_MEMORY_ALLOC_LIMIT FIO_MEMORY_BIG_ALLOC_LIMIT
#else
#define FIO_MEMORY_ALLOC_LIMIT FIO_MEMORY_BLOCK_ALLOC_LIMIT
#endif

/* *****************************************************************************
Memory Allocation - configuration access - UNSTABLE API!!!
***************************************************************************** */

FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_sys_alloc_size)(void) {
  return FIO_MEMORY_SYS_ALLOCATION_SIZE;
}

FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_cache_slots)(void) {
  return FIO_MEMORY_CACHE_SLOTS;
}
FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_alignment)(void) {
  return FIO_MEMORY_ALIGN_SIZE;
}
FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_alignment_log)(void) {
  return FIO_MEMORY_ALIGN_LOG;
}

FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_alloc_limit)(void) {
  return (FIO_MEMORY_BLOCK_ALLOC_LIMIT > FIO_MEMORY_ALLOC_LIMIT)
             ? FIO_MEMORY_BLOCK_ALLOC_LIMIT
             : FIO_MEMORY_ALLOC_LIMIT;
}

FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_arena_alloc_limit)(void) {
  return FIO_MEMORY_BLOCK_ALLOC_LIMIT;
}

/* will realloc2 return junk data? */
FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, realloc_is_safe)(void) {
  return FIO_MEMORY_INITIALIZE_ALLOCATIONS;
}

/* Returns the calculated block size. */
SFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_block_size)(void);

/** Prints the allocator's data structure. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_state)(void);

/** Prints the settings used to define the allocator. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)(void);

/* *****************************************************************************
Temporarily (at least) set memory allocation macros to use this allocator
***************************************************************************** */
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_

#ifndef FIO_MALLOC_TMP_USE_SYSTEM
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  FIO_NAME(FIO_MEMORY_NAME, realloc2)((ptr), (new_size), (copy_len))
#define FIO_MEM_FREE_(ptr, size) FIO_NAME(FIO_MEMORY_NAME, free)((ptr))
#define FIO_MEM_REALLOC_IS_SAFE_ FIO_NAME(FIO_MEMORY_NAME, realloc_is_safe)()

#endif /* FIO_MALLOC_TMP_USE_SYSTEM */

/* *****************************************************************************





Memory Allocation - start implementation





***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE
/* internal workings start here */
/* *****************************************************************************
FIO_MEMORY_DISABLE - use the system allocator
***************************************************************************** */
#if defined(FIO_MEMORY_DISABLE)

SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, malloc)(size_t size) {
#if FIO_MEMORY_INITIALIZE_ALLOCATIONS
  return calloc(size, 1);
#else
  return malloc(size);
#endif
}
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME,
                                       calloc)(size_t size_per_unit,
                                               size_t unit_count) {
  return calloc(size_per_unit, unit_count);
}
SFUNC void FIO_NAME(FIO_MEMORY_NAME, free)(void *ptr) { free(ptr); }
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc)(void *ptr,
                                                             size_t new_size) {
  return realloc(ptr, new_size);
}
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc2)(void *ptr,
                                                              size_t new_size,
                                                              size_t copy_len) {
  return realloc(ptr, new_size);
  (void)copy_len;
}
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, mmap)(size_t size) {
  return calloc(size, 1);
}

SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_after_fork)(void) {}
/** Prints the allocator's data structure. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_state)(void) {}
/** Prints the settings used to define the allocator. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)(void) {}
SFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_block_size)(void) { return 0; }

#ifdef FIO_TEST_CSTL
SFUNC void FIO_NAME_TEST(FIO_NAME(stl, FIO_MEMORY_NAME), mem)(void) {
  fprintf(stderr, "* Custom memory allocator bypassed.\n");
}
#endif /* FIO_TEST_CSTL */

#else /* FIO_MEMORY_DISABLE */
/* *****************************************************************************







Helpers and System Memory Allocation




***************************************************************************** */
#ifndef H___FIO_MEM_INCLUDE_ONCE___H
#define H___FIO_MEM_INCLUDE_ONCE___H

#if FIO_HAVE_UNIX_TOOLS
#include <unistd.h>
#endif /* H___FIO_UNIX_TOOLS4STR_INCLUDED_H */

#define FIO_MEM_BYTES2PAGES(size)                                              \
  (((size) + ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)) >> (FIO_MEM_PAGE_SIZE_LOG))

/* *****************************************************************************
Aligned memory copying
***************************************************************************** */
#define FIO_MEMCOPY_FIO_IFUNC_ALIGNED(type, size)                              \
  FIO_IFUNC void fio___memcpy_##size##b(void *restrict dest_,                  \
                                        const void *restrict src_,             \
                                        size_t units) {                        \
    type *dest = (type *)dest_;                                                \
    type *src = (type *)src_;                                                  \
    while (units >= 16) {                                                      \
      dest[0] = src[0];                                                        \
      dest[1] = src[1];                                                        \
      dest[2] = src[2];                                                        \
      dest[3] = src[3];                                                        \
      dest[4] = src[4];                                                        \
      dest[5] = src[5];                                                        \
      dest[6] = src[6];                                                        \
      dest[7] = src[7];                                                        \
      dest[8] = src[8];                                                        \
      dest[9] = src[9];                                                        \
      dest[10] = src[10];                                                      \
      dest[11] = src[11];                                                      \
      dest[12] = src[12];                                                      \
      dest[13] = src[13];                                                      \
      dest[14] = src[14];                                                      \
      dest[15] = src[15];                                                      \
      dest += 16;                                                              \
      src += 16;                                                               \
      units -= 16;                                                             \
    }                                                                          \
    switch (units) {                                                           \
    case 15:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 14:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 13:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 12:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 11:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 10:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 9:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 8:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 7:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 6:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 5:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 4:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 3:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 2:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 1:                                                                    \
      *(dest++) = *(src++);                                                    \
    }                                                                          \
  }
FIO_MEMCOPY_FIO_IFUNC_ALIGNED(uint16_t, 2)
FIO_MEMCOPY_FIO_IFUNC_ALIGNED(uint32_t, 4)
FIO_MEMCOPY_FIO_IFUNC_ALIGNED(uint64_t, 8)
#undef FIO_MEMCOPY_FIO_IFUNC_ALIGNED

/** Copies 16 byte `units` of size_t aligned memory blocks */
FIO_IFUNC void fio___memcpy_aligned(void *dest_,
                                    const void *src_,
                                    size_t bytes) {
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF /* 64 bit size_t */
  fio___memcpy_8b(dest_, src_, bytes >> 3);
#elif SIZE_MAX == 0xFFFFFFFF       /* 32 bit size_t */
  fio___memcpy_4b(dest_, src_, bytes >> 2);
#else                              /* unknown... assume 16 bit? */
  fio___memcpy_2b(dest_, src_, bytes >> 1);
#endif                             /* SIZE_MAX */
}

/** a 16 byte aligned memset implementation. */
FIO_SFUNC void fio___memset_aligned(void *restrict dest_,
                                    uint64_t data,
                                    size_t bytes) {
  uint64_t *dest = (uint64_t *)dest_;
  bytes >>= 3;
  while (bytes >= 16) {
    dest[0] = data;
    dest[1] = data;
    dest[2] = data;
    dest[3] = data;
    dest[4] = data;
    dest[5] = data;
    dest[6] = data;
    dest[7] = data;
    dest[8] = data;
    dest[9] = data;
    dest[10] = data;
    dest[11] = data;
    dest[12] = data;
    dest[13] = data;
    dest[14] = data;
    dest[15] = data;
    dest += 16;
    bytes -= 16;
  }
  switch (bytes) {
  case 15:
    *(dest++) = data; /* fallthrough */
  case 14:
    *(dest++) = data; /* fallthrough */
  case 13:
    *(dest++) = data; /* fallthrough */
  case 12:
    *(dest++) = data; /* fallthrough */
  case 11:
    *(dest++) = data; /* fallthrough */
  case 10:
    *(dest++) = data; /* fallthrough */
  case 9:
    *(dest++) = data; /* fallthrough */
  case 8:
    *(dest++) = data; /* fallthrough */
  case 7:
    *(dest++) = data; /* fallthrough */
  case 6:
    *(dest++) = data; /* fallthrough */
  case 5:
    *(dest++) = data; /* fallthrough */
  case 4:
    *(dest++) = data; /* fallthrough */
  case 3:
    *(dest++) = data; /* fallthrough */
  case 2:
    *(dest++) = data; /* fallthrough */
  case 1:
    *(dest++) = data;
  }
}

/* *****************************************************************************
POSIX Allocaion
***************************************************************************** */
#if FIO_HAVE_UNIX_TOOLS || __has_include("sys/mman.h")
#include <sys/mman.h>

/* Mitigates MAP_ANONYMOUS not being defined on older versions of MacOS */
#if !defined(MAP_ANONYMOUS)
#if defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0
#endif /* defined(MAP_ANONYMOUS) */
#endif /* FIO_MEM_PAGE_ALLOC */

/* inform the compiler that the returned value is aligned on 16 byte marker */
#if __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#define FIO_PAGE_ALIGN                                                         \
  __attribute__((assume_aligned((1UL << FIO_MEM_PAGE_SIZE_LOG))))
#define FIO_PAGE_ALIGN_NEW                                                     \
  __attribute__((malloc, assume_aligned((1UL << FIO_MEM_PAGE_SIZE_LOG))))
#else
#define FIO_PAGE_ALIGN
#define FIO_PAGE_ALIGN_NEW
#endif /* (__clang__ || __GNUC__)... */

/*
 * allocates memory using `mmap`, but enforces alignment.
 */
FIO_SFUNC void *FIO_MEM_PAGE_ALLOC_def_func(size_t pages,
                                            uint8_t alignment_log) {
  void *result;
  static void *next_alloc = (void *)0x01;
  const size_t alignment_mask = (1ULL << alignment_log) - 1;
  const size_t alignment_size = (1ULL << alignment_log);
  pages <<= FIO_MEM_PAGE_SIZE_LOG;
  next_alloc =
      (void *)(((uintptr_t)next_alloc + alignment_mask) & alignment_mask);
/* hope for the best? */
#ifdef MAP_ALIGNED
  result = mmap(next_alloc,
                pages,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED(alignment_log),
                -1,
                0);
#else
  result = mmap(next_alloc,
                pages,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0);
#endif /* MAP_ALIGNED */
  if (result == MAP_FAILED)
    return (void *)NULL;
  if (((uintptr_t)result & alignment_mask)) {
    munmap(result, pages);
    result = mmap(NULL,
                  pages + alignment_size,
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  -1,
                  0);
    if (result == MAP_FAILED) {
      return (void *)NULL;
    }
    const uintptr_t offset =
        (alignment_size - ((uintptr_t)result & alignment_mask));
    if (offset) {
      munmap(result, offset);
      result = (void *)((uintptr_t)result + offset);
    }
    munmap((void *)((uintptr_t)result + pages), alignment_size - offset);
  }
  next_alloc = (void *)((uintptr_t)result + (pages << 2));
  return result;
}

/*
 * Re-allocates memory using `mmap`, enforcing alignment.
 */
FIO_SFUNC void *FIO_MEM_PAGE_REALLOC_def_func(void *mem,
                                              size_t prev_pages,
                                              size_t new_pages,
                                              uint8_t alignment_log) {
  const size_t prev_len = prev_pages << FIO_MEM_PAGE_SIZE_LOG;
  const size_t new_len = new_pages << FIO_MEM_PAGE_SIZE_LOG;
  if (new_len > prev_len) {
    void *result;
#if defined(__linux__)
    result = mremap(mem, prev_len, new_len, 0);
    if (result != MAP_FAILED)
      return result;
#endif
    result = mmap((void *)((uintptr_t)mem + prev_len),
                  new_len - prev_len,
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  -1,
                  0);
    if (result == (void *)((uintptr_t)mem + prev_len)) {
      result = mem;
    } else {
      /* copy and free */
      munmap(result, new_len - prev_len); /* free the failed attempt */
      result =
          FIO_MEM_PAGE_ALLOC_def_func(new_pages,
                                      alignment_log); /* allocate new memory */
      if (!result) {
        return (void *)NULL;
      }
      fio___memcpy_aligned(result, mem, prev_len); /* copy data */
      // memcpy(result, mem, prev_len);
      munmap(mem, prev_len); /* free original memory */
    }
    return result;
  }
  if (prev_len != new_len) /* remove dangling pages */
    munmap((void *)((uintptr_t)mem + new_len), prev_len - new_len);
  return mem;
}

/* frees memory using `munmap`. */
FIO_IFUNC void FIO_MEM_PAGE_FREE_def_func(void *mem, size_t pages) {
  munmap(mem, (pages << FIO_MEM_PAGE_SIZE_LOG));
}

/* *****************************************************************************
Non-POSIX allocaion - unsupported at this time
***************************************************************************** */
#else /* FIO_HAVE_UNIX_TOOLS */

FIO_IFUNC void *FIO_MEM_PAGE_ALLOC_def_func(size_t pages,
                                            uint8_t alignment_log) {
  // return aligned_alloc((pages << 12), (1UL << alignment_log));
  exit(-1);
  (void)pages;
  (void)alignment_log;
}

FIO_IFUNC void *FIO_MEM_PAGE_REALLOC_def_func(void *mem,
                                              size_t prev_pages,
                                              size_t new_pages,
                                              uint8_t alignment_log) {
  (void)prev_pages;
  (void)alignment_log;
  return realloc(mem, (new_pages << 12));
}

FIO_IFUNC void FIO_MEM_PAGE_FREE_def_func(void *mem, size_t pages) {
  free(mem);
  (void)pages;
}

#endif /* FIO_HAVE_UNIX_TOOLS */
/* *****************************************************************************
Overridable system allocation macros
***************************************************************************** */
#ifndef FIO_MEM_PAGE_ALLOC
#define FIO_MEM_PAGE_ALLOC(pages, alignment_log)                               \
  FIO_MEM_PAGE_ALLOC_def_func((pages), (alignment_log))
#define FIO_MEM_PAGE_REALLOC(ptr, old_pages, new_pages, alignment_log)         \
  FIO_MEM_PAGE_REALLOC_def_func((ptr),                                         \
                                (old_pages),                                   \
                                (new_pages),                                   \
                                (alignment_log))
#define FIO_MEM_PAGE_FREE(ptr, pages) FIO_MEM_PAGE_FREE_def_func((ptr), (pages))
#endif /* FIO_MEM_PAGE_ALLOC */

/* *****************************************************************************
Testing helpers
***************************************************************************** */
#ifdef FIO_TEST_CSTL

FIO_IFUNC void fio___memset_test_aligned(void *restrict dest_,
                                         uint64_t data,
                                         size_t bytes,
                                         const char *msg) {
  uint64_t *dest = (uint64_t *)dest_;
  size_t units = bytes >> 3;
  FIO_ASSERT(*(dest) = data,
             "%s memory data was overwritten (first 8 bytes)",
             msg);
  while (units >= 16) {
    FIO_ASSERT(dest[0] == data && dest[1] == data && dest[2] == data &&
                   dest[3] == data && dest[4] == data && dest[5] == data &&
                   dest[6] == data && dest[7] == data && dest[8] == data &&
                   dest[9] == data && dest[10] == data && dest[11] == data &&
                   dest[12] == data && dest[13] == data && dest[14] == data &&
                   dest[15] == data,
               "%s memory data was overwritten",
               msg);
    dest += 16;
    units -= 16;
  }
  switch (units) {
  case 15:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 14:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 13:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 12:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 11:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 10:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 9:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 8:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 7:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 6:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 5:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 4:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 3:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 2:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten",
               msg); /* fallthrough */
  case 1:
    FIO_ASSERT(*(dest++) = data,
               "%s memory data was overwritten (last 8 bytes)",
               msg);
  }
  (void)msg; /* in case FIO_ASSERT is disabled */
}
#endif /* FIO_TEST_CSTL */
/* *****************************************************************************











                  Memory allocation implementation starts here
                    helper function and setup are complete











***************************************************************************** */
#endif /* H___FIO_MEM_INCLUDE_ONCE___H */

/* *****************************************************************************
memset / memcpy selectors
***************************************************************************** */

#if FIO_MEMORY_USE_FIO_MEMSET
#define FIO___MEMSET fio___memset_aligned
#else
#define FIO___MEMSET memset
#endif /* FIO_MEMORY_USE_FIO_MEMSET */

#if FIO_MEMORY_USE_FIO_MEMCOPY
#define FIO___MEMCPY2 fio___memcpy_aligned
#else
#define FIO___MEMCPY2 FIO___MEMCPY
#endif /* FIO_MEMORY_USE_FIO_MEMCOPY */

/* *****************************************************************************
Lock type choice
***************************************************************************** */
#if FIO_MEMORY_USE_PTHREAD_MUTEX
#include "pthread.h"
#define FIO_MEMORY_LOCK_TYPE            pthread_mutex_t
#define FIO_MEMORY_LOCK_TYPE_INIT(lock) pthread_mutex_init(&(lock), NULL)
#define FIO_MEMORY_TRYLOCK(lock)        pthread_mutex_trylock(&(lock))
#define FIO_MEMORY_LOCK(lock)           pthread_mutex_lock(&(lock))
#define FIO_MEMORY_UNLOCK(lock)                                                \
  do {                                                                         \
    int tmp__ = pthread_mutex_unlock(&(lock));                                 \
    if (tmp__) {                                                               \
      FIO_LOG_ERROR("Couldn't free mutex! error (%d): %s",                     \
                    tmp__,                                                     \
                    strerror(tmp__));                                          \
    }                                                                          \
  } while (0)

#define FIO_MEMORY_LOCK_NAME "pthread_mutex"
#else
#define FIO_MEMORY_LOCK_TYPE            fio_lock_i
#define FIO_MEMORY_LOCK_TYPE_INIT(lock) ((lock) = FIO_LOCK_INIT)
#define FIO_MEMORY_TRYLOCK(lock)        fio_trylock(&(lock))
#define FIO_MEMORY_LOCK(lock)           fio_lock(&(lock))
#define FIO_MEMORY_UNLOCK(lock)         fio_unlock(&(lock))
#define FIO_MEMORY_LOCK_NAME            "facil.io spinlocks"
#endif

/* *****************************************************************************
Allocator debugging helpers
***************************************************************************** */

#if defined(DEBUG) || defined(FIO_LEAK_COUNTER)
/* maximum block allocation count. */
static size_t FIO_NAME(fio___,
                       FIO_NAME(FIO_MEMORY_NAME, state_chunk_count_max));
/* current block allocation count. */
static size_t FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count));

#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY SYS-ALLOC - retrieved %p from system", ptr);        \
    fio_atomic_add(                                                            \
        &FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)),       \
        1);                                                                    \
    if (FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)) >       \
        FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count_max)))    \
      FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count_max)) =     \
          FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count));      \
  } while (0)
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)                                          \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY SYS-DEALLOC- returned %p to system", ptr);          \
    fio_atomic_sub_fetch(                                                      \
        &FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)),       \
        1);                                                                    \
  } while (0)
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-DEALLOC placed %p in cache", ptr);            \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)                                       \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-ALLOC retrieved %p from cache", ptr);         \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_DIRTY(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY MARK-DIRTY placed %p in dirty list", ptr);          \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_UNDIRTY(ptr)                                       \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY UNMARK-DIRTY retrieved %p from dirty list", ptr);   \
  } while (0);
#define FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(ptr, blk)                            \
  if (0)                                                                       \
    do {                                                                       \
      FIO_LOG_DEBUG2("MEMORY chunk %p block %zu reset in lock",                \
                     ptr,                                                      \
                     (size_t)blk);                                             \
    } while (0);

#define FIO_MEMORY_ON_BIG_BLOCK_SET(ptr)                                       \
  if (1)                                                                       \
    do {                                                                       \
      FIO_LOG_DEBUG2("MEMORY chunk %p used as big-block", ptr);                \
    } while (0);

#define FIO_MEMORY_ON_BIG_BLOCK_UNSET(ptr)                                     \
  if (1)                                                                       \
    do {                                                                       \
      FIO_LOG_DEBUG2("MEMORY chunk %p no longer used as big-block", ptr);      \
    } while (0);

#define FIO_MEMORY_PRINT_STATS()                                               \
  FIO_LOG_INFO(                                                                \
      "(" FIO_MACRO2STR(FIO_NAME(                                              \
          FIO_MEMORY_NAME,                                                     \
          malloc)) "):\n          "                                            \
                   "Total memory chunks allocated before cleanup %zu\n"        \
                   "          Maximum memory blocks allocated at a single "    \
                   "time %zu",                                                 \
      FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)),          \
      FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count_max)))
#define FIO_MEMORY_PRINT_STATS_END()                                           \
  do {                                                                         \
    if (FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count))) {      \
      FIO_LOG_ERROR(                                                           \
          "(" FIO_MACRO2STR(                                                   \
              FIO_NAME(FIO_MEMORY_NAME,                                        \
                       malloc)) "):\n          "                               \
                                "Total memory chunks allocated "               \
                                "after cleanup (POSSIBLE LEAKS): %zu\n",       \
          FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)));     \
    }                                                                          \
  } while (0)
#else /* defined(DEBUG) || defined(FIO_LEAK_COUNTER) */
#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)
#define FIO_MEMORY_ON_CHUNK_DIRTY(ptr)
#define FIO_MEMORY_ON_CHUNK_UNDIRTY(ptr)
#define FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(ptr, blk)
#define FIO_MEMORY_ON_BIG_BLOCK_SET(ptr)
#define FIO_MEMORY_ON_BIG_BLOCK_UNSET(ptr)
#define FIO_MEMORY_PRINT_STATS()
#define FIO_MEMORY_PRINT_STATS_END()
#endif /* defined(DEBUG) || defined(FIO_LEAK_COUNTER) */

/* *****************************************************************************






Memory chunk headers and block data (in chunk header)






***************************************************************************** */

/* *****************************************************************************
Chunk and Block data / header
***************************************************************************** */

typedef struct {
  volatile int32_t ref;
  volatile int32_t pos;
} FIO_NAME(FIO_MEMORY_NAME, __mem_block_s);

typedef struct {
  /* the head of the chunk... node->next says a lot */
  uint32_t marker;
  volatile int32_t ref;
  FIO_NAME(FIO_MEMORY_NAME, __mem_block_s)
  blocks[FIO_MEMORY_BLOCKS_PER_ALLOCATION];
} FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s);

#if FIO_MEMORY_ENABLE_BIG_ALLOC
/* big-blocks consumes a chunk, sizeof header MUST be <= chunk header */
typedef struct {
  /* marker and ref MUST overlay chunk header */
  uint32_t marker;
  volatile int32_t ref;
  volatile int32_t pos;
} FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s);
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

/* *****************************************************************************
Arena type
***************************************************************************** */
typedef struct {
  void *block;
  int32_t last_pos;
  FIO_MEMORY_LOCK_TYPE lock;
} FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s);

/* *****************************************************************************
Allocator State
***************************************************************************** */

typedef struct FIO_NAME(FIO_MEMORY_NAME, __mem_state_s)
    FIO_NAME(FIO_MEMORY_NAME, __mem_state_s);

static struct FIO_NAME(FIO_MEMORY_NAME, __mem_state_s) {
#if FIO_MEMORY_CACHE_SLOTS
  /** cache array container for available memory chunks */
  struct {
    /* chunk slot array */
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * a[FIO_MEMORY_CACHE_SLOTS];
    size_t pos;
  } cache;
#endif /* FIO_MEMORY_CACHE_SLOTS */

#if FIO_MEMORY_ENABLE_BIG_ALLOC
  /** a block for big allocations, shared (no arena) */
  FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) * big_block;
  int32_t big_last_pos;
  /** big allocation lock */
  FIO_MEMORY_LOCK_TYPE big_lock;
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
  /** main memory state lock */
  FIO_MEMORY_LOCK_TYPE lock;
  /** free list for available blocks */
  FIO_LIST_HEAD blocks;
  /** the arena count for the allocator */
  size_t arena_count;
  FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s) arena[];
} * FIO_NAME(FIO_MEMORY_NAME, __mem_state);

/* *****************************************************************************
Arena assignment
***************************************************************************** */

/* SublimeText marker */
void fio___mem_arena_unlock___(void);
/** Unlocks the thread's arena. */
FIO_SFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s) * a) {
  FIO_ASSERT_DEBUG(a, "unlocking a NULL arena?!");
  FIO_MEMORY_UNLOCK(a->lock);
}

/* SublimeText marker */
void fio___mem_arena_lock___(void);
/** Locks and returns the thread's arena. */
FIO_SFUNC FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s) *
    FIO_NAME(FIO_MEMORY_NAME, __mem_arena_lock)(void) {
#if FIO_MEMORY_ARENA_COUNT == 1
  FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[0].lock);
  return FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena;

#else /* FIO_MEMORY_ARENA_COUNT != 1 */

#if defined(DEBUG) && FIO_MEMORY_ARENA_COUNT > 0 && !defined(FIO_TEST_CSTL)
  static size_t warning_printed = 0;
#endif
  /** thread arena value */
  static __thread size_t FIO_NAME(FIO_MEMORY_NAME, __mem_arena_var);
  for (;;) {
    /* rotate all arenas to find one that's available */
    for (size_t i = 0; i < FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
         ++i) {
      /* first attempt is the last used arena, then cycle with offset */
      size_t index = i + FIO_NAME(FIO_MEMORY_NAME, __mem_arena_var);
      if (index >= FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count)
        index = 0;

      if (FIO_MEMORY_TRYLOCK(
              FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[index].lock))
        continue;
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_var) = index;
      return (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena + index);
    }
#if defined(DEBUG) && FIO_MEMORY_ARENA_COUNT > 0 && !defined(FIO_TEST_CSTL)
    if (!warning_printed)
      FIO_LOG_WARNING(FIO_MACRO2STR(
          FIO_NAME(FIO_MEMORY_NAME,
                   malloc)) " high arena contention.\n"
                            "          Consider recompiling with more arenas.");
    warning_printed = 1;
#endif /* DEBUG */
#if FIO_MEMORY_USE_PTHREAD_MUTEX
    /* slow wait for last arena used by the thread */
    FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)
                        ->arena[FIO_NAME(FIO_MEMORY_NAME, __mem_arena_var)]
                        .lock);
    return FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena +
           FIO_NAME(FIO_MEMORY_NAME, __mem_arena_var);
#else
    FIO_THREAD_RESCHEDULE();
#endif /* FIO_MEMORY_USE_PTHREAD_MUTEX */
  }
#endif /* FIO_MEMORY_ARENA_COUNT != 1 */
}

/* *****************************************************************************
Converting between chunk & block data to pointers (and back)
***************************************************************************** */

#define FIO_MEMORY_HEADER_SIZE                                                 \
  ((sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)) +                         \
    (FIO_MEMORY_ALIGN_SIZE - 1)) &                                             \
   (~(FIO_MEMORY_ALIGN_SIZE - 1)))

#define FIO_MEMORY_BLOCK_SIZE                                                  \
  (((FIO_MEMORY_SYS_ALLOCATION_SIZE - FIO_MEMORY_HEADER_SIZE) /                \
    FIO_MEMORY_BLOCKS_PER_ALLOCATION) &                                        \
   (~(FIO_MEMORY_ALIGN_SIZE - 1)))

#define FIO_MEMORY_UNITS_PER_BLOCK                                             \
  (FIO_MEMORY_BLOCK_SIZE / FIO_MEMORY_ALIGN_SIZE)

/* SublimeText marker */
void fio___mem_chunk2ptr___(void);
/** returns a pointer within a chunk, given it's block and offset value. */
FIO_IFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c,
    size_t block,
    size_t offset) {
  return (void *)(((uintptr_t)(c) + FIO_MEMORY_HEADER_SIZE) +
                  (block * FIO_MEMORY_BLOCK_SIZE) +
                  (offset << FIO_MEMORY_ALIGN_LOG));
}

/* SublimeText marker */
void fio___mem_ptr2chunk___(void);
/** returns a chunk given a pointer. */
FIO_IFUNC FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *
    FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(void *p) {
  return FIO_PTR_MATH_RMASK(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s),
                            p,
                            FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
}

/* SublimeText marker */
void fio___mem_ptr2index___(void);
/** returns a pointer's block index within it's chunk. */
FIO_IFUNC size_t FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c,
    void *p) {
  FIO_ASSERT_DEBUG(c == FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(p),
                   "chunk-pointer offset argument error");
  size_t i =
      (size_t)FIO_PTR_MATH_LMASK(void, p, FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  i -= FIO_MEMORY_HEADER_SIZE;
  i /= FIO_MEMORY_BLOCK_SIZE;
  return i;
  (void)c;
}

/* *****************************************************************************
Allocator State Initialization & Cleanup
***************************************************************************** */
#define FIO_MEMORY_STATE_SIZE(arean_count)                                     \
  FIO_MEM_BYTES2PAGES(                                                         \
      (sizeof(*FIO_NAME(FIO_MEMORY_NAME, __mem_state)) +                       \
       (sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s)) * (arean_count))))

/* function declerations for functions called during cleanup */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c);
FIO_IFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)(void);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(void *ptr);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)(void *ptr);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c);

/* SublimeText marker */
void fio___mem_state_cleanup___(void);
FIO_SFUNC __attribute__((destructor)) void FIO_NAME(FIO_MEMORY_NAME,
                                                    __mem_state_cleanup)(void) {
  if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state))
    return;

#if DEBUG
  FIO_LOG_INFO("starting facil.io memory allocator cleanup for " FIO_MACRO2STR(
      FIO_NAME(FIO_MEMORY_NAME, malloc)) ".");
#endif /* DEBUG */
  FIO_MEMORY_PRINT_STATS();
  /* free arena blocks */
  for (size_t i = 0; i < FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
       ++i) {
    if (FIO_MEMORY_TRYLOCK(
            FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].lock)) {
      FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].lock);
      FIO_LOG_ERROR(FIO_MACRO2STR(
          FIO_NAME(FIO_MEMORY_NAME,
                   malloc)) "cleanup called while some arenas are in use!");
    }
    FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].lock);
    FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)
    (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block = NULL;
    FIO_MEMORY_LOCK_TYPE_INIT(
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].lock);
  }

#if FIO_MEMORY_ENABLE_BIG_ALLOC
  /* cleanup big-alloc chunk */
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block) {
    if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block->ref > 1) {
      FIO_LOG_WARNING("(" FIO_MACRO2STR(FIO_NAME(
          FIO_MEMORY_NAME,
          malloc)) ") active big-block reference count error at %p\n"
                   "          Possible memory leaks for big-block allocation.");
    }
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)
    (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block = NULL;
    FIO_MEMORY_LOCK_TYPE_INIT(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
  }
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

#if FIO_MEMORY_CACHE_SLOTS
  /* deallocate all chunks in the cache */
  while (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos) {
    const size_t pos = --FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos;
    FIO_MEMORY_ON_CHUNK_UNCACHE(
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.a[pos]);
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)
    (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.a[pos]);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.a[pos] = NULL;
  }
#endif /* FIO_MEMORY_CACHE_SLOTS */

  /* report any blocks in the allocation list - even if not in DEBUG mode */
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.next !=
      &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks) {
    struct t_s {
      FIO_LIST_NODE node;
    };
    void *last_chunk = NULL;
    FIO_LOG_WARNING("(" FIO_MACRO2STR(
        FIO_NAME(FIO_MEMORY_NAME,
                 malloc)) ") blocks left after cleanup - memory leaks?");
    FIO_LIST_EACH(struct t_s,
                  node,
                  &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks,
                  pos) {
      if (last_chunk == (void *)FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(pos))
        continue;
      last_chunk = (void *)FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(pos);
      FIO_LOG_WARNING(
          "(" FIO_MACRO2STR(FIO_NAME(FIO_MEMORY_NAME,
                                     malloc)) ") leaked block(s) for chunk %p",
          (void *)pos,
          last_chunk);
    }
  }

  /* dealloc the state machine */
  const size_t s = FIO_MEMORY_STATE_SIZE(
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count);
  FIO_MEM_PAGE_FREE(FIO_NAME(FIO_MEMORY_NAME, __mem_state), s);
  FIO_NAME(FIO_MEMORY_NAME, __mem_state) =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_state_s *))NULL;

  FIO_MEMORY_PRINT_STATS_END();
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("finished facil.io memory allocator cleanup for " FIO_MACRO2STR(
      FIO_NAME(FIO_MEMORY_NAME, malloc)) ".");
#endif /* DEBUG */
}

/* initializes (allocates) the arenas and state machine */
FIO_SFUNC void __attribute__((constructor))
FIO_NAME(FIO_MEMORY_NAME, __mem_state_setup)(void) {
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state))
    return;
  /* allocate the state machine */
  {
#if FIO_MEMORY_ARENA_COUNT > 0
    size_t const arean_count = FIO_MEMORY_ARENA_COUNT;
#else
    size_t arean_count = FIO_MEMORY_ARENA_COUNT_FALLBACK;
#ifdef _SC_NPROCESSORS_ONLN
    arean_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (arean_count == (size_t)-1UL)
      arean_count = FIO_MEMORY_ARENA_COUNT_FALLBACK;
#else
#warning Dynamic CPU core count is unavailable - assuming FIO_MEMORY_ARENA_COUNT_FALLBACK cores.
#endif /* _SC_NPROCESSORS_ONLN */

    if (arean_count >= FIO_MEMORY_ARENA_COUNT_MAX)
      arean_count = FIO_MEMORY_ARENA_COUNT_MAX;

#endif /* FIO_MEMORY_ARENA_COUNT > 0 */

    const size_t s = FIO_MEMORY_STATE_SIZE(arean_count);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state) =
        (FIO_NAME(FIO_MEMORY_NAME, __mem_state_s *))FIO_MEM_PAGE_ALLOC(s, 0);
    FIO_ASSERT_ALLOC(FIO_NAME(FIO_MEMORY_NAME, __mem_state));
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count = arean_count;
  }
  FIO_LIST_INIT(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks);
  FIO_NAME(FIO_MEMORY_NAME, malloc_after_fork)();

#if defined(FIO_MEMORY_WARMUP) && FIO_MEMORY_WARMUP
  for (size_t i = 0; i < (size_t)FIO_MEMORY_WARMUP &&
                     i < FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
       ++i) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block =
        FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)();
  }
#endif
#ifdef DEBUG
  FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)();
#endif /* DEBUG */
  (void)FIO_NAME(FIO_MEMORY_NAME, malloc_print_state);
  (void)FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings);
}

/* SublimeText marker */
void fio_after_fork___(void);
/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_after_fork)(void) {
  if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_state_setup)();
    return;
  }
  FIO_LOG_DEBUG2("MEMORY reinitializeing " FIO_MACRO2STR(
      FIO_NAME(FIO_MEMORY_NAME, malloc)) " state");
  FIO_MEMORY_LOCK_TYPE_INIT(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
#if FIO_MEMORY_ENABLE_BIG_ALLOC
  FIO_MEMORY_LOCK_TYPE_INIT(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
  for (size_t i = 0; i < FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
       ++i) {
    FIO_MEMORY_LOCK_TYPE_INIT(
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].lock);
  }
}

/* *****************************************************************************
Memory Allocation - state printing (debug helper)
***************************************************************************** */

/* SublimeText marker */
void fio_malloc_print_state___(void);
/** Prints the allocator's data structure. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_state)(void) {
  fprintf(
      stderr,
      FIO_MACRO2STR(FIO_NAME(FIO_MEMORY_NAME, malloc)) " allocator state:\n");
  for (size_t i = 0; i < FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
       ++i) {
    fprintf(stderr,
            "\t* arena[%zu] block: %p\n",
            i,
            (void *)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block);
    if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
          FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(
              FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block);
      size_t b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(
          c,
          FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[i].block);
      fprintf(stderr, "\t\tchunk-ref: %zu (%p)\n", (size_t)c->ref, (void *)c);
      fprintf(stderr,
              "\t\t- block[%zu]-ref: %zu\n"
              "\t\t- block[%zu]-pos: %zu\n",
              b,
              (size_t)c->blocks[b].ref,
              b,
              (size_t)c->blocks[b].pos);
    }
  }
#if FIO_MEMORY_ENABLE_BIG_ALLOC
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block) {
    fprintf(stderr, "\t---big allocations---\n");
    fprintf(stderr,
            "\t* big-block: %p\n"
            "\t\t ref: %zu\n"
            "\t\t pos: %zu\n",
            (void *)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block,
            (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block->ref,
            (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block->pos);
  } else {
    fprintf(stderr,
            "\t---big allocations---\n"
            "\t* big-block: NULL\n");
  }

#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

#if FIO_MEMORY_CACHE_SLOTS
  fprintf(stderr, "\t---caches---\n");
  for (size_t i = 0; i < FIO_MEMORY_CACHE_SLOTS; ++i) {
    fprintf(stderr,
            "\t* cache[%zu] chunk: %p\n",
            i,
            (void *)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.a[i]);
  }
#endif /* FIO_MEMORY_CACHE_SLOTS */
}

/* *****************************************************************************
chunk allocation / deallocation
***************************************************************************** */

/* SublimeText marker */
void fio___mem_chunk_dealloc___(void);
/* returns memory to system */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c) {
  if (!c)
    return;
  FIO_MEM_PAGE_FREE(((void *)c),
                    (FIO_MEMORY_SYS_ALLOCATION_SIZE >> FIO_MEM_PAGE_SIZE_LOG));
  FIO_MEMORY_ON_CHUNK_FREE(c);
}

FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c) {
  /* should we free the chunk? */
  if (!c || fio_atomic_sub_fetch(&c->ref, 1))
    return;

  FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  /* reference could have been added while waiting for the lock */
  if (c->ref)
    goto in_use;

  /* remove all blocks from the block allocation list */
  for (size_t b = 0; b < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++b) {
    FIO_LIST_NODE *n =
        (FIO_LIST_NODE *)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0);
    if (n->prev && n->next) {
      FIO_LIST_REMOVE(n);
    }
  }
#if FIO_MEMORY_CACHE_SLOTS
  /* place in cache...? */
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos <
      FIO_MEMORY_CACHE_SLOTS) {
    FIO_MEMORY_ON_CHUNK_CACHE(c);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)
        ->cache.a[FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos++] = c;
    c = NULL;
  }
#endif /* FIO_MEMORY_CACHE_SLOTS */

  FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);

  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)(c);
  return;

in_use:
  FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  return;
}

/* SublimeText marker */
void fio___mem_chunk_new___(void);
/* UNSAFE! returns a clean chunk (cache / allocation). */
FIO_IFUNC FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_new)(const size_t needs_lock) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c = NULL;
#if FIO_MEMORY_CACHE_SLOTS
  /* cache allocation */
  if (needs_lock) {
    FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  }
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos) {
    c = FIO_NAME(FIO_MEMORY_NAME, __mem_state)
            ->cache.a[--FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos];
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)
        ->cache.a[FIO_NAME(FIO_MEMORY_NAME, __mem_state)->cache.pos] = NULL;
  }
  if (needs_lock) {
    FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  }
  if (c) {
    FIO_MEMORY_ON_CHUNK_UNCACHE(c);
    c->ref = 1;
    return c;
  }
#endif /* FIO_MEMORY_CACHE_SLOTS */

  /* system allocation */
  c = (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)FIO_MEM_PAGE_ALLOC(
      (FIO_MEMORY_SYS_ALLOCATION_SIZE >> FIO_MEM_PAGE_SIZE_LOG),
      FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);

  if (!c)
    return c;
  FIO_MEMORY_ON_CHUNK_ALLOC(c);
  c->ref = 1;
  return c;
  (void)needs_lock; /* in case it isn't used */
}

/* *****************************************************************************
block allocation / deallocation
***************************************************************************** */

FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c,
    size_t b) {
#if !FIO_MEMORY_INITIALIZE_ALLOCATIONS
  /** only reset a block't free-list header */
  FIO___MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
               0,
               sizeof(FIO_LIST_NODE));
#else
  if (c->blocks[b].pos >= (int32_t)(FIO_MEMORY_UNITS_PER_BLOCK - 4)) {
    FIO___MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
                 0,
                 FIO_MEMORY_BLOCK_SIZE);
  } else {
    FIO___MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
                 0,
                 (((size_t)c->blocks[b].pos) << FIO_MEMORY_ALIGN_LOG));
  }
#endif /*FIO_MEMORY_INITIALIZE_ALLOCATIONS*/
  c->blocks[b].pos = 0;
}

/* SublimeText marker */
void fio___mem_block_free___(void);
/** frees a block / decreases it's reference count */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(void *p) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
      FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(p);
  size_t b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, p);
  if (!c || fio_atomic_sub_fetch(&c->blocks[b].ref, 1))
    return;

  /* reset memory */
  FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)(c, b);

  /* place in free list */
  if (c->ref > 1) {
    FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
    FIO_LIST_NODE *n =
        (FIO_LIST_NODE *)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0);
    FIO_LIST_PUSH(&FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks, n);
    FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  }
  /* free chunk reference */
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)(c);
}

/* SublimeText marker */
void fio___mem_block_new___(void);
/** returns a new block with a reference count of 1 */
FIO_IFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)(void) {
  void *p = NULL;
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c = NULL;
  size_t b;

  FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);

  /* try to collect from list */
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.next !=
      &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks) {
    FIO_LIST_NODE *n = FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.prev;
    FIO_LIST_REMOVE(n);
    p = (void *)n;
    c = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(p);
    fio_atomic_add(&c->ref, 1); /* update chunk reference, needs atomic op. */
    goto done;
  }

  /* allocate from cache / system (sets chunk reference to 1) */
  c = FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_new)(0);
  if (!c)
    goto done;

  /* use the first block in the chunk as the new block */
  p = FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, 0, 0);

  /* place the rest of the blocks in the block allocation list */
  for (b = 1; b < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++b) {
    FIO_LIST_NODE *n =
        (FIO_LIST_NODE *)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0);
    FIO_LIST_PUSH(&FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks, n);
  }

done:
  FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  if (!p)
    return p;
  b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, p);
  /* update block reference and alloccation position */
  c->blocks[b].ref = 1;
  c->blocks[b].pos = 0;
  return p;
}

/* *****************************************************************************
Small allocation internal API
***************************************************************************** */

/* SublimeText marker */
void fio___mem_slice_new___(void);
/** slice a block to allocate a set number of bytes. */
FIO_SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME,
                                           __mem_slice_new)(size_t bytes,
                                                            void *is_realloc) {
  int32_t last_pos = 0;
  void *p = NULL;
  bytes = (bytes + ((1UL << FIO_MEMORY_ALIGN_LOG) - 1)) >> FIO_MEMORY_ALIGN_LOG;
  FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s) *a =
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_lock)();

  if (!a->block)
    a->block = FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)();
  else if (is_realloc)
    last_pos = a->last_pos;
  for (;;) {
    if (!a->block)
      goto no_mem;
    void *const block = a->block;

    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *const c =
        FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(block);
    const size_t b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, block);

    /* if we are the only thread holding a reference to this block... reset. */
    if (c->blocks[b].ref == 1 && c->blocks[b].pos) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)(c, b);
      FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(c, b);
    }

    /* a lucky realloc? */
    if (last_pos && is_realloc == FIO_NAME(FIO_MEMORY_NAME,
                                           __mem_chunk2ptr)(c, b, last_pos)) {
      c->blocks[b].pos = bytes + last_pos;
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
      return is_realloc;
    }

    /* enough space? allocate */
    if (c->blocks[b].pos + bytes < FIO_MEMORY_UNITS_PER_BLOCK) {
      p = FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, c->blocks[b].pos);
      fio_atomic_add(&c->blocks[b].ref, 1); /* keep inside lock for reset */
      a->last_pos = c->blocks[b].pos;
      c->blocks[b].pos += bytes;
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
      return p;
    }

    /*
     * allocate a new block before freeing the existing block
     * this prevents the last chunk from deallocating and reallocating
     */
    a->block = FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)();
    last_pos = 0;
    /* release the reference held by the allocator */
    FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(block);
  }

no_mem:
  FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
  return p;
}

/* SublimeText marker */
void fio_____mem_slice_free___(void);
/** slice a block to allocate a set number of bytes. */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_slice_free)(void *p) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(p);
}

/* *****************************************************************************
big block allocation / deallocation
***************************************************************************** */
#if FIO_MEMORY_ENABLE_BIG_ALLOC

#define FIO_MEMORY_BIG_BLOCK_MARKER ((~(uint32_t)0) << 2)
#define FIO_MEMORY_BIG_BLOCK_HEADER_SIZE                                       \
  (((sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s)) +                    \
     ((FIO_MEMORY_ALIGN_SIZE - 1))) &                                          \
    ((~(0UL)) << FIO_MEMORY_ALIGN_LOG)))

#define FIO_MEMORY_BIG_BLOCK_SIZE                                              \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE - FIO_MEMORY_BIG_BLOCK_HEADER_SIZE)

#define FIO_MEMORY_UNITS_PER_BIG_BLOCK                                         \
  (FIO_MEMORY_BIG_BLOCK_SIZE / FIO_MEMORY_ALIGN_SIZE)

/* SublimeText marker */
void fio___mem_big_block__reset_memory___(void);
/** zeros out a big-block's memory, keeping it's reference count at 1. */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_big_block__reset_memory)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) * b) {

#if !FIO_MEMORY_INITIALIZE_ALLOCATIONS
  /* reset chunk header, which is always bigger than big_block header*/
  FIO___MEMSET((void *)b, 0, sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)));
#else

  /* zero out memory */
  if (b->pos >= (int32_t)(FIO_MEMORY_UNITS_PER_BIG_BLOCK - 10)) {
    FIO___MEMSET((void *)b, 0, FIO_MEMORY_SYS_ALLOCATION_SIZE);
  } else {
    FIO___MEMSET((void *)b,
                 0,
                 (((size_t)b->pos << FIO_MEMORY_ALIGN_LOG) +
                  FIO_MEMORY_BIG_BLOCK_HEADER_SIZE));
  }
#endif /* FIO_MEMORY_INITIALIZE_ALLOCATIONS */
  b->ref = 1;
}

/* SublimeText marker */
void fio___mem_big_block_free___(void);
/** frees a block / decreases it's reference count */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)(void *p) {
  // FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)      ;
  FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *b =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *)
          FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(p);
  /* should we free the block? */
  if (!b || fio_atomic_sub_fetch(&b->ref, 1))
    return;
  FIO_MEMORY_ON_BIG_BLOCK_UNSET(b);

  /* zero out memory */
  FIO_NAME(FIO_MEMORY_NAME, __mem_big_block__reset_memory)(b);
/* zero out possible block memory (if required) */
#if !FIO_MEMORY_INITIALIZE_ALLOCATIONS
  for (size_t i = 0; i < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++i) {
    FIO_LIST_NODE *n = (FIO_LIST_NODE *)FIO_NAME(
        FIO_MEMORY_NAME,
        __mem_chunk2ptr)((FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)b, i, 0);
    n->prev = n->next = NULL;
  }
#endif /* FIO_MEMORY_INITIALIZE_ALLOCATIONS */
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)
  ((FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)b);
}

/* SublimeText marker */
void fio___mem_big_block_new___(void);
/** returns a new block with a reference count of 1 */
FIO_IFUNC FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_new)(void) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *b =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *)
          FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_new)(1);
  if (!b)
    return b;
  b->marker = FIO_MEMORY_BIG_BLOCK_MARKER;
  b->ref = 1;
  b->pos = 0;
  FIO_MEMORY_ON_BIG_BLOCK_SET(b);
  return b;
}

/* *****************************************************************************
Big allocation internal API
***************************************************************************** */

/* SublimeText marker */
void fio___mem_big2ptr___(void);
/** returns a pointer within a chunk, given it's block and offset value. */
FIO_IFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_big2ptr)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) * b,
    size_t offset) {
  return (void *)(((uintptr_t)(b) + FIO_MEMORY_BIG_BLOCK_HEADER_SIZE) +
                  (offset << FIO_MEMORY_ALIGN_LOG));
}

/* SublimeText marker */
void fio___mem_big_slice_new___(void);
FIO_SFUNC void *FIO_MEM_ALIGN_NEW
FIO_NAME(FIO_MEMORY_NAME, __mem_big_slice_new)(size_t bytes, void *is_realloc) {
  int32_t last_pos = 0;
  void *p = NULL;
  bytes = (bytes + ((1UL << FIO_MEMORY_ALIGN_LOG) - 1)) >> FIO_MEMORY_ALIGN_LOG;
  for (;;) {
    FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
    if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block)
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block =
          FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_new)();
    else if (is_realloc)
      last_pos = FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos;
    if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block)
      goto done;
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *b =
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block;

    /* are we the only thread holding a reference to this block... reset? */
    if (b->ref == 1 && b->pos) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_big_block__reset_memory)(b);
      FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(b, 0);
      b->marker = FIO_MEMORY_BIG_BLOCK_MARKER;
    }

    /* a lucky realloc? */
    if (last_pos &&
        is_realloc == FIO_NAME(FIO_MEMORY_NAME, __mem_big2ptr)(b, last_pos)) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = bytes + last_pos;
      FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
      return is_realloc;
    }

    /* enough space? */
    if (b->pos + bytes < FIO_MEMORY_UNITS_PER_BIG_BLOCK) {
      p = FIO_NAME(FIO_MEMORY_NAME, __mem_big2ptr)(b, b->pos);
      fio_atomic_add(&b->ref, 1); /* keep inside lock to enable reset */
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = b->pos;
      b->pos += bytes;
      FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
      return p;
    }

    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block = NULL;
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = 0;
    FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)(b);
  }
done:
  FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
  return p;
}

/* SublimeText marker */
void fio_____mem_big_slice_free___(void);
/** slice a block to allocate a set number of bytes. */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_big_slice_free)(void *p) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)(p);
}

#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
/* *****************************************************************************
Memory Allocation - malloc(0) pointer
***************************************************************************** */

static long double FIO_NAME(
    FIO_MEMORY_NAME,
    malloc_zero)[((1UL << (FIO_MEMORY_ALIGN_LOG)) / sizeof(long double)) + 1];

#define FIO_MEMORY_MALLOC_ZERO_POINTER                                         \
  ((void *)(((uintptr_t)FIO_NAME(FIO_MEMORY_NAME, malloc_zero) +               \
             (FIO_MEMORY_ALIGN_SIZE - 1)) &                                    \
            ((~(uintptr_t)0) << FIO_MEMORY_ALIGN_LOG)))

/* *****************************************************************************
Memory Allocation - API implementation - debugging and info
***************************************************************************** */

/* SublimeText marker */
void fio_malloc_block_size___(void);
/* public API obligation */
SFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_block_size)(void) {
  return FIO_MEMORY_BLOCK_SIZE;
}

void fio_malloc_arenas___(void);
SFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_arenas)(void) {
  return FIO_NAME(FIO_MEMORY_NAME, __mem_state)
             ? FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count
             : 0;
}

SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)(void) {
  // FIO_LOG_DEBUG2(
  fprintf(
      stderr,
      "Custom memory allocator " FIO_MACRO2STR(FIO_NAME(
          FIO_MEMORY_NAME,
          malloc)) " initialized with:\n"
                   "\t* system allocation size:                   %zu bytes\n"
                   "\t* system allocation arenas:                 %zu arenas\n"
                   "\t* cached allocation limit:                  %zu units\n"
                   "\t* system allocation overhead (theoretical): %zu bytes\n"
                   "\t* system allocation overhead (actual):      %zu bytes\n"
                   "\t* memory block size:                        %zu bytes\n"
                   "\t* allocation units per block:               %zu units\n"
                   "\t* arena per-allocation limit:               %zu bytes\n"
                   "\t* local per-allocation limit (before mmap): %zu bytes\n"
                   "\t* malloc(0) pointer:                        %p\n"
                   "\t* always initializes memory  (zero-out):    %s\n"
                   "\t* " FIO_MEMORY_LOCK_NAME " locking system\n",
      (size_t)FIO_MEMORY_SYS_ALLOCATION_SIZE,
      (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count,
      (size_t)FIO_MEMORY_CACHE_SLOTS,
      (size_t)FIO_MEMORY_HEADER_SIZE,
      (size_t)FIO_MEMORY_SYS_ALLOCATION_SIZE % (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_UNITS_PER_BLOCK,
      (size_t)FIO_MEMORY_BLOCK_ALLOC_LIMIT,
      (size_t)FIO_MEMORY_ALLOC_LIMIT,
      FIO_MEMORY_MALLOC_ZERO_POINTER,
      (FIO_MEMORY_INITIALIZE_ALLOCATIONS ? "true" : "false"));
}

/* *****************************************************************************
Malloc implementation
***************************************************************************** */

/* SublimeText marker */
void fio___malloc__(void);
/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
 * as if `mempool_mmap` was called.
 *
 * `mempool_malloc` promises a best attempt at providing locality between
 * consecutive calls, but locality can't be guaranteed.
 */
FIO_IFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME,
                                           ___malloc)(size_t size,
                                                      void *is_realloc) {
  void *p = NULL;
  if (!size)
    goto malloc_zero;
  if ((is_realloc && size > (FIO_MEMORY_BIG_BLOCK_SIZE -
                             (FIO_MEMORY_BIG_BLOCK_HEADER_SIZE << 1))) ||
      (!is_realloc && size > FIO_MEMORY_ALLOC_LIMIT)) {
#ifdef DEBUG
    FIO_LOG_WARNING(
        "unintended " FIO_MACRO2STR(
            FIO_NAME(FIO_MEMORY_NAME, mmap)) " allocation (slow): %zu pages",
        FIO_MEM_BYTES2PAGES(size));
#endif
    return FIO_NAME(FIO_MEMORY_NAME, mmap)(size);
  }
  if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_state_setup)();
  }
#if FIO_MEMORY_ENABLE_BIG_ALLOC
  if ((is_realloc &&
       size > FIO_MEMORY_BLOCK_SIZE - (2 << FIO_MEMORY_ALIGN_LOG)) ||
      (!is_realloc && size > FIO_MEMORY_BLOCK_ALLOC_LIMIT))
    return FIO_NAME(FIO_MEMORY_NAME, __mem_big_slice_new)(size, is_realloc);
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

  return FIO_NAME(FIO_MEMORY_NAME, __mem_slice_new)(size, is_realloc);
malloc_zero:
  p = FIO_MEMORY_MALLOC_ZERO_POINTER;
  return p;
}

/* *****************************************************************************
Memory Allocation - API implementation
***************************************************************************** */

/* SublimeText marker */
void fio_malloc__(void);
/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
 * as if `mempool_mmap` was called.
 *
 * `mempool_malloc` promises a best attempt at providing locality between
 * consecutive calls, but locality can't be guaranteed.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, malloc)(size_t size) {
  return FIO_NAME(FIO_MEMORY_NAME, ___malloc)(size, NULL);
}

/* SublimeText marker */
void fio_calloc__(void);
/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
 * as if `mempool_mmap` was called.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME,
                                       calloc)(size_t size_per_unit,
                                               size_t unit_count) {
#if FIO_MEMORY_INITIALIZE_ALLOCATIONS
  return FIO_NAME(FIO_MEMORY_NAME, malloc)(size_per_unit * unit_count);
#else
  void *p;
  /* round up to alignment size. */
  const size_t len =
      ((size_per_unit * unit_count) + (FIO_MEMORY_ALIGN_SIZE - 1)) &
      (~((size_t)FIO_MEMORY_ALIGN_SIZE - 1));
  p = FIO_NAME(FIO_MEMORY_NAME, malloc)(len);
  /* initialize memory only when required */
  FIO___MEMSET(p, 0, len);
  return p;
#endif /* FIO_MEMORY_INITIALIZE_ALLOCATIONS */
}

/* SublimeText marker */
void fio_free__(void);
/** Frees memory that was allocated using this library. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, free)(void *ptr) {
  if (!ptr || ptr == FIO_MEMORY_MALLOC_ZERO_POINTER)
    return;
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
      FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(ptr);

#if FIO_MEMORY_ENABLE_BIG_ALLOC
  if (c->marker == FIO_MEMORY_BIG_BLOCK_MARKER) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_slice_free)(ptr);
    return;
  }
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

  /* big mmap allocation? */
  if (((uintptr_t)c + FIO_MEMORY_ALIGN_SIZE) == (uintptr_t)ptr && c->marker)
    goto mmap_free;
  FIO_NAME(FIO_MEMORY_NAME, __mem_slice_free)(ptr);
  return;
mmap_free:
  /* zero out memory before returning it to the system */
  FIO___MEMSET(ptr,
               0,
               ((size_t)c->marker << FIO_MEM_PAGE_SIZE_LOG) -
                   FIO_MEMORY_ALIGN_SIZE);
  FIO_MEM_PAGE_FREE(c, c->marker);
  FIO_MEMORY_ON_CHUNK_FREE(c);
}

/* SublimeText marker */
void fio_realloc__(void);
/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc)(void *ptr,
                                                             size_t new_size) {
  return FIO_NAME(FIO_MEMORY_NAME, realloc2)(ptr, new_size, new_size);
}

/**
 * Uses system page maps for reallocation.
 */
FIO_SFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_realloc2_big)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c,
    size_t new_size) {
  const size_t new_page_len =
      FIO_MEM_BYTES2PAGES(new_size + FIO_MEMORY_ALIGN_SIZE);
  c = (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)FIO_MEM_PAGE_REALLOC(
      c,
      c->marker,
      new_page_len,
      FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    return NULL;
  c->marker = (uint32_t)new_page_len;
  return (void *)((uintptr_t)c + FIO_MEMORY_ALIGN_SIZE);
}

/* SublimeText marker */
void fio_realloc2__(void);
/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_MEM_ALIGN FIO_NAME(FIO_MEMORY_NAME, realloc2)(void *ptr,
                                                              size_t new_size,
                                                              size_t copy_len) {
  void *mem = NULL;
  if (!new_size)
    goto act_as_free;
  if (!ptr || ptr == FIO_MEMORY_MALLOC_ZERO_POINTER)
    goto act_as_malloc;

  { /* test for big-paged malloc and limit copy_len */
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
        FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(ptr);
    size_t b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, ptr);

    register size_t max_len =
        ((uintptr_t)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0) +
         FIO_MEMORY_BLOCK_SIZE) -
        ((uintptr_t)ptr);
#if FIO_MEMORY_ENABLE_BIG_ALLOC
    if (c->marker == FIO_MEMORY_BIG_BLOCK_MARKER) {
      /* extend max_len to accomodate possible length */
      max_len =
          ((uintptr_t)c + FIO_MEMORY_SYS_ALLOCATION_SIZE) - ((uintptr_t)ptr);
    } else
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
        if ((uintptr_t)(c) + FIO_MEMORY_ALIGN_SIZE == (uintptr_t)ptr &&
            c->marker) {
      if (new_size > FIO_MEMORY_ALLOC_LIMIT)
        return (mem =
                    FIO_NAME(FIO_MEMORY_NAME, __mem_realloc2_big)(c, new_size));
      max_len = new_size; /* shrinking from mmap to allocator */
    }

    if (copy_len > max_len)
      copy_len = max_len;
    if (copy_len > new_size)
      copy_len = new_size;
  }

  mem = FIO_NAME(FIO_MEMORY_NAME, ___malloc)(new_size, ptr);
  if (!mem || mem == ptr) {
    return mem;
  }

  /* when allocated from the same block, the max length might be adjusted */
  if ((uintptr_t)mem > (uintptr_t)ptr &&
      (uintptr_t)ptr + copy_len >= (uintptr_t)mem) {
    copy_len = (uintptr_t)mem - (uintptr_t)ptr;
  }

  FIO___MEMCPY2(mem,
                ptr,
                ((copy_len + (FIO_MEMORY_ALIGN_SIZE - 1)) &
                 ((~(size_t)0) << FIO_MEMORY_ALIGN_LOG)));
  // zero out leftover bytes, if any.
  while (copy_len & (FIO_MEMORY_ALIGN_SIZE - 1)) {
    ((uint8_t *)mem)[copy_len++] = 0;
  }

  FIO_NAME(FIO_MEMORY_NAME, free)(ptr);

  return mem;

act_as_malloc:
  mem = FIO_NAME(FIO_MEMORY_NAME, malloc)(new_size);
  return mem;

act_as_free:
  FIO_NAME(FIO_MEMORY_NAME, free)(ptr);
  mem = FIO_MEMORY_MALLOC_ZERO_POINTER;
  return mem;
}

/* SublimeText marker */
void fio_mmap__(void);
/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `mempoll_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, mmap)(size_t size) {
  if (!size)
    return FIO_NAME(FIO_MEMORY_NAME, malloc)(0);
  size_t pages = FIO_MEM_BYTES2PAGES(size + FIO_MEMORY_ALIGN_SIZE);
  if (((uint64_t)pages >> 32))
    return NULL;
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)
          FIO_MEM_PAGE_ALLOC(pages, FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    return NULL;
  FIO_MEMORY_ON_CHUNK_ALLOC(c);
  c->marker = (uint32_t)pages;
  return (void *)((uintptr_t)c + FIO_MEMORY_ALIGN_SIZE);
}

/* *****************************************************************************
Override the system's malloc functions if required
***************************************************************************** */
#if defined(FIO_MALLOC_OVERRIDE_SYSTEM) && !defined(H___FIO_MALLOC_OVERRIDE___H)
#define H___FIO_MALLOC_OVERRIDE___H
void *malloc(size_t size) { return FIO_NAME(FIO_MEMORY_NAME, malloc)(size); }
void *calloc(size_t size, size_t count) {
  return FIO_NAME(FIO_MEMORY_NAME, calloc)(size, count);
}
void free(void *ptr) { FIO_NAME(FIO_MEMORY_NAME, free)(ptr); }
void *realloc(void *ptr, size_t new_size) {
  return FIO_NAME(FIO_MEMORY_NAME, realloc2)(ptr, new_size, new_size);
}
#endif /* FIO_MALLOC_OVERRIDE_SYSTEM */
#undef FIO_MALLOC_OVERRIDE_SYSTEM

/* *****************************************************************************





Memory Allocation - test





***************************************************************************** */
#ifdef FIO_TEST_CSTL

#include "pthread.h"

/* contention testing (multi-threaded) */
FIO_IFUNC void *FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio),
                              mem_tsk)(void *i_) {
  uintptr_t cycles = (uintptr_t)i_;
  const size_t test_byte_count =
      FIO_MEMORY_SYS_ALLOCATION_SIZE + (FIO_MEMORY_SYS_ALLOCATION_SIZE >> 1);
  uint64_t marker;
  do {
    marker = fio_rand64();
  } while (!marker);

  const size_t limit = (test_byte_count / cycles);
  char **ary = (char **)FIO_NAME(FIO_MEMORY_NAME, calloc)(sizeof(*ary), limit);
  const uintptr_t alignment_mask = (FIO_MEMORY_ALIGN_SIZE - 1);
  FIO_ASSERT(ary, "allocation failed for test container");
  for (size_t i = 0; i < limit; ++i) {
    if (1) {
      /* add some fragmentation */
      char *tmp = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(16);
      FIO_NAME(FIO_MEMORY_NAME, free)(tmp);
      FIO_ASSERT(tmp, "small allocation failed!")
      FIO_ASSERT(!((uintptr_t)tmp & alignment_mask),
                 "allocation alignment error!");
    }
    ary[i] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
    FIO_ASSERT(ary[i], "allocation failed!");
    FIO_ASSERT(!((uintptr_t)ary[i] & alignment_mask),
               "allocation alignment error!");
    FIO_ASSERT(!FIO_MEMORY_INITIALIZE_ALLOCATIONS || !ary[i][(cycles - 1)],
               "allocated memory not zero (end): %p",
               (void *)ary[i]);
    FIO_ASSERT(!FIO_MEMORY_INITIALIZE_ALLOCATIONS || !ary[i][0],
               "allocated memory not zero (start): %p",
               (void *)ary[i]);
    fio___memset_aligned(ary[i], marker, (cycles));
  }
  for (size_t i = 0; i < limit; ++i) {
    char *tmp = (char *)FIO_NAME(FIO_MEMORY_NAME,
                                 realloc2)(ary[i], (cycles << 1), (cycles));
    FIO_ASSERT(tmp, "re-allocation failed!")
    ary[i] = tmp;
    FIO_ASSERT(!((uintptr_t)ary[i] & alignment_mask),
               "allocation alignment error!");
    FIO_ASSERT(!FIO_MEMORY_INITIALIZE_ALLOCATIONS || !ary[i][(cycles)],
               "realloc2 copy overflow!");
    fio___memset_test_aligned(ary[i], marker, (cycles), "realloc grow");
    tmp =
        (char *)FIO_NAME(FIO_MEMORY_NAME, realloc2)(ary[i], (cycles), (cycles));
    FIO_ASSERT(tmp, "re-allocation (shrinking) failed!")
    ary[i] = tmp;
    fio___memset_test_aligned(ary[i], marker, (cycles), "realloc shrink");
  }
  for (size_t i = 0; i < limit; ++i) {
    fio___memset_test_aligned(ary[i], marker, (cycles), "mem review");
    FIO_NAME(FIO_MEMORY_NAME, free)(ary[i]);
  }
  FIO_NAME(FIO_MEMORY_NAME, free)(ary);
  return NULL;
}

/* main test functin */
FIO_SFUNC void FIO_NAME_TEST(FIO_NAME(stl, FIO_MEMORY_NAME), mem)(void) {
  fprintf(stderr,
          "* Testing core memory allocator " FIO_MACRO2STR(
              FIO_NAME(FIO_MEMORY_NAME, malloc)) ".\n");

  const uintptr_t alignment_mask = (FIO_MEMORY_ALIGN_SIZE - 1);
  fprintf(stderr,
          "* validating allocation alignment on %zu byte border.\n",
          FIO_MEMORY_ALIGN_SIZE);
  for (size_t i = 0; i < alignment_mask; ++i) {
    void *p = FIO_NAME(FIO_MEMORY_NAME, malloc)(i);
    FIO_ASSERT(!((uintptr_t)p & alignment_mask),
               "allocation alignment error allocatiing %zu bytes!",
               i);
    FIO_NAME(FIO_MEMORY_NAME, free)(p);
  }
  const size_t thread_count =
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count +
      (1 + (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count >> 1));
  for (uintptr_t cycles = 16; cycles <= (FIO_MEMORY_ALLOC_LIMIT); cycles *= 2) {
    pthread_t threads[thread_count];
    fprintf(stderr,
            "* Testing %zu byte allocation blocks with %zu threads.\n",
            (size_t)(cycles),
            (thread_count + 1));
    for (size_t i = 1; i < thread_count; ++i) {
      if (pthread_create(threads + i,
                         NULL,
                         FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio), mem_tsk),
                         (void *)cycles)) {
        abort();
      }
    }
    FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio), mem_tsk)((void *)cycles);
    for (size_t i = 1; i < thread_count; ++i) {
      pthread_join(threads[i], NULL);
    }
  }
  fprintf(stderr,
          "* re-validating allocation alignment on %zu byte border.\n",
          FIO_MEMORY_ALIGN_SIZE);
  for (size_t i = 0; i < alignment_mask; ++i) {
    void *p = FIO_NAME(FIO_MEMORY_NAME, malloc)(i);
    FIO_ASSERT(!((uintptr_t)p & alignment_mask),
               "allocation alignment error allocatiing %zu bytes!",
               i);
    FIO_NAME(FIO_MEMORY_NAME, free)(p);
  }

#if DEBUG && FIO_EXTERN_COMPLETE
  FIO_NAME(FIO_MEMORY_NAME, malloc_print_state)();
  FIO_NAME(FIO_MEMORY_NAME, __mem_state_cleanup)();
  FIO_ASSERT(!FIO_NAME(fio___, FIO_NAME(FIO_MEMORY_NAME, state_chunk_count)),
             "memory leaks?");
#endif /* DEBUG && FIO_EXTERN_COMPLETE */
}
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Memory pool cleanup
***************************************************************************** */
#undef FIO___MEMSET
#undef FIO___MEMCPY2
#undef FIO_MEM_ALIGN
#undef FIO_MEM_ALIGN_NEW
#undef FIO_MEMORY_MALLOC_ZERO_POINTER

#endif /* FIO_MEMORY_DISABLE */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MEMORY_NAME */

#undef FIO_MEMORY_ON_CHUNK_ALLOC
#undef FIO_MEMORY_ON_CHUNK_FREE
#undef FIO_MEMORY_ON_CHUNK_CACHE
#undef FIO_MEMORY_ON_CHUNK_UNCACHE
#undef FIO_MEMORY_ON_CHUNK_DIRTY
#undef FIO_MEMORY_ON_CHUNK_UNDIRTY
#undef FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK
#undef FIO_MEMORY_ON_BIG_BLOCK_SET
#undef FIO_MEMORY_ON_BIG_BLOCK_UNSET
#undef FIO_MEMORY_PRINT_STATS
#undef FIO_MEMORY_PRINT_STATS_END

#undef FIO_MEMORY_ARENA_COUNT
#undef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
#undef FIO_MEMORY_CACHE_SLOTS
#undef FIO_MEMORY_ALIGN_LOG
#undef FIO_MEMORY_INITIALIZE_ALLOCATIONS
#undef FIO_MEMORY_USE_PTHREAD_MUTEX
#undef FIO_MEMORY_USE_FIO_MEMSET
#undef FIO_MEMORY_USE_FIO_MEMCOPY
#undef FIO_MEMORY_BLOCK_SIZE
#undef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
#undef FIO_MEMORY_BLOCKS_PER_ALLOCATION
#undef FIO_MEMORY_ENABLE_BIG_ALLOC
#undef FIO_MEMORY_ARENA_COUNT_FALLBACK
#undef FIO_MEMORY_ARENA_COUNT_MAX
#undef FIO_MEMORY_WARMUP

#undef FIO_MEMORY_LOCK_NAME
#undef FIO_MEMORY_LOCK_TYPE
#undef FIO_MEMORY_LOCK_TYPE_INIT
#undef FIO_MEMORY_TRYLOCK
#undef FIO_MEMORY_LOCK
#undef FIO_MEMORY_UNLOCK

/* don't undefine FIO_MEMORY_NAME due to possible use in allocation macros */

/* *****************************************************************************
Memory management macros
***************************************************************************** */

#if !defined(FIO_MEM_REALLOC_) || !defined(FIO_MEM_FREE_)
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_

#ifdef FIO_MALLOC_TMP_USE_SYSTEM /* force malloc */
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  realloc((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) free((ptr))
#define FIO_MEM_REALLOC_IS_SAFE_ 0

#else /* FIO_MALLOC_TMP_USE_SYSTEM */
#define FIO_MEM_REALLOC_         FIO_MEM_REALLOC
#define FIO_MEM_FREE_            FIO_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIO_MEM_REALLOC_IS_SAFE
#endif /* FIO_MALLOC_TMP_USE_SYSTEM */

#endif /* !defined(FIO_MEM_REALLOC_)... */
