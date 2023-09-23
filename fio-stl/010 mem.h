/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MEMORY_NAME fio    /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      Custom Memory Allocator / Pooling



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */

/* *****************************************************************************
Memory Allocation - Setup Alignment Info
***************************************************************************** */
#if defined(FIO_MEMORY_NAME) && !defined(FIO___RECURSIVE_INCLUDE)

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

/* Make sure the system's allocator is marked as unsafe. */
#if FIO_MALLOC_TMP_USE_SYSTEM
#undef FIO_MEMORY_INITIALIZE_ALLOCATIONS
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#endif

#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/**
 * The logarithmic size of a single allocation "chunk" (16 blocks).
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
 * return initialized memory (bytes are all zeros).
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
#define FIO_MEMORY_ARENA_COUNT_FALLBACK 24
#endif

#ifndef FIO_MEMORY_ARENA_COUNT_MAX
/*
 * Used when dynamic arena count calculations fail.
 *
 * NOTE: if FIO_MEMORY_ARENA_COUNT is negative, dynamic arena calculation is
 * performed using CPU core calculation.
 */
#define FIO_MEMORY_ARENA_COUNT_MAX 64
#endif

#ifndef FIO_MEMORY_WARMUP
#define FIO_MEMORY_WARMUP 0
#endif

#ifndef FIO_MEMORY_USE_THREAD_MUTEX
#if FIO_USE_THREAD_MUTEX_TMP
#define FIO_MEMORY_USE_THREAD_MUTEX 1
#else
#if FIO_MEMORY_ARENA_COUNT > 0
/**
 * If arena count isn't linked to the CPU count, threads might busy-spin.
 * It is better to slow wait than fast busy spin when the work in the lock is
 * longer... and system allocations are performed inside arena locks.
 */
#define FIO_MEMORY_USE_THREAD_MUTEX 1
#else
/* defaults to use a spinlock when no contention is expected. */
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#endif
#endif
#endif

#if !defined(FIO_MEM_SYS_ALLOC) || !defined(FIO_MEM_SYS_REALLOC) ||            \
    !defined(FIO_MEM_SYS_FREE)
/**
 * The following MACROS, when all of them are defined, allow the memory
 * allocator to collect memory from the system using an alternative method.
 *
 * - FIO_MEM_SYS_ALLOC(pages, alignment_log)
 *
 * - FIO_MEM_SYS_REALLOC(ptr, old_pages, new_pages, alignment_log)
 *
 * - FIO_MEM_SYS_FREE(ptr, pages) FIO_MEM_SYS_FREE_def_func((ptr), (pages))
 *
 * Note that the alignment property for the allocated memory is essential and
 * may me quite large.
 */
#undef FIO_MEM_SYS_ALLOC
#undef FIO_MEM_SYS_REALLOC
#undef FIO_MEM_SYS_FREE
#endif /* undefined FIO_MEM_SYS_ALLOC... */

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

/** Prints the allocator's free block list. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_free_block_list)(void);

/** Prints the settings used to define the allocator. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)(void);

/* *****************************************************************************
Set global macros to use this allocator if FIO_MALLOC
***************************************************************************** */
#ifdef FIO_MALLOC
/* prevent double declaration of FIO_MALLOC */
#define H___FIO_MALLOC___H
#undef FIO_MEM_REALLOC
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))
#undef FIO_MEM_FREE
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MEM_REALLOC_IS_SAFE fio_realloc_is_safe()
#undef FIO_MALLOC
#endif /* FIO_MALLOC */

/* *****************************************************************************
Temporarily (at least) set memory allocation macros to use this allocator
***************************************************************************** */
#ifndef FIO_MALLOC_TMP_USE_SYSTEM

#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_REALLOC_IS_SAFE_

#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  FIO_NAME(FIO_MEMORY_NAME, realloc2)((ptr), (new_size), (copy_len))
#define FIO_MEM_FREE_(ptr, size) FIO_NAME(FIO_MEMORY_NAME, free)((ptr))
#define FIO_MEM_REALLOC_IS_SAFE_ FIO_NAME(FIO_MEMORY_NAME, realloc_is_safe)()

#endif /* FIO_MALLOC_TMP_USE_SYSTEM */

/* *****************************************************************************





Memory Allocation - start implementation





***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
/* internal workings start here */

/* *****************************************************************************







Helpers and System Memory Allocation




***************************************************************************** */
#ifndef H___FIO_MEM_INCLUDE_ONCE___H
#define H___FIO_MEM_INCLUDE_ONCE___H

#define FIO_MEM_BYTES2PAGES(size)                                              \
  (((size_t)(size) + ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)) &                   \
   ((~(size_t)0) << FIO_MEM_PAGE_SIZE_LOG))

/* *****************************************************************************



POSIX Allocation



***************************************************************************** */
#if FIO_OS_POSIX || __has_include("sys/mman.h")
#include <sys/mman.h>

