/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "005 riskyhash.h"          /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "051 json.h"               /* Development inclusion - ignore line */
#include "201 array.h"              /* Development inclusion - ignore line */
#include "210 hashmap.h"            /* Development inclusion - ignore line */
#include "220 string.h"             /* Development inclusion - ignore line */
#include "299 reference counter.h"  /* Development inclusion - ignore line */
#include "700 cleanup.h"            /* Development inclusion - ignore line */
#define FIO_FIOBJ                   /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************








                          FIOBJ - soft (dynamic) types



FIOBJ - dynamic types

These are dynamic types that use pointer tagging for fast type identification.

Pointer tagging on 64 bit systems allows for 3 bits at the lower bits. On most
32 bit systems this is also true due to allocator alignment. When in doubt, use
the provided custom allocator.

To keep the 64bit memory address alignment on 32bit systems, a 32bit metadata
integer is added when a virtual function table is missing. This doesn't effect
memory consumption on 64 bit systems and uses 4 bytes on 32 bit systems.

Note: this code is placed at the end of the STL file, since it leverages most of
the SLT features and could be affected by their inclusion.
***************************************************************************** */
#if defined(FIO_FIOBJ) && !defined(H___FIOBJ___H)
#define H___FIOBJ___H

/* *****************************************************************************
FIOBJ compilation settings (type names and JSON nesting limits).

Type Naming Macros for FIOBJ types. By default, results in:
- fiobj_true()
- fiobj_false()
- fiobj_null()
- fiobj_num_new() ... (etc')
- fiobj_float_new() ... (etc')
- fiobj_str_new() ... (etc')
- fiobj_array_new() ... (etc')
- fiobj_hash_new() ... (etc')
***************************************************************************** */

#define FIOBJ___NAME_TRUE   true
#define FIOBJ___NAME_FALSE  false
#define FIOBJ___NAME_NULL   null
#define FIOBJ___NAME_NUMBER num
#define FIOBJ___NAME_FLOAT  float
#define FIOBJ___NAME_STRING str
#define FIOBJ___NAME_ARRAY  array
#define FIOBJ___NAME_HASH   hash

#ifndef FIOBJ_MAX_NESTING
/**
 * Sets the limit on nesting level transversal by recursive functions.
 *
 * This effects JSON output / input and the `fiobj_each2` function since they
 * are recursive.
 *
 * HOWEVER: this value will NOT effect the recursive `fiobj_free` which could
 * (potentially) expload the stack if given melformed input such as cyclic data
 * structures.
 *
 * Values should be less than 32K.
 */
#define FIOBJ_MAX_NESTING 512
#endif

/* make sure roundtrips work */
#ifndef JSON_MAX_DEPTH
#define JSON_MAX_DEPTH FIOBJ_MAX_NESTING
#endif

#ifndef FIOBJ_JSON_APPEND
#define FIOBJ_JSON_APPEND 1
#endif
/* *****************************************************************************
General Requirements / Macros
***************************************************************************** */

#define FIO_ATOL   1
#define FIO_ATOMIC 1
#include __FILE__

#ifdef FIOBJ_EXTERN
#define FIOBJ_FUNC
#define FIOBJ_IFUNC
#define FIOBJ_EXTERN_OBJ     extern
#define FIOBJ_EXTERN_OBJ_IMP __attribute__((weak))

#else /* FIO_EXTERN */
#define FIOBJ_FUNC           static __attribute__((unused))
#define FIOBJ_IFUNC          static inline __attribute__((unused))
#define FIOBJ_EXTERN_OBJ     static __attribute__((unused))
#define FIOBJ_EXTERN_OBJ_IMP static __attribute__((unused))
#ifndef FIOBJ_EXTERN_COMPLETE /* force implementation, emitting static data */
#define FIOBJ_EXTERN_COMPLETE 2
#endif /* FIOBJ_EXTERN_COMPLETE */

#endif /* FIO_EXTERN */

#ifdef FIO_LOG_PRINT__
#define FIOBJ_LOG_PRINT__(...) FIO_LOG_PRINT__(__VA_ARGS__)
#else
#define FIOBJ_LOG_PRINT__(...)
#endif

#ifdef __cplusplus /* C++ doesn't allow declarations for static variables */
#undef FIOBJ_EXTERN_OBJ
#undef FIOBJ_EXTERN_OBJ_IMP
#define FIOBJ_EXTERN_OBJ     extern "C"
#define FIOBJ_EXTERN_OBJ_IMP extern "C" __attribute__((weak))
#endif

/* *****************************************************************************
Dedicated memory allocator for FIOBJ types? (recommended for locality)
***************************************************************************** */
#ifdef FIOBJ_MALLOC
#define FIO_MEMORY_NAME fiobj_mem
#ifndef FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG
/* 4Mb per system call */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 22
#endif
#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG
/* fight fragmentation */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 4
#endif
#ifndef FIO_MEMORY_ALIGN_LOG
/* align on 8 bytes, it's enough */
#define FIO_MEMORY_ALIGN_LOG 3
#endif
#ifndef FIO_MEMORY_CACHE_SLOTS
/* cache up to 64Mb */
#define FIO_MEMORY_CACHE_SLOTS 16
#endif
#ifndef FIO_MEMORY_ENABLE_BIG_ALLOC
/* for big arrays / maps */
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
#endif
#ifndef FIO_MEMORY_ARENA_COUNT
/* CPU core arena count */
#define FIO_MEMORY_ARENA_COUNT -1
#endif
#if FIO_OS_POSIX && !defined(FIO_MEMORY_USE_THREAD_MUTEX)
/* yes, well... POSIX Mutexes are decent on the machines I tested. */
#define FIO_MEMORY_USE_THREAD_MUTEX 1
#endif
/* make sure functions are exported if requested */
#ifdef FIOBJ_EXTERN
#define FIO_EXTERN
#if defined(FIOBJ_EXTERN_COMPLETE) && !defined(FIO_EXTERN_COMPLETE)
#define FIO_EXTERN_COMPLETE 2
#endif
#endif
#include __FILE__

#define FIOBJ_MEM_REALLOC(ptr, old_size, new_size, copy_len)                   \
  FIO_NAME(fiobj_mem, realloc2)((ptr), (new_size), (copy_len))
#define FIOBJ_MEM_FREE(ptr, size) FIO_NAME(fiobj_mem, free)((ptr))
#define FIOBJ_MEM_REALLOC_IS_SAFE 0

#else

FIO_IFUNC void *FIO_NAME(fiobj_mem, realloc2)(void *ptr,
                                              size_t new_size,
                                              size_t copy_len) {
  return FIO_MEM_REALLOC(ptr, new_size, new_size, copy_len);
  (void)copy_len; /* might be unused */
}
FIO_IFUNC void FIO_NAME(fiobj_mem, free)(void *ptr) { FIO_MEM_FREE(ptr, -1); }

#define FIOBJ_MEM_REALLOC         FIO_MEM_REALLOC
#define FIOBJ_MEM_FREE            FIO_MEM_FREE
#define FIOBJ_MEM_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE

#endif /* FIOBJ_MALLOC */
/* *****************************************************************************
Debugging / Leak Detection
***************************************************************************** */
#if defined(TEST) || defined(DEBUG) || defined(FIO_LEAK_COUNTER)
#define FIOBJ_MARK_MEMORY 1
#endif

#if FIOBJ_MARK_MEMORY
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_ALLOC_COUNTER;
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_FREE_COUNTER;
#define FIOBJ_MARK_MEMORY_ALLOC()                                              \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_ALLOC_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_FREE()                                               \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_FREE_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_PRINT()                                              \
  FIOBJ_LOG_PRINT__(                                                           \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? 4 /* FIO_LOG_LEVEL_INFO */                                        \
           : 3 /* FIO_LOG_LEVEL_WARNING */),                                   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? "INFO: total FIOBJ allocations: %zu (%zu/%zu)"                    \
           : "WARNING: LEAKED! FIOBJ allocations: %zu (%zu/%zu)"),             \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER - FIOBJ_MARK_MEMORY_FREE_COUNTER,        \
      FIOBJ_MARK_MEMORY_FREE_COUNTER,                                          \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER)
#define FIOBJ_MARK_MEMORY_ENABLED 1

#else

#define FIOBJ_MARK_MEMORY_ALLOC_COUNTER 0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_FREE_COUNTER  0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_ALLOC()
#define FIOBJ_MARK_MEMORY_FREE()
#define FIOBJ_MARK_MEMORY_PRINT()
#define FIOBJ_MARK_MEMORY_ENABLED 0
#endif

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Use the FIOBJ type for dynamic types. */
typedef struct FIOBJ_s {
  struct FIOBJ_s *compiler_validation_type;
} * FIOBJ;

/** FIOBJ type enum for common / primitive types. */
typedef enum {
  FIOBJ_T_NUMBER = 0x01, /* 0b001 3 bits taken for small numbers */
  FIOBJ_T_PRIMITIVE = 2, /* 0b010 a lonely second bit signifies a primitive */
  FIOBJ_T_STRING = 3,    /* 0b011 */
  FIOBJ_T_ARRAY = 4,     /* 0b100 */
  FIOBJ_T_HASH = 5,      /* 0b101 */
  FIOBJ_T_FLOAT = 6,     /* 0b110 */
  FIOBJ_T_OTHER = 7,     /* 0b111 dynamic type - test content */
} fiobj_class_en;

