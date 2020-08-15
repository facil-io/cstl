/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#define FIO_MALLOC                  /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                          Custom Memory Allocation
                  Memory allocator for short lived objects






The allocator has multiple allocation "arenas" that elevate thread contention.

Arenas are "sticky", which means that each thread will first try to access it's
previoiusly accessed arena. Only if the previously accessed arena is busy will
the thread attempt to access a different arena.

Each Arena has a "block" of memory which is used for memory allocation.

Once the user frees all of a block's memory, the block will be placed in an
"available" list until the memory can be returned to the system (or cached).

Allocations
==============

Memory "chunks" are allocated from the system (by default through `mmap`).

Chunks are divided into "blocks" that are used for actual memory allocation.

Both chunks and blocks are always allocated on a specific memory boundary where
a specific amount of least significant bits are always zero.

Allocations are performed by "slicing" the block and passing the pointer to the
user. No metadata is attached to the allocated "slices" since both the block and
the chunk (where metadata is stored) can be easily inferred using a bit mask.

Once a block was fully utiliized, the allocator will no longer keep a record of
that block until the user frees all it's memory.

It should be noted that the first block header and the chunk header occupy the
same 16 bytes (interleaved).

Deallocations
==============

Deallocations are performed by atomically marking a deallocation in the block.

Once all the block slices were deallocated, and assuming the block isn't the
active allocation block, that block is marked as free and placed in an
"available" list.

If all the blocks in a chunk were marked as available, all it's blocks are
removed from the list and the chunk will be returned to the system (or cached).

Cons
==============

1. By avoiding a detailed "free list", there is no way to re-use parts of a
"dirty" block. This increases the price of fragmentation may be the whole block
that contains withheld memory.

2. Large overall memory allocation-deallocation cycles (bigger than (cache +
arena) * sys_alloc) will result in a lot of system calls, as the allocator will
return memory to the system before requesting it back and using it.

This approach shouldn't be used for objects with mixed lifetime, as it will
result in fragmentation where the cost of a single allocation (i.e., a
persistent 16 byte allocation) could be a whole block that can't be utilized.

Pros
==============

1. Consecative allocations in the same thread are likely to be from the same
block, resulting in improved memory locality.

2. The use of multiple arenas for allocation minimizes thread contention during
allocation.

3. The use of atomic operations for deallocations minimizes thread contention
until a whole block is placed in the available list or a chunk is ready to be
freed.

4. The allocator's overrhead for even the smallest of allocations is fairly low,
in addition to a set memory consumption (~1 page for core data + 16 bytes for
`fio_malloc(0)` + 8 bytes per thread), only 16 bytes per block are
used by the allocator (%0.006), the rest is available for the application.

***************************************************************************** */
#if defined(FIO_MALLOC) && !defined(H___FIO_MALLOC_H)
#define H___FIO_MALLOC_H

/* *****************************************************************************
Memory Allocation - API
***************************************************************************** */

/* inform the compiler that the returned value is aligned on 16 byte marker */
#if __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#define FIO_ALIGN __attribute__((assume_aligned(16)))
#define FIO_ALIGN_NEW __attribute__((malloc, assume_aligned(16)))
#else
#define FIO_ALIGN
#define FIO_ALIGN_NEW
#endif /* (__clang__ || __GNUC__)... */

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 *
 * `fio_malloc` promises a best attempt at providing locality between
 * consecutive calls, but locality can't be guaranteed.
 */
SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size);

/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count);

/** Frees memory that was allocated using this library. */
SFUNC void fio_free(void *ptr);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_ALIGN fio_realloc2(void *ptr, size_t new_size, size_t copy_len);