/* Mitigates MAP_ANONYMOUS not being defined on older versions of MacOS */
#if !defined(MAP_ANONYMOUS)
#if defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0
#endif /* defined(MAP_ANONYMOUS) */
#endif /* FIO_MEM_SYS_ALLOC */

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
FIO_SFUNC void *FIO_MEM_SYS_ALLOC_def_func(size_t bytes,
                                           uint8_t alignment_log) {
  void *result;
  static void *next_alloc = (void *)0x01;
  const size_t alignment_mask = (1ULL << alignment_log) - 1;
  const size_t alignment_size = (1ULL << alignment_log);
  bytes = FIO_MEM_BYTES2PAGES(bytes);
  next_alloc =
      (void *)(((uintptr_t)next_alloc + alignment_mask) & alignment_mask);
/* hope for the best? */
#ifdef MAP_ALIGNED
  result = mmap(next_alloc,
                bytes,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED(alignment_log),
                -1,
                0);
#else
  result = mmap(next_alloc,
                bytes,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0);
#endif /* MAP_ALIGNED */
  if (result == MAP_FAILED)
    return (void *)NULL;
  if (((uintptr_t)result & alignment_mask)) {
    munmap(result, bytes);
    result = mmap(NULL,
                  bytes + alignment_size,
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
    munmap((void *)((uintptr_t)result + bytes), alignment_size - offset);
  }
  next_alloc = (void *)((uintptr_t)result + (bytes << 2));
  return result;
}

/*
 * Re-allocates memory using `mmap`, enforcing alignment.
 */
FIO_SFUNC void *FIO_MEM_SYS_REALLOC_def_func(void *mem,
                                             size_t old_len,
                                             size_t new_len,
                                             uint8_t alignment_log) {
  old_len = FIO_MEM_BYTES2PAGES(old_len);
  new_len = FIO_MEM_BYTES2PAGES(new_len);
  if (new_len > old_len) {
    void *result;
#if defined(__linux__)
    result = mremap(mem, old_len, new_len, 0);
    if (result != MAP_FAILED)
      return result;
#endif
    result = mmap((void *)((uintptr_t)mem + old_len),
                  new_len - old_len,
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  -1,
                  0);
    if (result == (void *)((uintptr_t)mem + old_len)) {
      result = mem;
    } else {
      /* copy and free */
      munmap(result, new_len - old_len); /* free the failed attempt */
      result =
          FIO_MEM_SYS_ALLOC_def_func(new_len,
                                     alignment_log); /* allocate new memory */
      if (!result) {
        return (void *)NULL;
      }
      FIO_MEMCPY(result, mem, old_len); /* copy data */
      munmap(mem, old_len);             /* free original memory */
    }
    return result;
  }
  if (old_len != new_len) /* remove dangling pages */
    munmap((void *)((uintptr_t)mem + new_len), old_len - new_len);
  return mem;
}

/* frees memory using `munmap`. */
FIO_IFUNC void FIO_MEM_SYS_FREE_def_func(void *mem, size_t bytes) {
  bytes = FIO_MEM_BYTES2PAGES(bytes);
  munmap(mem, bytes);
}

/* *****************************************************************************



Windows Allocation



***************************************************************************** */
#elif FIO_OS_WIN
#include <memoryapi.h>

FIO_IFUNC void FIO_MEM_SYS_FREE_def_func(void *mem, size_t bytes) {
  bytes = FIO_MEM_BYTES2PAGES(bytes);
  if (!VirtualFree(mem, 0, MEM_RELEASE))
    FIO_LOG_ERROR("Memory address at %p couldn't be returned to the system",
                  mem);
  (void)bytes;
}

FIO_IFUNC void *FIO_MEM_SYS_ALLOC_def_func(size_t bytes,
                                           uint8_t alignment_log) {
  // return aligned_alloc((pages << 12), (1UL << alignment_log));
  void *result;
  size_t attempts = 0;
  static void *next_alloc = (void *)0x01;
  const uintptr_t alignment_rounder = (1ULL << alignment_log) - 1;
  const uintptr_t alignment_mask = ~alignment_rounder;
  bytes = FIO_MEM_BYTES2PAGES(bytes);
  do {
    next_alloc =
        (void *)(((uintptr_t)next_alloc + alignment_rounder) & alignment_mask);
    FIO_ASSERT_DEBUG(!((uintptr_t)next_alloc & alignment_rounder),
                     "alignment allocation rounding error?");
    result =
        VirtualAlloc(next_alloc, (bytes << 2), MEM_RESERVE, PAGE_READWRITE);
    next_alloc = (void *)((uintptr_t)next_alloc + (bytes << 2));
  } while (!result && (attempts++) < 1024);
  if (result) {
    result = VirtualAlloc(result, bytes, MEM_COMMIT, PAGE_READWRITE);
    FIO_ASSERT_DEBUG(result, "couldn't commit memory after reservation?!");

  } else {
    FIO_LOG_ERROR("Couldn't allocate memory from the system, error %zu."
                  "\n\t%zu attempts with final address %p",
                  GetLastError(),
                  attempts,
                  next_alloc);
  }
  return result;
}

FIO_IFUNC void *FIO_MEM_SYS_REALLOC_def_func(void *mem,
                                             size_t old_len,
                                             size_t new_len,
                                             uint8_t alignment_log) {
  if (!new_len)
    goto free_mem;
  old_len = FIO_MEM_BYTES2PAGES(old_len);
  new_len = FIO_MEM_BYTES2PAGES(new_len);
  if (new_len > old_len) {
    /* extend allocation */
    void *tmp = VirtualAlloc((void *)((uintptr_t)mem + old_len),
                             new_len - old_len,
                             MEM_COMMIT,
                             PAGE_READWRITE);
    if (tmp)
      return mem;
    /* Alloc, Copy, Free... sorry... */
    tmp = FIO_MEM_SYS_ALLOC_def_func(new_len, alignment_log);
    if (!tmp) {
      FIO_LOG_ERROR("sysem realloc failed to allocate memory.");
      return NULL;
    }
    FIO_MEMCPY(tmp, mem, old_len);
    FIO_MEM_SYS_FREE_def_func(mem, old_len);
    mem = tmp;
  } else if (old_len > new_len) {
    /* shrink allocation */
    if (!VirtualFree((void *)((uintptr_t)mem + new_len),
                     old_len - new_len,
                     MEM_DECOMMIT))
      FIO_LOG_ERROR("failed to decommit memory range @ %p.", mem);
  }
  return mem;
free_mem:
  FIO_MEM_SYS_FREE_def_func(mem, old_len);
  mem = NULL;
  return NULL;
}

/* *****************************************************************************


Unknown OS... Unsupported?


***************************************************************************** */
#else /* FIO_OS_POSIX / FIO_OS_WIN => unknown...? */

FIO_IFUNC void *FIO_MEM_SYS_ALLOC_def_func(size_t bytes,
                                           uint8_t alignment_log) {
  // return aligned_alloc((pages << 12), (1UL << alignment_log));
  exit(-1);
  (void)bytes;
  (void)alignment_log;
}

FIO_IFUNC void *FIO_MEM_SYS_REALLOC_def_func(void *mem,
                                             size_t old_len,
                                             size_t new_len,
                                             uint8_t alignment_log) {
  (void)old_len;
  (void)alignment_log;
  new_len = FIO_MEM_BYTES2PAGES(new_len);
  return realloc(mem, new_len);
}

FIO_IFUNC void FIO_MEM_SYS_FREE_def_func(void *mem, size_t bytes) {
  free(mem);
  (void)bytes;
}

#endif /* FIO_OS_POSIX / FIO_OS_WIN */
/* *****************************************************************************
Overridable system allocation macros
***************************************************************************** */
#ifndef FIO_MEM_SYS_ALLOC
#define FIO_MEM_SYS_ALLOC(pages, alignment_log)                                \
  FIO_MEM_SYS_ALLOC_def_func((pages), (alignment_log))
#define FIO_MEM_SYS_REALLOC(ptr, old_pages, new_pages, alignment_log)          \
  FIO_MEM_SYS_REALLOC_def_func((ptr), (old_pages), (new_pages), (alignment_log))
#define FIO_MEM_SYS_FREE(ptr, pages) FIO_MEM_SYS_FREE_def_func((ptr), (pages))
#endif /* FIO_MEM_SYS_ALLOC */

#endif /* H___FIO_MEM_INCLUDE_ONCE___H */

/* *****************************************************************************
FIO_MEMORY_DISABLE - use the system allocator
***************************************************************************** */
#if defined(FIO_MALLOC_TMP_USE_SYSTEM)

SFUNC void *FIO_MEM_ALIGN_NEW FIO_NAME(FIO_MEMORY_NAME, malloc)(size_t size) {
#if FIO_MEMORY_INITIALIZE_ALLOCATIONS
  return calloc(size, 1);
#elif defined(DEBUG) && DEBUG
  void *ret = malloc(size);
  if (ret)
    FIO_MEMSET(ret, 0xFA, size);
  return ret;
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
/** Prints the allocator's free block list. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_free_block_list)(void) {}
/** Prints the settings used to define the allocator. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_settings)(void) {}
SFUNC size_t FIO_NAME(FIO_MEMORY_NAME, malloc_block_size)(void) { return 0; }

#ifdef FIO_TEST_ALL
SFUNC void FIO_NAME_TEST(FIO_NAME(stl, FIO_MEMORY_NAME), mem)(void) {
  fprintf(stderr, "* Custom memory allocator bypassed.\n");
}
#endif /* FIO_TEST_ALL */

#else /* FIO_MEMORY_DISABLE */

/* *****************************************************************************





                  Memory allocation implementation starts here
                    helper function and setup are complete





***************************************************************************** */

/* *****************************************************************************
Lock type choice
***************************************************************************** */
#if FIO_MEMORY_USE_THREAD_MUTEX
#define FIO_MEMORY_LOCK_TYPE fio_thread_mutex_t
#define FIO_MEMORY_LOCK_TYPE_INIT(lock)                                        \
  ((lock) = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT)
#define FIO_MEMORY_TRYLOCK(lock) fio_thread_mutex_trylock(&(lock))
#define FIO_MEMORY_LOCK(lock)    fio_thread_mutex_lock(&(lock))
#define FIO_MEMORY_UNLOCK(lock)                                                \
  do {                                                                         \
    int tmp__ = fio_thread_mutex_unlock(&(lock));                              \
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
FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_MEMORY_NAME, __malloc_chunk))
FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_MEMORY_NAME, malloc))
static volatile size_t FIO_NAME(FIO_MEMORY_NAME, __malloc_total);
#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)                                         \
  do {                                                                         \
    FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MEMORY_NAME, __malloc_chunk));    \
    FIO_LOG_DEBUG2("MEMORY CACHE-ALLOC allocated %p", ptr);                    \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)                                          \
  do {                                                                         \
    FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MEMORY_NAME, __malloc_chunk));     \
    FIO_LOG_DEBUG2("MEMORY CACHE-DEALLOC de-allocated %p", ptr);               \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-DEALLOC placed %p in cache", ptr);            \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)                                       \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-ALLOC retrieved %p from cache", ptr);         \
  } while (0);