#define FIOBJ_T_NULL  2  /* 0b010 a lonely second bit signifies a primitive */
#define FIOBJ_T_TRUE  18 /* 0b010 010 - primitive value */
#define FIOBJ_T_FALSE 34 /* 0b100 010 - primitive value */

/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE(o) fiobj_type(o)
/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
/** Identifies an invalid type identifier (returned from FIOBJ_TYPE(o) */
#define FIOBJ_T_INVALID 0
/** Identifies an invalid object */
#define FIOBJ_INVALID 0
/** Tests if the object is (probably) a valid FIOBJ */
#define FIOBJ_IS_INVALID(o)       (((uintptr_t)(o)&7UL) == 0)
#define FIOBJ_TYPE_CLASS(o)       ((fiobj_class_en)(((uintptr_t)(o)) & 7UL))
#define FIOBJ_PTR_TAG(o, klass)   ((uintptr_t)((uintptr_t)(o) | (klass)))
#define FIOBJ_PTR_UNTAG(o)        ((uintptr_t)((uintptr_t)(o) & (~7ULL)))
#define FIOBJ_PTR_TAG_VALIDATE(o) ((uintptr_t)((uintptr_t)(o) & (7ULL)))
/** Returns an objects type. This isn't limited to known types. */
FIO_IFUNC size_t fiobj_type(FIOBJ o);

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIO_IFUNC FIOBJ fiobj_dup(FIOBJ o);

/** Decreases an object's reference count or frees it. */
FIO_IFUNC void fiobj_free(FIOBJ o);

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Compares two objects. */
FIO_IFUNC unsigned char FIO_NAME_BL(fiobj, eq)(FIOBJ a, FIOBJ b);

/** Returns a temporary String representation for any FIOBJ object. */
FIO_IFUNC fio_str_info_s FIO_NAME2(fiobj, cstr)(FIOBJ o);

/** Returns an integer representation for any FIOBJ object. */
FIO_IFUNC intptr_t FIO_NAME2(fiobj, i)(FIOBJ o);

/** Returns a float (double) representation for any FIOBJ object. */
FIO_IFUNC double FIO_NAME2(fiobj, f)(FIOBJ o);

/* *****************************************************************************
FIOBJ Containers (iteration)
***************************************************************************** */

/** Iteration information structure passed to the callback. */
typedef struct fiobj_each_s {
  /** The being iterated. Once set, cannot be safely changed. */
  FIOBJ const parent;
  /** The index to start at / the current object's index */
  uint64_t index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(struct fiobj_each_s *info);
  /** The argument passed along to the task. */
  void *udata;
  /** The value of the current object in the Array or Hash Map */
  FIOBJ value;
  /* The key, if a Hash Map */
  FIOBJ key;
} fiobj_each_s;

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIO_SFUNC uint32_t fiobj_each1(FIOBJ o,
                               int (*task)(fiobj_each_s *info),
                               void *udata,
                               int32_t start_at);

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNC uint32_t fiobj_each2(FIOBJ o,
                                int (*task)(fiobj_each_s *info),
                                void *udata);

/* *****************************************************************************
FIOBJ Primitives (NULL, True, False)
***************************************************************************** */

/** Returns the `true` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(void) {
  return (FIOBJ)(FIOBJ_T_TRUE);
}

/** Returns the `false` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(void) {
  return (FIOBJ)(FIOBJ_T_FALSE);
}

/** Returns the `nil` / `null` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_NULL)(void) {
  return (FIOBJ)(FIOBJ_T_NULL);
}

/* *****************************************************************************
FIOBJ Type - Extensibility (FIOBJ_T_OTHER)
***************************************************************************** */

/** FIOBJ types can be extended using virtual function tables. */
typedef struct {
  /**
   * MUST return a unique number to identify object type.
   *
   * Numbers (type IDs) under 100 are reserved. Numbers under 40 are illegal.
   */
  size_t type_id;
  /** Test for equality between two objects with the same `type_id` */
  unsigned char (*is_eq)(FIOBJ restrict a, FIOBJ restrict b);
  /** Converts an object to a String */
  fio_str_info_s (*to_s)(FIOBJ o);
  /** Converts an object to an integer */
  intptr_t (*to_i)(FIOBJ o);
  /** Converts an object to a double */
  double (*to_f)(FIOBJ o);
  /** Returns the number of exposed elements held by the object, if any. */
  uint32_t (*count)(FIOBJ o);
  /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
  uint32_t (*each1)(FIOBJ o,
                    int (*task)(fiobj_each_s *e),
                    void *udata,
                    int32_t start_at);
  /**
   * Decreases the reference count and/or frees the object, calling `free2` for
   * any nested objects.
   */
  void (*free2)(FIOBJ o);
} FIOBJ_class_vtable_s;

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___OBJECT_CLASS_VTBL;

#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_NAME             fiobj_object
#define FIO_REF_TYPE             void *
#define FIO_REF_METADATA         const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___OBJECT_CLASS_VTBL;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
/* make sure functions are exported if requested */
#ifdef FIOBJ_EXTERN
#define FIO_EXTERN
#if defined(FIOBJ_EXTERN_COMPLETE) && !defined(FIO_EXTERN_COMPLETE)
#define FIO_EXTERN_COMPLETE 2
#endif
#endif
#include __FILE__

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

/** Creates a new Number object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(intptr_t i);

/** Reads the number from a FIOBJ Number. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(FIOBJ i);

/** Reads the number from a FIOBJ Number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(FIOBJ i);

/** Returns a String representation of the number (in base 10). */
FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                                    cstr)(FIOBJ i);

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), free)(FIOBJ i);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

/** Creates a new Float (double) object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(double i);

/** Reads the number from a FIOBJ Float rounding it to an integer. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(FIOBJ i);

/** Reads the value from a FIOBJ Float, as a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(FIOBJ i);

/** Returns a String representation of the float. */
FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                                    cstr)(FIOBJ i);

/** Frees a FIOBJ Float. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Strings
***************************************************************************** */

#define FIO_STR_NAME              FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_STR_OPTIMIZE_EMBEDDED 1
#define FIO_REF_NAME              FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_REF_CONSTRUCTOR_ONLY  1
#define FIO_REF_DESTROY(s)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)((FIOBJ)&s);        \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(s_)                                                       \
  do {                                                                         \
    s_ = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s))FIO_STR_INIT;      \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)

#if SIZE_T_MAX == 0xFFFFFFFF /* for 32bit system pointer alignment */
#define FIO_REF_METADATA uint32_t
#endif
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_STRING)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
/* make sure functions are exported if requested */
#ifdef FIOBJ_EXTERN
#define FIO_EXTERN
#if defined(FIOBJ_EXTERN_COMPLETE) && !defined(FIO_EXTERN_COMPLETE)
#define FIO_EXTERN_COMPLETE 2
#endif
#endif
#include __FILE__

/* Creates a new FIOBJ string object, copying the data to the new string. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_cstr)(const char *ptr, size_t len) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(s, ptr, len);
  return s;
}

/* Creates a new FIOBJ string object with (at least) the requested capacity. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_buf)(size_t capa) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), reserve)(s, capa);
  return s;
}

/* Creates a new FIOBJ string object, copying the origin (`fiobj2cstr`). */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_copy)(FIOBJ original) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  fio_str_info_s i = FIO_NAME2(fiobj, cstr)(original);
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(s, i.buf, i.len);
  return s;
}

/** Returns information about the string. Same as fiobj_str_info(). */
FIO_IFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                   cstr)(FIOBJ s) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(s);
}

/**
 * Creates a temporary FIOBJ String object on the stack.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR(str_name)                                           \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,               \
                                          0x7f7f7f7f7f7f7f7fULL,               \
                                          FIO_STR_INIT};                       \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/**
 * Creates a temporary FIOBJ String object on the stack, initialized with a
 * static string.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf_, len_)                        \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name,                                                         \
             __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,                         \
                                0x7f7f7f7f7f7f7f7fULL,                         \
                                FIO_STR_INIT_STATIC2((buf_), (len_))};         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/**
 * Creates a temporary FIOBJ String object on the stack, initialized with a
 * static string.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf_, len_, capa_)               \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                                     \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      FIO_STR_INIT_EXISTING((buf_), (len_), (capa_))};                         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/** Resets a temporary FIOBJ String, freeing and any resources allocated. */
#define FIOBJ_STR_TEMP_DESTROY(str_name)                                       \
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)(str_name);

/* *****************************************************************************
FIOBJ Arrays
***************************************************************************** */

#define FIO_ARRAY_NAME           FIO_NAME(fiobj, FIOBJ___NAME_ARRAY)
#define FIO_REF_NAME             FIO_NAME(fiobj, FIOBJ___NAME_ARRAY)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(a)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), destroy)((FIOBJ)&a);         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(a)                                                        \
  do {                                                                         \
    a = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), s))FIO_ARRAY_INIT;      \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#if SIZE_T_MAX == 0xFFFFFFFF /* for 32bit system pointer alignment */