/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `fio_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_ALIGN_NEW fio_mmap(size_t size);

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
SFUNC void fio_malloc_after_fork(void);

/* *****************************************************************************
Memory Allocation - configuration macros

NOTE: most configuration values should be a power of 2 or a logarithmic value.
***************************************************************************** */

#ifndef FIO_MEMORY_ARENA_COUNT_MAX
/**
 * The maximum number of memory arenas to initialize.
 *
 * When 0 - maximum detected cores X 2.
 */
#define FIO_MEMORY_ARENA_COUNT_MAX 64
#endif

#ifndef FIO_MEMORY_ARENA_COUNT_DEFAULT
/**
 * The default number of memory arenas to initialize when CPU core detection
 * fails.
 *
 * Normally, fio_malloc tries to initialize as many memory allocation arenas as
 * the number of CPU cores. This value will only be used if core detection isn't
 * available or fails.
 */
#define FIO_MEMORY_ARENA_COUNT_DEFAULT 5
#endif

#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/**
 * The logarithmic size of a single allocatiion "chunk" (16 blocks).
 *
 * Limited to >=17 and <=24.
 *
 * By default 22, whch is a ~4Mb allocation per system call.
 *
 * A block is always 1/16 of this value. A default block size is ~256Kb.
 */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 23
#endif

#ifndef FIO_MEMORY_BLOCK_SIZE_LOG
/** The each system allocation is divided into blocks, by default: 256Kb */
#define FIO_MEMORY_BLOCK_SIZE_LOG 18
#endif

#ifndef FIO_MEMORY_BLOCK_ALLOC_LIMIT
/**
 * The maximum allocation size, after which a "big block" allocation will be
 * used.
 *
 * Defaults to 6.25% of the block (16KB), after which big-blocks are used.
 */
#define FIO_MEMORY_BLOCK_ALLOC_LIMIT (1UL << (FIO_MEMORY_BLOCK_SIZE_LOG - 4))
#endif

#ifndef FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT
/**
 * The maximum allocation size, after which `mmap` will be called instead of the
 * facil.io allocator.
 *
 * Defaults to 6.25% of the big-block/chunk (0.5Mb), after which `mmap` is used
 * instead.
 */
// #define FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT FIO_MEMORY_BLOCK_ALLOC_LIMIT
#define FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT                                       \
  (1UL << (FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG - 4))
#endif

#ifndef FIO_MEMORY_CACHE_SLOTS
/**
 * The number of memory "chunks" / "blocks" to cache even if they are not in
 * use.
 *
 * This is the number of chunks that won't be returned to the system even though
 * they aren't assigned to any arena.
 *
 * This is ALSO the number of blocks that will be cached for lockless access
 * after being partially freed - their chunks not marked as free until they are
 * pushed out of the lockless queue.
 */
#define FIO_MEMORY_CACHE_SLOTS 8
#endif

#ifndef FIO_MEMORY_USE_PTHREAD_MUTEX
/** Uses a pthread mutex instead of a spinlock. */
#define FIO_MEMORY_USE_PTHREAD_MUTEX 1
#endif

/* *****************************************************************************
Memory Allocation - configuration value - results and constants
***************************************************************************** */

/* Helper macro, don't change this */
#undef FIO_MEMORY_BLOCKS_PER_ALLOCATION
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION                                       \
  (1UL << (FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG - FIO_MEMORY_BLOCK_SIZE_LOG))

/* Helper macro, don't change this */
#undef FIO_MEMORY_SYS_ALLOCATION_SIZE
#define FIO_MEMORY_SYS_ALLOCATION_SIZE                                         \
  (1UL << FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG)

#undef FIO_MEMORY_BLOCK_SIZE
/** The resulting memory block size = chunk size / 16, defaults to 256Kb */
#define FIO_MEMORY_BLOCK_SIZE (1UL << FIO_MEMORY_BLOCK_SIZE_LOG)

/* *****************************************************************************
Memory Allocation - redefine default allocation macros
***************************************************************************** */
#undef FIO_MEM_CALLOC
/** Allocates size X units of bytes, where all bytes equal zero. */
#define FIO_MEM_CALLOC(size, units) fio_calloc((size), (units))

#undef FIO_MEM_REALLOC
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))

#undef FIO_MEM_FREE
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))

#undef FIO_MEM_INTERNAL_MALLOC
#define FIO_MEM_INTERNAL_MALLOC 1

/* *****************************************************************************





Memory Allocation - Implementation





***************************************************************************** */

/* *****************************************************************************
Memory Allocation - forced bypass
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) && defined(FIO_MALLOC_FORCE_SYSTEM)

SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size) { return calloc(size, 1); }

SFUNC void *FIO_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count) {
  return calloc(size_per_unit, unit_count);
}

SFUNC void fio_free(void *ptr) { free(ptr); }

SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size) {
  return realloc(ptr, new_size);
}

SFUNC void *FIO_ALIGN fio_realloc2(void *ptr,
                                   size_t new_size,
                                   size_t copy_len) {
  return realloc(ptr, new_size);
  (void)copy_len;
}

SFUNC void *FIO_ALIGN_NEW fio_mmap(size_t size) { return calloc(size, 1); }

SFUNC void fio_malloc_after_fork(void) {}

/* *****************************************************************************
Memory Allocation - custom implementation
***************************************************************************** */
#elif defined(FIO_EXTERN_COMPLETE)

#if FIO_HAVE_UNIX_TOOLS
#include <unistd.h>
#endif /* H___FIO_UNIX_TOOLS4STR_INCLUDED_H */

/* *****************************************************************************
Aligned memory copying
***************************************************************************** */
#define FIO_MEMCOPY_FIO_IFUNC_ALIGNED(type, size)                              \
  FIO_IFUNC void fio___memcpy_##size##b(                                       \
      void *restrict dest_, const void *restrict src_, size_t units) {         \
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

/** Copies 16 byte `units` of size_t aligned memory blocks */
FIO_IFUNC void
fio___memcpy_16byte(void *dest_, const void *src_, size_t units) {
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF /* 64 bit size_t */
  fio___memcpy_8b(dest_, src_, units << 1);
#elif SIZE_MAX == 0xFFFFFFFF       /* 32 bit size_t */
  fio___memcpy_4b(dest_, src_, units << 2);
#else                              /* unknown... assume 16 bit? */
  fio___memcpy_2b(dest_, src_, units << 3);
#endif                             /* SIZE_MAX */
}

/* *****************************************************************************
Override the system's malloc functions if required
***************************************************************************** */
#ifdef FIO_MALLOC_OVERRIDE_SYSTEM
void *malloc(size_t size) { return fio_malloc(size); }
void *calloc(size_t size, size_t count) { return fio_calloc(size, count); }
void free(void *ptr) { fio_free(ptr); }
void *realloc(void *ptr, size_t new_size) { return fio_realloc(ptr, new_size); }
#endif /* FIO_MALLOC_OVERRIDE_SYSTEM */

/* *****************************************************************************
Big memory allocation macros and helpers (page allocation / mmap)
***************************************************************************** */
#ifndef FIO_MEM_PAGE_ALLOC

#ifndef FIO_MEM_PAGE_SIZE_LOG
#define FIO_MEM_PAGE_SIZE_LOG 12 /* 4096 bytes per page */
#endif                           /* FIO_MEM_PAGE_SIZE_LOG */

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
      result = FIO_MEM_PAGE_ALLOC_def_func(
          new_pages, alignment_log); /* allocate new memory */
      if (!result) {
        return (void *)NULL;
      }
      fio___memcpy_16byte(result, mem, prev_len >> 4); /* copy data */
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

#define FIO_MEM_PAGE_ALLOC(pages, alignment_log)                               \
  FIO_MEM_PAGE_ALLOC_def_func((pages), (alignment_log))
#define FIO_MEM_PAGE_REALLOC(ptr, old_pages, new_pages, alignment_log)         \
  FIO_MEM_PAGE_REALLOC_def_func(                                               \
      (ptr), (old_pages), (new_pages), (alignment_log))
#define FIO_MEM_PAGE_FREE(ptr, pages) FIO_MEM_PAGE_FREE_def_func((ptr), (pages))

#define FIO_MEM_BYTES2PAGES(size)                                              \
  (((size) + ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)) >> (FIO_MEM_PAGE_SIZE_LOG))

#endif /* FIO_MEM_PAGE_ALLOC */

/* *****************************************************************************
Allocator debugging helpers
***************************************************************************** */

#if DEBUG
/* maximum block allocation count. */
static size_t fio___mem_chunk_count_max;
/* current block allocation count. */
static size_t fio___mem_chunk_count;

#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY SYS-ALLOC - retrieved %p from system", ptr);        \
    fio_atomic_add(&fio___mem_chunk_count, 1);                                 \
    if (fio___mem_chunk_count > fio___mem_chunk_count_max)                     \
      fio___mem_chunk_count_max = fio___mem_chunk_count;                       \
  } while (0)
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)                                          \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY SYS-DEALLOC- returned %p to system", ptr);          \
    fio_atomic_sub_fetch(&fio___mem_chunk_count, 1);                           \
  } while (0)
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)                                         \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-DEALLOC placed %p in cache", ptr);            \
  } while (0);
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)                                       \
  do {                                                                         \
    FIO_LOG_DEBUG2("MEMORY CACHE-ALLOC retrieved %p from cache", ptr);         \
  } while (0);
#define FIO_MEMORY_PRINT_STATS()                                               \
  FIO_LOG_INFO(                                                                \
      "(fio) Total memory chunks allocated before cleanup %zu\n"               \
      "       Maximum memory blocks allocated at a single time %zu\n",         \
      fio___mem_chunk_count,                                                   \
      fio___mem_chunk_count_max)
#define FIO_MEMORY_PRINT_STATS_END()                                           \
  do {                                                                         \
    if (fio___mem_chunk_count) {                                               \
      FIO_LOG_ERROR("(fio) Total memory chunks allocated "                     \
                    "after cleanup (POSSIBLE LEAKS): %zu\n",                   \
                    fio___mem_chunk_count);                                    \
    } else {                                                                   \
      FIO_LOG_INFO("(fio) Total memory chunks allocated after cleanup: %zu\n", \
                   fio___mem_chunk_count);                                     \
    }                                                                          \
  } while (0)
#else /* DEBUG */
#define FIO_MEMORY_ON_CHUNK_ALLOC(ptr)
#define FIO_MEMORY_ON_CHUNK_FREE(ptr)
#define FIO_MEMORY_ON_CHUNK_CACHE(ptr)
#define FIO_MEMORY_ON_CHUNK_UNCACHE(ptr)
#define FIO_MEMORY_PRINT_STATS()
#define FIO_MEMORY_PRINT_STATS_END()
#endif /* DEBUG */

/* *****************************************************************************
Lock type choice
***************************************************************************** */
#if defined(FIO_MEMORY_USE_PTHREAD_MUTEX) && FIO_MEMORY_USE_PTHREAD_MUTEX
#include "pthread.h"
#define FIO_MEMORY_LOCK_TYPE pthread_mutex_t
#define FIO_MEMORY_LOCK_TYPE_INIT (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER
#define FIO_MEMORY_TRYLOCK(lock) pthread_mutex_trylock(&(lock))
#define FIO_MEMORY_LOCK(lock) pthread_mutex_lock(&(lock))
#define FIO_MEMORY_UNLOCK(lock) pthread_mutex_unlock(&(lock))
#else
#define FIO_MEMORY_LOCK_TYPE fio_lock_i
#define FIO_MEMORY_LOCK_TYPE_INIT FIO_LOCK_INIT
#define FIO_MEMORY_TRYLOCK(lock) fio_trylock(&(lock))
#define FIO_MEMORY_LOCK(lock) fio_lock(&(lock))
#define FIO_MEMORY_UNLOCK(lock) fio_unlock(&(lock))
#endif
/* *****************************************************************************
Memory Allocator Types
***************************************************************************** */

#ifndef FIO_LOCK_INIT
#define FIO_ATOMIC
#include __FILE__
#endif

typedef struct fio___mem_block_s fio___mem_block_s;
typedef struct fio___mem_big_block_s fio___mem_big_block_s;
typedef struct fio___mem_chunk_s fio___mem_chunk_s;
typedef struct fio___mem_arena_s fio___mem_arena_s;

/* must consume exactly 16 bytes for allocation alignment - overlays chunk */
struct fio___mem_block_s {
  uint32_t marker;
  uint32_t ref;
  uint32_t pos;
  uint32_t reserved;
};

/* must consume exactly 16 bytes for allocation alignment - overlays chunk */
struct fio___mem_big_block_s {
  uint32_t marker;
  uint32_t ref;
  uint32_t pos;
  uint32_t reserved;
};

/* must consume exactly 16 bytes for allocation alignment - overlays block */
struct fio___mem_chunk_s {
  /** if marker is non-zero, it contains the number of pages allocated */
  uint32_t marker;
  int32_t reserved[2];
  uint32_t ref;
};

/* must consume exactly 16 bytes for allocation alignment - overlays block */
struct fio___mem_arena_s {
  fio___mem_big_block_s *big;
  fio___mem_block_s *block;
  FIO_MEMORY_LOCK_TYPE lock;
};

/* *****************************************************************************
Memory Allocator State Containers
***************************************************************************** */

struct fio___mem_state_s {
  size_t cores;
#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
  struct {
    fio___mem_chunk_s *store[FIO_MEMORY_CACHE_SLOTS];
    size_t pos;
  } cache;
  fio___mem_block_s *bstore[FIO_MEMORY_CACHE_SLOTS];
#endif
  fio___list_node_s available;
  FIO_MEMORY_LOCK_TYPE
  lock; /* locks the cache and the available block linked list. */
  fio___mem_arena_s a[];
} *fio___mem_state = NULL;

#undef FIO_MEMORY___STATE_SIZE
#define FIO_MEMORY___STATE_SIZE(cores)                                         \
  FIO_MEM_BYTES2PAGES(                                                         \
      (sizeof(*fio___mem_state) + (sizeof(fio___mem_arena_s *) * cores)))

/* *****************************************************************************
Helpers for system allocation / deallocation of chunks.
***************************************************************************** */

FIO_IFUNC void fio___mem_chunk_dealloc(fio___mem_chunk_s *c) {
  FIO_MEM_PAGE_FREE(
      c, (1UL << (FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG - FIO_MEM_PAGE_SIZE_LOG)));
  FIO_MEMORY_ON_CHUNK_FREE(c);
}

FIO_IFUNC fio___mem_chunk_s *fio___mem_chunk_alloc(void) {
  fio___mem_chunk_s *c = (fio___mem_chunk_s *)FIO_MEM_PAGE_ALLOC(
      (1UL << (FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG - FIO_MEM_PAGE_SIZE_LOG)),
      FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  FIO_MEMORY_ON_CHUNK_ALLOC(c);
  return c;
}

/* *****************************************************************************
Helpers for pointer mathematics
***************************************************************************** */

FIO_IFUNC fio___mem_chunk_s *fio___mem_ptr2chunk(void *p) {
  /* use a bit mask to find the original chunk, adjust for header offset */
  fio___mem_chunk_s *c = FIO_PTR_MATH_RMASK(
      fio___mem_chunk_s, ((uintptr_t)p), FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  return c;
}

FIO_IFUNC fio___mem_block_s *fio___mem_ptr2block(void *p) {
  /* use a bit mask to find the offset of the block
   *  + adjust for the 16 byte chunk header offset
   */
  fio___mem_block_s *b = FIO_PTR_MATH_RMASK(
      fio___mem_block_s, ((uintptr_t)p), (FIO_MEMORY_BLOCK_SIZE_LOG));
  return b;
}

/* *****************************************************************************
Dirty list push / pop / remove
***************************************************************************** */

FIO_IFUNC void fio___mem_list___push_node(fio___list_node_s *restrict head,
                                          fio___list_node_s *restrict n) {
  // FIO_LOG_DEBUG("available block list pushing node %p", (void *)n);
  if (!n || n->next || n->prev)
    return; /* if non-zero, the  node is in use */
  n->prev = head->prev;
  n->next = head;
  head->prev->next = n;
  head->prev = n;
}
FIO_IFUNC void fio___mem_list_remove_node(fio___list_node_s *n) {
  // FIO_LOG_DEBUG("available block list popping node %p", (void *)n);
  if (!n || !n->next || !n->prev) {
    FIO_LOG_FATAL("memory allocator corruption? refused to remove node at %p "
                  "where next = %p prev = %p",
                  (void *)n,
                  (void *)(n ? n->next : n),
                  (void *)(n ? n->prev : n));
    abort();
    return; /* node isn't in list */
  }
  n->prev->next = n->next;
  n->next->prev = n->prev;
  n->next = n->prev = NULL;
}

FIO_IFUNC fio___list_node_s *fio___mem_block2node(fio___mem_block_s *b) {
  return (fio___list_node_s *)(b + 1);
}

FIO_IFUNC fio___mem_block_s *fio___mem_node2block(fio___list_node_s *n) {
  return ((fio___mem_block_s *)n) - 1;
}

FIO_IFUNC void fio___mem_list_push(fio___list_node_s *head,
                                   fio___mem_block_s *b) {
  fio___mem_list___push_node(head, fio___mem_block2node(b));
}

FIO_IFUNC fio___mem_block_s *fio___mem_list_pop(fio___list_node_s *head) {
  fio___mem_block_s *b = NULL;
  if (!head || head->next == head->prev)
    return b;
  fio___list_node_s *n = head->prev;
  fio___mem_list_remove_node(n);
  b = fio___mem_node2block(n);
  return b;
}

/** removes a chunk from the dirty list, returns -1 on failure. */
FIO_IFUNC void fio___mem_list_remove(fio___list_node_s *head,
                                     fio___mem_chunk_s *c) {
  if (!c || !head)
    return;
  if (head->next == head)
    return;
  for (size_t i = 0; i < FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++i) {
    fio___list_node_s *n = fio___mem_block2node(
        (fio___mem_block_s *)((uintptr_t)c + (FIO_MEMORY_BLOCK_SIZE * i)));
    fio___mem_list_remove_node(n);
  }
}

/* *****************************************************************************
Arena selection / locking
***************************************************************************** */

static __thread size_t fio___mem_arena_index = 0;

fio___mem_arena_s *fio___mem_arena_lock(void) {
  size_t a = fio___mem_arena_index;
  for (;;) {
    for (size_t i = 0; i < fio___mem_state->cores; ++i) {
      if (a >= fio___mem_state->cores)
        a = 0;
      if (!FIO_MEMORY_TRYLOCK(fio___mem_state->a[a].lock)) {
        fio___mem_arena_index = a;
        return &fio___mem_state->a[a];
      }
      ++a;
    }
    FIO_THREAD_RESCHEDULE();
  }
}

void fio___mem_arena_unlock(fio___mem_arena_s *a) {
  FIO_MEMORY_UNLOCK(a->lock);
}

/* *****************************************************************************
Chunk allocation / deallocation
***************************************************************************** */

/* seeks the cache first, then allocates from the system*/
FIO_SFUNC fio___mem_chunk_s *fio___mem_chunk_new(void) {
  fio___mem_chunk_s *c = NULL;

#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
  if (fio___mem_state->cache.pos) {
    c = fio___mem_state->cache.store[--fio___mem_state->cache.pos];
    FIO_MEMORY_ON_CHUNK_UNCACHE(c);
  }
#endif
  if (!c) {
    c = fio___mem_chunk_alloc();
  }
  return c;
}

FIO_SFUNC void fio___mem_chunk_free(fio___mem_chunk_s *c, int has_blocks) {
  FIO_ASSERT(!c->marker,
             "memory allocator corruption, block header overwritten?\n\t%p "
             "marked with %p",
             (void *)c,
             (void *)(*(uintptr_t *)c));
  if (!c || fio_atomic_sub_fetch(&c->ref, 1))
    return;
  if (fio___mem_state) {
    if (has_blocks) {
      fio___mem_list_remove(&fio___mem_state->available, c);
    }
#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
    if (fio___mem_state->cache.pos < FIO_MEMORY_CACHE_SLOTS) {
      fio___mem_state->cache.store[fio___mem_state->cache.pos++] = c;
      FIO_MEMORY_ON_CHUNK_CACHE(c);
      return;
    }
#endif
  }
  fio___mem_chunk_dealloc(c);
}

/* *****************************************************************************
Block allocation / deallocation
***************************************************************************** */

/* seeks the cache first, then allocates from the system*/
FIO_SFUNC fio___mem_block_s *fio___mem_block_new(void) {
  fio___mem_block_s *b = NULL;
#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
  for (size_t i = 0; i < FIO_MEMORY_CACHE_SLOTS; ++i) {
    b = fio_atomic_exchange(fio___mem_state->bstore + i, b);
    if (b) {
      b->pos = 1; /* 1st unit is the block header */
      b->ref = 1; /* block reference */
      return b;
    }
  }
#endif

  FIO_MEMORY_LOCK(fio___mem_state->lock);
  b = fio___mem_list_pop(&fio___mem_state->available);
  if (!b) {
    fio___mem_chunk_s *c = fio___mem_chunk_new();
    if (!c)
      goto finish;

    for (size_t i = 0; i < (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1); ++i) {
      fio___mem_list_push(
          &fio___mem_state->available,
          (fio___mem_block_s *)((uintptr_t)c + (FIO_MEMORY_BLOCK_SIZE * i)));
    }
    b = (fio___mem_block_s *)((uintptr_t)c +
                              (FIO_MEMORY_BLOCK_SIZE *
                               (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1)));
  }

finish:
  if (b) {
    b->pos = 1; /* 1st unit is the block header */
    b->ref = 1; /* block reference */
    fio_atomic_add(&fio___mem_ptr2chunk(b)->ref, 1); /* mark chunk reference */
  }
  FIO_MEMORY_UNLOCK(fio___mem_state->lock);
  return b;
}

FIO_SFUNC void fio___mem_block_free(fio___mem_block_s *b) {
  if (!b)
    return;
  FIO_ASSERT(!b->marker,
             "memory allocator corruption, block header overwritten?\n\t%p "
             "marked with %p",
             (void *)b,
             (void *)(*(uintptr_t *)b));
  if (fio_atomic_sub_fetch(&b->ref, 1))
    return;
  memset(b + 1, 0, FIO_MEMORY_BLOCK_SIZE - 16);

#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
  for (size_t i = 0; i < FIO_MEMORY_CACHE_SLOTS; ++i) {
    b = fio_atomic_exchange(fio___mem_state->bstore + i, b);
    if (!b)
      return;
  }
#endif
  fio___mem_chunk_s *c = fio___mem_ptr2chunk(b);

  FIO_MEMORY_LOCK(fio___mem_state->lock);
  fio___mem_list_push(&fio___mem_state->available, b);
  fio___mem_chunk_free(c, 1);
  FIO_MEMORY_UNLOCK(fio___mem_state->lock);
}

/* *****************************************************************************
Slice allocation / deallocation (slicing the block)
***************************************************************************** */

FIO_IFUNC void *fio___mem_slice(fio___mem_block_s *b, size_t units) {
  /* Note: each unit is 16 bytes long */
  void *ret = NULL;
  if (!b || (b->pos + units >= (FIO_MEMORY_BLOCK_SIZE >> 4)))
    return ret;
  ret = (void *)(b + b->pos);
  b->pos += units;
  fio_atomic_add(&b->ref, 1);
  return ret;
}

FIO_IFUNC void fio___mem_slice_free(void *p) {
  fio___mem_block_s *b = fio___mem_ptr2block(p);
  fio___mem_block_free(b);
}

/* *****************************************************************************
Big Block allocation / deallocation (slicing the big block)
***************************************************************************** */

FIO_IFUNC fio___mem_big_block_s *fio___mem_big_block_new(void) {
  FIO_MEMORY_LOCK(fio___mem_state->lock);
  fio___mem_big_block_s *b = (fio___mem_big_block_s *)fio___mem_chunk_new();
  FIO_MEMORY_UNLOCK(fio___mem_state->lock);
  b->ref = 1;
  b->pos = 1;
  b->marker = ~(uint32_t)0;
  return b;
}
FIO_IFUNC void *fio___mem_big_slice(fio___mem_big_block_s *b, size_t units) {
  /* Note: each unit is 16 bytes long */
  void *ret = NULL;
  if (!b || (b->pos + units >=
             (FIO_MEMORY_SYS_ALLOCATION_SIZE >> FIO_MEM_PAGE_SIZE_LOG)))
    return ret;
  ret = (void *)((uintptr_t)(b + 1) +
                 ((uintptr_t)b->pos << FIO_MEM_PAGE_SIZE_LOG));
  b->pos += units;
  fio_atomic_add(&b->ref, 1);
  return ret;
}

FIO_IFUNC void fio___mem_big_slice_free(void *p) {
  fio___mem_big_block_s *b = (fio___mem_big_block_s *)fio___mem_ptr2chunk(p);
  if (!b)
    return;
  if (fio_atomic_sub_fetch(&b->ref, 1))
    return;
  memset(b, 0, FIO_MEMORY_SYS_ALLOCATION_SIZE);
  fio___mem_chunk_s *c = (fio___mem_chunk_s *)b;
  c->ref = 1;
  FIO_MEMORY_LOCK(fio___mem_state->lock);
  fio___mem_chunk_free(c, 0);
  FIO_MEMORY_UNLOCK(fio___mem_state->lock);
}

/* *****************************************************************************
Arena based allocations
***************************************************************************** */

FIO_IFUNC void *fio___mem_alloc_normal(size_t size) {
  void *r = NULL;
  /* convert bytes to units */
  size = (size + 15) >> 4;
  fio___mem_arena_s *a = fio___mem_arena_lock();
  if (!a->block)
    a->block = fio___mem_block_new();
  r = fio___mem_slice(a->block, size);
  if (!r)
    goto rotate;
  fio___mem_arena_unlock(a);
  return r;
rotate:
  // FIO_LOG_DEBUG2("ROTATING memory block / chunk");
  fio___mem_block_free(a->block);
  a->block = fio___mem_block_new();
  r = fio___mem_slice(a->block, size);
  fio___mem_arena_unlock(a);
  return r;
}

FIO_IFUNC void *fio___mem_alloc_big(size_t size) {
  void *r = NULL;
  /* convert bytes to units */
  size = FIO_MEM_BYTES2PAGES(size);
  fio___mem_arena_s *a = fio___mem_arena_lock();
  if (!a->big)
    a->big = fio___mem_big_block_new();
  r = fio___mem_big_slice(a->big, size);
  if (!r)
    goto rotate;
  fio___mem_arena_unlock(a);
  return r;
rotate:
  // FIO_LOG_DEBUG2("ROTATING memory block / chunk");
  fio___mem_big_slice_free(a->big);
  a->big = fio___mem_big_block_new();
  r = fio___mem_big_slice(a->big, size);
  fio___mem_arena_unlock(a);
  return r;
}

/* *****************************************************************************
Core allocator state Construction
***************************************************************************** */

FIO_SFUNC void __attribute__((constructor)) fio___mem_state_allocate(void) {
  if (fio___mem_state)
    return;

  FIO_ASSERT_DEBUG(sizeof(fio___mem_block_s) == 16,
                   "fio___mem_block_s size error");
  FIO_ASSERT_DEBUG(sizeof(fio___mem_chunk_s) == 16,
                   "fio___mem_chunk_s size error");

#ifdef _SC_NPROCESSORS_ONLN
  size_t cores = sysconf(_SC_NPROCESSORS_ONLN);
  if ((intptr_t)cores <= 0)
    cores = FIO_MEMORY_ARENA_COUNT_DEFAULT;
#else
#warning Dynamic CPU core count is unavailable - assuming FIO_MEMORY_ARENA_COUNT_DEFAULT cores.
  size_t cores = FIO_MEMORY_ARENA_COUNT_DEFAULT;
  if ((intptr_t)cores <= 0)
    cores = 1;
#endif /* _SC_NPROCESSORS_ONLN */
#if FIO_MEMORY_ARENA_COUNT_MAX
  if (cores >= FIO_MEMORY_ARENA_COUNT_MAX)
    cores = FIO_MEMORY_ARENA_COUNT_MAX;
#endif /* FIO_MEMORY_ARENA_COUNT_MAX */
  if ((intptr_t)cores <= 0)
    cores = 1;

  const size_t pages = FIO_MEMORY___STATE_SIZE(cores);
  fio___mem_state = (struct fio___mem_state_s *)FIO_MEM_PAGE_ALLOC(pages, 1);
  FIO_ASSERT_ALLOC(fio___mem_state);
  *fio___mem_state = (struct fio___mem_state_s){
      .cores = cores,
      .available.next = &fio___mem_state->available,
      .available.prev = &fio___mem_state->available,
      .lock = FIO_MEMORY_LOCK_TYPE_INIT,
  };
  for (size_t i = 0; i < cores; ++i) {
    fio___mem_state->a[i].lock = FIO_MEMORY_LOCK_TYPE_INIT;
  }

#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO(
      "facil.io memory allocation initialized:\n"
      "\t* %zu concurrent arenas (@%p).\n"
      "\t* %zu pages required for arenas (~%zu bytes).\n"
      "\t* system allocation size:                     %zu bytes\n"
      "\t* system allocation overhead:                 %zu bytes\n"
      "\t* memory block size (allocation slice / bin): %zu bytes\n"
      "\t* memory blocks per system allocation:        %zu blocks\n"
      "\t* memory block overhead:                      %zu bytes\n"
      "\t* allocator small allocation limit:           %zu bytes\n"
      "\t* allocator limit (revert to mmap):           %zu bytes\n",
      cores,
      (void *)fio___mem_state,
      (size_t)pages,
      (size_t)(pages << FIO_MEM_PAGE_SIZE_LOG),
      (size_t)(FIO_MEMORY_BLOCKS_PER_ALLOCATION * FIO_MEMORY_BLOCK_SIZE),
      sizeof(fio___mem_chunk_s),
      (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_BLOCKS_PER_ALLOCATION,
      sizeof(fio___mem_block_s),
      (size_t)FIO_MEMORY_BLOCK_ALLOC_LIMIT,
      (size_t)FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT);
#endif /* DEBUG */
}

FIO_SFUNC void fio___mem_state_deallocate(void) {
  if (!fio___mem_state)
    return;
  const size_t pages = FIO_MEMORY___STATE_SIZE(fio___mem_state->cores);
  FIO_MEM_PAGE_FREE(fio___mem_state, pages);
  fio___mem_state = (struct fio___mem_state_s *)NULL;
}

/* *****************************************************************************
Allocator Destruction
***************************************************************************** */

FIO_SFUNC void __attribute__((destructor)) fio___mem_destroy(void) {
  if (!fio___mem_state)
    return;
  FIO_MEMORY_PRINT_STATS();
  for (size_t i = 0; i < fio___mem_state->cores; ++i) {
    /* free all blocks in the arenas */
    /* this may place chunks in the cache */
    fio___mem_block_s *b = fio___mem_state->a[i].block;
    fio___mem_big_block_s *big = fio___mem_state->a[i].big;
    fio___mem_state->a[i].block = NULL;
    fio___mem_block_free(b);
    fio___mem_big_slice_free(big);
  }
#if defined(FIO_MEMORY_CACHE_SLOTS) && FIO_MEMORY_CACHE_SLOTS
  for (size_t i = 0; i < FIO_MEMORY_CACHE_SLOTS; ++i) {
    fio___mem_block_s *b =
        fio_atomic_exchange(fio___mem_state->bstore + i, NULL);
    if (b) {
      fio___mem_list_push(&fio___mem_state->available, b);
      fio___mem_chunk_free(fio___mem_ptr2chunk(b), 1);
    }
  }
  while (fio___mem_state->cache.pos) {
    fio___mem_chunk_s *c =
        fio___mem_state->cache.store[--fio___mem_state->cache.pos];
    fio___mem_chunk_dealloc(c);
  }
#endif
  fio___mem_state_deallocate();
  FIO_MEMORY_PRINT_STATS_END();
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("facil.io memory allocation cleanup complete.");
#endif /* DEBUG */
}

/* *****************************************************************************
Public facing API
***************************************************************************** */

/* Address returned when allocating 0 bytes ( fio_malloc(0) ) */
static long double fio___mem_on_malloc_zero;

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 *
 * `fio_malloc` promises a best attempt at providing locality between
 * consecutive calls, but locality can't be guaranteed.
 */
SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size) {
  if (!size)
    return &fio___mem_on_malloc_zero;
  if (!fio___mem_state)
    fio___mem_state_allocate();
  if (size <= FIO_MEMORY_BLOCK_ALLOC_LIMIT) {
    return fio___mem_alloc_normal(size);
  }
  if (size <= FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT) {
    return fio___mem_alloc_big(size);
  }
  return fio_mmap(size);
}

/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count) {
  return fio_malloc(size_per_unit * unit_count);
}

/** Frees memory that was allocated using this library. */
SFUNC void fio_free(void *ptr) {
  if (!ptr || ptr == &fio___mem_on_malloc_zero)
    return;
  fio___mem_chunk_s *c = fio___mem_ptr2chunk(ptr);
  /* big slice allocation? */
  if (c->marker == ~(uint32_t)0)
    goto big_free;
  /* if the allocation is after a marked chunk header it was fio_mmap */
  if ((uintptr_t)(c + 1) == (uintptr_t)ptr && c->marker)
    goto mmap_free;
  fio___mem_slice_free(ptr);
  return;
big_free:
  fio___mem_big_slice_free(ptr);
  return;
mmap_free:
  /* zero out memory before returning it to the system */
  memset(ptr, 0, (c->marker) - sizeof(*c));
  FIO_MEM_PAGE_FREE(c, (c->marker >> FIO_MEM_PAGE_SIZE_LOG));
  FIO_MEMORY_ON_CHUNK_FREE(c);
}

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size) {
  return fio_realloc2(ptr, new_size, new_size);
}

/**
 * Uses system page maps for reallocation.
 */
FIO_SFUNC void *fio_realloc2_big(fio___mem_chunk_s *c, size_t new_size) {
  const size_t new_page_len = FIO_MEM_BYTES2PAGES(new_size + sizeof(*c));
  c = (fio___mem_chunk_s *)FIO_MEM_PAGE_REALLOC(
      c,
      c->marker >> FIO_MEM_PAGE_SIZE_LOG,
      new_page_len,
      FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    return NULL;
  c->marker = new_page_len;
  return (void *)(c + 1);
}

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_ALIGN fio_realloc2(void *ptr,
                                   size_t new_size,
                                   size_t copy_len) {
  void *mem = NULL;
  if (!new_size) {
    fio_free(ptr);
    mem = (void *)&fio___mem_on_malloc_zero;
    return mem;
  }
  if (!ptr || ptr == (void *)&fio___mem_on_malloc_zero) {
    mem = fio_malloc(new_size);
    return mem;
  }

  { /* test for big-paged malloc and limit copy_len */
    register fio___mem_chunk_s *c = fio___mem_ptr2chunk(ptr);
    register fio___mem_block_s *const b = fio___mem_ptr2block(ptr);
    register size_t max_len =
        ((uintptr_t)b + FIO_MEMORY_BLOCK_SIZE) - ((uintptr_t)ptr);
    if (c->marker == ~(uint32_t)0) {
      /* big-slice reallocation */
      max_len =
          ((uintptr_t)c + FIO_MEMORY_SYS_ALLOCATION_SIZE) - ((uintptr_t)ptr);
    } else if ((uintptr_t)(c + 1) == (uintptr_t)ptr && c->marker) {
      if (new_size > FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT)
        return (mem = fio_realloc2_big(c, new_size));
      max_len = new_size; /* shrinking from mmap to allocator */
    }

    if (copy_len > max_len)
      copy_len = max_len;
    if (copy_len > new_size)
      copy_len = new_size;
  }

  mem = fio_malloc(new_size);
  if (!mem) {
    return mem;
  }
  fio___memcpy_16byte(mem, ptr, ((copy_len + 15) >> 4));
  fio_free(ptr);
  // zero out leftover bytes, if any.
  while (copy_len & 15) {
    ((uint8_t *)mem)[(copy_len++) & 15] = 0;
  }

  return mem;
}

/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `fio_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_ALIGN_NEW fio_mmap(size_t size) {
  if (!size)
    return &fio___mem_on_malloc_zero;
  size_t pages = FIO_MEM_BYTES2PAGES(size + sizeof(fio___mem_chunk_s));
  if (((uint64_t)pages >> 32))
    return NULL;
  fio___mem_chunk_s *c = (fio___mem_chunk_s *)FIO_MEM_PAGE_ALLOC(
      pages, FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  if (!c)
    return NULL;
  FIO_MEMORY_ON_CHUNK_ALLOC(c);
  c->marker = (uint32_t)pages;
  return (void *)(c + 1);
}

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
SFUNC void fio_malloc_after_fork(void) {
  if (!fio___mem_state)
    return;
  for (size_t i = 0; i < fio___mem_state->cores; ++i) {
    fio___mem_state->a[i].lock = FIO_MEMORY_LOCK_TYPE_INIT;
  }
  fio___mem_state->lock = FIO_MEMORY_LOCK_TYPE_INIT;
}

/* *****************************************************************************
Print Memory Allocator State (for debugging)
***************************************************************************** */

SFUNC void fio_malloc_print_state(void) {
  if (!fio___mem_state) {
    FIO_LOG_INFO("Printing facil.io allocator memory state: UNINITIALIZED!");
  } else {
    char buffer[FIO_LOG_LENGTH_LIMIT];
    size_t pos = 0;
    memcpy(buffer + pos, "Printing facil.io allocator memory state:\n", 42);
    pos += 42;
    pos += snprintf(buffer + pos,
                    FIO_LOG_LENGTH_LIMIT - pos,
                    "\tCores: %zu",
                    fio___mem_state->cores);
    for (size_t i = 0; i < fio___mem_state->cores; ++i) {
      size_t w;
      w = snprintf(buffer + pos,
                   FIO_LOG_LENGTH_LIMIT - pos,
                   "\n\t[%zu] block: %p",
                   i,
                   (void *)fio___mem_state->a[i].block);
      if (w + pos + 32 > FIO_LOG_LENGTH_LIMIT)
        goto flush;
      if (fio___mem_state->a[i].block) {
        w += snprintf(buffer + pos + w,
                      FIO_LOG_LENGTH_LIMIT - (pos + w),
                      "\n\t[%zu] block ref: %u"
                      "\n\t[%zu] block pos: %u",
                      i,
                      fio___mem_state->a[i].block->ref,
                      i,
                      fio___mem_state->a[i].block->pos);
        if (w + pos + 32 > FIO_LOG_LENGTH_LIMIT)
          goto flush;
      }
      pos += w;
      continue;
    flush:
      buffer[pos] = 0;
      FIO_LOG_INFO("%s", buffer);
      pos = 0;
      --i;
    }
    buffer[pos] = 0;
    FIO_LOG_INFO("%s", buffer);
  }
}
/* *****************************************************************************
Memory Allocation - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL
#ifdef FIO_MALLOC_FORCE_SYSTEM
FIO_SFUNC void FIO_NAME_TEST(stl, mem)(void) {
  fprintf(stderr, "* Custom memory allocator bypassed.\n");
}

#else /* FIO_MALLOC_FORCE_SYSTEM */

#include "pthread.h"

FIO_IFUNC void
fio___memset_aligned(void *restrict dest_, uint64_t data, size_t bytes) {
  uint64_t *dest = (uint64_t *)dest_;
  size_t units = bytes >> 3;
  while (units >= 16) {
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
    units -= 16;
  }
  switch (units) {
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

FIO_IFUNC void fio___memset_test_aligned(void *restrict dest_,
                                         uint64_t data,
                                         size_t bytes,
                                         const char *msg) {
  uint64_t *dest = (uint64_t *)dest_;
  size_t units = bytes >> 3;
  FIO_ASSERT(
      *(dest) = data, "%s memory data was overwritten (first 8 bytes)", msg);
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
    FIO_ASSERT(
        *(dest++) = data, "%s memory data was overwritten (last 8 bytes)", msg);
  }
}

FIO_IFUNC void *FIO_NAME_TEST(stl, mem_tsk)(void *i_) {
  uintptr_t cycles = (uintptr_t)i_;
  const size_t test_block_count = (2UL << FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG);
  uint64_t marker;
  do {
    marker = fio_rand64();
  } while (!marker);

  const size_t limit = (test_block_count / cycles);
  char **ary = (char **)fio_calloc(sizeof(*ary), limit);
  FIO_ASSERT(ary, "allocation failed for test container");
  for (size_t i = 0; i < limit; ++i) {
    ary[i] = (char *)fio_malloc(cycles);
    FIO_ASSERT(ary[i], "allocation failed!");
    {
      char *tmp;
      fio_free((tmp = (char *)fio_malloc(16))); /* add some fragmentation */
      FIO_ASSERT(tmp, "allocation failed!")
    }
    FIO_ASSERT(!ary[i][0], "allocated memory not zero");
    FIO_ASSERT(!ary[i][(cycles - 1)], "allocated memory not zero (end)");
    fio___memset_aligned(ary[i], marker, (cycles));
  }
  for (size_t i = 0; i < limit; ++i) {
    char *tmp = (char *)fio_realloc2(ary[i], (cycles << 1), (cycles));
    FIO_ASSERT(tmp, "re-allocation failed!")
    ary[i] = tmp;
    FIO_ASSERT(!ary[i][(cycles)], "fio_realloc2 copy overflow!");
    fio___memset_test_aligned(ary[i], marker, (cycles), "realloc grow");
    tmp = (char *)fio_realloc2(ary[i], (cycles), (cycles));
    FIO_ASSERT(tmp, "re-allocation (shrinking) failed!")
    ary[i] = tmp;
    fio___memset_test_aligned(ary[i], marker, (cycles), "realloc shrink");
  }
  for (size_t i = 0; i < limit; ++i) {
    fio___memset_test_aligned(ary[i], marker, (cycles), "mem review");
    fio_free(ary[i]);
  }
  fio_free(ary);
  return NULL;
}

FIO_SFUNC void FIO_NAME_TEST(stl, mem)(void) {
  fprintf(stderr, "* Testing core memory allocator (fio_malloc).\n");
  FIO_ASSERT(fio___mem_state, "memory state machine uninitialized");

#if 0 /* help in bug tracking  */
  fprintf(stderr, "* State before allocating 1 unit (16 bytes).\n");
  fio_malloc_print_state();
  void *ptr = fio_malloc(16);
  fprintf(stderr, "* allocated 1 unit (16 bytes) at %p.\n", ptr);
  fprintf(
      stderr, "* Should be from block %p\n", (void *)fio___mem_ptr2block(ptr));
  fio_malloc_print_state();
  fio_free(ptr);
  fprintf(stderr, "* deallocated 1 unit (16 bytes) at %p.\n", ptr);
  fprintf(
      stderr, "* Should be from block %p\n", (void *)fio___mem_ptr2block(ptr));
  fio_malloc_print_state();
  ptr = NULL;
  for (size_t i = 1; i < ((FIO_MEMORY_BLOCK_SIZE >> 4) - 2); ++i) {
    fio_free(ptr);
    ptr = fio_malloc(16);
  }
  fprintf(stderr,
          "* allocated and deallocated a block worth of units (%zu units) "
          "(last: %p).\n",
          (size_t)((FIO_MEMORY_BLOCK_SIZE >> 4) - 1),
          ptr);
  fprintf(
      stderr, "* Should be from block %p\n", (void *)fio___mem_ptr2block(ptr));
  fio_malloc_print_state();
  {
    void *tmp2 = fio_malloc(16);
    fio_free(tmp2);
    fio_free(ptr);
    ptr = tmp2;
  }
  fprintf(stderr,
          "* deallocated block's last unit after 1 unit allocation at %p.\n",
          ptr);
  fprintf(
      stderr, "* Should be from block %p\n", (void *)fio___mem_ptr2block(ptr));
  fio_malloc_print_state();

#endif

  for (uintptr_t cycles = 16; cycles <= (FIO_MEMORY_BIG_BLOCK_ALLOC_LIMIT);
       cycles *= 2) {
    const size_t thread_count = ((fio___mem_state->cores >> 1) | 1);
    pthread_t threads[thread_count];
    fprintf(stderr,
            "* Testing %zu byte allocation blocks with %zu threads.\n",
            (size_t)(cycles),
            (thread_count + 1));
    for (size_t i = 1; i < thread_count; ++i) {
      if (pthread_create(
              threads + i, NULL, FIO_NAME_TEST(stl, mem_tsk), (void *)cycles)) {
        abort();
      }
    }
    FIO_NAME_TEST(stl, mem_tsk)((void *)cycles);
    for (size_t i = 1; i < thread_count; ++i) {
      pthread_join(threads[i], NULL);
    }
  }
#if DEBUG && FIO_EXTERN_COMPLETE
  // fio_malloc_print_state();
  fio___mem_destroy();
  FIO_ASSERT(!fio___mem_chunk_count, "memory leaks?");
#endif /* DEBUG && FIO_EXTERN_COMPLETE */
}
#endif /* FIO_MALLOC_FORCE_SYSTEM */

#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Memory Allocation - cleanup
***************************************************************************** */
#undef FIO_MEMORY_ON_BLOCK_ALLOC
#undef FIO_MEMORY_ON_BLOCK_FREE
#undef FIO_MEMORY_PRINT_BLOCK_STAT
#undef FIO_MEMORY_PRINT_BLOCK_STAT_END
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MEMORY_BLOCK_HEADER_SIZE
#undef FIO_MEMORY_BLOCK_MASK
#undef FIO_MEMORY_BLOCK_SLICES
#undef FIO_MEMORY_BLOCK_START_POS
#undef FIO_MEMORY_MAX_SLICES_PER_BLOCK
#undef FIO_ALIGN
#undef FIO_ALIGN_NEW
#undef FIO_MALLOC
#endif /* FIO_MALLOC */

/* *****************************************************************************










                          Memory Management MACROs
                    (used internally, for dynamic types)









***************************************************************************** */

/* *****************************************************************************
Memory management macros
***************************************************************************** */

#ifdef FIO_MALLOC_TMP_USE_SYSTEM /* force malloc */
#define FIO_MEM_CALLOC_(size, units) calloc((size), (units))
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  realloc((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) free((ptr))
#define FIO_MEM_INTERNAL_MALLOC_ 0
#else /* FIO_MALLOC_TMP_USE_SYSTEM */
#define FIO_MEM_CALLOC_ FIO_MEM_CALLOC
#define FIO_MEM_REALLOC_ FIO_MEM_REALLOC
#define FIO_MEM_FREE_ FIO_MEM_FREE
#define FIO_MEM_INTERNAL_MALLOC_ FIO_MEM_INTERNAL_MALLOC
#endif /* FIO_MALLOC_TMP_USE_SYSTEM */