#define FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(ptr, blk)                            \
  do {                                                                         \
    if (0)                                                                     \
      FIO_LOG_DEBUG2("MEMORY chunk %p block %zu reset in lock",                \
                     ptr,                                                      \
                     (size_t)blk);                                             \
  } while (0);

#define FIO_MEMORY_ON_BIG_BLOCK_SET(ptr)                                       \
  do {                                                                         \
    if (1)                                                                     \
      FIO_LOG_DEBUG2("MEMORY chunk %p used as big-block", ptr);                \
  } while (0);

#define FIO_MEMORY_ON_BIG_BLOCK_UNSET(ptr)                                     \
  do {                                                                         \
    if (1)                                                                     \
      FIO_LOG_DEBUG2("MEMORY chunk %p no longer used as big-block", ptr);      \
  } while (0);
#define FIO_MEMORY_PRINT_STATS_END()                                           \
  do {                                                                         \
    FIO_LOG_DEBUG2(                                                            \
        "(" FIO_MACRO2STR(                                                     \
            FIO_NAME(FIO_MEMORY_NAME, malloc)) ") total allocations: %zu",     \
        FIO_NAME(FIO_MEMORY_NAME, __malloc_total));                            \
  } while (0)
#define FIO_MEMORY_ON_ALLOC_FUNC()                                             \
  do {                                                                         \
    FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_MEMORY_NAME, malloc));            \
    fio_atomic_add(&FIO_NAME(FIO_MEMORY_NAME, __malloc_total), 1);             \
  } while (0)
#define FIO_MEMORY_ON_FREE_FUNC()                                              \
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_MEMORY_NAME, malloc))
#else /* defined(DEBUG) || defined(FIO_LEAK_COUNTER) */
#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)              ((void)0)
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)               ((void)0)
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)              ((void)0)
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)            ((void)0)
#define FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(ptr, blk) ((void)0)
#define FIO_MEMORY_ON_BIG_BLOCK_SET(ptr)            ((void)0)
#define FIO_MEMORY_ON_BIG_BLOCK_UNSET(ptr)          ((void)0)
#define FIO_MEMORY_PRINT_STATS_END()                ((void)0)
#define FIO_MEMORY_ON_ALLOC_FUNC()                  ((void)0)
#define FIO_MEMORY_ON_FREE_FUNC()                   ((void)0)
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
  uint8_t pad_for_cache___[115]; /* cache line padding */
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
  uint8_t pad_for_cache___[115]; /* cache line padding */
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
  /** main memory state lock */
  FIO_MEMORY_LOCK_TYPE lock;
  /** free list for available blocks */
  FIO_LIST_HEAD blocks;
  /** the arena count for the allocator */
  uint8_t pad_for_cache2___[111]; /* cache line padding */
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

#if defined(DEBUG) && FIO_MEMORY_ARENA_COUNT > 0 && !defined(FIO_TEST_ALL)
  static size_t warning_printed = 0;
#define FIO___MEMORY_ARENA_LOCK_WARNING()                                      \
  do {                                                                         \
    if (!warning_printed)                                                      \
      FIO_LOG_WARNING(FIO_MACRO2STR(FIO_NAME(                                  \
          FIO_MEMORY_NAME,                                                     \
          malloc)) " high arena contention.\n"                                 \
                   "          Consider recompiling with more arenas.");        \
    warning_printed = 1;                                                       \
  } while (0)
