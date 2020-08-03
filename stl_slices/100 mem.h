/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                          Custom Memory Allocation
                  Memory allocator for short lived objects









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

#ifndef FIO_MEMORY_BLOCK_SIZE_LOG
/**
 * The logarithmic value for a memory block, 15 == 32Kb, 16 == 64Kb, etc'
 *
 * Breaks at FIO_MEMORY_BLOCK_SIZE_LOG >= 20 || FIO_MEMORY_BLOCK_SIZE_LOG < 10
 *
 * By default, a block of memory is 256Kb silce from an 8Mb allocation.
 */
#define FIO_MEMORY_BLOCK_SIZE_LOG (18)
#endif

/* The number of blocks pre-allocated each system call, 8Mb by default */
#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION                                       \
  ((1 << 23) >> FIO_MEMORY_BLOCK_SIZE_LOG)
#endif

/*
 * The maximum number of memory arenas to initialize.
 *
 * When 0 - maximum detected cores.
 */
#ifndef FIO_MEMORY_ARENA_COUNT_MAX
#define FIO_MEMORY_ARENA_COUNT_MAX 64
#endif

/*
 * The default number of memory arenas to initialize when CPU core detection
 * fails.
 *
 * Normally, fio_malloc tries to initialize as many memory allocation arenas as
 * the number of CPU cores. This value will only be used if core detection isn't
 * available or fails.
 */
#ifndef FIO_MEMORY_ARENA_COUNT_DEFAULT
#define FIO_MEMORY_ARENA_COUNT_DEFAULT 5
#endif

#undef FIO_MEMORY_BLOCK_SIZE
/** The resulting memory block size, depends on `FIO_MEMORY_BLOCK_SIZE_LOG` */
#define FIO_MEMORY_BLOCK_SIZE ((uintptr_t)1 << FIO_MEMORY_BLOCK_SIZE_LOG)

/**
 * The maximum allocation size, after which `mmap` will be called instead of the
 * facil.io allocator.
 *
 * Defaults to 75% of the block (192Kb), after which `mmap` is used instead.
 */
#ifndef FIO_MEMORY_BLOCK_ALLOC_LIMIT
#define FIO_MEMORY_BLOCK_ALLOC_LIMIT                                           \
  ((uintptr_t)3 << (FIO_MEMORY_BLOCK_SIZE_LOG - 2))
#endif

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
  if (new_len + 4096 < prev_len) /* more than a single dangling page */
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
Allocator default
***************************************************************************** */

/* don't change these */
#undef FIO_MEMORY_BLOCK_SLICES
#undef FIO_MEMORY_BLOCK_HEADER_SIZE
#undef FIO_MEMORY_BLOCK_START_POS
#undef FIO_MEMORY_MAX_SLICES_PER_BLOCK
#undef FIO_MEMORY_BLOCK_MASK

#define FIO_MEMORY_BLOCK_SLICES (FIO_MEMORY_BLOCK_SIZE >> 4) /* 16B slices */

#define FIO_MEMORY_BLOCK_MASK (FIO_MEMORY_BLOCK_SIZE - 1) /* 0b0...1... */

/* must be divisable by 16 bytes, bigger than min(sizeof(FIO_MEM_BLOCK), 16) */
#define FIO_MEMORY_BLOCK_HEADER_SIZE                                           \
  ((sizeof(fio___mem_block_s) + 15UL) & (~15UL))

/* allocation counter position (start) */
#define FIO_MEMORY_BLOCK_START_POS (FIO_MEMORY_BLOCK_HEADER_SIZE >> 4)

#define FIO_MEMORY_MAX_SLICES_PER_BLOCK                                        \
  (FIO_MEMORY_BLOCK_SLICES - FIO_MEMORY_BLOCK_START_POS)

/* The basic block header. Starts a 32Kib memory block */
typedef struct fio___mem_block_s fio___mem_block_s;
/* A memory caching "arena"  */
typedef struct fio___mem_arena_s fio___mem_arena_s;

/* *****************************************************************************
Memory state machine
***************************************************************************** */

struct fio___mem_arena_s {
  fio___mem_block_s *block;
  fio_lock_i lock;
};