#define FIO_REF_METADATA uint32_t
#endif
#define FIO_ARRAY_TYPE            FIOBJ
#define FIO_ARRAY_TYPE_CMP(a, b)  FIO_NAME_BL(fiobj, eq)((a), (b))
#define FIO_ARRAY_TYPE_DESTROY(o) fiobj_free(o)
#define FIO_ARRAY_TYPE_CONCAT_COPY(dest, obj)                                  \
  do {                                                                         \
    dest = fiobj_dup(obj);                                                     \
  } while (0)
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_ARRAY)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
/* make sure functions are exported if requested */
#ifdef FIOBJ_EXTERN
#define FIO_EXTERN
#if defined(FIOBJ_EXTERN_COMPLETE) && !defined(FIO_EXTERN_COMPLETE)
#define FIO_EXTERN_COMPLETE 2
#endif
#endif
#include __FILE__

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

#define FIO_OMAP_NAME            FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_REF_NAME             FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(a)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), destroy)((FIOBJ)&a);          \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(a)                                                        \
  do {                                                                         \
    a = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), s))FIO_MAP_INIT;         \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#if SIZE_T_MAX == 0xFFFFFFFF /* for 32bit system pointer alignment */
#define FIO_REF_METADATA uint32_t
#endif
#define FIO_MAP_TYPE              FIOBJ
#define FIO_MAP_TYPE_DESTROY(o)   fiobj_free(o)
#define FIO_MAP_TYPE_DISCARD(o)   fiobj_free(o)
#define FIO_MAP_KEY               FIOBJ
#define FIO_MAP_KEY_CMP(a, b)     FIO_NAME_BL(fiobj, eq)((a), (b))
#define FIO_MAP_KEY_COPY(dest, o) (dest = fiobj_dup(o))
#define FIO_MAP_KEY_DESTROY(o)    fiobj_free(o)
#define FIO_PTR_TAG(p)            FIOBJ_PTR_TAG(p, FIOBJ_T_HASH)
#define FIO_PTR_UNTAG(p)          FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE          FIOBJ
#define FIO_MEM_REALLOC_          FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_             FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_  FIOBJ_MEM_REALLOC_IS_SAFE
/* make sure functions are exported if requested */
#ifdef FIOBJ_EXTERN
#define FIO_EXTERN
#if defined(FIOBJ_EXTERN_COMPLETE) && !defined(FIO_EXTERN_COMPLETE)
#define FIO_EXTERN_COMPLETE 2
#endif
#endif
#include __FILE__

/** Calculates an object's hash value for a specific hash map object. */
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ target_hash, FIOBJ object_key);

/** Inserts a value to a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set2)(FIOBJ hash, FIOBJ key, FIOBJ value);

/**
 * Inserts a value to a hash map, with a default hash value calculation.
 *
 * If the key already exists in the Hash Map, the value will be freed instead.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set_if_missing2)(FIOBJ hash, FIOBJ key, FIOBJ value);

/** Finds a value in a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(FIOBJ hash,
                                                                   FIOBJ key);

/** Removes a value from a hash map, with a default hash value calculation. */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash, FIOBJ key, FIOBJ *old);

/**
 * Sets a value in a hash map, allocating the key String and automatically
 * calculating the hash value.
 */
FIO_IFUNC
FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
               set3)(FIOBJ hash, const char *key, size_t len, FIOBJ value);

/**
 * Finds a value in the hash map, using a temporary String and automatically
 * calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get3)(FIOBJ hash, const char *buf, size_t len);

/**
 * Removes a value in a hash map, using a temporary String and automatically
 * calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove3)(FIOBJ hash,
                                const char *buf,
                                size_t len,
                                FIOBJ *old);

/* *****************************************************************************
FIOBJ JSON support
***************************************************************************** */

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIO_IFUNC FIOBJ FIO_NAME2(fiobj, json)(FIOBJ dest, FIOBJ o, uint8_t beautify);

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (preferring the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                           update_json)(FIOBJ hash, fio_str_info_s str);

/** Helper function, calls `fiobj_hash_update_json` with string information */
FIO_IFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                          update_json2)(FIOBJ hash, char *ptr, size_t len);

/**
 * Parses a C string for JSON data. If `consumed` is not NULL, the `size_t`
 * variable will contain the number of bytes consumed before the parser stopped
 * (due to either error or end of a valid JSON data segment).
 *
 * Returns a FIOBJ object matching the JSON valid C string `str`.
 *
 * If the parsing failed (no complete valid JSON data) `FIOBJ_INVALID` is
 * returned.
 */
FIOBJ_FUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

/** Helper macro, calls `fiobj_json_parse` with string information */
#define fiobj_json_parse2(data_, len_, consumed)                               \
  fiobj_json_parse((fio_str_info_s){.buf = data_, .len = len_}, consumed)

/**
 * Uses JavaScript style notation to find data in an object structure.
 *
 * For example, "[0].name" will return the "name" property of the first object
 * in an array object.
 *
 * Returns a temporary reference to the object or FIOBJ_INVALID on an error.
 *
 * Use `fiobj_dup` to collect an actual reference to the returned object.
 */
FIOBJ_FUNC FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation);
/**
 * Uses JavaScript style notation to find data in an object structure.
 *
 * For example, "[0].name" will return the "name" property of the first object
 * in an array object.
 *
 * Returns a temporary reference to the object or FIOBJ_INVALID on an error.
 *
 * Use `fiobj_dup` to collect an actual reference to the returned object.
 */
#define fiobj_json_find2(object, str, length)                                  \
  fiobj_json_find(object, (fio_str_info_s){.buf = str, .len = length})
/* *****************************************************************************







FIOBJ - Implementation - Inline / Macro like fucntions







***************************************************************************** */

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Returns an objects type. This isn't limited to known types. */
FIO_IFUNC size_t fiobj_type(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return FIOBJ_T_NULL;
    case FIOBJ_T_TRUE:
      return FIOBJ_T_TRUE;
    case FIOBJ_T_FALSE:
      return FIOBJ_T_FALSE;
    };
    return FIOBJ_T_INVALID;
  case FIOBJ_T_NUMBER:
    return FIOBJ_T_NUMBER;
  case FIOBJ_T_FLOAT:
    return FIOBJ_T_FLOAT;
  case FIOBJ_T_STRING:
    return FIOBJ_T_STRING;
  case FIOBJ_T_ARRAY:
    return FIOBJ_T_ARRAY;
  case FIOBJ_T_HASH:
    return FIOBJ_T_HASH;
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->type_id;
  }
  if (!o)
    return FIOBJ_T_NULL;
  return FIOBJ_T_INVALID;
}

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIO_IFUNC FIOBJ fiobj_dup(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_FLOAT:     /* fall through */
    return o;
  case FIOBJ_T_STRING: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), dup)(o);
    break;
  case FIOBJ_T_ARRAY: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), dup)(o);
    break;
  case FIOBJ_T_HASH: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), dup)(o);
    break;
  case FIOBJ_T_OTHER: /* fall through */
    fiobj_object_dup(o);
  }
  return o;
}

/** Decreases an object's reference count or frees it. */
FIO_IFUNC void fiobj_free(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_FLOAT:
    return;
  case FIOBJ_T_STRING:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), free)(o);
    return;
  case FIOBJ_T_ARRAY:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), free)(o);
    return;
  case FIOBJ_T_HASH:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), free)(o);
    return;
  case FIOBJ_T_OTHER:
    (*fiobj_object_metadata(o))->free2(o);
    return;
  }
}

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Internal: compares two nestable objects. */
FIOBJ_FUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
                                                FIOBJ restrict b,
                                                size_t nesting);

/** Compares two objects. */
FIO_IFUNC unsigned char FIO_NAME_BL(fiobj, eq)(FIOBJ a, FIOBJ b) {
  if (a == b)
    return 1;
  if (FIOBJ_TYPE_CLASS(a) != FIOBJ_TYPE_CLASS(b))
    return 0;
  switch (FIOBJ_TYPE_CLASS(a)) {
  case FIOBJ_T_PRIMITIVE:
  case FIOBJ_T_NUMBER: /* fall through */
  case FIOBJ_T_FLOAT:  /* fall through */
    return a == b;
  case FIOBJ_T_STRING:
    return FIO_NAME_BL(FIO_NAME(fiobj, FIOBJ___NAME_STRING), eq)(a, b);
  case FIOBJ_T_ARRAY:
    return fiobj___test_eq_nested(a, b, 0);
  case FIOBJ_T_HASH:
    return fiobj___test_eq_nested(a, b, 0);
  case FIOBJ_T_OTHER:
    if ((*fiobj_object_metadata(a))->count(a) ||
        (*fiobj_object_metadata(b))->count(b)) {
      if ((*fiobj_object_metadata(a))->count(a) !=
          (*fiobj_object_metadata(b))->count(b))
        return 0;
      return fiobj___test_eq_nested(a, b, 0);
    }
    return (*fiobj_object_metadata(a))->type_id ==
               (*fiobj_object_metadata(b))->type_id &&
           (*fiobj_object_metadata(a))->is_eq(a, b);
  }
  return 0;
}

