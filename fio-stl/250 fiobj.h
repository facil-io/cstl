/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_FIOBJ              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
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

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_FIOBJ) && !defined(H___FIO_FIOBJ___H) &&                       \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_FIOBJ___H
#undef FIO_FIOBJ
#define FIO___RECURSIVE_INCLUDE 99 /* 99 keeps EXTERN rules the same */
/* *****************************************************************************
FIOBJ compilation settings (type names and JSON nesting limits).

Type Naming Macros for FIOBJ types. By default, results in:
- fiobj_true()     (constant, cannot be changed except manually)
- fiobj_false()    (constant, cannot be changed except manually)
- fiobj_null()
- fiobj_num_new() ... (etc')
- fiobj_float_new() ... (etc')
- fiobj_str_new() ... (etc')
- fiobj_array_new() ... (etc')
- fiobj_hash_new() ... (etc')
***************************************************************************** */

#ifndef FIOBJ___NAME_NULL
#define FIOBJ___NAME_NULL null
#endif
#ifndef FIOBJ___NAME_NUMBER
#define FIOBJ___NAME_NUMBER num
#endif
#ifndef FIOBJ___NAME_FLOAT
#define FIOBJ___NAME_FLOAT float
#endif
#ifndef FIOBJ___NAME_STRING
#define FIOBJ___NAME_STRING str
#endif
#ifndef FIOBJ___NAME_ARRAY
#define FIOBJ___NAME_ARRAY array
#endif
#ifndef FIOBJ___NAME_HASH
#define FIOBJ___NAME_HASH hash
#endif

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

/* make sure roundtrips work: parsed JSON must be printable */
FIO_ASSERT_STATIC(FIO_JSON_MAX_DEPTH <= FIOBJ_MAX_NESTING,
                  "FIO_JSON_MAX_DEPTH must not exceed FIOBJ_MAX_NESTING");

#ifndef FIOBJ_JSON_APPEND
#define FIOBJ_JSON_APPEND 1
#endif
/* *****************************************************************************
General Requirements / Macros
***************************************************************************** */

#ifdef __cplusplus /* C++ doesn't allow declarations for static variables */
#define FIOBJ_EXTERN_OBJ     extern "C" FIO_WEAK
#define FIOBJ_EXTERN_OBJ_IMP extern "C" FIO_WEAK
#elif defined(FIO_EXTERN)
#define FIOBJ_EXTERN_OBJ     extern
#define FIOBJ_EXTERN_OBJ_IMP FIO_WEAK
#else
#define FIOBJ_EXTERN_OBJ     static FIO_MAYBE_UNUSED
#define FIOBJ_EXTERN_OBJ_IMP static FIO_MAYBE_UNUSED
#endif
/* *****************************************************************************
Debugging / Leak Detection
***************************************************************************** */
#if defined(TEST) || defined(DEBUG) || defined(FIO_LEAK_COUNTER)
size_t FIO_WEAK_VAR FIOBJ_MARK_MEMORY_ALLOC_COUNTER;
size_t FIO_WEAK_VAR FIOBJ_MARK_MEMORY_FREE_COUNTER;
#define FIOBJ_MARK_MEMORY_ALLOC()                                              \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_ALLOC_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_FREE()                                               \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_FREE_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_PRINT()                                              \
  FIO___LOG_PRINT_LEVEL(                                                       \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? 4 /* FIO_LOG_LEVEL_INFO */                                        \
           : 3 /* FIO_LOG_LEVEL_WARNING */),                                   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? "INFO: total remaining FIOBJ allocations: %zu (%zu - %zu)"        \
           : "WARNING: LEAKED! FIOBJ allocations: %zu (%zu - %zu)"),           \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER - FIOBJ_MARK_MEMORY_FREE_COUNTER,        \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER,                                         \
      FIOBJ_MARK_MEMORY_FREE_COUNTER)
/** If true, FIOBJ memory allocation counting is enabled. */
#define FIOBJ_MARK_MEMORY_ENABLED 1

#else

#define FIOBJ_MARK_MEMORY_ALLOC_COUNTER 0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_FREE_COUNTER  0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_ALLOC()
#define FIOBJ_MARK_MEMORY_FREE()
#define FIOBJ_MARK_MEMORY_PRINT()
/** If true, FIOBJ memory allocation counting is enabled. */
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
#define FIOBJ_IS_INVALID(o)     (((uintptr_t)(o)&7UL) == 0)
#define FIOBJ_IS_NULL(o)        (FIOBJ_IS_INVALID(o) || ((o) == FIOBJ_T_NULL))
#define FIOBJ_TYPE_CLASS(o)     ((fiobj_class_en)(((uintptr_t)(o)) & 7UL))
#define FIOBJ_PTR_TAG(o, klass) ((uintptr_t)(((uintptr_t)(o)) | (klass)))
#define FIOBJ_PTR_UNTAG(o)      ((uintptr_t)(((uintptr_t)(o)) & (~7ULL)))
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

/** Calculates an object's hash value for a specific hash map object. */
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ object_key);

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
SFUNC uint32_t fiobj_each2(FIOBJ o,
                           int (*task)(fiobj_each_s *info),
                           void *udata);

/* *****************************************************************************
FIOBJ Primitives (NULL, True, False)
***************************************************************************** */

/** Returns the `true` primitive. */
FIO_IFUNC FIOBJ fiobj_true(void) { return (FIOBJ)(FIOBJ_T_TRUE); }

/** Returns the `false` primitive. */
FIO_IFUNC FIOBJ fiobj_false(void) { return (FIOBJ)(FIOBJ_T_FALSE); }

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
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_OTHER)
#define FIO_PTR_TAG_TYPE        FIOBJ
#include FIO_INCLUDE_FILE

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
SFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
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
SFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                               cstr)(FIOBJ i);

/** Frees a FIOBJ Float. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Data (FIOBJ_T_OTHER, a `fio_data_s` store)
***************************************************************************** */