typedef struct {
  FIO_LIST_HEAD available; /* free list for memory blocks */
  // intptr_t count;          /* free list counter */
  size_t cores;    /* the number of detected CPU cores*/
  fio_lock_i lock; /* a global lock */
  uint8_t forked;  /* a forked collection indicator. */
  fio___mem_arena_s arenas[];
} fio___mem_state_s;
/* The memory allocators persistent state */
static fio___mem_state_s *fio___mem_state = (fio___mem_state_s *)NULL;

/* *****************************************************************************
Slices and Blocks - types
***************************************************************************** */

struct fio___mem_block_s {
  uint64_t reserved;          /* should always be zero, or page sized */
  uint16_t root;              /* REQUIRED, root == 0 => is root to self */
  volatile uint16_t root_ref; /* root reference memory padding */
  volatile uint16_t ref;      /* reference count (per memory page) */
  uint16_t pos;               /* position into the block */
};

typedef struct fio___mem_block_node_s fio___mem_block_node_s;
struct fio___mem_block_node_s {
  fio___mem_block_s
      dont_touch;     /* prevent block internal data from being corrupted */
  FIO_LIST_NODE node; /* next block */
};

#define FIO_LIST_NAME fio___mem_available_blocks
#define FIO_LIST_TYPE fio___mem_block_node_s
#ifndef FIO_STL_KEEP__
#define FIO_STL_KEEP__ 1
#endif /* FIO_STL_KEEP__ */
#include __FILE__
#if FIO_STL_KEEP__ == 1
#undef FIO_STL_KEEP__
#endif /* FIO_STL_KEEP__ */
/* Address returned when allocating 0 bytes ( fio_malloc(0) ) */
static long double fio___mem_on_malloc_zero;

/* retrieve root block */
FIO_SFUNC fio___mem_block_s *fio___mem_block_root(fio___mem_block_s *b) {
  return FIO_PTR_MATH_SUB(
      fio___mem_block_s, b, b->root * FIO_MEMORY_BLOCK_SIZE);
}

/* *****************************************************************************
Arena allocation / deallocation
***************************************************************************** */

/* see destructor at: fio___mem_destroy */
FIO_SFUNC void __attribute__((constructor)) fio___mem_state_allocate(void) {
  if (fio___mem_state)
    return;
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
  if ((intptr_t)cores <= 0)
    cores = 1;
#endif /* FIO_MEMORY_ARENA_COUNT_MAX */
  const size_t pages = FIO_MEM_BYTES2PAGES(sizeof(*fio___mem_state) +
                                           (cores * sizeof(fio___mem_arena_s)));
  fio___mem_state = (fio___mem_state_s *)FIO_MEM_PAGE_ALLOC(pages, 1);
  FIO_ASSERT_ALLOC(fio___mem_state);
  *fio___mem_state = (fio___mem_state_s){
      .available = FIO_LIST_INIT(fio___mem_state->available),
      .cores = cores,
      .lock = FIO_LOCK_INIT,
  };
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO(
      "facil.io memory allocation initialized:\n"
      "\t* %zu concurrent arenas (@%p).\n"
      "\t* %zu pages required for arenas (~%zu bytes).\n"
      "\t* system allocation size:                     %zu bytes\n"
      "\t* memory block size (allocation slice / bin): %zu bytes\n"
      "\t* memory blocks per system allocation:        %zu blocks\n"
      "\t* memory block overhead:                      %zu bytes\n",
      cores,
      (void *)fio___mem_state,
      (size_t)pages,
      (size_t)(pages << 12),
      (size_t)(FIO_MEMORY_BLOCKS_PER_ALLOCATION * FIO_MEMORY_BLOCK_SIZE),
      (size_t)FIO_MEMORY_BLOCK_SIZE,
      (size_t)FIO_MEMORY_BLOCKS_PER_ALLOCATION,
      (size_t)FIO_MEMORY_BLOCK_HEADER_SIZE);
#endif /* DEBUG */
}

FIO_SFUNC void fio___mem_state_deallocate(void) {
  if (!fio___mem_state)
    return;
  const size_t pages =
      FIO_MEM_BYTES2PAGES(sizeof(*fio___mem_state) +
                          (fio___mem_state->cores * sizeof(fio___mem_arena_s)));
  FIO_MEM_PAGE_FREE(fio___mem_state, pages);
  fio___mem_state = (fio___mem_state_s *)NULL;
}