/** Returns a temporary String representation for any FIOBJ object. */
FIO_IFUNC fio_str_info_s FIO_NAME2(fiobj, cstr)(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return (fio_str_info_s){.buf = (char *)"null", .len = 4};
    case FIOBJ_T_TRUE:
      return (fio_str_info_s){.buf = (char *)"true", .len = 4};
    case FIOBJ_T_FALSE:
      return (fio_str_info_s){.buf = (char *)"false", .len = 5};
    };
    return (fio_str_info_s){.buf = (char *)""};
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), cstr)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), cstr)(o);
  case FIOBJ_T_STRING:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
  case FIOBJ_T_ARRAY: /* fall through */
    return (fio_str_info_s){.buf = (char *)"[...]", .len = 5};
  case FIOBJ_T_HASH: {
    return (fio_str_info_s){.buf = (char *)"{...}", .len = 5};
  }
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_s(o);
  }
  if (!o)
    return (fio_str_info_s){.buf = (char *)"null", .len = 4};
  return (fio_str_info_s){.buf = (char *)""};
}

/** Returns an integer representation for any FIOBJ object. */
FIO_IFUNC intptr_t FIO_NAME2(fiobj, i)(FIOBJ o) {
  fio_str_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return 0;
    case FIOBJ_T_TRUE:
      return 1;
    case FIOBJ_T_FALSE:
      return 0;
    };
    return -1;
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(o);
  case FIOBJ_T_STRING:
    tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
    if (!tmp.len)
      return 0;
    return fio_atol(&tmp.buf);
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_i(o);
  }
  if (!o)
    return 0;
  return -1;
}

/** Returns a float (double) representation for any FIOBJ object. */
FIO_IFUNC double FIO_NAME2(fiobj, f)(FIOBJ o) {
  fio_str_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_FALSE: /* fall through */
    case FIOBJ_T_NULL:
      return 0.0;
    case FIOBJ_T_TRUE:
      return 1.0;
    };
    return -1.0;
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(o);
  case FIOBJ_T_STRING:
    tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
    if (!tmp.len)
      return 0;
    return (double)fio_atof(&tmp.buf);
  case FIOBJ_T_ARRAY:
    return (double)FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return (double)FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_f(o);
  }
  if (!o)
    return 0.0;
  return -1.0;
}

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

#define FIO_REF_NAME     fiobj___bignum
#define FIO_REF_TYPE     intptr_t
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___NUMBER_CLASS_VTBL;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
#include __FILE__

/* Places a 61 or 29 bit signed integer in the leftmost bits of a word. */
#define FIO_NUMBER_ENCODE(i) (((uintptr_t)(i) << 3) | FIOBJ_T_NUMBER)
/* Reads a 61 or 29 bit signed integer from the leftmost bits of a word. */
#define FIO_NUMBER_DECODE(i)                                                   \
  ((intptr_t)(((uintptr_t)(i) >> 3) |                                          \
              ((((uintptr_t)(i) >> ((sizeof(uintptr_t) * 8) - 1)) *            \
                ((uintptr_t)3 << ((sizeof(uintptr_t) * 8) - 3))))))

/** Creates a new Number object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                         new)(intptr_t i) {
  FIOBJ o = (FIOBJ)FIO_NUMBER_ENCODE(i);
  if (FIO_NUMBER_DECODE(o) == i)
    return o;
  o = fiobj___bignum_new2();

  FIO_PTR_MATH_RMASK(intptr_t, o, 3)[0] = i;
  return o;
}

/** Reads the number from a FIOBJ number. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return FIO_NUMBER_DECODE(i);
  return FIO_PTR_MATH_RMASK(intptr_t, i, 3)[0];
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(FIOBJ i) {
  return (double)FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i);
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return;
  fiobj___bignum_free2(i);
  return;
}

FIO_IFUNC unsigned char FIO_NAME_BL(fiobj___num, eq)(FIOBJ restrict a,
                                                     FIOBJ restrict b) {
  /* it should be safe to assume that FIOBJ_TYPE_CLASS(i) != FIOBJ_T_NUMBER */
  return FIO_PTR_MATH_RMASK(intptr_t, a, 3)[0] ==
         FIO_PTR_MATH_RMASK(intptr_t, b, 3)[0];
  // return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(a) ==
  //        FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(b);
}

#undef FIO_NUMBER_ENCODE
#undef FIO_NUMBER_DECODE

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

#define FIO_REF_NAME     fiobj___bigfloat
#define FIO_REF_TYPE     double
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___FLOAT_CLASS_VTBL;                                             \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
#include __FILE__

/** Creates a new Float object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(double i) {
  FIOBJ ui;
  if (sizeof(double) <= sizeof(FIOBJ)) {
    union {
      double d;
      uintptr_t i;
    } punned;
    punned.i = 0; /* dead code, but leave it, just in case */
    punned.d = i;
    if ((punned.i & 7) == 0) {
      return (FIOBJ)(punned.i | FIOBJ_T_FLOAT);
    }
  }
  ui = fiobj___bigfloat_new2();

  FIO_PTR_MATH_RMASK(double, ui, 3)[0] = i;
  return ui;
}

/** Reads the integer part from a FIOBJ Float. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(FIOBJ i) {
  return (intptr_t)floor(FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(i));
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(FIOBJ i) {
  if (sizeof(double) <= sizeof(FIOBJ) && FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT) {
    union {
      double d;
      uint64_t i;
    } punned;
    punned.d = 0; /* dead code, but leave it, just in case */
    punned.i = (uint64_t)(uintptr_t)i;
    punned.i = ((uint64_t)(uintptr_t)i & (~(uintptr_t)7ULL));
    return punned.d;
  }
  return FIO_PTR_MATH_RMASK(double, i, 3)[0];
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT)
    return;
  fiobj___bigfloat_free2(i);
  return;
}

/* *****************************************************************************
FIOBJ Basic Iteration
***************************************************************************** */

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIO_SFUNC uint32_t fiobj_each1(FIOBJ o,
                               int (*task)(fiobj_each_s *e),
                               void *udata,
                               int32_t start_at) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_STRING:    /* fall through */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), each)(
        o,
        (int (*)(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), each_s *)))task,
        udata,
        start_at);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), each)(
        o,
        (int (*)(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), each_s *)))task,
        udata,
        start_at);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->each1(o, task, udata, start_at);
  }
  return 0;
}

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

/** Calculates an object's hash value for a specific hash map object. */
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ target_hash, FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    return fio_risky_hash(&o,
                          sizeof(o),
                          (uint64_t)(uintptr_t)target_hash + (uintptr_t)o);
  case FIOBJ_T_NUMBER: {
    uintptr_t tmp = FIO_NAME2(fiobj, i)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), (uint64_t)(uintptr_t)target_hash);
  }
  case FIOBJ_T_FLOAT: {
    double tmp = FIO_NAME2(fiobj, f)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), (uint64_t)(uintptr_t)target_hash);
  }
  case FIOBJ_T_STRING: /* fall through */
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                    hash)(o, (uint64_t)(uintptr_t)target_hash);
  case FIOBJ_T_ARRAY: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
    h += fio_risky_hash(&h,
                        sizeof(h),
                        (uint64_t)(uintptr_t)target_hash + FIOBJ_T_ARRAY);
    {
      FIOBJ *a = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), ptr)(o);
      const size_t count =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
      if (a) {
        for (size_t i = 0; i < count; ++i) {
          h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_ARRAY + i, a[i]);
        }
      }
    }
    return h;
  }
  case FIOBJ_T_HASH: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
    size_t c = 0;
    h += fio_risky_hash(&h,
                        sizeof(h),
                        (uint64_t)(uintptr_t)target_hash + FIOBJ_T_HASH);
    FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), o, pos) {
      h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_HASH + (c++),
                                  pos->obj.key);
      h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_HASH + (c++),
                                  pos->obj.value);
    }
    return h;
  }
  case FIOBJ_T_OTHER: {
    /* TODO: can we avoid "stringifying" the object? */
    fio_str_info_s tmp = (*fiobj_object_metadata(o))->to_s(o);
    return fio_risky_hash(tmp.buf, tmp.len, (uint64_t)(uintptr_t)target_hash);
  }
  }
  return 0;
}

/** Inserts a value to a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set2)(FIOBJ hash, FIOBJ key, FIOBJ value) {
  return FIO_NAME(
      FIO_NAME(fiobj, FIOBJ___NAME_HASH),
      set)(hash, FIO_NAME2(fiobj, hash)(hash, key), key, value, NULL);
}

/**
 * Inserts a value to a hash map, with a default hash value calculation.
 *
 * If the key already exists in the Hash Map, the value will be freed instead.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set_if_missing2)(FIOBJ hash, FIOBJ key, FIOBJ value) {
  return FIO_NAME(
      FIO_NAME(fiobj, FIOBJ___NAME_HASH),
      set_if_missing)(hash, FIO_NAME2(fiobj, hash)(hash, key), key, value);
}

/** Finds a value in a hash map, automatically calculating the hash value. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(FIOBJ hash,
                                                                   FIOBJ key) {
  if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH)
    return FIOBJ_INVALID;
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  get)(hash, FIO_NAME2(fiobj, hash)(hash, key), key);
}

/** Removes a value from a hash map, with a default hash value calculation. */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash, FIOBJ key, FIOBJ *old) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  remove)(hash, FIO_NAME2(fiobj, hash)(hash, key), key, old);
}