/** The `FIOBJ_TYPE` of Data objects (a `FIOBJ_T_OTHER` class). */
#define FIOBJ_T_DATA 40

/**
 * Creates a new Data object - a `fio_data_s` store that keeps data in RAM
 * until it exceeds `ram_limit` bytes (0 = `FIO_DATA_RAM_LIMIT`), then moves it
 * to a temporary file.
 */
FIO_IFUNC FIOBJ fiobj_data_new(size_t ram_limit);

/**
 * Returns the `fio_data_s` store of a Data object (use the `fio_data_*` API to
 * read / write), or NULL if `o` isn't a Data object.
 *
 * Note: `fiobj2cstr`, JSON output and equality tests read through the store
 * and MAY invalidate buffers previously returned by `fio_data_*` calls (and by
 * an earlier `fiobj2cstr` of the same object).
 */
FIO_IFUNC fio_data_s *fiobj_data_ptr(FIOBJ o);

/**
 * Returns borrowed, NUL-terminated upload metadata. The view remains valid
 * until the next setter for that property or until `data` is freed.
 * An unset property or non-Data object returns a NULL `buf`; a set empty
 * property returns a non-NULL `buf` with `len == 0`.
 */
FIO_IFUNC fio_str_info_s fiobj_data_filename(FIOBJ data);
FIO_IFUNC fio_str_info_s fiobj_data_type(FIOBJ data);
FIO_IFUNC fio_str_info_s fiobj_data_headers(FIOBJ data);

/**
 * Copies upload metadata; a NULL `buf` clears the property. A non-Data
 * object is ignored. On allocation failure the property becomes unset.
 */
FIO_IFUNC void fiobj_data_filename_set(FIOBJ data, fio_str_info_s filename);
FIO_IFUNC void fiobj_data_type_set(FIOBJ data, fio_str_info_s type);
FIO_IFUNC void fiobj_data_headers_set(FIOBJ data, fio_str_info_s headers);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___DATA_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Strings
***************************************************************************** */

#define FIO_STR_NAME              FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_STR_OPTIMIZE_EMBEDDED 1
#define FIO_REF_NAME              FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_REF_CONSTRUCTOR_ONLY  1
#define FIO_REF_DESTROY(s)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)                    \
    ((FIOBJ)FIOBJ_PTR_TAG(&s, FIOBJ_T_STRING));                                \
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
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_STRING)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_STRING)
#define FIO_PTR_TAG_TYPE        FIOBJ
#include FIO_INCLUDE_FILE

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
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), destroy)                     \
    ((FIOBJ)FIOBJ_PTR_TAG(&a, FIOBJ_T_ARRAY));                                 \
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
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_ARRAY)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_ARRAY)
#define FIO_PTR_TAG_TYPE        FIOBJ
#include FIO_INCLUDE_FILE

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

#define FIO_REF_NAME             FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(a)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), destroy)                      \
    ((FIOBJ)FIOBJ_PTR_TAG(&(a), FIOBJ_T_HASH));                                \
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
#define FIO_MAP_NAME              FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_MAP_ORDERED           1
#define FIO_MAP_KEY               FIOBJ
#define FIO_MAP_KEY_CMP(a, b)     FIO_NAME_BL(fiobj, eq)((a), (b))
#define FIO_MAP_KEY_COPY(dest, o) (dest = fiobj_dup(o))
#define FIO_MAP_KEY_DESTROY(o)    fiobj_free(o)
#define FIO_MAP_VALUE             FIOBJ
#define FIO_MAP_HASH_FN(o)        FIO_NAME2(fiobj, hash)(o)
#define FIO_MAP_VALUE_DESTROY(o)  fiobj_free(o)
#define FIO_MAP_VALUE_DISCARD(o)  fiobj_free(o)
#define FIO_PTR_TAG(p)            FIOBJ_PTR_TAG(p, FIOBJ_T_HASH)
#define FIO_PTR_UNTAG(p)          FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p)   (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_HASH)
#define FIO_PTR_TAG_TYPE          FIOBJ
/* TODO! auto-hash object value */
#include FIO_INCLUDE_FILE
/**
 * Sets a value in a hash map, allocating the key String and automatically
 * calculating the hash value.
 */
FIO_IFUNC
FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
               set2)(FIOBJ hash, const char *key, size_t len, FIOBJ value);

/**
 * Finds a value in the hash map, using a temporary String and automatically
 * calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get2)(FIOBJ hash, const char *buf, size_t len);

/**
 * Removes a value in a hash map, using a temporary String and automatically
 * calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash,
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
 * Updates a Hash using JSON data (the JSON root must be an object).
 *
 * Nested objects merge into existing Hash members and nested arrays append to
 * existing Array members; any other conflicting value is overwritten
 * (preferring the new over the old).
 *
 * On a parsing error, members merged before the error remain in the Hash. A
 * non-object root changes nothing.
 *
 * Returns the number of bytes consumed before parsing stopped (0 if `hash` is
 * `FIOBJ_INVALID`). The Hash is never freed or consumed.
 */
SFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
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
SFUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

/** Helper macro, calls `fiobj_json_parse` with string information */
#define fiobj_json_parse2(data_, len_, consumed)                               \
  fiobj_json_parse(FIO_STR_INFO2(data_, len_), consumed)

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
SFUNC FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation);
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
  fiobj_json_find(object, FIO_STR_INFO2(str, length))

/* *****************************************************************************
FIOBJ Mustache support
***************************************************************************** */

/**
 * Builds a Mustache template using a FIOBJ context (usually a Hash).
 *
 * Returns a FIOBJ String with the rendered template. May return `FIOBJ_INVALID`
 * if nothing was written.
 */
FIO_IFUNC FIOBJ fiobj_mustache_build(fio_mustache_s *m, FIOBJ ctx);

/**
 * Builds a Mustache template using a FIOBJ context (usually a Hash).
 *
 * Writes output to `dest` string (may be `FIOBJ_INVALID` / `NULL`).
 *
 * Returns `dest` (or a new String). May return `FIOBJ_INVALID` if nothing was
 * written and `dest` was empty.
 */