/* *****************************************************************************
Memory arena management and selection
***************************************************************************** */

/* Last available arena for thread. */
static __thread volatile fio___mem_arena_s *fio___mem_arena =
    (fio___mem_arena_s *)NULL;

FIO_SFUNC void fio___mem_arena_aquire(void) {
  if (!fio___mem_state) {
    fio___mem_state_allocate();
  }
  if (fio___mem_arena)
    if (!fio_trylock(&fio___mem_arena->lock))
      return;
  for (;;) {
    const size_t cores = fio___mem_state->cores;
    for (size_t i = 0; i < cores; ++i) {
      if (!fio_trylock(&fio___mem_state->arenas[i].lock)) {
        fio___mem_arena = fio___mem_state->arenas + i;
        return;
      }
    }
    FIO_THREAD_RESCHEDULE();
  }
}

FIO_IFUNC void fio___mem_arena_release() { fio_unlock(&fio___mem_arena->lock); }

/* *****************************************************************************
Allocator debugging helpers
***************************************************************************** */

#if DEBUG
/* maximum block allocation count. */
static size_t fio___mem_block_count_max;
/* current block allocation count. */
static size_t fio___mem_block_count;

// void fio_memory_dump_missing(void) {
//   fprintf(stderr, "\n ==== Attempting Memory Dump (will crash) ====\n");
//   if (available_blocks_is_empty(&memory.available)) {
//     fprintf(stderr, "- Memory dump attempt canceled\n");
//     return;
//   }
//   block_node_s *smallest = available_blocks_root(memory.available.next);
//   FIO_LIST_EACH(block_node_s, node, &memory.available, tmp) {
//     if (smallest > tmp)
//       smallest = tmp;
//   }

//   for (size_t i = 0;
//        i < FIO_MEMORY_BLOCK_SIZE * FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++i) {
//     if ((((uintptr_t)smallest + i) & FIO_MEMORY_BLOCK_MASK) == 0) {
//       i += 32;
//       fprintf(stderr, "---block jump---\n");
//       continue;
//     }
//     if (((char *)smallest)[i])
//       fprintf(stderr, "%c", ((char *)smallest)[i]);
//   }
// }

#define FIO_MEMORY_ON_BLOCK_ALLOC()                                            \
  do {                                                                         \
    fio_atomic_add(&fio___mem_block_count, 1);                                 \
    if (fio___mem_block_count > fio___mem_block_count_max)                     \
      fio___mem_block_count_max = fio___mem_block_count;                       \
  } while (0)
#define FIO_MEMORY_ON_BLOCK_FREE()                                             \
  do {                                                                         \
    fio_atomic_sub_fetch(&fio___mem_block_count, 1);                           \
  } while (0)
#ifdef FIO_LOG_INFO
#define FIO_MEMORY_PRINT_BLOCK_STAT()                                          \
  FIO_LOG_INFO(                                                                \
      "(fio) Total memory blocks allocated before cleanup %zu\n"               \
      "       Maximum memory blocks allocated at a single time %zu\n",         \
      fio___mem_block_count,                                                   \
      fio___mem_block_count_max)
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()                                      \
  do {                                                                         \
    if (fio___mem_block_count) {                                               \
      FIO_LOG_ERROR("(fio) Total memory blocks allocated "                     \
                    "after cleanup (POSSIBLE LEAKS): %zu\n",                   \
                    fio___mem_block_count);                                    \
    } else {                                                                   \
      FIO_LOG_INFO("(fio) Total memory blocks allocated after cleanup: %zu\n", \
                   fio___mem_block_count);                                     \
    }                                                                          \
  } while (0)
#else /* FIO_LOG_INFO */
#define FIO_MEMORY_PRINT_BLOCK_STAT()
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()
#endif /* FIO_LOG_INFO */
#else  /* DEBUG */
#define FIO_MEMORY_ON_BLOCK_ALLOC()
#define FIO_MEMORY_ON_BLOCK_FREE()
#define FIO_MEMORY_PRINT_BLOCK_STAT()
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()
#endif /* DEBUG */

/* *****************************************************************************
Block allocation and rotation
***************************************************************************** */