/**
 * Sets a String value in a hash map, allocating the String and automatically
 * calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set3)(FIOBJ hash,
                               const char *key,
                               size_t len,
                               FIOBJ value) {
  FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(tmp, (char *)key, len);
  FIOBJ v = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                     set)(hash,
                          fio_risky_hash(key, len, (uint64_t)(uintptr_t)hash),
                          tmp,
                          value,
                          NULL);
  fiobj_free(tmp);
  return v;
}

/**
 * Finds a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get3)(FIOBJ hash, const char *buf, size_t len) {
  if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH)
    return FIOBJ_INVALID;
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  FIOBJ v = FIO_NAME(
      FIO_NAME(fiobj, FIOBJ___NAME_HASH),
      get)(hash, fio_risky_hash(buf, len, (uint64_t)(uintptr_t)hash), tmp);
  return v;
}

/**
 * Removes a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove3)(FIOBJ hash,
                                const char *buf,
                                size_t len,
                                FIOBJ *old) {
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  int r = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                   remove)(hash,
                           fio_risky_hash(buf, len, (uint64_t)(uintptr_t)hash),
                           tmp,
                           old);
  FIOBJ_STR_TEMP_DESTROY(tmp);
  return r;
}

/** Updates a hash using information from another Hash. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), update)(FIOBJ dest,
                                                                    FIOBJ src) {
  if (FIOBJ_TYPE_CLASS(dest) != FIOBJ_T_HASH ||
      FIOBJ_TYPE_CLASS(src) != FIOBJ_T_HASH)
    return;
  FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), src, i) {
    if (i->obj.key == FIOBJ_INVALID ||
        FIOBJ_TYPE_CLASS(i->obj.key) == FIOBJ_T_NULL) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove2)
      (dest, i->obj.key, NULL);
      continue;
    }
    register FIOBJ tmp;
    switch (FIOBJ_TYPE_CLASS(i->obj.value)) {
    case FIOBJ_T_ARRAY:
      /* TODO? decide if we should merge elements or overwrite...? */
      tmp =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(dest, i->obj.key);
      if (FIOBJ_TYPE_CLASS(tmp) == FIOBJ_T_ARRAY) {
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), concat)
        (tmp, i->obj.value);
        continue;
      }
      break;
    case FIOBJ_T_HASH:
      tmp =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(dest, i->obj.key);
      if (FIOBJ_TYPE_CLASS(tmp) == FIOBJ_T_HASH)
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), update)
      (dest, i->obj.value);
      else break;
      continue;
    case FIOBJ_T_NUMBER:    /* fall through */
    case FIOBJ_T_PRIMITIVE: /* fall through */
    case FIOBJ_T_STRING:    /* fall through */
    case FIOBJ_T_FLOAT:     /* fall through */
    case FIOBJ_T_OTHER:
      break;
    }
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)
    (dest, i->obj.key, fiobj_dup(i->obj.value));
  }
}

/* *****************************************************************************
FIOBJ JSON support (inline functions)
***************************************************************************** */

typedef struct {
  FIOBJ json;
  size_t level;
  uint8_t beautify;
} fiobj___json_format_internal__s;

/* internal helper function for recursive JSON formatting. */
FIOBJ_FUNC void fiobj___json_format_internal__(
    fiobj___json_format_internal__s *,
    FIOBJ);

/** Helper function, calls `fiobj_hash_update_json` with string information */
FIO_IFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                          update_json2)(FIOBJ hash, char *ptr, size_t len) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  update_json)(hash, (fio_str_info_s){.buf = ptr, .len = len});
}

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIO_IFUNC FIOBJ FIO_NAME2(fiobj, json)(FIOBJ dest, FIOBJ o, uint8_t beautify) {
  fiobj___json_format_internal__s args =
      (fiobj___json_format_internal__s){.json = dest, .beautify = beautify};
  if (FIOBJ_TYPE_CLASS(dest) != FIOBJ_T_STRING)
    args.json = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  fiobj___json_format_internal__(&args, o);
  return args.json;
}

/* *****************************************************************************
FIOBJ - Implementation
***************************************************************************** */
#ifdef FIOBJ_EXTERN_COMPLETE

/* *****************************************************************************
FIOBJ Basic Object vtable
***************************************************************************** */

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___OBJECT_CLASS_VTBL = {
    .type_id = 99, /* type IDs below 100 are reserved. */
};

/* *****************************************************************************
FIOBJ Complex Iteration
***************************************************************************** */
typedef struct {
  FIOBJ obj;
  size_t pos;
} fiobj____stack_element_s;

#define FIO_ARRAY_NAME fiobj____active_stack
#define FIO_ARRAY_TYPE fiobj____stack_element_s
#define FIO_ARRAY_COPY(dest, src)                                              \
  do {                                                                         \
    (dest).obj = fiobj_dup((src).obj);                                         \
    (dest).pos = (src).pos;                                                    \
  } while (0)
#define FIO_ARRAY_TYPE_CMP(a, b) (a).obj == (b).obj
#define FIO_ARRAY_DESTROY(o)     fiobj_free(o)
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
#include __FILE__
#define FIO_ARRAY_TYPE_CMP(a, b) (a).obj == (b).obj
#define FIO_ARRAY_NAME           fiobj____stack
#define FIO_ARRAY_TYPE           fiobj____stack_element_s
#define FIO_MEM_REALLOC_         FIOBJ_MEM_REALLOC
#define FIO_MEM_FREE_            FIOBJ_MEM_FREE
#define FIO_MEM_REALLOC_IS_SAFE_ FIOBJ_MEM_REALLOC_IS_SAFE
#include __FILE__

typedef struct {
  int (*task)(fiobj_each_s *info);
  void *arg;
  FIOBJ next;
  size_t count;
  fiobj____stack_s stack;
  uint32_t end;
  uint8_t stop;
} fiobj_____each2_data_s;

FIO_SFUNC uint32_t fiobj____each2_element_count(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_STRING:    /* fall through */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER: /* fall through */
    return (*fiobj_object_metadata(o))->count(o);
  }
  return 0;
}
FIO_SFUNC int fiobj____each2_wrapper_task(fiobj_each_s *e) {
  fiobj_____each2_data_s *d = (fiobj_____each2_data_s *)e->udata;
  e->task = d->task;
  e->udata = d->arg;
  d->stop = (d->task(e) == -1);
  d->task = e->task;
  d->arg = e->udata;
  e->task = fiobj____each2_wrapper_task;
  e->udata = d;
  ++d->count;
  if (d->stop)
    return -1;
  uint32_t c = fiobj____each2_element_count(e->value);
  if (c) {
    d->next = e->value;
    d->end = c;
    return -1;
  }
  return 0;
}

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNC uint32_t fiobj_each2(FIOBJ o,
                                int (*task)(fiobj_each_s *),
                                void *udata) {
  /* TODO - move to recursion with nesting limiter? */
  fiobj_____each2_data_s d = {
      .task = task,
      .arg = udata,
      .next = FIOBJ_INVALID,
      .stack = FIO_ARRAY_INIT,
  };
  struct FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), each_s) e_tmp = {

      .parent = FIOBJ_INVALID,
      .task = (int (*)(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                                each_s) *))fiobj____each2_wrapper_task,
      .udata = &d,
      .value = o,
  };
  fiobj____stack_element_s i = {.obj = o, .pos = 0};
  uint32_t end = fiobj____each2_element_count(o);
  fiobj____each2_wrapper_task((fiobj_each_s *)&e_tmp);
  while (!d.stop && i.obj && i.pos < end) {
    i.pos = fiobj_each1(i.obj, fiobj____each2_wrapper_task, &d, i.pos);
    if (d.next != FIOBJ_INVALID) {
      if (fiobj____stack_count(&d.stack) + 1 > FIOBJ_MAX_NESTING) {
        FIO_LOG_ERROR("FIOBJ nesting level too deep (%u)."
                      "`fiobj_each2` stopping loop early.",
                      (unsigned int)fiobj____stack_count(&d.stack));
        d.stop = 1;
        continue;
      }
      fiobj____stack_push(&d.stack, i);
      i.pos = 0;
      i.obj = d.next;
      d.next = FIOBJ_INVALID;
      end = d.end;
    } else {
      /* re-collect end position to acommodate for changes */
      end = fiobj____each2_element_count(i.obj);
    }
    while (i.pos >= end && fiobj____stack_count(&d.stack)) {
      fiobj____stack_pop(&d.stack, &i);
      end = fiobj____each2_element_count(i.obj);
    }
  };
  fiobj____stack_destroy(&d.stack);
  return d.count;
}

/* *****************************************************************************
FIOBJ Hash / Array / Other (enumerable) Equality test.
***************************************************************************** */