FIO_IFUNC FIOBJ fiobj_mustache_build2(fio_mustache_s *m, FIOBJ dest, FIOBJ ctx);

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
    case FIOBJ_T_NULL: return FIOBJ_T_NULL;
    case FIOBJ_T_TRUE: return FIOBJ_T_TRUE;
    case FIOBJ_T_FALSE: return FIOBJ_T_FALSE;
    };
    return FIOBJ_T_INVALID;
  case FIOBJ_T_NUMBER: return FIOBJ_T_NUMBER;
  case FIOBJ_T_FLOAT: return FIOBJ_T_FLOAT;
  case FIOBJ_T_STRING: return FIOBJ_T_STRING;
  case FIOBJ_T_ARRAY: return FIOBJ_T_ARRAY;
  case FIOBJ_T_HASH: return FIOBJ_T_HASH;
  case FIOBJ_T_OTHER: return (*fiobj_object_metadata(o))->type_id;
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
  case FIOBJ_T_FLOAT: /* fall through */ return o;
  case FIOBJ_T_STRING: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), dup)(o);
    break;
  case FIOBJ_T_ARRAY: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), dup)(o);
    break;
  case FIOBJ_T_HASH: /* fall through */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), dup)(o);
    break;
  case FIOBJ_T_OTHER: /* fall through */ fiobj_object_dup(o);
  }
  return o;
}

/** Decreases an object's reference count or frees it. */
FIO_IFUNC void fiobj_free(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_FLOAT: return;
  case FIOBJ_T_STRING:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), free)(o);
    return;
  case FIOBJ_T_ARRAY:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), free)(o);
    return;
  case FIOBJ_T_HASH:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), free)(o);
    return;
  case FIOBJ_T_OTHER: (*fiobj_object_metadata(o))->free2(o); return;
  }
}

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Internal: compares two nestable objects. */
SFUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
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
  case FIOBJ_T_FLOAT: /* fall through */ return a == b;
  case FIOBJ_T_STRING:
    return FIO_NAME_BL(FIO_NAME(fiobj, FIOBJ___NAME_STRING), eq)(a, b);
  case FIOBJ_T_ARRAY: return fiobj___test_eq_nested(a, b, 0);
  case FIOBJ_T_HASH: return fiobj___test_eq_nested(a, b, 0);
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
    case FIOBJ_T_NULL: return FIO_STR_INFO2((char *)"null", 4);
    case FIOBJ_T_TRUE: return FIO_STR_INFO2((char *)"true", 4);
    case FIOBJ_T_FALSE: return FIO_STR_INFO2((char *)"false", 5);
    };
    return (fio_str_info_s){.buf = (char *)""};
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), cstr)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), cstr)(o);
  case FIOBJ_T_STRING:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
  case FIOBJ_T_ARRAY: /* fall through */
    return FIO_STR_INFO2((char *)"[...]", 5);
  case FIOBJ_T_HASH: {
    return FIO_STR_INFO2((char *)"{...}", 5);
  }
  case FIOBJ_T_OTHER: return (*fiobj_object_metadata(o))->to_s(o);
  }
  /* a non-explicit NULL is an empty string. */
  return (fio_str_info_s){.buf = (char *)""};
}

/** Returns an integer representation for any FIOBJ object. */
FIO_IFUNC intptr_t FIO_NAME2(fiobj, i)(FIOBJ o) {
  fio_str_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL: return 0;
    case FIOBJ_T_TRUE: return 1;
    case FIOBJ_T_FALSE: return 0;
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
  case FIOBJ_T_OTHER: return (*fiobj_object_metadata(o))->to_i(o);
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
    case FIOBJ_T_NULL: return 0.0;
    case FIOBJ_T_TRUE: return 1.0;
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
  case FIOBJ_T_OTHER: return (*fiobj_object_metadata(o))->to_f(o);
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
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_OTHER)
#define FIO_PTR_TAG_TYPE        FIOBJ
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 99

/* Places a 61 or 29 bit signed integer in the leftmost bits of a word. */
#define FIO_NUMBER_ENCODE(i) (((uintptr_t)(i) << 3) | FIOBJ_T_NUMBER)
/* Reads a 61 or 29 bit signed integer from the leftmost bits of a word. */
#define FIO_NUMBER_DECODE(i)                                                   \
  ((intptr_t)(((uintptr_t)(i) >> 3) |                                          \
              ((uintptr_t)0 -                                                  \
               (((uintptr_t)(i) >> 3) &                                        \
                ((uintptr_t)1 << ((sizeof(uintptr_t) * 8) - 4))))))

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
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_OTHER)
    return FIO_PTR_MATH_RMASK(intptr_t, i, 3)[0];
  return 0;
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(FIOBJ i) {
  return (double)FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i);
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_OTHER)
    fiobj___bignum_free2(i);
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
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_OTHER)
#define FIO_PTR_TAG_TYPE        FIOBJ
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 99

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
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_OTHER)
    return FIO_PTR_MATH_RMASK(double, i, 3)[0];
  return 0.0;
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_OTHER)
    fiobj___bigfloat_free2(i);
  return;
}

/* *****************************************************************************
FIOBJ Data
***************************************************************************** */

typedef struct {
  fio_str_info_s filename;
  fio_str_info_s type;
  fio_str_info_s headers;
} fiobj___data_metadata_s;

typedef struct {
  fio_data_s store; /* first: existing Data casts access the store */
  fiobj___data_metadata_s *metadata;
} fiobj___data_s;

FIO_SFUNC void fiobj___data_destroy(fiobj___data_s *d) {
  fio_data_destroy(&d->store);
  if (d->metadata) {
    FIO_MEM_FREE(d->metadata->filename.buf, d->metadata->filename.len + 1);
    FIO_MEM_FREE(d->metadata->type.buf, d->metadata->type.len + 1);
    FIO_MEM_FREE(d->metadata->headers.buf, d->metadata->headers.len + 1);
    FIO_MEM_FREE(d->metadata, sizeof(*d->metadata));
  }
}