FIO_SFUNC fio___mem_block_s *fio___mem_block_alloc(void) {
  fio___mem_block_s *b = (fio___mem_block_s *)FIO_MEM_PAGE_ALLOC(
      FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                          FIO_MEMORY_BLOCK_SIZE),
      FIO_MEMORY_BLOCK_SIZE_LOG);
  FIO_ASSERT_ALLOC(b);
#if DEBUG
  FIO_LOG_DEBUG2("memory allocator allocated %zu pages from the system: %p",
                 (size_t)FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                                             FIO_MEMORY_BLOCK_SIZE),
                 (void *)b);
#endif /* DEBUG */
  /* initialize and push all block slices into memory pool */
  for (size_t i = 0; i < (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1); ++i) {
    fio___mem_block_s *tmp =
        FIO_PTR_MATH_ADD(fio___mem_block_s, b, (FIO_MEMORY_BLOCK_SIZE * i));
    *tmp = (fio___mem_block_s){.root = (uint16_t)i, .ref = 0};
    fio___mem_available_blocks_push(&fio___mem_state->available,
                                    (fio___mem_block_node_s *)tmp);
  }
  /* initialize and return last slice (it's in the cache) */
  b = FIO_PTR_MATH_ADD(
      fio___mem_block_s,
      b,
      (FIO_MEMORY_BLOCK_SIZE * (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1)));
  b->root = (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1);
  /* debug counter */
  FIO_MEMORY_ON_BLOCK_ALLOC();
  return b;
}

FIO_SFUNC void fio___mem_block_free(fio___mem_block_s *b) {
  b = FIO_PTR_MATH_RMASK(fio___mem_block_s, b, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b || fio_atomic_sub_fetch(&b->ref, 1)) {
    /* block slice still in use */
    return;
  }
  memset(b + 1, 0, (FIO_MEMORY_BLOCK_SIZE - sizeof(*b)));
  fio_lock(&fio___mem_state->lock);
  fio___mem_available_blocks_push(&fio___mem_state->available,
                                  (fio___mem_block_node_s *)b);
  b = fio___mem_block_root(b);
  if (fio_atomic_sub_fetch(&b->root_ref, 1)) {
    /* block still has at least one used slice */
    fio_unlock(&fio___mem_state->lock);
    return;
  }
  /* remove all block slices from memory pool */
  for (size_t i = 0; i < (FIO_MEMORY_BLOCKS_PER_ALLOCATION); ++i) {
    fio___mem_block_node_s *tmp = FIO_PTR_MATH_ADD(
        fio___mem_block_node_s, b, (FIO_MEMORY_BLOCK_SIZE * i));
    fio___mem_available_blocks_remove(tmp);
  }
  fio_unlock(&fio___mem_state->lock);
  /* return memory to system */
  FIO_MEM_PAGE_FREE(b,
                    FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                                        FIO_MEMORY_BLOCK_SIZE));
  /* debug counter */
  FIO_MEMORY_ON_BLOCK_FREE();
#if DEBUG
  FIO_LOG_DEBUG2("memory allocator returned %p to the system", (void *)b);
#endif /* DEBUG */
}

/* rotates block in arena */
FIO_SFUNC void fio___mem_block_rotate(void) {
  fio___mem_block_s *to_free = fio___mem_arena->block; /* keep memory pool */
  fio___mem_arena->block = (fio___mem_block_s *)NULL;
  fio_lock(&fio___mem_state->lock);
  fio___mem_arena->block = (fio___mem_block_s *)fio___mem_available_blocks_pop(
      &fio___mem_state->available);
  if (!fio___mem_arena->block) {
    /* allocate block */
    fio___mem_arena->block = fio___mem_block_alloc();
  } else {
    FIO_ASSERT(!fio___mem_arena->block->reserved,
               "memory header corruption, overflowed right?");
  }
  /* update the root reference count before releasing the lock */
  fio_atomic_add(&fio___mem_block_root(fio___mem_arena->block)->root_ref, 1);
  fio_unlock(&fio___mem_state->lock);
  /* zero out memory used for available block linked list, keep root data */
  fio___mem_arena->block->ref = 1;
  fio___mem_arena->block->pos = 0;
  ((fio___mem_block_node_s *)fio___mem_arena->block)->node =
      (FIO_LIST_NODE){.next = NULL};
  fio___mem_block_free(to_free);
}