/** Internal: compares two nestable objects. */
FIOBJ_FUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
                                                FIOBJ restrict b,
                                                size_t nesting) {
  if (a == b)
    return 1;
  if (FIOBJ_TYPE_CLASS(a) != FIOBJ_TYPE_CLASS(b))
    return 0;
  if (fiobj____each2_element_count(a) != fiobj____each2_element_count(b))
    return 0;
  if (nesting >= FIOBJ_MAX_NESTING)
    return 0;

  ++nesting;

  switch (FIOBJ_TYPE_CLASS(a)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_FLOAT:
    return a == b;
  case FIOBJ_T_STRING:
    return FIO_NAME_BL(FIO_NAME(fiobj, FIOBJ___NAME_STRING), eq)(a, b);

  case FIOBJ_T_ARRAY:
    if (!fiobj____each2_element_count(a))
      return 1;
    /* test each array member with matching index */
    {
      const size_t count = fiobj____each2_element_count(a);
      for (size_t i = 0; i < count; ++i) {
        if (!fiobj___test_eq_nested(
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a, i),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(b, i),
                nesting))
          return 0;
      }
    }
    return 1;

  case FIOBJ_T_HASH:
    if (!fiobj____each2_element_count(a))
      return 1;
    FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), a, pos) {
      FIOBJ val = fiobj_hash_get2(b, pos->obj.key);
      if (!fiobj___test_eq_nested(val, pos->obj.value, nesting))
        return 0;
    }
    return 1;
  case FIOBJ_T_OTHER:
    if (!fiobj____each2_element_count(a) &&
        (*fiobj_object_metadata(a))->is_eq(a, b))
      return 1;
    /* TODO: iterate through objects and test equality within nesting */
    return (*fiobj_object_metadata(a))->is_eq(a, b);
    return 1;
  }
  return 0;
}

/* *****************************************************************************
FIOBJ general helpers
***************************************************************************** */

FIO_SFUNC uint32_t fiobj___count_noop(FIOBJ o) {
  return 0;
  (void)o;
}

/* *****************************************************************************
FIOBJ Integers (bigger numbers)
***************************************************************************** */

FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                                    cstr)(FIOBJ i) {
  static char buf[22 * 256];
  static uint8_t pos = 0;
  size_t at = fio_atomic_add(&pos, 1);
  char *tmp = buf + (at * 22);
  size_t len =
      fio_ltoa(tmp, FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i), 10);
  tmp[len] = 0;
  return (fio_str_info_s){.buf = tmp, .len = len};
}

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_NUMBER,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = FIO_NAME_BL(fiobj___num, eq),
    /** Converts an object to a String */
    .to_s = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), cstr),
    /** Converts and object to an integer */
    .to_i = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i),
    /** Converts and object to a float */
    .to_f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f),
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj___count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj___bignum_free2,
};

/* *****************************************************************************
FIOBJ Floats (bigger / smaller doubles)
***************************************************************************** */

FIO_SFUNC unsigned char FIO_NAME_BL(fiobj___float, eq)(FIOBJ restrict a,
                                                       FIOBJ restrict b) {
  unsigned char r = 0;
  union {
    uint64_t u;
    double f;
  } da, db;
  da.f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(a);
  db.f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(b);
  /* regular equality? */
  r |= da.f == db.f;
  /* test for small rounding errors (4 bit difference) on normalize floats */
  r |= !((da.u ^ db.u) & UINT64_C(0xFFFFFFFFFFFFFFF0)) &&
       (da.u & UINT64_C(0x7FF0000000000000));
  /* test for small ULP: */
  r |= (((da.u > db.u) ? da.u - db.u : db.u - da.u) < 2);
  /* test for +-0 */
  r |= !((da.u | db.u) & UINT64_C(0x7FFFFFFFFFFFFFFF));
  return r;
}

FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                                    cstr)(FIOBJ i) {
  static char buf[32 * 256];
  static uint8_t pos = 0;
  size_t at = fio_atomic_add(&pos, 1);
  char *tmp = buf + (at << 5);
  size_t len =
      fio_ftoa(tmp, FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(i), 10);
  tmp[len] = 0;
  return (fio_str_info_s){.buf = tmp, .len = len};
}

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_FLOAT,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = FIO_NAME_BL(fiobj___float, eq),
    /** Converts an object to a String */
    .to_s = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), cstr),
    /** Converts and object to an integer */
    .to_i = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i),
    /** Converts and object to a float */
    .to_f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f),
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj___count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj___bigfloat_free2,
};

/* *****************************************************************************
FIOBJ JSON support - output
***************************************************************************** */

FIO_IFUNC void fiobj___json_format_internal_beauty_pad(FIOBJ json,
                                                       size_t level) {
  size_t pos = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json);
  fio_str_info_s tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                resize)(json, (level << 1) + pos + 2);
  tmp.buf[pos++] = '\r';
  tmp.buf[pos++] = '\n';
  for (size_t i = 0; i < level; ++i) {
    tmp.buf[pos++] = ' ';
    tmp.buf[pos++] = ' ';
  }
}

FIOBJ_FUNC void fiobj___json_format_internal__(
    fiobj___json_format_internal__s *args,
    FIOBJ o) {
  switch (FIOBJ_TYPE(o)) {
  case FIOBJ_T_TRUE:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
    (args->json, "true", 4);
    return;
  case FIOBJ_T_FALSE:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
    (args->json, "false", 5);
    return;
  case FIOBJ_T_NULL:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
    (args->json, "null", 4);
    return;
  case FIOBJ_T_NUMBER:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)
    (args->json, FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(o));
    return;
  case FIOBJ_T_FLOAT: {
    char tmp_buf[256];
    size_t len = fio_ftoa(tmp_buf,
                          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(o),
                          10);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
    (args->json, tmp_buf, len);
    return;
  }
  case FIOBJ_T_STRING: /* fall through */
  default: {
    fio_str_info_s info = FIO_NAME2(fiobj, cstr)(o);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "\"", 1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
    (args->json, info.buf, info.len);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "\"", 1);
    return;
  }
  case FIOBJ_T_ARRAY:
    if (!FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o))
      goto empty_array;
    if (args->level == FIOBJ_MAX_NESTING)
      goto err_array_nesting;
    {
      ++args->level;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "[", 1);
      const uint32_t len =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
      if (args->beautify) {
        fiobj___json_format_internal_beauty_pad(args->json, args->level);
      }
      fiobj___json_format_internal__(
          args,
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(o, 0));
      if (args->beautify) {
        for (size_t i = 1; i < len; ++i) {
          FIOBJ child =
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(o, i);
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
          (args->json, ",", 1);
          fiobj___json_format_internal_beauty_pad(args->json, args->level);
          fiobj___json_format_internal__(args, child);
        }
      } else {
        for (size_t i = 1; i < len; ++i) {
          FIOBJ child =
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(o, i);
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
          (args->json, ",", 1);
          fiobj___json_format_internal__(args, child);
        }
      }
      --args->level;
      if (args->beautify) {
        fiobj___json_format_internal_beauty_pad(args->json, args->level);
      }
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "]", 1);
    }
    return;
  case FIOBJ_T_HASH:
    if (!FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o))
      goto empty_hash;
    if (args->level == FIOBJ_MAX_NESTING)
      goto err_hash_nesting;
    {
      size_t i = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
      (args->json, "{", 1);
      ++args->level;
      FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), o, couplet) {
        if (args->beautify) {
          fiobj___json_format_internal_beauty_pad(args->json, args->level);
        }
        fio_str_info_s info = FIO_NAME2(fiobj, cstr)(couplet->obj.key);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, "\"", 1);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
        (args->json, info.buf, info.len);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, "\":", 2);
        fiobj___json_format_internal__(args, couplet->obj.value);
        if (--i)
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, ",", 1);
      }
      --args->level;
      if (args->beautify) {
        fiobj___json_format_internal_beauty_pad(args->json, args->level);
      }
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
      (args->json, "}", 1);
    }
    return;
  }
empty_hash:
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
  (args->json, "{}", 2);
  return;
empty_array:
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
  (args->json, "[]", 2);
  return;
err_array_nesting:
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
  (args->json, "[ ]", 3);
  goto log_nesting_error;
err_hash_nesting:
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
  (args->json, "{ }", 3);
log_nesting_error:
  FIO_LOG_ERROR("JSON formatting truncated - nesting level too deep.");
}

/* *****************************************************************************
FIOBJ JSON parsing
***************************************************************************** */

#define FIO_JSON
#include __FILE__

/* FIOBJ JSON parser */
typedef struct {
  fio_json_parser_s p;
  FIOBJ key;
  FIOBJ top;
  FIOBJ target;
  FIOBJ stack[JSON_MAX_DEPTH + 1];
  uint8_t so; /* stack offset */
} fiobj_json_parser_s;

static inline void fiobj_json_add2parser(fiobj_json_parser_s *p, FIOBJ o) {
  if (p->top) {
    if (FIOBJ_TYPE_CLASS(p->top) == FIOBJ_T_HASH) {
      if (p->key) {
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(p->top, p->key, o);
        fiobj_free(p->key);
        p->key = FIOBJ_INVALID;
      } else {
        p->key = o;
      }
    } else {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(p->top, o);
    }
  } else {
    p->top = o;
  }
}