#define FIO_REF_NAME fiobj___data
#define FIO_REF_TYPE fiobj___data_s
#define FIO_REF_INIT(d)                                                        \
  (d) = (fiobj___data_s) { .store = FIO_DATA_INIT }
#define FIO_REF_DESTROY(d) fiobj___data_destroy(&(d))
#define FIO_REF_METADATA   const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___DATA_CLASS_VTBL;                                              \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_VALIDATE(p) (FIOBJ_TYPE_CLASS(p) == FIOBJ_T_OTHER)
#define FIO_PTR_TAG_TYPE        FIOBJ
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#define FIO___RECURSIVE_INCLUDE 99

FIO_IFUNC FIOBJ fiobj_data_new(size_t ram_limit) {
  FIOBJ o = fiobj___data_new2();
  if (o)
    ((fio_data_s *)FIOBJ_PTR_UNTAG(o))->limit = ram_limit;
  return o;
}

FIO_IFUNC fio_data_s *fiobj_data_ptr(FIOBJ o) {
  if (FIOBJ_TYPE(o) != FIOBJ_T_DATA)
    return NULL;
  return (fio_data_s *)FIOBJ_PTR_UNTAG(o);
}

FIO_SFUNC void fiobj___data_metadata_set(FIOBJ data,
                                         fio_str_info_s value,
                                         size_t offset) {
  if (FIOBJ_TYPE(data) != FIOBJ_T_DATA)
    return;
  fiobj___data_s *d = (fiobj___data_s *)FIOBJ_PTR_UNTAG(data);
  fiobj___data_metadata_s *m = d->metadata;
  if (!m && value.buf) {
    m = (fiobj___data_metadata_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*m), 0);
    if (!m)
      return;
    *m = (fiobj___data_metadata_s){0};
    d->metadata = m;
  }
  if (!m)
    return;
  fio_str_info_s *field = (fio_str_info_s *)((char *)m + offset);
  char *copy = NULL;
  if (value.buf && value.len != SIZE_MAX) {
    copy = (char *)FIO_MEM_REALLOC(NULL, 0, value.len + 1, 0);
    if (copy) {
      if (value.len)
        FIO_MEMCPY(copy, value.buf, value.len);
      copy[value.len] = 0;
    }
  }
  FIO_MEM_FREE(field->buf, field->len + 1);
  *field = (fio_str_info_s){.buf = copy, .len = copy ? value.len : 0};
}

FIO_SFUNC fio_str_info_s fiobj___data_metadata_get(FIOBJ data, size_t offset) {
  if (FIOBJ_TYPE(data) != FIOBJ_T_DATA)
    return (fio_str_info_s){0};
  fiobj___data_metadata_s *m =
      ((fiobj___data_s *)FIOBJ_PTR_UNTAG(data))->metadata;
  if (!m)
    return (fio_str_info_s){0};
  return *(fio_str_info_s *)((char *)m + offset);
}

FIO_IFUNC fio_str_info_s fiobj_data_filename(FIOBJ data) {
  return fiobj___data_metadata_get(data,
                                   offsetof(fiobj___data_metadata_s, filename));
}
FIO_IFUNC fio_str_info_s fiobj_data_type(FIOBJ data) {
  return fiobj___data_metadata_get(data,
                                   offsetof(fiobj___data_metadata_s, type));
}
FIO_IFUNC fio_str_info_s fiobj_data_headers(FIOBJ data) {
  return fiobj___data_metadata_get(data,
                                   offsetof(fiobj___data_metadata_s, headers));
}
FIO_IFUNC void fiobj_data_filename_set(FIOBJ data, fio_str_info_s filename) {
  fiobj___data_metadata_set(data,
                            filename,
                            offsetof(fiobj___data_metadata_s, filename));
}
FIO_IFUNC void fiobj_data_type_set(FIOBJ data, fio_str_info_s type) {
  fiobj___data_metadata_set(data,
                            type,
                            offsetof(fiobj___data_metadata_s, type));
}
FIO_IFUNC void fiobj_data_headers_set(FIOBJ data, fio_str_info_s headers) {
  fiobj___data_metadata_set(data,
                            headers,
                            offsetof(fiobj___data_metadata_s, headers));
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
  case FIOBJ_T_FLOAT: return 0;
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
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ o) {
  uint64_t seed = (uint64_t)(uintptr_t)&FIO_NAME2(fiobj, hash);
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    return fio_risky_hash(&o, sizeof(o), seed + (uintptr_t)o);
  case FIOBJ_T_NUMBER: {
    uintptr_t tmp = FIO_NAME2(fiobj, i)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), seed);
  }
  case FIOBJ_T_FLOAT: {
    double tmp = FIO_NAME2(fiobj, f)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), seed);
  }
  case FIOBJ_T_STRING: /* fall through */
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), hash)(o, seed);
  case FIOBJ_T_ARRAY: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
    h += fio_risky_hash(&h, sizeof(h), seed + FIOBJ_T_ARRAY);
    {
      FIOBJ *a = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), ptr)(o);
      const size_t count =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
      if (a) {
        for (size_t i = 0; i < count; ++i) {
          h += FIO_NAME2(fiobj, hash)(a[i]);
        }
      }
    }
    return h;
  }
  case FIOBJ_T_HASH: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
    h += fio_risky_hash(&h, sizeof(h), seed + FIOBJ_T_HASH);
    FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), o, i) {
      h += i.hash;
      h += FIO_NAME2(fiobj, hash)(i.value);
    }
    return h;
  }
  case FIOBJ_T_OTHER: {
    /* TODO: can we avoid "stringifying" the object? */
    fio_str_info_s tmp = (*fiobj_object_metadata(o))->to_s(o);
    return fio_risky_hash(tmp.buf, tmp.len, seed);
  }
  }
  return 0;
}