#else
#define FIO___MEMORY_ARENA_LOCK_WARNING()
#endif
  /** thread arena value */
  size_t arena_index;
  size_t loop_count = 0;
  {
    /* select the default arena selection using a thread ID. */
    union {
      void *p;
      fio_thread_t t;
    } u = {.t = fio_thread_current()};
    arena_index = fio_risky_ptr(u.p) %
                  FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count;
#if defined(DEBUG) && 0
    static void *pthread_last = NULL;
    if (pthread_last != u.p) {
      FIO_LOG_DEBUG(
          "thread %p (%p) associated with arena %zu / %zu",
          u.p,
          (void *)fio_risky_ptr(u.p),
          arena_index,
          (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count);
      pthread_last = u.p;
    }
#endif
  }
  for (;;) {
    /* rotate all arenas to find one that's available */
    if (!FIO_MEMORY_TRYLOCK(
            FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[arena_index].lock))
      return (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena + arena_index);
    FIO_LOG_DDEBUG("thread %p had to switch arena from %zu / %zu",
                   fio_thread_current(),
                   arena_index,
                   (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count);
    ++arena_index;
    if (arena_index == FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count)
      arena_index = 0;
    if (++loop_count <
        (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count << 1))
      continue;
    FIO___MEMORY_ARENA_LOCK_WARNING();
#undef FIO___MEMORY_ARENA_LOCK_WARNING
#if FIO_MEMORY_USE_THREAD_MUTEX && FIO_OS_POSIX
    /* slow wait for arena */
    FIO_MEMORY_LOCK(
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena[arena_index].lock);
    return FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena + arena_index;
#else
    // FIO_THREAD_RESCHEDULE();
#endif /* FIO_MEMORY_USE_THREAD_MUTEX */
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

/* function declarations for functions called during cleanup */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c);
FIO_IFUNC void *FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)(void);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(void *ptr);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_free)(void *ptr);
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c);

/* IDE marker */
void fio___mem_state_cleanup___(void);
FIO_SFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_state_cleanup)(void *ignr_) {
  if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)) {
    FIO_LOG_DEBUG2(FIO_MACRO2STR(
        FIO_NAME(FIO_MEMORY_NAME,
                 __mem_state_cleanup)) " called more than once (NULL state).");
    return;
  }
  (void)ignr_;
  FIO_LOG_DDEBUG2(
      "starting facil.io memory allocator cleanup for " FIO_MACRO2STR(
          FIO_NAME(FIO_MEMORY_NAME, malloc)) ".");
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
    if ((uint32_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block->ref > 1) {
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
  FIO_MEM_SYS_FREE(FIO_NAME(FIO_MEMORY_NAME, __mem_state), s);
  FIO_NAME(FIO_MEMORY_NAME, __mem_state) =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_state_s *))NULL;

  FIO_MEMORY_PRINT_STATS_END();
  FIO_LOG_DDEBUG2(
      "finished facil.io memory allocator cleanup for " FIO_MACRO2STR(
          FIO_NAME(FIO_MEMORY_NAME, malloc)) ".");
}

FIO_SFUNC void FIO_NAME(FIO_MEMORY_NAME,
                        __malloc_after_fork_task)(void *ignr_) {
  (void)ignr_;
  FIO_NAME(FIO_MEMORY_NAME, malloc_after_fork)();
}

/* initializes (allocates) the arenas and state machine */
FIO_CONSTRUCTOR(FIO_NAME(FIO_MEMORY_NAME, __mem_state_setup)) {
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state))
    return;
  fio_state_callback_add(FIO_CALL_IN_CHILD,
                         FIO_NAME(FIO_MEMORY_NAME, __malloc_after_fork_task),
                         NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT,
                         FIO_NAME(FIO_MEMORY_NAME, __mem_state_cleanup),
                         NULL);
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
    else /* arenas !> threads (birthday) */
      arean_count = (arean_count << 1) + 2;
#else
#if _MSC_VER || __MINGW32__
    /* https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
     */
    SYSTEM_INFO win_system_info;
    GetSystemInfo(&win_system_info);
    arean_count = (size_t)win_system_info.dwNumberOfProcessors;
#else
#warning Dynamic CPU core count is unavailable - assuming FIO_MEMORY_ARENA_COUNT_FALLBACK cores.
#endif
#endif /* _SC_NPROCESSORS_ONLN */

    if (arean_count >= FIO_MEMORY_ARENA_COUNT_MAX)
      arean_count = FIO_MEMORY_ARENA_COUNT_MAX;

#endif /* FIO_MEMORY_ARENA_COUNT > 0 */

    const size_t s = FIO_MEMORY_STATE_SIZE(arean_count);
    FIO_NAME(FIO_MEMORY_NAME, __mem_state) =
        (FIO_NAME(FIO_MEMORY_NAME, __mem_state_s *))FIO_MEM_SYS_ALLOC(s, 0);
    FIO_ASSERT_ALLOC(FIO_NAME(FIO_MEMORY_NAME, __mem_state));
    FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count = arean_count;
  }
  FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks =
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
  (void)FIO_NAME(FIO_MEMORY_NAME, malloc_print_free_block_list);
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
  FIO_LOG_DEBUG2("MEMORY reinitializing " FIO_MACRO2STR(
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

void fio_malloc_print_free_block_list___(void);
/** Prints the allocator's free block list. May be used for debugging. */
SFUNC void FIO_NAME(FIO_MEMORY_NAME, malloc_print_free_block_list)(void) {
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.prev ==
      &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks)
    return;
  fprintf(stderr,
          FIO_MACRO2STR(FIO_NAME(FIO_MEMORY_NAME,
                                 malloc)) " allocator free block list:\n");
  FIO_LIST_NODE *n = FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.prev;
  for (size_t i = 0; n != &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks;
       ++i) {
    fprintf(stderr, "\t[%zu] %p\n", i, (void *)n);
    n = n->prev;
  }
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
  FIO_MEMORY_ON_CHUNK_FREE(c);
  FIO_MEM_SYS_FREE(((void *)c), FIO_MEMORY_SYS_ALLOCATION_SIZE);
}

FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_cache_or_dealloc)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c) {
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
}

FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_free)(
    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) * c) {
  /* should we free the chunk? */
  if (!c || fio_atomic_sub_fetch(&c->ref, 1)) {
    FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
    return;
  }

  /* remove all blocks from the block allocation list */
  for (size_t b = 0; b < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++b) {
    FIO_LIST_NODE *n =
        (FIO_LIST_NODE *)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0);
    if (n->prev && n->next) {
      FIO_LIST_REMOVE(n);
      n->prev = n->next = NULL;
    }
  }
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_cache_or_dealloc)(c);
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
    *c = (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)){.ref = 1};
    return c;
  }
#endif /* FIO_MEMORY_CACHE_SLOTS */

  /* system allocation */
  c = (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)FIO_MEM_SYS_ALLOC(
      FIO_MEMORY_SYS_ALLOCATION_SIZE,
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
#if FIO_MEMORY_INITIALIZE_ALLOCATIONS
  if (c->blocks[b].pos >= (int32_t)(FIO_MEMORY_UNITS_PER_BLOCK - 4)) {
    /* zero out the whole block */
    FIO_MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
               0,
               FIO_MEMORY_BLOCK_SIZE);
  } else {
    /* zero out only the memory that was used */
    FIO_MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
               0,
               (((size_t)c->blocks[b].pos) << FIO_MEMORY_ALIGN_LOG));
  }