/** a NULL object was detected */
static inline void fio_json_on_null(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
}
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_TRUE)());
}
/** a FALSE object was detected */
static inline void fio_json_on_false(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_FALSE)());
}
/** a Numberl was detected (long long). */
static inline void fio_json_on_number(fio_json_parser_s *p, long long i) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i));
}
/** a Float was detected (double). */
static inline void fio_json_on_float(fio_json_parser_s *p, double f) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(f));
}
/** a String was detected (int / float). update `pos` to point at ending */
static inline void fio_json_on_string(fio_json_parser_s *p,
                                      const void *start,
                                      size_t len) {
  FIOBJ str = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_unescape)
  (str, start, len);
  fiobj_json_add2parser((fiobj_json_parser_s *)p, str);
}
/** a dictionary object was detected */
static inline int fio_json_on_start_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->target) {
    /* push NULL, don't free the objects */
    pr->stack[pr->so++] = FIOBJ_INVALID;
    pr->top = pr->target;
    pr->target = FIOBJ_INVALID;
  } else {
    FIOBJ hash;
#if FIOBJ_JSON_APPEND
    hash = FIOBJ_INVALID;
    if (pr->key && FIOBJ_TYPE_CLASS(pr->top) == FIOBJ_T_HASH) {
      hash =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(pr->top, pr->key);
    }
    if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH) {
      hash = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
      fiobj_json_add2parser(pr, hash);
    } else {
      fiobj_free(pr->key);
      pr->key = FIOBJ_INVALID;
    }
#else
    hash = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    fiobj_json_add2parser(pr, hash);
#endif
    pr->stack[pr->so++] = pr->top;
    pr->top = hash;
  }
  return 0;
}
/** a dictionary object closure detected */
static inline void fio_json_on_end_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->key) {
    FIO_LOG_WARNING("(JSON parsing) malformed JSON, "
                    "ignoring dangling Hash key.");
    fiobj_free(pr->key);
    pr->key = FIOBJ_INVALID;
  }
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** an array object was detected */
static int fio_json_on_start_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  FIOBJ ary = FIOBJ_INVALID;
  if (pr->target != FIOBJ_INVALID) {
    if (FIOBJ_TYPE_CLASS(pr->target) != FIOBJ_T_ARRAY)
      return -1;
    ary = pr->target;
    pr->target = FIOBJ_INVALID;
  }
#if FIOBJ_JSON_APPEND
  if (pr->key && FIOBJ_TYPE_CLASS(pr->top) == FIOBJ_T_HASH) {
    ary = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(pr->top, pr->key);
  }
  if (FIOBJ_TYPE_CLASS(ary) != FIOBJ_T_ARRAY) {
    ary = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    fiobj_json_add2parser(pr, ary);
  } else {
    fiobj_free(pr->key);
    pr->key = FIOBJ_INVALID;
  }
#else
  FIOBJ ary = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
  fiobj_json_add2parser(pr, ary);
#endif

  pr->stack[pr->so++] = pr->top;
  pr->top = ary;
  return 0;
}
/** an array closure was detected */
static inline void fio_json_on_end_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** the JSON parsing is complete */
static void fio_json_on_json(fio_json_parser_s *p) {
  (void)p; /* nothing special... right? */
}
/** the JSON parsing is complete */
static inline void fio_json_on_error(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  fiobj_free(pr->stack[0]);
  fiobj_free(pr->key);
  *pr = (fiobj_json_parser_s){.top = FIOBJ_INVALID};
  FIO_LOG_DEBUG("JSON on_error callback called.");
}

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (preferring the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                           update_json)(FIOBJ hash, fio_str_info_s str) {
  if (hash == FIOBJ_INVALID)
    return 0;
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID, .target = hash};
  size_t consumed = fio_json_parse(&p.p, str.buf, str.len);
  fiobj_free(p.key);
  if (p.top != hash)
    fiobj_free(p.top);
  return consumed;
}

/** Returns a JSON valid FIOBJ String, representing the object. */
FIOBJ_FUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed_p) {
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID};
  register const size_t consumed = fio_json_parse(&p.p, str.buf, str.len);
  if (consumed_p) {
    *consumed_p = consumed;
  }
  if (!consumed || p.p.depth) {
    if (p.top) {
      FIO_LOG_DEBUG("WARNING - JSON failed secondary validation, no on_error");
    }
#ifdef DEBUG
    FIOBJ s = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, p.top, 0);
    FIO_LOG_DEBUG("JSON data being deleted:\n%s",
                  FIO_NAME2(fiobj, cstr)(s).buf);
    fiobj_free(s);
#endif
    fiobj_free(p.stack[0]);
    p.top = FIOBJ_INVALID;
  }
  fiobj_free(p.key);
  return p.top;
}

/** Uses JSON (JavaScript) notation to find data in an object structure. Returns
 * a temporary object. */
FIOBJ_FUNC FIOBJ fiobj_json_find(FIOBJ o, fio_str_info_s n) {
  for (;;) {
  top:
    if (!n.len)
      return o;
    switch (FIOBJ_TYPE_CLASS(o)) {
    case FIOBJ_T_ARRAY: {
      if (n.len <= 2 || n.buf[0] != '[' || n.buf[1] < '0' || n.buf[1] > '9')
        return FIOBJ_INVALID;
      size_t i = 0;
      ++n.buf;
      --n.len;
      while (n.len && fio_c2i(n.buf[0]) < 10) {
        i = (i * 10) + fio_c2i(n.buf[0]);
        ++n.buf;
        --n.len;
      }
      if (!n.len || n.buf[0] != ']')
        return FIOBJ_INVALID;
      o = fiobj_array_get(o, i);
      ++n.buf;
      --n.len;
      if (n.len) {
        if (n.buf[0] == '.') {
          ++n.buf;
          --n.len;
        } else if (n.buf[0] != '[') {
          return FIOBJ_INVALID;
        }
        continue;
      }
      return o;
    }
    case FIOBJ_T_HASH: {
      FIOBJ tmp = fiobj_hash_get3(o, n.buf, n.len);
      if (tmp != FIOBJ_INVALID)
        return tmp;
      char *end = n.buf + n.len - 1;
      while (end > n.buf) {
        while (end > n.buf && end[0] != '.' && end[0] != '[')
          --end;
        if (end == n.buf)
          return FIOBJ_INVALID;
        const size_t t_len = end - n.buf;
        tmp = fiobj_hash_get3(o, n.buf, t_len);
        if (tmp != FIOBJ_INVALID) {
          o = tmp;
          n.len -= t_len + (end[0] == '.');
          n.buf = end + (end[0] == '.');
          goto top;
        }
        --end;
      }
    } /* fall through */
    default:
      return FIOBJ_INVALID;
    }
  }
}