/**
 * Sets a String value in a hash map, allocating the String and automatically
 * calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set2)(FIOBJ hash,
                               const char *key,
                               size_t len,
                               FIOBJ value) {
  FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(tmp, (char *)key, len);
  FIOBJ v =
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)(hash, tmp, value, NULL);
  fiobj_free(tmp);
  return v;
}

/**
 * Finds a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get2)(FIOBJ hash, const char *buf, size_t len) {
  if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH)
    return FIOBJ_INVALID;
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  FIOBJ v = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get)(hash, tmp);
  return v;
}

/**
 * Removes a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash,
                                const char *buf,
                                size_t len,
                                FIOBJ *old) {
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  int r = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove)(hash, tmp, old);
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
    if (i.key == FIOBJ_INVALID || FIOBJ_TYPE_CLASS(i.key) == FIOBJ_T_NULL) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove)(dest, i.key, NULL);
      continue;
    }
    register FIOBJ tmp;
    switch (FIOBJ_TYPE_CLASS(i.value)) {
    case FIOBJ_T_ARRAY:
      /* TODO? decide if we should merge elements or overwrite...? */
      tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get)(dest, i.key);
      if (FIOBJ_TYPE_CLASS(tmp) == FIOBJ_T_ARRAY) {
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), concat)
        (tmp, i.value);
        continue;
      }
      break;
    case FIOBJ_T_HASH:
      tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get)(dest, i.key);
      if (FIOBJ_TYPE_CLASS(tmp) == FIOBJ_T_HASH)
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), update)
      (dest, i.value);
      else break;
      continue;
    case FIOBJ_T_NUMBER:    /* fall through */
    case FIOBJ_T_PRIMITIVE: /* fall through */
    case FIOBJ_T_STRING:    /* fall through */
    case FIOBJ_T_FLOAT:     /* fall through */
    case FIOBJ_T_OTHER: break;
    }
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)
    (dest, i.key, fiobj_dup(i.value), NULL);
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
SFUNC void fiobj___json_format_internal__(fiobj___json_format_internal__s *,
                                          FIOBJ);

/** Helper function, calls `fiobj_hash_update_json` with string information */
FIO_IFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                          update_json2)(FIOBJ hash, char *ptr, size_t len) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  update_json)(hash, FIO_STR_INFO2(ptr, len));
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

#undef FIO___RECURSIVE_INCLUDE /* from now on, type helpers are internal */

/* *****************************************************************************
FIOBJ Mustache support - inline implementation
***************************************************************************** */

/* callback should write `txt` to output and return updated `udata.` */
FIO_SFUNC void *fiobj___mustache_write_text(void *udata, fio_buf_info_s txt);
/* same as `write_text`, but should also  HTML escape (sanitize) data. */
FIO_SFUNC void *fiobj___mustache_write_text_escaped(void *udata,
                                                    fio_buf_info_s raw);
/* callback should return a new context pointer with the value of `name`. */
FIO_SFUNC void *fiobj___mustache_get_var(void *ctx, fio_buf_info_s name);
/* if context is an Array, should return its length. */
FIO_SFUNC size_t fiobj___mustache_array_length(void *ctx);
/* if context is an Array, should return a context pointer @ index. */
FIO_SFUNC void *fiobj___mustache_get_var_index(void *ctx, size_t index);
/* should return the String value of context `var` as a `fio_buf_info_s`. */
FIO_SFUNC fio_buf_info_s fiobj___mustache_var2str(void *var);
/* should return non-zero if the context pointer refers to a valid value. */
FIO_SFUNC int fiobj___mustache_var_is_truthful(void *ctx);

/**
 * Builds a Mustache template using a FIOBJ context (usually a Hash).
 *
 * Returns a FIOBJ String with the rendered template. May return `FIOBJ_INVALID`
 * if nothing was written.
 */
FIO_IFUNC FIOBJ fiobj_mustache_build(fio_mustache_s *m, FIOBJ ctx) {
  return (FIOBJ)fio_mustache_build(
      m,
      .write_text = fiobj___mustache_write_text,
      .write_text_escaped = fiobj___mustache_write_text_escaped,
      .get_var = fiobj___mustache_get_var,
      .array_length = fiobj___mustache_array_length,
      .get_var_index = fiobj___mustache_get_var_index,
      .var2str = fiobj___mustache_var2str,
      .var_is_truthful = fiobj___mustache_var_is_truthful,
      .ctx = ctx,
      .udata = NULL);
}

/**
 * Builds a Mustache template using a FIOBJ context (usually a Hash).
 *
 * Writes output to `dest` string (may be `FIOBJ_INVALID` / `NULL`).
 *
 * Returns `dest` (or a new String). May return `FIOBJ_INVALID` if nothing was
 * written and `dest` was empty.
 */
FIO_IFUNC FIOBJ fiobj_mustache_build2(fio_mustache_s *m,
                                      FIOBJ dest,
                                      FIOBJ ctx) {
  dest = (FIOBJ)fio_mustache_build(
      m,
      .write_text = fiobj___mustache_write_text,
      .write_text_escaped = fiobj___mustache_write_text_escaped,
      .get_var = fiobj___mustache_get_var,
      .array_length = fiobj___mustache_array_length,
      .get_var_index = fiobj___mustache_get_var_index,
      .var2str = fiobj___mustache_var2str,
      .var_is_truthful = fiobj___mustache_var_is_truthful,
      .ctx = ctx,
      .udata = dest);
  return dest;
}