FIO_SFUNC void *fio___mem_block_slice(size_t bytes) {
  const uint16_t max =
      ((FIO_MEMORY_BLOCK_SIZE - FIO_MEMORY_BLOCK_HEADER_SIZE) >> 4);
  void *r = NULL;
  bytes = (bytes + 15) >> 4; /* convert to 16 byte units */
  fio___mem_arena_aquire();
  fio___mem_block_s *b = fio___mem_arena->block;
  if (!b || ((size_t)b->pos + bytes) >= (size_t)max) {
    fio___mem_block_rotate();
    b = fio___mem_arena->block;
  }
  if (!b)
    goto finish;
  fio_atomic_add(&b->ref, 1);
  r = FIO_PTR_MATH_ADD(
      void, b, FIO_MEMORY_BLOCK_HEADER_SIZE + ((size_t)b->pos << 4));
  fio_atomic_add(&b->pos, bytes);
finish:
  fio___mem_arena_release();
  return r;
}

/* *****************************************************************************
Allocator Destruction
***************************************************************************** */

FIO_SFUNC void __attribute__((destructor)) fio___mem_destroy(void) {
  if (!fio___mem_state)
    return;
  FIO_MEMORY_PRINT_BLOCK_STAT();
  for (size_t i = 0; i < fio___mem_state->cores; ++i) {
    /* free all blocks in the arean memory pools */
    /* this should return memory to system unless a memory leak occurred */
    fio___mem_block_s *b = fio___mem_state->arenas[i].block;
    fio___mem_state->arenas[i].block = NULL;
    fio___mem_state->arenas[i].lock = FIO_LOCK_INIT;
    fio___mem_block_free(b);
  }
  fio___mem_arena = NULL;
  fio___mem_state_deallocate();
  FIO_MEMORY_PRINT_BLOCK_STAT_END();
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("facil.io memory allocation cleanup complete.");
#endif /* DEBUG */
}

/* *****************************************************************************
API implementation
***************************************************************************** */