/* *****************************************************************************
FIOBJ and JSON testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC int FIO_NAME_TEST(stl, fiobj_task)(fiobj_each_s *e) {
  static size_t index = 0;
  if (!e) {
    index = 0;
    return -1;
  }
  int *expect = (int *)e->udata;
  FIO_ASSERT(e->key == FIOBJ_INVALID, "key is set in an Array loop?");
  if (expect[index] == -1) {
    FIO_ASSERT(FIOBJ_TYPE(e->value) == FIOBJ_T_ARRAY,
               "each2 ordering issue [%zu] (array).",
               index);
  } else {
    FIO_ASSERT(FIO_NAME2(fiobj, i)(e->value) == expect[index],
               "each2 ordering issue [%zu] (number) %ld != %d",
               index,
               FIO_NAME2(fiobj, i)(e->value),
               expect[index]);
  }
  ++index;
  return 0;
}

FIO_SFUNC void FIO_NAME_TEST(stl, fiobj)(void) {
  FIOBJ o = FIOBJ_INVALID;
  if (!FIOBJ_MARK_MEMORY_ENABLED) {
    FIO_LOG_WARNING("FIOBJ defined without allocation counter. "
                    "Tests might not be complete.");
  }
  /* primitives - (in)sanity */
  {
    fprintf(stderr, "* Testing FIOBJ primitives.\n");
    FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_NULL,
               "invalid FIOBJ type should be FIOBJ_T_NULL.");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(o, FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "invalid FIOBJ is NOT a fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(),
                                       FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_true() is NOT fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                       FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_false() is NOT fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                       FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()),
               "fiobj_false() is NOT fiobj_true().");
    FIO_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_NULL)()) == FIOBJ_T_NULL,
               "fiobj_null() type should be FIOBJ_T_NULL.");
    FIO_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()) == FIOBJ_T_TRUE,
               "fiobj_true() type should be FIOBJ_T_TRUE.");
    FIO_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)()) ==
                   FIOBJ_T_FALSE,
               "fiobj_false() type should be FIOBJ_T_FALSE.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_NULL)(),
                                      FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_null() should be equal to self.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(),
                                      FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()),
               "fiobj_true() should be equal to self.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                      FIO_NAME(fiobj, FIOBJ___NAME_FALSE)()),
               "fiobj_false() should be equal to self.");
  }
  {
    fprintf(stderr, "* Testing FIOBJ integers.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i);
      FIO_ASSERT(FIO_NAME2(fiobj, i)(o) == (intptr_t)i,
                 "Number not reversible at bit %d (%zd != %zd)!",
                 (int)bit,
                 (ssize_t)FIO_NAME2(fiobj, i)(o),
                 (ssize_t)i);
      fio_str_info_s str = FIO_NAME2(fiobj, cstr)(o);
      char *str_buf = str.buf;
      FIO_ASSERT(fio_atol(&str_buf) == (intptr_t)i,
                 "Number atol not reversible at bit %d (%s != %zd)!",
                 (int)bit,
                 str.buf,
                 (ssize_t)i);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_ASSERT(allocation_flags == 3,
               "no bits are allocated / no allocations optimized away (%d)",
               (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ floats.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(double) * 8); ++bit) {
      union {
        double d;
        uint64_t i;
      } punned;
      punned.i = (uint64_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(punned.d);
      FIO_ASSERT(FIO_NAME2(fiobj, f)(o) == punned.d,
                 "Float not reversible at bit %d (%lf != %lf)!",
                 (int)bit,
                 FIO_NAME2(fiobj, f)(o),
                 punned.d);

      fio_str_info_s str = FIO_NAME2(fiobj, cstr)(o);
      char buf_tmp[32];
      FIO_ASSERT(fio_ftoa(buf_tmp, FIO_NAME2(fiobj, f)(o), 10) == str.len,
                 "fio_atof length didn't match Float's fiobj2cstr length.");
      FIO_ASSERT(!memcmp(str.buf, buf_tmp, str.len),
                 "fio_atof string didn't match Float's fiobj2cstr.");
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_FLOAT) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_ASSERT(allocation_flags == 3,
               "no bits are allocated / no allocations optimized away (%d)",
               (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ each2.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
    for (int i = 1; i < 10; ++i) // 1, 2, 3 ... 10
    {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i));
      if (i % 3 == 0) {
        a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
      }
    }
    int expectation[] =
        {-1 /* array */, -1, 1, 2, 3, -1, 4, 5, 6, -1, 7, 8, 9, -1};
    size_t c =
        fiobj_each2(o, FIO_NAME_TEST(stl, fiobj_task), (void *)expectation);
    FIO_ASSERT(c == FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o) +
                        9 + 1,
               "each2 repetition count error");
    fiobj_free(o);
    FIO_NAME_TEST(stl, fiobj_task)(NULL);
  }
  {
    fprintf(stderr, "* Testing FIOBJ JSON handling.\n");
    char json[] =
        "                    "
        "\n# comment 1"
        "\n// comment 2"
        "\n/* comment 3 */"
        "{\"true\":true,\"false\":false,\"null\":null,\"array\":[1,2,3,4.2,"
        "\"five\"],"
        "\"string\":\"hello\\tjson\\bworld!\\r\\n\",\"hash\":{\"true\":true,"
        "\"false\":false},\"array2\":[1,2,3,4.2,\"five\",{\"hash\":true},[{"
        "\"hash\":{\"true\":true}}]]}";
    o = fiobj_json_parse2(json, strlen(json), NULL);
    FIO_ASSERT(o, "JSON parsing failed - no data returned.");
    FIO_ASSERT(fiobj_json_find2(o, (char *)"array2[6][0].hash.true", 22) ==
                   fiobj_true(),
               "fiobj_json_find2 failed");
    FIOBJ j = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, o, 0);
#ifdef DEBUG
    fprintf(stderr, "JSON: %s\n", FIO_NAME2(fiobj, cstr)(j).buf);
#endif
    FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(j) ==
                   strlen(json + 61),
               "JSON roundtrip failed (length error).");
    FIO_ASSERT(!memcmp(json + 61,
                       FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(j),
                       strlen(json + 61)),
               "JSON roundtrip failed (data error).");
    fiobj_free(o);
    fiobj_free(j);
    o = FIOBJ_INVALID;
  }
  {
    fprintf(stderr, "* Testing FIOBJ array equality test (fiobj_is_eq).\n");
    FIOBJ a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, n1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, n2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a1, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a2, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(a1, a2), "equal arrays aren't equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_false());
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n1, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n2, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a1, 0, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a2, -1, NULL);
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    fiobj_free(a1);
    fiobj_free(a2);
  }
  {
    fprintf(stderr, "* Testing FIOBJ array ownership.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (int i = 1; i <= TEST_REPEAT; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, tmp);
    }
    FIOBJ shifted = FIOBJ_INVALID;
    FIOBJ popped = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ set = FIOBJ_INVALID;
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), shift)(a, &shifted);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), pop)(a, &popped);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), set)
    (a, 1, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(), &set);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a, 2, &removed);
    fiobj_free(a);
    if (1) {
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped) ==
                  strlen("number: " FIO_MACRO2STR(TEST_REPEAT)) &&
              !memcmp(
                  "number: " FIO_MACRO2STR(TEST_REPEAT),
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped),
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped)),
          "Object popped from Array lost it's value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped));
      FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(shifted) ==
                         9 &&
                     !memcmp("number: 1",
                             FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                       ptr)(shifted),
                             9),
                 "Object shifted from Array lost it's value %s",
                 FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(shifted));
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) == 9 &&
              !memcmp("number: 3",
                      FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                      9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) == 9 &&
              !memcmp(
                  "number: 4",
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                  9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    }
    fiobj_free(shifted);
    fiobj_free(popped);
    fiobj_free(set);
    fiobj_free(removed);
  }
  {
    fprintf(stderr, "* Testing FIOBJ array ownership after concat.\n");
    FIOBJ a1, a2;
    a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (int i = 0; i < TEST_REPEAT; ++i) {
      FIOBJ str = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(str, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, str);
    }
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), concat)(a2, a1);
    fiobj_free(a1);
    for (int i = 0; i < TEST_REPEAT; ++i) {
      FIOBJ_STR_TEMP_VAR(tmp);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)) ==
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp),
          "string length zeroed out - string freed?");
      FIO_ASSERT(
          !memcmp(
              FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(tmp),
              FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)),
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp)),
          "string data error - string freed?");
      FIOBJ_STR_TEMP_DESTROY(tmp);
    }
    fiobj_free(a2);
  }
  {
    fprintf(stderr, "* Testing FIOBJ hash ownership.\n");
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    for (int i = 1; i <= TEST_REPEAT; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(o, k, tmp);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set_if_missing2)
      (o, k, fiobj_dup(tmp));
      fiobj_free(k);
    }

    FIOBJ set = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove2)(o, k, &removed);
    fiobj_free(k);
    k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)
    (o, fiobj2hash(o, k), k, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(), &set);
    fiobj_free(k);
    FIO_ASSERT(set, "fiobj_hash_set2 didn't copy information to old pointer?");
    FIO_ASSERT(removed,
               "fiobj_hash_remove2 didn't copy information to old pointer?");
    // fiobj_hash_set(o, uintptr_t hash, FIOBJ key, FIOBJ value, FIOBJ *old)
    FIO_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) ==
                strlen("number: 1") &&
            !memcmp(
                "number: 1",
                FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    FIO_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) ==
                strlen("number: 2") &&
            !memcmp("number: 2",
                    FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));

    fiobj_free(removed);
    fiobj_free(set);
    fiobj_free(o);
  }

#if FIOBJ_MARK_MEMORY
  {
    fprintf(stderr, "* Testing FIOBJ for memory leaks.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), reserve)(a, 64);
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i));
    }
    FIOBJ h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    FIOBJ key = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(key, "array", 5);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(h, key, a);
    FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(h, key) == a,
               "FIOBJ Hash retrival failed");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, key);
    if (0) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_FALSE)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(0.42));

      FIOBJ json = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, h, 0);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(json, "\n", 1);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), reserve)
      (json,
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json)
           << 1); /* prevent memory realloc */
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
      (json,
       FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(json),
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json) - 1);
      fprintf(stderr, "%s\n", FIO_NAME2(fiobj, cstr)(json).buf);
      fiobj_free(json);
    }
    fiobj_free(h);

    FIO_ASSERT(FIOBJ_MARK_MEMORY_ALLOC_COUNTER ==
                   FIOBJ_MARK_MEMORY_FREE_COUNTER,
               "FIOBJ leak detected (freed %zu/%zu)",
               FIOBJ_MARK_MEMORY_FREE_COUNTER,
               FIOBJ_MARK_MEMORY_ALLOC_COUNTER);
  }
#endif
  fprintf(stderr, "* Passed.\n");
}
#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
FIOBJ cleanup
***************************************************************************** */

#endif /* FIOBJ_EXTERN_COMPLETE */
#undef FIOBJ_FUNC
#undef FIOBJ_IFUNC
#undef FIOBJ_EXTERN
#undef FIOBJ_EXTERN_COMPLETE
#undef FIOBJ_EXTERN_OBJ
#undef FIOBJ_EXTERN_OBJ_IMP
#endif /* FIO_FIOBJ */
#undef FIO_FIOBJ