/* callback should write `txt` to output and return updated `udata.` */
FIO_SFUNC void *fiobj___mustache_write_text(void *udata, fio_buf_info_s txt) {
  FIOBJ d = (FIOBJ)udata;
  if (!d)
    d = fiobj_str_new_buf(txt.len + 32);
  fiobj_str_write(d, txt.buf, txt.len);
  return (void *)d;
}
/* same as `write_text`, but should also  HTML escape (sanitize) data. */
FIO_SFUNC void *fiobj___mustache_write_text_escaped(void *ud,
                                                    fio_buf_info_s raw) {
  FIOBJ d = (FIOBJ)ud;
  if (!d)
    d = fiobj_str_new_buf(raw.len + 32);
  fiobj_str_write_html_escape(d, raw.buf, raw.len);
  return (void *)d;
}
/* callback should return a new context pointer with the value of `name`. */
FIO_SFUNC void *fiobj___mustache_get_var(void *ctx, fio_buf_info_s name) {
  if (!ctx)
    return NULL;
  if (!FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_HASH))
    return NULL;
  return fiobj_hash_get2((FIOBJ)ctx, name.buf, name.len);
}
/* if context is an Array, should return its length. */
FIO_SFUNC size_t fiobj___mustache_array_length(void *ctx) {
  if (!FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_ARRAY))
    return 0;
  return fiobj_array_count((FIOBJ)ctx);
}
/* if context is an Array, should return a context pointer @ index. */
FIO_SFUNC void *fiobj___mustache_get_var_index(void *ctx, size_t index) {
  if (!FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_ARRAY) || index > 0xFFFFFFFFUL)
    return NULL;
  return fiobj_array_get((FIOBJ)ctx, (uint32_t)index);
}
/* should return the String value of context `var` as a `fio_buf_info_s`. */
FIO_SFUNC fio_buf_info_s fiobj___mustache_var2str(void *var) {
  fio_buf_info_s r = {0};
  if (!var || var == fiobj_null())
    return r;
  fio_str_info_s tmp = fiobj2cstr((FIOBJ)var);
  r = FIO_STR2BUF_INFO(tmp);
  return r;
}
/* should return non-zero if the context pointer refers to a valid value. */
FIO_SFUNC int fiobj___mustache_var_is_truthful(void *v) {
  return v && (FIOBJ)v != fiobj_null() && (FIOBJ)v != fiobj_false() &&
         (!FIOBJ_TYPE_IS((FIOBJ)v, FIOBJ_T_ARRAY) ||
          fiobj_array_count((FIOBJ)v));
}

/* *****************************************************************************


FIOBJ - Externed Implementation


***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
} fiobj___stack_element_s;

#define FIO_ARRAY_NAME fiobj___active_stack
#define FIO_ARRAY_TYPE fiobj___stack_element_s
#define FIO_ARRAY_TYPE_COPY(dest, src)                                         \
  do {                                                                         \
    (dest).obj = fiobj_dup((src).obj);                                         \
    (dest).pos = (src).pos;                                                    \
  } while (0)
#define FIO_ARRAY_TYPE_CMP(a, b)  (a).obj == (b).obj
#define FIO_ARRAY_TYPE_DESTROY(o) fiobj_free(o.obj)
#define FIO___RECURSIVE_INCLUDE   1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

#define FIO_ARRAY_TYPE_CMP(a, b) (a).obj == (b).obj
#define FIO_ARRAY_NAME           fiobj___stack
#define FIO_ARRAY_TYPE           fiobj___stack_element_s
#define FIO___RECURSIVE_INCLUDE  1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

typedef struct {
  int (*task)(fiobj_each_s *info);
  void *arg;
  FIOBJ next;
  size_t count;
  fiobj___stack_s stack;
  uint32_t end;
  uint8_t stop;
} fiobj_____each2_data_s;

FIO_SFUNC uint32_t fiobj____each2_element_count(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fall through */
  case FIOBJ_T_NUMBER:    /* fall through */
  case FIOBJ_T_STRING:    /* fall through */
  case FIOBJ_T_FLOAT: return 0;
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
SFUNC uint32_t fiobj_each2(FIOBJ o, int (*task)(fiobj_each_s *), void *udata) {
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
  fiobj___stack_element_s i = {.obj = o, .pos = 0};
  uint32_t end = fiobj____each2_element_count(o);
  fiobj____each2_wrapper_task((fiobj_each_s *)&e_tmp);
  while (!d.stop && i.obj && i.pos < end) {
    i.pos =
        fiobj_each1(i.obj, fiobj____each2_wrapper_task, &d, (uint32_t)i.pos);
    if (d.next != FIOBJ_INVALID) {
      if (fiobj___stack_count(&d.stack) + 1 > FIOBJ_MAX_NESTING) {
        FIO_LOG_ERROR("FIOBJ nesting level too deep (%u)."
                      "`fiobj_each2` stopping loop early.",
                      (unsigned int)fiobj___stack_count(&d.stack));
        d.stop = 1;
        continue;
      }
      fiobj___stack_push(&d.stack, i);
      i.pos = 0;
      i.obj = d.next;
      d.next = FIOBJ_INVALID;
      end = d.end;
    } else {
      /* re-collect end position to accommodate for changes */
      end = fiobj____each2_element_count(i.obj);
    }
    while (i.pos >= end && fiobj___stack_count(&d.stack)) {
      fiobj___stack_pop(&d.stack, &i);
      end = fiobj____each2_element_count(i.obj);
    }
  };
  fiobj___stack_destroy(&d.stack);
  return (uint32_t)d.count;
}

/* *****************************************************************************
FIOBJ Hash / Array / Other (enumerable) Equality test.
***************************************************************************** */

/** Internal: compares two nestable objects. */
SFUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
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
  case FIOBJ_T_FLOAT: return a == b;
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
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a,
                                                                   (int32_t)i),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(b,
                                                                   (int32_t)i),
                nesting))
          return 0;
      }
    }
    return 1;

  case FIOBJ_T_HASH:
    if (!fiobj____each2_element_count(a))
      return 1;
    FIO_MAP_EACH(FIO_NAME(fiobj, FIOBJ___NAME_HASH), a, pos) {
      FIOBJ val = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get)(b, pos.key);
      if (!fiobj___test_eq_nested(val, pos.value, nesting))
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

SFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                               cstr)(FIOBJ i) {
  static char buf[32 * 128];
  static uint8_t pos = 0;
  size_t at = fio_atomic_add(&pos, 1);
  fio_str_info_s s = {.buf = buf + ((at & 127) << 5), .capa = 31};
  fio_string_write_i(&s,
                     NULL,
                     FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i));
  return s;
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

SFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                               cstr)(FIOBJ i) {
  static char buf[32 * 128];
  static uint8_t pos = 0;
  size_t at = fio_atomic_add(&pos, 1);
  char *tmp = buf + ((at & 127) << 5);
  size_t len =
      fio_ftoa(tmp, FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(i), 10);
  tmp[len] = 0;
  return FIO_STR_INFO2(tmp, len);
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
FIOBJ Data - vtable
***************************************************************************** */

/**
 * Returns the whole content (borrowed; a spilled store is memory mapped, or
 * read to a heap buffer where mapping isn't available - empty if that fails).
 */
FIO_SFUNC fio_str_info_s fiobj___data2cstr(FIOBJ o) {
  fio_data_s *d = (fio_data_s *)FIOBJ_PTR_UNTAG(o);
  size_t pos = fio_data_pos(d);
  fio_data_seek(d, 0);
  fio_str_info_s r = fio_data_read(d, fio_data_len(d));
  fio_data_seek(d, (ssize_t)pos);
  if (!r.buf)
    r = FIO_STR_INFO2((char *)"", 0);
  return r;
}

/** Data objects compare by content (unequal if the content can't be read). */
FIO_SFUNC unsigned char fiobj___data_eq(FIOBJ restrict a, FIOBJ restrict b) {
  fio_data_s *da = (fio_data_s *)FIOBJ_PTR_UNTAG(a);
  fio_data_s *db = (fio_data_s *)FIOBJ_PTR_UNTAG(b);
  if (da == db)
    return 1;
  if (fio_data_len(da) != fio_data_len(db))
    return 0;
  size_t pa = fio_data_pos(da), pb = fio_data_pos(db);
  unsigned char r;
  fio_data_seek(da, 0);
  fio_data_seek(db, 0);
  fio_str_info_s x = fio_data_read(da, (size_t)-1);
  fio_str_info_s y = fio_data_read(db, (size_t)-1);
  r = (x.len == fio_data_len(da) && y.len == x.len &&
       (!x.len || !FIO_MEMCMP(x.buf, y.buf, x.len)));
  fio_data_seek(da, (ssize_t)pa);
  fio_data_seek(db, (ssize_t)pb);
  return r;
}

/** A Data object's numeral value is its length. */
FIO_SFUNC intptr_t fiobj___data2i(FIOBJ o) {
  return (intptr_t)fio_data_len((fio_data_s *)FIOBJ_PTR_UNTAG(o));
}
FIO_SFUNC double fiobj___data2f(FIOBJ o) {
  return (double)fio_data_len((fio_data_s *)FIOBJ_PTR_UNTAG(o));
}

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___DATA_CLASS_VTBL = {
    .type_id = FIOBJ_T_DATA,
    .is_eq = fiobj___data_eq,
    .to_s = fiobj___data2cstr,
    .to_i = fiobj___data2i,
    .to_f = fiobj___data2f,
    .count = fiobj___count_noop,
    .each1 = NULL,
    .free2 = fiobj___data_free2,
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

SFUNC void fiobj___json_format_internal__(fiobj___json_format_internal__s *args,
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
        fio_str_info_s info = FIO_NAME2(fiobj, cstr)(couplet.key);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, "\"", 1);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
        (args->json, info.buf, info.len);
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, "\":", 2);
        fiobj___json_format_internal__(args, couplet.value);
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
#define FIOBJ___JSON_OBJ(o) ((fio_json_obj_result_s){.obj = (void *)(o)})
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_null(void *udata) {
  (void)udata;
  return FIOBJ___JSON_OBJ(FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_true(void *udata) {
  (void)udata;
  return FIOBJ___JSON_OBJ(fiobj_true());
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_false(void *udata) {
  (void)udata;
  return FIOBJ___JSON_OBJ(fiobj_false());
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_number(void *udata,
                                                       int64_t i) {
  (void)udata;
  return FIOBJ___JSON_OBJ(FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_NUMBER, new))(i));
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_float(void *udata, double f) {
  (void)udata;
  return FIOBJ___JSON_OBJ(FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_FLOAT, new))(f));
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_string(void *udata,
                                                       fio_buf_info_s s) {
  (void)udata;
  FIOBJ str = FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_STRING, new))();
  FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_STRING, write_unescape))
  (str, s.buf, s.len);
  return FIOBJ___JSON_OBJ(str);
}
FIO_SFUNC fio_json_obj_result_s fiobj___json_on_string_simple(void *udata,
                                                              fio_buf_info_s s) {
  (void)udata;
  FIOBJ str = FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_STRING, new))();
  FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_STRING, write))(str, s.buf, s.len);
  return FIOBJ___JSON_OBJ(str);
}
/* Opens a container: reuses a matching one (merge / append), otherwise
 * overwrites (frees) any other existing value. */