/** Frees memory that was allocated using this library. */
SFUNC void fio_free(void *ptr) {
  if (ptr == &fio___mem_on_malloc_zero)
    return;
  fio___mem_block_s *b =
      FIO_PTR_MATH_RMASK(fio___mem_block_s, ptr, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b)
    return;
  if (b->reserved)
    goto test_reserved;
  fio___mem_block_free(b);
  return;
test_reserved:
  FIO_ASSERT(!(b->reserved & ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)),
             "memory allocator corruption, block header overwritten?");
  /* zero out memory before returning it to the system */
  memset(ptr, 0, (b->reserved) - sizeof(*b));
  FIO_MEM_PAGE_FREE(b, (b->reserved >> FIO_MEM_PAGE_SIZE_LOG));
  FIO_MEMORY_ON_BLOCK_FREE();
}

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size) {
  if (!size)
    return &fio___mem_on_malloc_zero;
  if (size <= FIO_MEMORY_BLOCK_ALLOC_LIMIT) {
    return fio___mem_block_slice(size);
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

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size) {
  return fio_realloc2(ptr, new_size, new_size);
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
  if (!ptr || ptr == &fio___mem_on_malloc_zero)
    return fio_malloc(new_size);
  const size_t max_len = ((0 - (uintptr_t)FIO_PTR_MATH_LMASK(
                                   void, ptr, FIO_MEMORY_BLOCK_SIZE_LOG)) +
                          FIO_MEMORY_BLOCK_SIZE);

  fio___mem_block_s *b =
      FIO_PTR_MATH_RMASK(fio___mem_block_s, ptr, FIO_MEMORY_BLOCK_SIZE_LOG);
  void *mem = NULL;

  if (copy_len > new_size)
    copy_len = new_size;
  if (b->reserved)
    goto big_realloc;
  if (copy_len > max_len)
    copy_len = max_len;

  mem = fio_malloc(new_size);
  if (!mem) {
    return NULL;
  }
  fio___memcpy_16byte(mem, ptr, ((copy_len + 15) >> 4));
  fio___mem_block_free(b);
  // zero out leftover bytes, if any.
  while (copy_len & 15) {
    ((uint8_t *)mem)[(copy_len++) & 15] = 0;
  }
  return mem;

big_realloc:
  FIO_ASSERT(!(b->reserved & ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)),
             "memory allocator corruption, block header overwritten?");
  const size_t new_page_len =
      FIO_MEM_BYTES2PAGES(new_size + FIO_MEMORY_BLOCK_HEADER_SIZE);
  if (new_page_len <= 2) {
    /* shrink from big allocation to small one */
    mem = fio_malloc(new_size);
    if (!mem) {
      return NULL;
    }
    fio___memcpy_16byte(mem, ptr, (copy_len >> 4));
    FIO_MEM_PAGE_FREE(b, b->reserved);
    return mem;
  }
  fio___mem_block_s *tmp = (fio___mem_block_s *)FIO_MEM_PAGE_REALLOC(
      b,
      b->reserved >> FIO_MEM_PAGE_SIZE_LOG,
      new_page_len,
      FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!tmp)
    return NULL;
  tmp->reserved = new_page_len << FIO_MEM_PAGE_SIZE_LOG;
  return (void *)(tmp + 1);
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
  size_t pages = FIO_MEM_BYTES2PAGES(size + FIO_MEMORY_BLOCK_HEADER_SIZE);
  fio___mem_block_s *b =
      (fio___mem_block_s *)FIO_MEM_PAGE_ALLOC(pages, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b)
    return NULL;
  FIO_MEMORY_ON_BLOCK_ALLOC();
  b->reserved = pages << FIO_MEM_PAGE_SIZE_LOG;
  return (void *)(b + 1);
}

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
void fio_malloc_after_fork(void) {
  if (!fio___mem_state)
    return;
  for (size_t i = 0; i < fio___mem_state->cores; ++i) {
    fio___mem_state->arenas[i].lock = FIO_LOCK_INIT;
  }
  fio___mem_state->lock = FIO_LOCK_INIT;
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
Memory Allocation - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL
#ifdef FIO_MALLOC_FORCE_SYSTEM
FIO_SFUNC void FIO_NAME_TEST(stl, mem)(void) {
  fprintf(stderr, "* Custom memory allocator bypassed.\n");
}

#else /* FIO_MALLOC_FORCE_SYSTEM */
FIO_SFUNC void FIO_NAME_TEST(stl, mem)(void) {
  fprintf(stderr, "* Testing core memory allocator (fio_malloc).\n");
  const size_t three_blocks = ((size_t)3ULL * FIO_MEMORY_BLOCKS_PER_ALLOCATION)
                              << FIO_MEMORY_BLOCK_SIZE_LOG;
  for (int cycles = 4; cycles < FIO_MEMORY_BLOCK_SIZE_LOG; ++cycles) {
    fprintf(stderr,
            "* Testing %zu byte allocation blocks.\n",
            (size_t)(1UL << cycles));
    const size_t limit = (three_blocks >> cycles);
    char **ary = (char **)fio_calloc(sizeof(*ary), limit);
    FIO_ASSERT(ary, "allocation failed for test container");
    for (size_t i = 0; i < limit; ++i) {
      ary[i] = (char *)fio_malloc(1UL << cycles);
      FIO_ASSERT(ary[i], "allocation failed!")
      FIO_ASSERT(!ary[i][0], "allocated memory not zero");
      memset(ary[i], 0xff, (1UL << cycles));
    }
    for (size_t i = 0; i < limit; ++i) {
      char *tmp =
          (char *)fio_realloc2(ary[i], (2UL << cycles), (1UL << cycles));
      FIO_ASSERT(tmp, "re-allocation failed!")
      ary[i] = tmp;
      FIO_ASSERT(!ary[i][(2UL << cycles) - 1], "fio_realloc2 copy overflow!");
      tmp = (char *)fio_realloc2(ary[i], (1UL << cycles), (2UL << cycles));
      FIO_ASSERT(tmp, "re-allocation (shrinking) failed!")
      ary[i] = tmp;
      FIO_ASSERT(ary[i][(1UL << cycles) - 1] == (char)0xFF,
                 "fio_realloc2 copy underflow!");
    }
    for (size_t i = 0; i < limit; ++i) {
      fio_free(ary[i]);
    }
    fio_free(ary);
  }
#if DEBUG && FIO_EXTERN_COMPLETE
  fio___mem_destroy();
  FIO_ASSERT(fio___mem_block_count <= 1, "memory leaks?");
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