#elif defined(DEBUG) && DEBUG
  /* set all bytes to 0xAF to better catch initialization bugs */
  FIO_MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
             0xFA,
             FIO_MEMORY_BLOCK_SIZE);
#else
  /** only reset a block's free-list header */
  FIO_MEMSET(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0),
             0,
             (((FIO_MEMORY_ALIGN_SIZE - 1) + sizeof(FIO_LIST_NODE)) &
              (~(FIO_MEMORY_ALIGN_SIZE - 1))));
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
  if (!c)
    return;
  FIO_ASSERT_DEBUG(
      (uint32_t)c->blocks[b].ref <= FIO_MEMORY_UNITS_PER_BLOCK + 1,
      "block reference count corrupted, possible double free? (%zd)",
      (size_t)c->blocks[b].ref);
  FIO_ASSERT_DEBUG(
      (uint32_t)c->blocks[b].pos <= FIO_MEMORY_UNITS_PER_BLOCK + 1,
      "block allocation position corrupted, possible double free? (%zd)",
      (size_t)c->blocks[b].pos);
  if (fio_atomic_sub_fetch(&c->blocks[b].ref, 1))
    return;

  /* reset memory */
  FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)(c, b);

  /* place in free list */
  FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  FIO_LIST_NODE *n =
      (FIO_LIST_NODE *)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0);
  FIO_LIST_PUSH(&FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks, n);
  /* free chunk reference while in locked state */
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
  if (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.prev !=
      &FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks) {
    FIO_LIST_NODE *n = FIO_NAME(FIO_MEMORY_NAME, __mem_state)->blocks.prev;
    FIO_LIST_REMOVE(n);
    n->next = n->prev = NULL;
    c = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)((void *)n);
    fio_atomic_add_fetch(&c->ref, 1);
    p = (void *)n;
    b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, p);
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
  /* set block index to zero */
  b = 0;

done:
  FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  if (!p)
    return p;
  /* update block reference and allocation position */
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
  void *p = NULL;
  bytes = (bytes + ((1UL << FIO_MEMORY_ALIGN_LOG) - 1)) >> FIO_MEMORY_ALIGN_LOG;
  FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s) *a =
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_lock)();

  if (!a->block) {
    a->block = FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)();
    a->last_pos = 0;
  }
  for (;;) {
    if (!a->block)
      goto no_mem;
    void *const block = a->block;

    FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *const c =
        FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2chunk)(block);
    const size_t b = FIO_NAME(FIO_MEMORY_NAME, __mem_ptr2index)(c, block);

    /* add allocation reference */
    /* if we are the only thread holding a reference to this block - reset. */
    if (fio_atomic_add(&c->blocks[b].ref, 1) == 1 && c->blocks[b].pos) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)(c, b);
      FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(c, b);
      a->last_pos = 0;
    }

    /* enough space? allocate */
    if (c->blocks[b].pos + bytes < FIO_MEMORY_UNITS_PER_BLOCK) {
      /* a lucky realloc? */
      if (is_realloc &&
          is_realloc ==
              FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, a->last_pos)) {
        c->blocks[b].pos += bytes;
        fio_atomic_sub(&c->blocks[b].ref, 1); /* release reference added */
        FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
        return is_realloc;
      }
      p = FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, c->blocks[b].pos);
      a->last_pos = c->blocks[b].pos;
      c->blocks[b].pos += bytes;
      FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
      return p;
    }
    is_realloc = NULL;

    /*
     * allocate a new block before freeing the existing block
     * this prevents the last chunk from de-allocating and reallocating
     */
    a->block = FIO_NAME(FIO_MEMORY_NAME, __mem_block_new)();
    a->last_pos = 0;

    /* release allocation reference added */
    fio_atomic_sub(&c->blocks[b].ref, 1);
    /* release the reference held by the arena (allocator) */
    FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(block);
  }

no_mem:
  FIO_NAME(FIO_MEMORY_NAME, __mem_arena_unlock)(a);
  errno = ENOMEM;
  return p;
}

/* SublimeText marker */
void fio_____mem_slice_free___(void);
/** slice a block to allocate a set number of bytes. */
FIO_IFUNC void FIO_NAME(FIO_MEMORY_NAME, __mem_slice_free)(void *p) {
  FIO_NAME(FIO_MEMORY_NAME, __mem_block_free)(p);
}

/* *****************************************************************************
big block allocation / de-allocation
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

#if FIO_MEMORY_INITIALIZE_ALLOCATIONS
  /* zero out memory */
  if (b->pos >= (int32_t)(FIO_MEMORY_UNITS_PER_BIG_BLOCK - 10)) {
    /* zero out everything */
    FIO_MEMSET((void *)b, 0, FIO_MEMORY_SYS_ALLOCATION_SIZE);
  } else {
    /* zero out only the used part of the memory */
    FIO_MEMSET((void *)b,
               0,
               (((size_t)b->pos << FIO_MEMORY_ALIGN_LOG) +
                FIO_MEMORY_BIG_BLOCK_HEADER_SIZE));
  }
#else
#if defined(DEBUG) && DEBUG
  /* set all bytes to 0xAF to better catch initialization bugs */
  FIO_MEMSET((void *)b, 0xFA, FIO_MEMORY_SYS_ALLOCATION_SIZE);
#endif /* DEBUG */
  /* reset chunk header, which is always bigger than big_block header*/
  FIO_MEMSET((void *)b, 0, sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)));
  /* zero out possible block memory (if required) */
  for (size_t i = 0; i < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++i) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_block__reset_memory)
    ((FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)b, i);
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
#if FIO_MEMORY_CACHE_SLOTS
  /* lock for chunk de-allocation review () */
  FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->lock);
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_cache_or_dealloc)
  ((FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)b);
#else
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_dealloc)
  ((FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)b);
#endif
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
    goto no_mem;
  b->marker = FIO_MEMORY_BIG_BLOCK_MARKER;
  b->ref = 1;
  b->pos = 0;
  FIO_MEMORY_ON_BIG_BLOCK_SET(b);
  return b;