FIO_IFUNC fio_json_push_result_s fiobj___json_on_container(void *parent,
                                                           fio_buf_info_s key,
                                                           fiobj_class_en type) {
  fio_json_push_result_s r = {0};
  FIOBJ o = FIOBJ_INVALID;
  if (parent && key.buf) {
    o = FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_HASH, get2))((FIOBJ)parent,
                                                           key.buf,
                                                           key.len);
    if (FIOBJ_TYPE_CLASS(o) == type) {
      r.obj = (void *)o;
      return r;
    }
  }
  o = (type == FIOBJ_T_HASH)
          ? FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_HASH, new))()
          : FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_ARRAY, new))();
  if (parent) {
    if (!key.buf)
      FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_ARRAY, push))((FIOBJ)parent, o);
    else
      FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_HASH, set2))
      ((FIOBJ)parent, key.buf, key.len, o);
  }
  r.obj = (void *)o;
  return r;
}
FIO_SFUNC fio_json_push_result_s fiobj___json_on_map(void *udata,
                                                     void *parent,
                                                     fio_buf_info_s key) {
  (void)udata;
  return fiobj___json_on_container(parent, key, FIOBJ_T_HASH);
}
FIO_SFUNC fio_json_push_result_s fiobj___json_on_array(void *udata,
                                                       void *parent,
                                                       fio_buf_info_s key) {
  (void)udata;
  return fiobj___json_on_container(parent, key, FIOBJ_T_ARRAY);
}
/* Overwrites (frees) a replaced value. */
FIO_SFUNC fio_json_obj_result_s fiobj___json_map_push(void *udata,
                                                      void *map,
                                                      fio_buf_info_s key,
                                                      void *value) {
  (void)udata;
  FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_HASH, set2))
  ((FIOBJ)map, key.buf, key.len, (FIOBJ)value);
  return FIOBJ___JSON_OBJ(NULL);
}
FIO_SFUNC int fiobj___json_array_push(void *udata, void *array, void *value) {
  (void)udata;
  FIO_NAME(fiobj, FIO_NAME(FIOBJ___NAME_ARRAY, push))
  ((FIOBJ)array, (FIOBJ)value);
  return 0;
}
FIO_SFUNC int fiobj___json_is_array(void *udata, void *obj) {
  (void)udata;
  return FIOBJ_TYPE_CLASS((FIOBJ)obj) == FIOBJ_T_ARRAY;
}
FIO_SFUNC int fiobj___json_is_map(void *udata, void *obj) {
  (void)udata;
  return FIOBJ_TYPE_CLASS((FIOBJ)obj) == FIOBJ_T_HASH;
}
FIO_SFUNC void fiobj___json_free_unused_object(void *udata, void *obj) {
  (void)udata;
  fiobj_free((FIOBJ)obj);
}
static const fio_json_parser_callbacks_s FIOBJ_JSON_PARSER_CALLBACKS = {
    .on_null = fiobj___json_on_null,
    .on_true = fiobj___json_on_true,
    .on_false = fiobj___json_on_false,
    .on_number = fiobj___json_on_number,
    .on_float = fiobj___json_on_float,
    .on_string = fiobj___json_on_string,
    .on_string_simple = fiobj___json_on_string_simple,
    .on_map = fiobj___json_on_map,
    .on_array = fiobj___json_on_array,
    .map_push = fiobj___json_map_push,
    .array_push = fiobj___json_array_push,
    .is_array = fiobj___json_is_array,
    .is_map = fiobj___json_is_map,
    .free_unused_object = fiobj___json_free_unused_object,
};
#undef FIOBJ___JSON_OBJ

/** Returns a JSON valid FIOBJ String, representing the object. */
SFUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed_p) {
  fio_json_result_s result = fio_json_parse(&FIOBJ_JSON_PARSER_CALLBACKS,
                                            NULL,
                                            NULL,
                                            str.buf,
                                            str.len);
  if (consumed_p)
    *consumed_p = result.stop_pos;
  if (result.err) {
#ifdef DEBUG
    FIOBJ s = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, (FIOBJ)result.ctx, 0);
    FIO_LOG_DEBUG("JSON data being deleted:\n%s",
                  FIO_NAME2(fiobj, cstr)(s).buf);
    fiobj_free(s);
#endif
    fiobj_free((FIOBJ)result.ctx); /* the partial root is ours */
    result.ctx = FIOBJ_INVALID;
  }
  return (FIOBJ)result.ctx;
}

/**
 * Updates a Hash using JSON data (the JSON root must be an object).
 *
 * Nested objects merge into existing Hash members and nested arrays append to
 * existing Array members; any other conflicting value is overwritten
 * (preferring the new over the old).
 *
 * On a parsing error, members merged before the error remain in the Hash. A
 * non-object root changes nothing.
 *
 * Returns the number of bytes consumed before parsing stopped (0 if `hash` is
 * `FIOBJ_INVALID`). The Hash is never freed or consumed.
 */
SFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                      update_json)(FIOBJ hash, fio_str_info_s str) {
  if (hash == FIOBJ_INVALID)
    return 0;
  /* `hash` is the explicit root: it stays caller-owned, even on error. */
  return fio_json_parse(&FIOBJ_JSON_PARSER_CALLBACKS,
                        NULL,
                        (void *)hash,
                        str.buf,
                        str.len)
      .stop_pos;
}

/** Uses JSON (JavaScript) notation to find data in an object structure. Returns
 * a temporary object. */
SFUNC FIOBJ fiobj_json_find(FIOBJ o, fio_str_info_s n) {
  for (;;) {
  top:
    if (!n.len || (n.len == 1 && n.buf[0] == '.'))
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
      if (!n.len || n.buf[0] != ']' || i > 0xFFFFFFFFU)
        return FIOBJ_INVALID;
      o = fiobj_array_get(o, (uint32_t)i);
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
      FIOBJ tmp = fiobj_hash_get2(o, n.buf, n.len);
      if (tmp != FIOBJ_INVALID)
        return tmp;
      char *end = n.buf + n.len - 1;
      while (end > n.buf) {
        while (end > n.buf && end[0] != '.' && end[0] != '[')
          --end;
        if (end == n.buf)
          return FIOBJ_INVALID;
        const size_t t_len = end - n.buf;
        tmp = fiobj_hash_get2(o, n.buf, t_len);
        if (tmp != FIOBJ_INVALID) {
          o = tmp;
          n.len -= t_len + (end[0] == '.');
          n.buf = end + (end[0] == '.');
          goto top;
        }
        --end;
      }
    } /* fall through */
    default: return FIOBJ_INVALID;
    }
  }
}
/* *****************************************************************************
FIOBJ cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIOBJ_EXTERN_OBJ
#undef FIOBJ_EXTERN_OBJ_IMP
#endif /* FIO_FIOBJ */