no_mem:
  errno = ENOMEM;
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
  void *p = NULL;
  bytes = (bytes + ((1UL << FIO_MEMORY_ALIGN_LOG) - 1)) >> FIO_MEMORY_ALIGN_LOG;
  for (;;) {
    FIO_MEMORY_LOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
    if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block =
          FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_new)();
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = 0;
    }

    if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block)
      goto done;
    FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s) *b =
        FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_block;

    /* are we the only thread holding a reference to this block... reset? */
    if (b->ref == 1 && b->pos) {
      FIO_NAME(FIO_MEMORY_NAME, __mem_big_block__reset_memory)(b);
      FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK(b, 0);
      b->marker = FIO_MEMORY_BIG_BLOCK_MARKER;
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = 0;
    }

    /* enough space? */
    if (b->pos + bytes < FIO_MEMORY_UNITS_PER_BIG_BLOCK) {
      /* a lucky realloc? */
      if (is_realloc &&
          is_realloc ==
              FIO_NAME(FIO_MEMORY_NAME, __mem_big2ptr)(
                  b,
                  FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos)) {
        b->pos += bytes;
        FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
        return is_realloc;
      }

      p = FIO_NAME(FIO_MEMORY_NAME, __mem_big2ptr)(b, b->pos);
      fio_atomic_add(&b->ref, 1); /* keep inside lock to enable reset */
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_last_pos = b->pos;
      b->pos += bytes;
      FIO_MEMORY_UNLOCK(FIO_NAME(FIO_MEMORY_NAME, __mem_state)->big_lock);
      return p;
    }

    is_realloc = NULL;
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
                   "\t* system allocation arenas:                 %zu arenas\n"
                   "\t* system allocation size:                   %zu bytes\n"
                   "\t* system allocation overhead (theoretical): %zu bytes\n"
                   "\t* system allocation overhead (actual):      %zu bytes\n"
                   "\t* cached system allocations (max):          %zu units\n"
                   "\t* memory block size:                        %zu bytes\n"
                   "\t* blocks per system allocation:             %zu blocks\n"
                   "\t* allocation units per block:               %zu units\n"
                   "\t* arena per-allocation limit:               %zu bytes\n"
                   "\t* local per-allocation limit (before mmap): %zu bytes\n"
                   "\t* malloc(0) pointer:                        %p\n"
                   "\t* always initializes memory  (zero-out):    %s\n"
                   "\t* " FIO_MEMORY_LOCK_NAME " locking system\n",
      (size_t)FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count,
      (size_t)FIO_MEMORY_SYS_ALLOCATION_SIZE,
      (size_t)FIO_MEMORY_HEADER_SIZE,
      (size_t)FIO_MEMORY_SYS_ALLOCATION_SIZE % (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_CACHE_SLOTS,
      (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_BLOCKS_PER_ALLOCATION,
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

#if FIO_MEMORY_ENABLE_BIG_ALLOC
  if ((is_realloc && size > (FIO_MEMORY_BIG_BLOCK_SIZE -
                             (FIO_MEMORY_BIG_BLOCK_HEADER_SIZE << 1))) ||
      (!is_realloc && size > FIO_MEMORY_ALLOC_LIMIT))
#else
  if (!is_realloc && size > FIO_MEMORY_ALLOC_LIMIT)
#endif
  {
#ifdef DEBUG
    FIO_LOG_WARNING(
        "unintended " FIO_MACRO2STR(
            FIO_NAME(FIO_MEMORY_NAME, mmap)) " allocation (slow): %zu bytes",
        FIO_MEM_BYTES2PAGES(size));
#endif
    p = FIO_NAME(FIO_MEMORY_NAME, mmap)(size);
    return p;
  }
  if (!FIO_NAME(FIO_MEMORY_NAME, __mem_state)) {
    FIO_NAME(FIO_MEMORY_NAME, __mem_state_setup)();
  }
#if FIO_MEMORY_ENABLE_BIG_ALLOC
  if ((is_realloc &&
       size > FIO_MEMORY_BLOCK_SIZE - (2 << FIO_MEMORY_ALIGN_LOG)) ||
      (!is_realloc && size > FIO_MEMORY_BLOCK_ALLOC_LIMIT)) {
    p = FIO_NAME(FIO_MEMORY_NAME, __mem_big_slice_new)(size, is_realloc);
    if (p && p != is_realloc) {
      FIO_MEMORY_ON_ALLOC_FUNC();
    }
    return p;
  }
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */

  p = FIO_NAME(FIO_MEMORY_NAME, __mem_slice_new)(size, is_realloc);
  if (p && p != is_realloc) {
    FIO_MEMORY_ON_ALLOC_FUNC();
  }
  return p;
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
  void *p = FIO_NAME(FIO_MEMORY_NAME, ___malloc)(size, NULL);
#if !FIO_MEMORY_INITIALIZE_ALLOCATIONS && defined(DEBUG) && DEBUG
  /* set all bytes to 0xAF to better catch initialization bugs */
  FIO_MEMSET(p, 0xFA, size);
#endif /* DEBUG dirtify */
  return p;
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
  FIO_MEMSET(p, 0, len);
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
  if (!c) {
    FIO_LOG_ERROR(FIO_MACRO2STR(
        FIO_NAME(FIO_MEMORY_NAME,
                 free)) " attempting to free a pointer owned by a NULL chunk.");
    return;
  }
  FIO_MEMORY_ON_FREE_FUNC();

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
  FIO_MEMSET(ptr,
             0,
             ((size_t)c->marker << FIO_MEM_PAGE_SIZE_LOG) -
                 FIO_MEMORY_ALIGN_SIZE);
  FIO_MEMORY_ON_CHUNK_FREE(c);
  FIO_MEM_SYS_FREE(c, (size_t)c->marker << FIO_MEM_PAGE_SIZE_LOG);
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
  const size_t new_len = FIO_MEM_BYTES2PAGES(new_size + FIO_MEMORY_ALIGN_SIZE);
  c = (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)FIO_MEM_SYS_REALLOC(
      c,
      (size_t)c->marker << FIO_MEM_PAGE_SIZE_LOG,
      new_len,
      FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    return NULL;
  c->marker = (uint32_t)(new_len >> FIO_MEM_PAGE_SIZE_LOG);
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
    FIO_ASSERT(c, "cannot reallocate a pointer with a NULL system allocation");

    register size_t max_len =
        ((uintptr_t)FIO_NAME(FIO_MEMORY_NAME, __mem_chunk2ptr)(c, b, 0) +
         FIO_MEMORY_BLOCK_SIZE) -
        ((uintptr_t)ptr);
#if FIO_MEMORY_ENABLE_BIG_ALLOC
    if (c->marker == FIO_MEMORY_BIG_BLOCK_MARKER) {
      /* extend max_len to accommodate possible length */
      max_len =
          ((uintptr_t)c + FIO_MEMORY_SYS_ALLOCATION_SIZE) - ((uintptr_t)ptr);
    } else
#endif /* FIO_MEMORY_ENABLE_BIG_ALLOC */
      if ((uintptr_t)(c) + FIO_MEMORY_ALIGN_SIZE == (uintptr_t)ptr &&
          c->marker) {
        if (new_size > FIO_MEMORY_ALLOC_LIMIT)
          return (
              mem = FIO_NAME(FIO_MEMORY_NAME, __mem_realloc2_big)(c, new_size));
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

  FIO_MEMCPY(mem,
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
  mem = FIO_NAME(FIO_MEMORY_NAME, ___malloc)(new_size, NULL);
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
  if (((uint64_t)pages >> (31 + FIO_MEM_PAGE_SIZE_LOG)))
    return NULL;
  FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *c =
      (FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s) *)
          FIO_MEM_SYS_ALLOC(pages, FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    goto no_mem;
  FIO_MEMORY_ON_ALLOC_FUNC();
  FIO_MEMORY_ON_CHUNK_ALLOC(c);
  c->marker = (uint32_t)(pages >> FIO_MEM_PAGE_SIZE_LOG);
  return (void *)((uintptr_t)c + FIO_MEMORY_ALIGN_SIZE);
no_mem:
  errno = ENOMEM;
  return NULL;
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





Memory Allocation - test - tests specific allocator settings





***************************************************************************** */
#ifdef FIO_TEST_ALL

#ifndef H___FIO_TEST_MEMORY_HELPERS_H
#define H___FIO_TEST_MEMORY_HELPERS_H

FIO_IFUNC void fio___memset_test_aligned(void *restrict dest_,
                                         uint64_t data,
                                         size_t bytes,
                                         const char *msg) {
  uint8_t *r = (uint8_t *)dest_;
  uint8_t *e_group = r + (bytes & (~(size_t)63ULL));
  uint64_t d[8] = {data, data, data, data, data, data, data, data};
  while (r < e_group) {
    fio_memcpy64(d, r);
    FIO_ASSERT(d[0] == data && d[1] == data && d[2] == data && d[3] == data &&
                   d[4] == data && d[5] == data && d[6] == data && d[7] == data,
               "%s memory data was overwritten",
               msg);
    r += 64;
  }
  fio_memcpy63x(d, r, bytes);
  FIO_ASSERT(d[0] == data && d[1] == data && d[2] == data && d[3] == data &&
                 d[4] == data && d[5] == data && d[6] == data && d[7] == data,
             "%s memory data was overwritten",
             msg);
  (void)msg; /* in case FIO_ASSERT is disabled */
}
#endif /* H___FIO_TEST_MEMORY_HELPERS_H */

#ifndef FIO_TEST_MULTI_THREADED
#define FIO_TEST_MULTI_THREADED 0
#endif

/* contention testing (multi-threaded) */
FIO_IFUNC void *FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio),
                              mem_tsk)(void *i_) {
  uintptr_t cycles = (uintptr_t)i_;
  const size_t test_byte_count =
      FIO_MEMORY_SYS_ALLOCATION_SIZE + (FIO_MEMORY_SYS_ALLOCATION_SIZE >> 1);
  uint64_t marker[2];
  do {
    marker[0] = fio_rand64();
    marker[1] = fio_rand64();
  } while (!marker[0] || !marker[1] || marker[0] == marker[1]);

  const size_t limit = (test_byte_count / cycles);
  char **ary = (char **)FIO_NAME(FIO_MEMORY_NAME, calloc)(sizeof(*ary), limit);
  const uintptr_t alignment_mask = (FIO_MEMORY_ALIGN_SIZE - 1);
  FIO_ASSERT(ary, "allocation failed for test container");
  for (size_t i = 0; i < limit; ++i) {
    if (1) {
      /* add some fragmentation */
      char *tmp = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(16);
      FIO_NAME(FIO_MEMORY_NAME, free)(tmp);
      FIO_ASSERT(tmp, "small allocation failed!");
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
    FIO_MEMSET(ary[i], 0, cycles);
    fio_xmask(ary[i], cycles, marker[i & 1]);
  }
  for (size_t i = 0; i < limit; ++i) {
    char *tmp = (char *)FIO_NAME(FIO_MEMORY_NAME,
                                 realloc2)(ary[i], (cycles << 1), (cycles));
    FIO_ASSERT(tmp, "re-allocation failed!");
    ary[i] = tmp;
    FIO_ASSERT(!((uintptr_t)ary[i] & alignment_mask),
               "allocation alignment error!");
    FIO_ASSERT(!FIO_MEMORY_INITIALIZE_ALLOCATIONS || !ary[i][(cycles)],
               "realloc2 copy overflow!");
    fio___memset_test_aligned(ary[i], marker[i & 1], (cycles), "realloc grow");
    tmp =
        (char *)FIO_NAME(FIO_MEMORY_NAME, realloc2)(ary[i], (cycles), (cycles));
    FIO_ASSERT(tmp, "re-allocation (shrinking) failed!");
    ary[i] = tmp;
    fio___memset_test_aligned(ary[i],
                              marker[i & 1],
                              (cycles),
                              "realloc shrink");
  }
  for (size_t i = 0; i < limit; ++i) {
    fio___memset_test_aligned(ary[i], marker[i & 1], (cycles), "mem review");
    FIO_NAME(FIO_MEMORY_NAME, free)(ary[i]);
    ary[i] = NULL;
  }

  uint64_t mark;
  void *old = &mark;
  mark = fio_risky_hash(&old, sizeof(mark), 0);

  for (int repeat_cycle_test = 0; repeat_cycle_test < 4; ++repeat_cycle_test) {
    for (size_t i = 0; i < limit - 4; i += 4) {
      if (ary[i])
        fio___memset_test_aligned(ary[i], mark, 16, "mark missing at ary[0]");
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i]);
      if (ary[i + 1])
        fio___memset_test_aligned(ary[i + 1],
                                  mark,
                                  cycles,
                                  "mark missing at ary[1]");
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + 1]);
      if (ary[i + 2])
        fio___memset_test_aligned(ary[i + 2],
                                  mark,
                                  cycles,
                                  "mark missing at ary[2]");
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + 2]);
      if (ary[i + 3])
        fio___memset_test_aligned(ary[i + 3],
                                  mark,
                                  cycles,
                                  "mark missing at ary[3]");
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + 3]);

      ary[i] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_MEMSET(ary[i], 0, cycles);
      fio_xmask(ary[i], cycles, mark);

      ary[i + 1] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + 1]);
      ary[i + 1] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_MEMSET(ary[i + 1], 0, cycles);
      fio_xmask(ary[i + 1], cycles, mark);

      ary[i + 2] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_MEMSET(ary[i + 2], 0, cycles);
      fio_xmask(ary[i + 2], cycles, mark);
      ary[i + 2] = (char *)FIO_NAME(FIO_MEMORY_NAME,
                                    realloc2)(ary[i + 2], cycles * 2, cycles);

      ary[i + 3] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + 3]);
      ary[i + 3] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
      FIO_MEMSET(ary[i + 3], 0, cycles);
      fio_xmask(ary[i + 3], cycles, mark);
      ary[i + 3] = (char *)FIO_NAME(FIO_MEMORY_NAME,
                                    realloc2)(ary[i + 3], cycles * 2, cycles);

      for (int b = 0; b < 4; ++b) {
        for (size_t pos = 0; pos < (cycles / sizeof(uint64_t)); ++pos) {
          FIO_ASSERT(((uint64_t *)(ary[i + b]))[pos] == mark,
                     "memory mark corrupted at test ptr %zu",
                     i + b);
        }
      }
      for (int b = 1; b < 4; ++b) {
        FIO_NAME(FIO_MEMORY_NAME, free)(ary[b]);
        ary[b] = NULL;
        FIO_NAME(FIO_MEMORY_NAME, free)(ary[i + b]);
      }
      for (int b = 1; b < 4; ++b) {
        ary[i + b] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
        if (i) {
          ary[b] = (char *)FIO_NAME(FIO_MEMORY_NAME, malloc)(cycles);
          FIO_MEMSET(ary[b], 0, cycles);
          fio_xmask(ary[b], cycles, mark);
        }
        FIO_MEMSET(ary[i + b], 0, cycles);
        fio_xmask(ary[i + b], cycles, mark);
      }

      for (int b = 0; b < 4; ++b) {
        for (size_t pos = 0; pos < (cycles / sizeof(uint64_t)); ++pos) {
          FIO_ASSERT(((uint64_t *)(ary[b]))[pos] == mark,
                     "memory mark corrupted at test ptr %zu",
                     i + b);
          FIO_ASSERT(((uint64_t *)(ary[i + b]))[pos] == mark,
                     "memory mark corrupted at test ptr %zu",
                     i + b);
        }
      }
    }
  }
  for (size_t i = 0; i < limit; ++i) {
    FIO_NAME(FIO_MEMORY_NAME, free)(ary[i]);
    ary[i] = NULL;
  }

  FIO_NAME(FIO_MEMORY_NAME, free)(ary);
  return NULL;
}

/* main test function */
FIO_SFUNC void FIO_NAME_TEST(FIO_NAME(stl, FIO_MEMORY_NAME), mem)(void) {
  fprintf(stderr,
          "* Testing core memory allocator " FIO_MACRO2STR(
              FIO_NAME(FIO_MEMORY_NAME, malloc)) ".\n");

  const uintptr_t alignment_mask = (FIO_MEMORY_ALIGN_SIZE - 1);
  fprintf(stderr,
          "* Validating allocation alignment on %zu byte border.\n",
          (size_t)(FIO_MEMORY_ALIGN_SIZE));
  for (size_t i = 0; i < alignment_mask; ++i) {
    void *p = FIO_NAME(FIO_MEMORY_NAME, malloc)(i);
    FIO_ASSERT(!((uintptr_t)p & alignment_mask),
               "allocation alignment error allocating %zu bytes!",
               i);
    FIO_NAME(FIO_MEMORY_NAME, free)(p);
  }
  const size_t thread_count =
      FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count +
      (FIO_NAME(FIO_MEMORY_NAME, __mem_state)->arena_count >> 1);

  for (uintptr_t cycles = 16; cycles <= (FIO_MEMORY_ALLOC_LIMIT); cycles *= 2) {
    fprintf(stderr,
            "* Testing %zu byte allocation blocks, single threaded.\n",
            (size_t)(cycles));
    FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio), mem_tsk)((void *)cycles);
  }

  if (FIO_TEST_MULTI_THREADED) {

    for (uintptr_t cycles = 16; cycles <= (FIO_MEMORY_ALLOC_LIMIT);
         cycles *= 2) {
#if _MSC_VER
      fio_thread_t threads[(FIO_MEMORY_ARENA_COUNT_MAX + 1) * 2];
      FIO_ASSERT(((FIO_MEMORY_ARENA_COUNT_MAX + 1) * 2) >= thread_count,
                 "Please use CLang or GCC to test this memory allocator");
#else
      fio_thread_t threads[thread_count];
#endif

      fprintf(stderr,
              "* Testing %zu byte allocation blocks, using %zu threads.\n",
              (size_t)(cycles),
              (thread_count + 1));
      for (size_t i = 0; i < thread_count; ++i) {
        if (fio_thread_create(
                threads + i,
                FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio), mem_tsk),
                (void *)cycles)) {
          abort();
        }
      }
      FIO_NAME_TEST(FIO_NAME(FIO_MEMORY_NAME, fio), mem_tsk)((void *)cycles);
      for (size_t i = 0; i < thread_count; ++i) {
        fio_thread_join(threads + i);
      }
    }
  }
  fprintf(stderr,
          "* Re-validating allocation alignment on %zu byte border.\n",
          (size_t)(FIO_MEMORY_ALIGN_SIZE));
  for (size_t i = 0; i < alignment_mask; ++i) {
    void *p = FIO_NAME(FIO_MEMORY_NAME, malloc)(i);
    FIO_ASSERT(!((uintptr_t)p & alignment_mask),
               "allocation alignment error allocating %zu bytes!",
               i);
    FIO_NAME(FIO_MEMORY_NAME, free)(p);
  }

#if DEBUG
  FIO_NAME(FIO_MEMORY_NAME, malloc_print_state)();
  FIO_NAME(FIO_MEMORY_NAME, __mem_state_cleanup)(NULL);
#endif /* DEBUG */
}
#endif /* FIO_TEST_ALL */

/* *****************************************************************************
Memory pool cleanup
***************************************************************************** */
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
#undef FIO_MEMORY_ON_BLOCK_RESET_IN_LOCK
#undef FIO_MEMORY_ON_BIG_BLOCK_SET
#undef FIO_MEMORY_ON_BIG_BLOCK_UNSET
#undef FIO_MEMORY_ON_ALLOC_FUNC
#undef FIO_MEMORY_ON_FREE_FUNC
#undef FIO_MEMORY_PRINT_STATS_END

#undef FIO_MEMORY_ARENA_COUNT
#undef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
#undef FIO_MEMORY_CACHE_SLOTS
#undef FIO_MEMORY_ALIGN_LOG
#undef FIO_MEMORY_INITIALIZE_ALLOCATIONS
#undef FIO_MEMORY_USE_THREAD_MUTEX
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
