/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STR_NAME fio            /* Development inclusion - ignore line */
#define FIO_ATOL                    /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                        Dynamic Strings (binary safe)




***************************************************************************** */
#ifdef FIO_STR_SMALL
#ifndef FIO_STR_NAME
#define FIO_STR_NAME FIO_STR_SMALL
#endif
#ifndef FIO_STR_OPTIMIZE4IMMUTABILITY
#define FIO_STR_OPTIMIZE4IMMUTABILITY 1
#endif
#endif /* FIO_STR_SMALL */

#if defined(FIO_STR_NAME)

#ifndef FIO_STR_OPTIMIZE_EMBEDDED
/**
 * For each unit (0 by default), adds `sizeof(char *)` bytes to the type size,
 * increasing the amount of strings that could be embedded within the type
 * without memory allocation.
 *
 * For example, when using a reference counter wrapper on a 64bit system, it
 * would make sense to set this value to 1 - allowing the type size to fully
 * utilize a 16 byte memory allocation alignment.
 */
#define FIO_STR_OPTIMIZE_EMBEDDED 0
#endif

#ifndef FIO_STR_OPTIMIZE4IMMUTABILITY
/**
 * Optimizes the struct to minimal size that can store the string length and a
 * pointer.
 *
 * By avoiding extra (mutable related) data, such as the allocated memory's
 * capacity, strings require less memory. However, this does introduce a
 * performance penalty when editing the string data.
 */
#define FIO_STR_OPTIMIZE4IMMUTABILITY 0
#endif

#if FIO_STR_OPTIMIZE4IMMUTABILITY
/* enforce limit after which FIO_STR_OPTIMIZE4IMMUTABILITY makes no sense */
#if FIO_STR_OPTIMIZE_EMBEDDED > 1
#undef FIO_STR_OPTIMIZE_EMBEDDED
#define FIO_STR_OPTIMIZE_EMBEDDED 1
#endif
#else
/* enforce limit due to 6 bit embedded string length limit */
#if FIO_STR_OPTIMIZE_EMBEDDED > 4
#undef FIO_STR_OPTIMIZE_EMBEDDED
#define FIO_STR_OPTIMIZE_EMBEDDED 4
#endif
#endif /* FIO_STR_OPTIMIZE4IMMUTABILITY*/

/* *****************************************************************************
String API - Initialization and Destruction
***************************************************************************** */

/**
 * The `fio_str_s` type should be considered opaque.
 *
 * The type's attributes should be accessed ONLY through the accessor
 * functions: `fio_str2cstr`, `fio_str_len`, `fio_str2ptr`, `fio_str_capa`,
 * etc'.
 *
 * Note: when the `small` flag is present, the structure is ignored and used
 * as raw memory for a small String (no additional allocation). This changes
 * the String's behavior drastically and requires that the accessor functions
 * be used.
 */
typedef struct {
  /* String flags:
   *
   * bit 1: small string.
   * bit 2: frozen string.
   * bit 3: static (non allocated) string (big strings only).
   * bit 3-8: small string length (up to 64 bytes).
   */
  uint8_t special;
  uint8_t reserved[(sizeof(void *) * (1 + FIO_STR_OPTIMIZE_EMBEDDED)) -
                   (sizeof(uint8_t))]; /* padding length */
#if !FIO_STR_OPTIMIZE4IMMUTABILITY
  size_t capa; /* known capacity for longer Strings */
  size_t len;  /* String length for longer Strings */
#endif         /* FIO_STR_OPTIMIZE4IMMUTABILITY */
  char *buf;   /* pointer for longer Strings */
} FIO_NAME(FIO_STR_NAME, s);

#ifdef FIO_PTR_TAG_TYPE
#define FIO_STR_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_STR_PTR FIO_NAME(FIO_STR_NAME, s) *
#endif

#ifndef FIO_STR_INIT
/**
 * This value should be used for initialization. For example:
 *
 *      // on the stack
 *      fio_str_s str = FIO_STR_INIT;
 *
 *      // or on the heap
 *      fio_str_s *str = malloc(sizeof(*str));
 *      *str = FIO_STR_INIT;
 *
 * Remember to cleanup:
 *
 *      // on the stack
 *      fio_str_destroy(&str);
 *
 *      // or on the heap
 *      fio_str_free(str);
 *      free(str);
 */
#define FIO_STR_INIT                                                           \
  { .special = 0 }

/**
 * This macro allows the container to be initialized with existing data, as long
 * as it's memory was allocated with the same allocator (`malloc` /
 * `fio_malloc`).
 *
 * The `capacity` value should exclude the NUL character (if exists).
 *
 * NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
 * FIO_STR_OPTIMIZE4IMMUTABILITY optimization)
 */
#define FIO_STR_INIT_EXISTING(buffer, length, capacity)                        \
  { .capa = (capacity), .len = (length), .buf = (buffer) }

/**
 * This macro allows the container to be initialized with existing static data,
 * that shouldn't be freed.
 *
 * NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
 * FIO_STR_OPTIMIZE4IMMUTABILITY optimization)
 */
#define FIO_STR_INIT_STATIC(buffer)                                            \
  { .special = 4, .len = strlen((buffer)), .buf = (char *)(buffer) }

/**
 * This macro allows the container to be initialized with existing static data,
 * that shouldn't be freed.
 *
 * NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
 * FIO_STR_OPTIMIZE4IMMUTABILITY optimization)
 */
#define FIO_STR_INIT_STATIC2(buffer, length)                                   \
  { .special = 4, .len = (length), .buf = (char *)(buffer) }

#endif /* FIO_STR_INIT */

#ifndef FIO_REF_CONSTRUCTOR_ONLY

/** Allocates a new String object on the heap. */
FIO_IFUNC FIO_STR_PTR FIO_NAME(FIO_STR_NAME, new)(void);

/**
 * Destroys the string and frees the container (if allocated with `new`).
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, free)(FIO_STR_PTR s);

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Initializes the container with the provided static / constant string.
 *
 * The string will be copied to the container **only** if it will fit in the
 * container itself. Otherwise, the supplied pointer will be used as is and it
 * should remain valid until the string is destroyed.
 *
 * The final string can be safely be destroyed (using the `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_const)(FIO_STR_PTR s,
                                                            const char *str,
                                                            size_t len);

/**
 * Initializes the container with a copy of the provided dynamic string.
 *
 * The string is always copied and the final string must be destroyed (using the
 * `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_copy)(FIO_STR_PTR s,
                                                           const char *str,
                                                           size_t len);

/**
 * Initializes the container with a copy of an existing String object.
 *
 * The string is always copied and the final string must be destroyed (using the
 * `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_copy2)(FIO_STR_PTR dest,
                                                            FIO_STR_PTR src);

/**
 * Frees the String's resources and reinitializes the container.
 *
 * Note: if the container isn't allocated on the stack, it should be freed
 * separately using the appropriate `free` function.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, destroy)(FIO_STR_PTR s);

/**
 * Returns a C string with the existing data, re-initializing the String.
 *
 * Note: the String data is removed from the container, but the container
 * isn't freed.
 *
 * Returns NULL if there's no String data.
 *
 * NOTE: Returned string is ALWAYS dynamically allocated. Remember to free.
 */
FIO_IFUNC char *FIO_NAME(FIO_STR_NAME, detach)(FIO_STR_PTR s);

/** Frees the pointer returned by `detach`. */
SFUNC void FIO_NAME(FIO_STR_NAME, dealloc)(void *ptr);

/* *****************************************************************************
String API - String state (data pointers, length, capacity, etc')
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, info)(const FIO_STR_PTR s);

/** Returns a pointer (`char *`) to the String's content. */
FIO_IFUNC char *FIO_NAME2(FIO_STR_NAME, ptr)(FIO_STR_PTR s);

/** Returns the String's length in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, len)(FIO_STR_PTR s);

/** Returns the String's existing capacity (total used & available memory). */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, capa)(FIO_STR_PTR s);

/**
 * Prevents further manipulations to the String's content.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, freeze)(FIO_STR_PTR s);

/**
 * Returns true if the string is frozen.
 */
FIO_IFUNC uint8_t FIO_NAME_BL(FIO_STR_NAME, frozen)(FIO_STR_PTR s);

/** Returns 1 if memory was allocated and (the String must be destroyed). */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, allocated)(const FIO_STR_PTR s);

/**
 * Binary comparison returns `1` if both strings are equal and `0` if not.
 */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, eq)(const FIO_STR_PTR str1,
                                            const FIO_STR_PTR str2);

/**
 * Returns the string's Risky Hash value.
 *
 * Note: Hash algorithm might change without notice.
 */
FIO_IFUNC uint64_t FIO_NAME(FIO_STR_NAME, hash)(const FIO_STR_PTR s,
                                                uint64_t seed);

/* *****************************************************************************
String API - Memory management
***************************************************************************** */

/**
 * Sets the new String size without reallocating any memory (limited by
 * existing capacity).
 *
 * Returns the updated state of the String.
 *
 * Note: When shrinking, any existing data beyond the new size may be
 * corrupted.
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, resize)(FIO_STR_PTR s,
                                                        size_t size);

/**
 * Performs a best attempt at minimizing memory consumption.
 *
 * Actual effects depend on the underlying memory allocator and it's
 * implementation. Not all allocators will free any memory.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, compact)(FIO_STR_PTR s);

#if !FIO_STR_OPTIMIZE4IMMUTABILITY
/**
 * Reserves (at least) `amount` of bytes for the string's data.
 *
 * The reserved count includes used data. If `amount` is less than the current
 * string length, the string will be truncated(!).
 *
 * May corrupt the string length information (if string is assumed to be
 * immutable), make sure to call `resize` with the updated information once the
 * editing is done.
 *
 * Returns the updated state of the String.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, reserve)(FIO_STR_PTR s,
                                                     size_t amount);
#define FIO_STR_RESERVE_NAME reserve
#else
/** INTERNAL - DO NOT USE! */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, __reserve)(FIO_STR_PTR s,
                                                       size_t amount);
#define FIO_STR_RESERVE_NAME __reserve
#endif
/* *****************************************************************************
String API - UTF-8 State
***************************************************************************** */

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_valid)(FIO_STR_PTR s);

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_len)(FIO_STR_PTR s);

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int FIO_NAME(FIO_STR_NAME,
                   utf8_select)(FIO_STR_PTR s, intptr_t *pos, size_t *len);

/* *****************************************************************************
String API - Content Manipulation and Review
***************************************************************************** */

/** Writes data at the end of the String. */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write)(FIO_STR_PTR s,
                                                       const void *src,
                                                       size_t src_len);

/** Writes a number at the end of the String using normal base 10 notation. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_i)(FIO_STR_PTR s,
                                                     int64_t num);

/** Writes a number at the end of the String using Hex (base 16) notation. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_hex)(FIO_STR_PTR s,
                                                       int64_t num);
/**
 * Appends the `src` String to the end of the `dest` String.
 *
 * If `dest` is empty, the resulting Strings will be equal.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, concat)(FIO_STR_PTR dest,
                                                    FIO_STR_PTR const src);

/** Alias for fio_str_concat */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, join)(FIO_STR_PTR dest,
                                                      FIO_STR_PTR const src) {
  return FIO_NAME(FIO_STR_NAME, concat)(dest, src);
}

/**
 * Replaces the data in the String - replacing `old_len` bytes starting at
 * `start_pos`, with the data at `src` (`src_len` bytes long).
 *
 * Negative `start_pos` values are calculated backwards, `-1` == end of
 * String.
 *
 * When `old_len` is zero, the function will insert the data at `start_pos`.
 *
 * If `src_len == 0` than `src` will be ignored and the data marked for
 * replacement will be erased.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, replace)(FIO_STR_PTR s,
                                                     intptr_t start_pos,
                                                     size_t old_len,
                                                     const void *src,
                                                     size_t src_len);

/**
 * Writes to the String using a vprintf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, vprintf)(FIO_STR_PTR s,
                                                     const char *format,
                                                     va_list argv);

/**
 * Writes to the String using a printf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              printf)(FIO_STR_PTR s, const char *format, ...);

/**
 * Reads data from a file descriptor `fd` at offset `start_at` and pastes it's
 * contents (or a slice of it) at the end of the String. If `limit == 0`, than
 * the data will be read until EOF.
 *
 * The file should be a regular file or the operation might fail (can't be used
 * for sockets).
 *
 * The file descriptor will remain open and should be closed manually.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfd)(FIO_STR_PTR s,
                                                    int fd,
                                                    intptr_t start_at,
                                                    intptr_t limit);
/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfile)(FIO_STR_PTR s,
                                                      const char *filename,
                                                      intptr_t start_at,
                                                      intptr_t limit);

/* *****************************************************************************
String API - C / JSON escaping
***************************************************************************** */

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_escape)(FIO_STR_PTR s,
                                                          const void *data,
                                                          size_t data_len);

/**
 * Writes an escaped data into the string after unescaping the data.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_unescape)(FIO_STR_PTR s,
                                                            const void *escaped,
                                                            size_t len);

/* *****************************************************************************
String API - Base64 support
***************************************************************************** */

/**
 * Writes data at the end of the String, encoding the data as Base64 encoded
 * data.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_base64enc)(FIO_STR_PTR s,
                                               const void *data,
                                               size_t data_len,
                                               uint8_t url_encoded);

/**
 * Writes decoded base64 data to the end of the String.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_base64dec)(FIO_STR_PTR s,
                                               const void *encoded,
                                               size_t encoded_len);

/* *****************************************************************************
String API - Testing
***************************************************************************** */
#ifdef FIO_STR_WRITE_TEST_FUNC
/**
 * Tests the fio_str functionality.
 */
SFUNC void FIO_NAME_TEST(stl, FIO_STR_NAME)(void);
#endif
/* *****************************************************************************


                             String Implementation

                           IMPLEMENTATION - INLINED


***************************************************************************** */

/* *****************************************************************************
String Macro Helpers
***************************************************************************** */

#define FIO_STR_IS_SMALL(s)  (((s)->special & 1) || !(s)->buf)
#define FIO_STR_SMALL_LEN(s) ((size_t)((s)->special >> 2))
#define FIO_STR_SMALL_LEN_SET(s, l)                                            \
  ((s)->special = (((s)->special & 2) | ((uint8_t)(l) << 2) | 1))
#define FIO_STR_SMALL_CAPA(s) ((sizeof(*(s)) - 2) & 63)
#define FIO_STR_SMALL_DATA(s) ((char *)((s)->reserved))

#define FIO_STR_BIG_DATA(s)       ((s)->buf)
#define FIO_STR_BIG_IS_DYNAMIC(s) (!((s)->special & 4))
#define FIO_STR_BIG_SET_STATIC(s) ((s)->special |= 4)
#define FIO_STR_BIG_FREE_BUF(s)   (FIO_MEM_FREE_((s)->buf, FIO_STR_BIG_CAPA((s))))

#define FIO_STR_IS_FROZEN(s) ((s)->special & 2)
#define FIO_STR_FREEZE_(s)   ((s)->special |= 2)
#define FIO_STR_THAW_(s)     ((s)->special ^= (uint8_t)2)

#if FIO_STR_OPTIMIZE4IMMUTABILITY

#define FIO_STR_BIG_LEN(s)                                                     \
  ((sizeof(void *) == 4)                                                       \
       ? (((uint32_t)(s)->reserved[0]) | ((uint32_t)(s)->reserved[1] << 8) |   \
          ((uint32_t)(s)->reserved[2] << 16))                                  \
       : (((uint64_t)(s)->reserved[0]) | ((uint64_t)(s)->reserved[1] << 8) |   \
          ((uint64_t)(s)->reserved[2] << 16) |                                 \
          ((uint64_t)(s)->reserved[3] << 24) |                                 \
          ((uint64_t)(s)->reserved[4] << 32) |                                 \
          ((uint64_t)(s)->reserved[5] << 40) |                                 \
          ((uint64_t)(s)->reserved[6] << 48)))
#define FIO_STR_BIG_LEN_SET(s, l)                                              \
  do {                                                                         \
    if (sizeof(void *) == 4) {                                                 \
      if (!((l) & ((~(uint32_t)0) << 24))) {                                   \
        (s)->reserved[0] = (l)&0xFF;                                           \
        (s)->reserved[1] = ((uint32_t)(l) >> 8) & 0xFF;                        \
        (s)->reserved[2] = ((uint32_t)(l) >> 16) & 0xFF;                       \
      } else {                                                                 \
        FIO_LOG_ERROR("facil.io small string length error - too long");        \
        (s)->reserved[0] = 0xFF;                                               \
        (s)->reserved[1] = 0xFF;                                               \
        (s)->reserved[2] = 0xFF;                                               \
      }                                                                        \
    } else {                                                                   \
      if (!((l) & ((~(uint64_t)0) << 56))) {                                   \
        (s)->reserved[0] = (l)&0xff;                                           \
        (s)->reserved[1] = ((uint64_t)(l) >> 8) & 0xFF;                        \
        (s)->reserved[2] = ((uint64_t)(l) >> 16) & 0xFF;                       \
        (s)->reserved[3] = ((uint64_t)(l) >> 24) & 0xFF;                       \
        (s)->reserved[4] = ((uint64_t)(l) >> 32) & 0xFF;                       \
        (s)->reserved[5] = ((uint64_t)(l) >> 40) & 0xFF;                       \
        (s)->reserved[6] = ((uint64_t)(l) >> 48) & 0xFF;                       \
      } else {                                                                 \
        FIO_LOG_ERROR("facil.io small string length error - too long");        \
        (s)->reserved[0] = 0xFF;                                               \
        (s)->reserved[1] = 0xFF;                                               \
        (s)->reserved[2] = 0xFF;                                               \
        (s)->reserved[3] = 0xFF;                                               \
        (s)->reserved[4] = 0xFF;                                               \
        (s)->reserved[5] = 0xFF;                                               \
        (s)->reserved[6] = 0xFF;                                               \
      }                                                                        \
    }                                                                          \
  } while (0)
#define FIO_STR_BIG_CAPA(s) FIO_STR_CAPA2WORDS(FIO_STR_BIG_LEN((s)))
#define FIO_STR_BIG_CAPA_SET(s, capa)
#else
#define FIO_STR_BIG_LEN(s)            ((s)->len)
#define FIO_STR_BIG_LEN_SET(s, l)     ((s)->len = (l))
#define FIO_STR_BIG_CAPA(s)           ((s)->capa)
#define FIO_STR_BIG_CAPA_SET(s, capa) (FIO_STR_BIG_CAPA(s) = (capa))
#endif

/**
 * Rounds up allocated capacity to the closest 2 words byte boundary (leaving 1
 * byte space for the NUL byte).
 *
 * This shouldn't effect actual allocation size and should only minimize the
 * effects of the memory allocator's alignment rounding scheme.
 *
 * To clarify:
 *
 * Memory allocators are required to allocate memory on the minimal alignment
 * required by the largest type (`long double`), which usually results in memory
 * allocations using this alignment as a minimal spacing.
 *
 * For example, on 64 bit architectures, it's likely that `malloc(18)` will
 * allocate the same amount of memory as `malloc(32)` due to alignment concerns.
 *
 * In fact, with some allocators (i.e., jemalloc), spacing increases for larger
 * allocations - meaning the allocator will round up to more than 16 bytes, as
 * noted here: http://jemalloc.net/jemalloc.3.html#size_classes
 *
 * Note that this increased spacing, doesn't occure with facil.io's allocator,
 * since it uses 16 byte alignment right up until allocations are routed
 * directly to `mmap` (due to their size, usually over 12KB).
 */
#define FIO_STR_CAPA2WORDS(num)                                                \
  ((size_t)((size_t)(num) |                                                    \
            ((sizeof(long double) > 16) ? (sizeof(long double) - 1)            \
                                        : (size_t)15)))

/* *****************************************************************************
String Constructors (inline)
***************************************************************************** */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/** Allocates a new String object on the heap. */
FIO_IFUNC FIO_STR_PTR FIO_NAME(FIO_STR_NAME, new)(void) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*s), 0);
  if (!FIO_MEM_REALLOC_IS_SAFE_ && s) {
    *s = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT;
  }
#ifdef DEBUG
  {
    FIO_NAME(FIO_STR_NAME, s) tmp = {0};
    FIO_ASSERT(!memcmp(&tmp, s, sizeof(tmp)),
               "new " FIO_MACRO2STR(
                   FIO_NAME(FIO_STR_NAME, s)) " object not initialized!");
  }
#endif
  return (FIO_STR_PTR)FIO_PTR_TAG(s);
}

/** Destroys the string and frees the container (if allocated with `new`). */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, free)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, destroy)(s_);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (s) {
    FIO_MEM_FREE_(s, sizeof(*s));
  }
}

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Frees the String's resources and reinitializes the container.
 *
 * Note: if the container isn't allocated on the stack, it should be freed
 * separately using the appropriate `free` function.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, destroy)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return;
  if (!FIO_STR_IS_SMALL(s) && FIO_STR_BIG_IS_DYNAMIC(s)) {
    FIO_STR_BIG_FREE_BUF(s);
  }
  *s = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT;
}

/**
 * Returns a C string with the existing data, re-initializing the String.
 *
 * Note: the String data is removed from the container, but the container
 * isn't freed.
 *
 * Returns NULL if there's no String data.
 */
FIO_IFUNC char *FIO_NAME(FIO_STR_NAME, detach)(FIO_STR_PTR s_) {
  char *data = NULL;
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_) {
    return data;
  }
  if (FIO_STR_IS_SMALL(s)) {
    if (FIO_STR_SMALL_LEN(s)) { /* keep these ifs apart */
      data =
          (char *)FIO_MEM_REALLOC_(NULL,
                                   0,
                                   sizeof(*data) * (FIO_STR_SMALL_LEN(s) + 1),
                                   0);
      if (data)
        FIO_MEMCPY(data, FIO_STR_SMALL_DATA(s), (FIO_STR_SMALL_LEN(s) + 1));
    }
  } else {
    if (FIO_STR_BIG_IS_DYNAMIC(s)) {
      data = FIO_STR_BIG_DATA(s);
    } else if (FIO_STR_BIG_LEN(s)) {
      data = (char *)FIO_MEM_REALLOC_(NULL,
                                      0,
                                      sizeof(*data) * (FIO_STR_BIG_LEN(s) + 1),
                                      0);
      if (data)
        FIO_MEMCPY(data, FIO_STR_BIG_DATA(s), FIO_STR_BIG_LEN(s) + 1);
    }
  }
  *s = (FIO_NAME(FIO_STR_NAME, s)){0};
  return data;
}

/**
 * Performs a best attempt at minimizing memory consumption.
 *
 * Actual effects depend on the underlying memory allocator and it's
 * implementation. Not all allocators will free any memory.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, compact)(FIO_STR_PTR s_) {
#if FIO_STR_OPTIMIZE4IMMUTABILITY
  (void)s_;
#else
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s_ || !s || FIO_STR_IS_SMALL(s) || FIO_STR_BIG_IS_DYNAMIC(s) ||
      FIO_STR_CAPA2WORDS(FIO_NAME(FIO_STR_NAME, len)(s_)) >=
          FIO_NAME(FIO_STR_NAME, capa)(s_))
    return;
  FIO_NAME(FIO_STR_NAME, s) tmp = FIO_STR_INIT;
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  FIO_NAME(FIO_STR_NAME, init_copy)
  ((FIO_STR_PTR)FIO_PTR_TAG(&tmp), i.buf, i.len);
  FIO_NAME(FIO_STR_NAME, destroy)(s_);
  *s = tmp;
#endif
}

/* *****************************************************************************
String Initialization (inline)
***************************************************************************** */

/**
 * Initializes the container with the provided static / constant string.
 *
 * The string will be copied to the container **only** if it will fit in the
 * container itself. Otherwise, the supplied pointer will be used as is and it
 * should remain valid until the string is destroyed.
 *
 * The final string can be safely be destroyed (using the `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_const)(FIO_STR_PTR s_,
                                                            const char *str,
                                                            size_t len) {
  fio_str_info_s i = {0};
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_) {
    FIO_LOG_ERROR("Attempted to initialize a NULL String.");
    return i;
  }
  *s = (FIO_NAME(FIO_STR_NAME, s)){0};
  if (len < FIO_STR_SMALL_CAPA(s)) {
    FIO_STR_SMALL_LEN_SET(s, len);
    if (len && str)
      FIO_MEMCPY(FIO_STR_SMALL_DATA(s), str, len);
    FIO_STR_SMALL_DATA(s)[len] = 0;

    i = (fio_str_info_s){.buf = FIO_STR_SMALL_DATA(s),
                         .len = len,
                         .capa = FIO_STR_SMALL_CAPA(s)};
    return i;
  }
  FIO_STR_BIG_DATA(s) = (char *)str;
  FIO_STR_BIG_LEN_SET(s, len);
  FIO_STR_BIG_SET_STATIC(s);
  i = (fio_str_info_s){.buf = FIO_STR_BIG_DATA(s), .len = len, .capa = 0};
  return i;
}

/**
 * Initializes the container with the provided dynamic string.
 *
 * The string is always copied and the final string must be destroyed (using the
 * `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_copy)(FIO_STR_PTR s_,
                                                           const char *str,
                                                           size_t len) {
  fio_str_info_s i = {0};
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_) {
    FIO_LOG_ERROR("Attempted to initialize a NULL String.");
    return i;
  }
  *s = (FIO_NAME(FIO_STR_NAME, s)){0};
  if (len < FIO_STR_SMALL_CAPA(s)) {
    FIO_STR_SMALL_LEN_SET(s, len);
    if (len && str)
      FIO_MEMCPY(FIO_STR_SMALL_DATA(s), str, len);
    FIO_STR_SMALL_DATA(s)[len] = 0;

    i = (fio_str_info_s){.buf = FIO_STR_SMALL_DATA(s),
                         .len = len,
                         .capa = FIO_STR_SMALL_CAPA(s)};
    return i;
  }

  {
    char *buf =
        (char *)FIO_MEM_REALLOC_(NULL,
                                 0,
                                 sizeof(*buf) * (FIO_STR_CAPA2WORDS(len) + 1),
                                 0);
    if (!buf)
      return i;
    buf[len] = 0;
    i = (fio_str_info_s){.buf = buf,
                         .len = len,
                         .capa = FIO_STR_CAPA2WORDS(len)};
  }
  FIO_STR_BIG_CAPA_SET(s, i.capa);
  FIO_STR_BIG_DATA(s) = i.buf;
  FIO_STR_BIG_LEN_SET(s, len);
  if (str)
    FIO_MEMCPY(FIO_STR_BIG_DATA(s), str, len);
  return i;
}

/**
 * Initializes the container with a copy of an existing String object.
 *
 * The string is always copied and the final string must be destroyed (using the
 * `destroy` function).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, init_copy2)(FIO_STR_PTR dest,
                                                            FIO_STR_PTR src) {
  fio_str_info_s i;
  i = FIO_NAME(FIO_STR_NAME, info)(src);
  i = FIO_NAME(FIO_STR_NAME, init_copy)(dest, i.buf, i.len);
  return i;
}

/* *****************************************************************************
String Information (inline)
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, info)(const FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return (fio_str_info_s){0};
  if (FIO_STR_IS_SMALL(s))
    return (fio_str_info_s){
        .buf = FIO_STR_SMALL_DATA(s),
        .len = FIO_STR_SMALL_LEN(s),
        .capa = (FIO_STR_IS_FROZEN(s) ? 0 : FIO_STR_SMALL_CAPA(s)),
    };

  return (fio_str_info_s){
      .buf = FIO_STR_BIG_DATA(s),
      .len = FIO_STR_BIG_LEN(s),
      .capa = (FIO_STR_IS_FROZEN(s) ? 0 : FIO_STR_BIG_CAPA(s)),
  };
}

/** Returns a pointer (`char *`) to the String's content. */
FIO_IFUNC char *FIO_NAME2(FIO_STR_NAME, ptr)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return NULL;
  return FIO_STR_IS_SMALL(s) ? FIO_STR_SMALL_DATA(s) : FIO_STR_BIG_DATA(s);
}

/** Returns the String's length in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, len)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return 0;
  return FIO_STR_IS_SMALL(s) ? FIO_STR_SMALL_LEN(s) : FIO_STR_BIG_LEN(s);
}

/** Returns the String's existing capacity (total used & available memory). */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, capa)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return 0;
  if (FIO_STR_IS_SMALL(s))
    return FIO_STR_SMALL_CAPA(s);
  if (FIO_STR_BIG_IS_DYNAMIC(s))
    return FIO_STR_BIG_CAPA(s);
  return 0;
}

/**
 * Sets the new String size without reallocating any memory (limited by
 * existing capacity).
 */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, resize)(FIO_STR_PTR s_,
                                                        size_t size) {
  fio_str_info_s i = {.capa = FIO_NAME(FIO_STR_NAME, capa)(s_)};
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s_ || !s || FIO_STR_IS_FROZEN(s)) {
    i = FIO_NAME(FIO_STR_NAME, info)(s_);
    return i;
  }
  /* resize may be used to reserve memory in advance while setting size  */
  if (i.capa < size) {
    i = FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)(s_, size);
  }

  if (FIO_STR_IS_SMALL(s)) {
    FIO_STR_SMALL_DATA(s)[size] = 0;
    FIO_STR_SMALL_LEN_SET(s, size);
    i = (fio_str_info_s){
        .buf = FIO_STR_SMALL_DATA(s),
        .len = size,
        .capa = FIO_STR_SMALL_CAPA(s),
    };
  } else {
    FIO_STR_BIG_DATA(s)[size] = 0;
    FIO_STR_BIG_LEN_SET(s, size);
    i = (fio_str_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = size,
        .capa = i.capa,
    };
  }
  return i;
}

/**
 * Prevents further manipulations to the String's content.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, freeze)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return;
  FIO_STR_FREEZE_(s);
}

/**
 * Returns true if the string is frozen.
 */
FIO_IFUNC uint8_t FIO_NAME_BL(FIO_STR_NAME, frozen)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return 1;
  return FIO_STR_IS_FROZEN(s);
}

/** Returns 1 if memory was allocated and (the String must be destroyed). */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, allocated)(const FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  return (s_ && s && !FIO_STR_IS_SMALL(s) && FIO_STR_BIG_IS_DYNAMIC(s));
}

/**
 * Binary comparison returns `1` if both strings are equal and `0` if not.
 */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, eq)(const FIO_STR_PTR str1_,
                                            const FIO_STR_PTR str2_) {
  if (str1_ == str2_)
    return 1;
  FIO_NAME(FIO_STR_NAME, s) *const str1 =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(str1_);
  FIO_NAME(FIO_STR_NAME, s) *const str2 =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(str2_);
  if (str1 == str2)
    return 1;
  if (!str1 || !str2)
    return 0;
  fio_str_info_s s1 = FIO_NAME(FIO_STR_NAME, info)(str1_);
  fio_str_info_s s2 = FIO_NAME(FIO_STR_NAME, info)(str2_);
  return FIO_STR_INFO_IS_EQ(s1, s2);
}

/**
 * Returns the string's Risky Hash value.
 *
 * Note: Hash algorithm might change without notice.
 */
FIO_IFUNC uint64_t FIO_NAME(FIO_STR_NAME, hash)(const FIO_STR_PTR s_,
                                                uint64_t seed) {
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return fio_risky_hash(NULL, 0, seed);
  if (FIO_STR_IS_SMALL(s))
    return fio_risky_hash((void *)FIO_STR_SMALL_DATA(s),
                          FIO_STR_SMALL_LEN(s),
                          seed);
  return fio_risky_hash((void *)FIO_STR_BIG_DATA(s), FIO_STR_BIG_LEN(s), seed);
}

/* *****************************************************************************
String API - Content Manipulation and Review (inline)
***************************************************************************** */

/** Writes data at the end of the String. */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write)(FIO_STR_PTR s_,
                                                       const void *src,
                                                       size_t src_len) {
  FIO_NAME(FIO_STR_NAME, s) *s = (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s_ || !s || !src_len || FIO_STR_IS_FROZEN(s))
    return FIO_NAME(FIO_STR_NAME, info)(s_);
  size_t const org_len = FIO_NAME(FIO_STR_NAME, len)(s_);
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, resize)(s_, src_len + org_len);
  if (src)
    FIO_MEMCPY(state.buf + org_len, src, src_len);
  return state;
}

/* *****************************************************************************


                             String Implementation

                               IMPLEMENTATION


***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
String Implementation - Memory management
***************************************************************************** */

/** Frees the pointer returned by `detach`. */
SFUNC void FIO_NAME(FIO_STR_NAME, dealloc)(void *ptr) {
  FIO_MEM_FREE_(ptr, -1);
}

/**
 * Reserves at least `amount` of bytes for the string's data (reserved count
 * includes used data).
 *
 * Returns the current state of the String.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              FIO_STR_RESERVE_NAME)(FIO_STR_PTR s_,
                                                    size_t amount) {
  fio_str_info_s i = {0};
  FIO_NAME(FIO_STR_NAME, s) *const s =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s_ || !s || FIO_STR_IS_FROZEN(s)) {
    i = FIO_NAME(FIO_STR_NAME, info)(s_);
    return i;
  }
  /* result is an embedded string */
  if (amount <= FIO_STR_SMALL_CAPA(s)) {
    if (!FIO_STR_IS_SMALL(s)) {
      /* shrink from allocated(?) string */
      FIO_NAME(FIO_STR_NAME, s) tmp = FIO_STR_INIT;
      FIO_NAME(FIO_STR_NAME, init_copy)
      ((FIO_STR_PTR)FIO_PTR_TAG(&tmp),
       FIO_STR_BIG_DATA(s),
       ((FIO_STR_SMALL_CAPA(s) > FIO_STR_BIG_LEN(s)) ? FIO_STR_BIG_LEN(s)
                                                     : FIO_STR_SMALL_CAPA(s)));
      if (FIO_STR_BIG_IS_DYNAMIC(s))
        FIO_STR_BIG_FREE_BUF(s);
      *s = tmp;
    }
    i = (fio_str_info_s){
        .buf = FIO_STR_SMALL_DATA(s),
        .len = FIO_STR_SMALL_LEN(s),
        .capa = FIO_STR_SMALL_CAPA(s),
    };
    return i;
  }
  /* round up to allocation boundary */
  amount = FIO_STR_CAPA2WORDS(amount);
  if (FIO_STR_IS_SMALL(s)) {
    /* from small to big */
    FIO_NAME(FIO_STR_NAME, s) tmp = FIO_STR_INIT;
    FIO_NAME(FIO_STR_NAME, init_copy)
    ((FIO_STR_PTR)FIO_PTR_TAG(&tmp), NULL, amount);
    FIO_MEMCPY(FIO_STR_BIG_DATA(&tmp),
               FIO_STR_SMALL_DATA(s),
               FIO_STR_SMALL_CAPA(s));
    FIO_STR_BIG_LEN_SET(&tmp, FIO_STR_SMALL_LEN(s));
    *s = tmp;
    i = (fio_str_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = FIO_STR_BIG_LEN(s),
        .capa = amount,
    };
    return i;
  } else if (FIO_STR_BIG_IS_DYNAMIC(s) && FIO_STR_BIG_CAPA(s) == amount) {
    i = (fio_str_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = FIO_STR_BIG_LEN(s),
        .capa = amount,
    };
  } else {
    /* from big to big - grow / shrink */
    const size_t __attribute__((unused)) old_capa = FIO_STR_BIG_CAPA(s);
    size_t data_len = FIO_STR_BIG_LEN(s);
    if (data_len > amount) {
      /* truncate */
      data_len = amount;
      FIO_STR_BIG_LEN_SET(s, data_len);
    }
    char *tmp = NULL;
    if (FIO_STR_BIG_IS_DYNAMIC(s)) {
      tmp = (char *)FIO_MEM_REALLOC_(FIO_STR_BIG_DATA(s),
                                     old_capa,
                                     (amount + 1) * sizeof(char),
                                     data_len);
      (void)old_capa; /* might not be used by macro */
    } else {
      tmp = (char *)FIO_MEM_REALLOC_(NULL, 0, (amount + 1) * sizeof(char), 0);
      if (tmp) {
        s->special = 0;
        tmp[data_len] = 0;
        if (data_len)
          FIO_MEMCPY(tmp, FIO_STR_BIG_DATA(s), data_len);
      }
    }
    if (tmp) {
      tmp[data_len] = 0;
      FIO_STR_BIG_DATA(s) = tmp;
      FIO_STR_BIG_CAPA_SET(s, amount);
    } else {
      amount = FIO_STR_BIG_CAPA(s);
    }
    i = (fio_str_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = data_len,
        .capa = amount,
    };
  }
  return i;
}

/* *****************************************************************************
String Implementation - UTF-8 State
***************************************************************************** */

#ifndef FIO_STR_UTF8_CODE_POINT
/**
 * Maps the first 5 bits in a byte (0b11111xxx) to a UTF-8 codepoint length.
 *
 * Codepoint length 0 == error.
 *
 * The first valid length can be any value between 1 to 4.
 *
 * A continuation byte (second, third or forth) valid length marked as 5.
 *
 * To map was populated using the following Ruby script:
 *
 *      map = []; 32.times { map << 0 }; (0..0b1111).each {|i| map[i] = 1} ;
 *      (0b10000..0b10111).each {|i| map[i] = 5} ;
 *      (0b11000..0b11011).each {|i| map[i] = 2} ;
 *      (0b11100..0b11101).each {|i| map[i] = 3} ;
 *      map[0b11110] = 4; map;
 */
static __attribute__((unused))
uint8_t fio__str_utf8_map[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               5, 5, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 3, 3, 4, 0};

/**
 * Advances the `ptr` by one utf-8 character, placing the value of the UTF-8
 * character into the i32 variable (which must be a signed integer with 32bits
 * or more). On error, `i32` will be equal to `-1` and `ptr` will not step
 * forwards.
 *
 * The `end` value is only used for overflow protection.
 */
#define FIO_STR_UTF8_CODE_POINT(ptr, end, i32)                                 \
  do {                                                                         \
    switch (fio__str_utf8_map[((uint8_t *)(ptr))[0] >> 3]) {                   \
    case 1:                                                                    \
      (i32) = ((uint8_t *)(ptr))[0];                                           \
      ++(ptr);                                                                 \
      break;                                                                   \
    case 2:                                                                    \
      if (((ptr) + 2 > (end)) ||                                               \
          fio__str_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) =                                                                  \
          ((((uint8_t *)(ptr))[0] & 31) << 6) | (((uint8_t *)(ptr))[1] & 63);  \
      (ptr) += 2;                                                              \
      break;                                                                   \
    case 3:                                                                    \
      if (((ptr) + 3 > (end)) ||                                               \
          fio__str_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||                \
          fio__str_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 15) << 12) |                           \
              ((((uint8_t *)(ptr))[1] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[2] & 63);                                    \
      (ptr) += 3;                                                              \
      break;                                                                   \
    case 4:                                                                    \
      if (((ptr) + 4 > (end)) ||                                               \
          fio__str_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||                \
          fio__str_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5 ||                \
          fio__str_utf8_map[((uint8_t *)(ptr))[3] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 7) << 18) |                            \
              ((((uint8_t *)(ptr))[1] & 63) << 12) |                           \
              ((((uint8_t *)(ptr))[2] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[3] & 63);                                    \
      (ptr) += 4;                                                              \
      break;                                                                   \
    default: (i32) = -1; break;                                                \
    }                                                                          \
  } while (0);
#endif

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_valid)(FIO_STR_PTR s_) {
  FIO_NAME(FIO_STR_NAME, s) *s = (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || !s_)
    return 0;
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!state.len)
    return 1;
  char *const end = state.buf + state.len;
  int32_t c = 0;
  do {
    FIO_STR_UTF8_CODE_POINT(state.buf, end, c);
  } while (c > 0 && state.buf < end);
  return state.buf == end && c >= 0;
}

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_len)(FIO_STR_PTR s_) {
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!state.len)
    return 0;
  char *end = state.buf + state.len;
  size_t utf8len = 0;
  int32_t c = 0;
  do {
    ++utf8len;
    FIO_STR_UTF8_CODE_POINT(state.buf, end, c);
  } while (c > 0 && state.buf < end);
  if (state.buf != end || c == -1) {
    /* invalid */
    return 0;
  }
  return utf8len;
}

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int FIO_NAME(FIO_STR_NAME,
                   utf8_select)(FIO_STR_PTR s_, intptr_t *pos, size_t *len) {
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  int32_t c = 0;
  char *p = state.buf;
  char *const end = state.buf + state.len;
  size_t start;

  if (!state.buf)
    goto error;
  if (!state.len || *pos == -1)
    goto at_end;

  if (*pos) {
    if ((*pos) > 0) {
      start = *pos;
      while (start && p < end && c >= 0) {
        FIO_STR_UTF8_CODE_POINT(p, end, c);
        --start;
      }
      if (c == -1)
        goto error;
      if (start || p >= end)
        goto at_end;
      *pos = p - state.buf;
    } else {
      /* walk backwards */
      p = state.buf + state.len - 1;
      c = 0;
      ++*pos;
      do {
        switch (fio__str_utf8_map[((uint8_t *)p)[0] >> 3]) {
        case 5: ++c; break;
        case 4:
          if (c != 3)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 3:
          if (c != 2)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 2:
          if (c != 1)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 1:
          if (c)
            goto error;
          ++(*pos);
          break;
        default: goto error;
        }
        --p;
      } while (p > state.buf && *pos);
      if (c)
        goto error;
      ++p; /* There's always an extra back-step */
      *pos = (p - state.buf);
    }
  }

  /* find end */
  start = *len;
  while (start && p < end && c >= 0) {
    FIO_STR_UTF8_CODE_POINT(p, end, c);
    --start;
  }
  if (c == -1 || p > end)
    goto error;
  *len = p - (state.buf + (*pos));
  return 0;

at_end:
  *pos = state.len;
  *len = 0;
  return 0;
error:
  *pos = -1;
  *len = 0;
  return -1;
}

/* *****************************************************************************
String Implementation - Content Manipulation and Review
***************************************************************************** */

/**
 * Writes a number at the end of the String using normal base 10 notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_i)(FIO_STR_PTR s_,
                                                     int64_t num) {
  /* because fio_ltoa uses an internal buffer, we "save" a `memcpy` loop and
   * minimize memory allocations by re-implementing the same logic in a
   * dedicated fasion. */
  FIO_NAME(FIO_STR_NAME, s) *s = (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  if (!s || FIO_STR_IS_FROZEN(s))
    return FIO_NAME(FIO_STR_NAME, info)(s_);
  fio_str_info_s i;
  if (!num)
    goto write_zero;
  {
    char buf[22];
    uint64_t l = 0;
    uint8_t neg = 0;
    int64_t t = num / 10;
    if (num < 0) {
      num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
      t = (int64_t)0 - t;
      neg = 1;
    }
    while (num) {
      buf[l++] = '0' + (num - (t * 10));
      num = t;
      t = num / 10;
    }
    if (neg) {
      buf[l++] = '-';
    }
    i = FIO_NAME(FIO_STR_NAME, resize)(s_, FIO_NAME(FIO_STR_NAME, len)(s_) + l);

    while (l) {
      --l;
      i.buf[i.len - (l + 1)] = buf[l];
    }
  }
  return i;
write_zero:
  i = FIO_NAME(FIO_STR_NAME, resize)(s_, FIO_NAME(FIO_STR_NAME, len)(s_) + 1);
  i.buf[i.len - 1] = '0';
  return i;
}

/**
 * Writes a number at the end of the String using Hex (base 16) notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_hex)(FIO_STR_PTR s,
                                                       int64_t num) {
  /* using base 16 with fio_ltoa, buffer might minimize memory allocation. */
  char buf[(sizeof(int64_t) * 2) + 8];
  size_t written = fio_ltoa(buf, num, 16);
  return FIO_NAME(FIO_STR_NAME, write)(s, buf, written);
}

/**
 * Appends the `src` String to the end of the `dest` String.
 *
 * If `dest` is empty, the resulting Strings will be equal.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, concat)(FIO_STR_PTR dest_,
                                                    FIO_STR_PTR const src_) {
  FIO_NAME(FIO_STR_NAME, s) *dest =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(dest_);
  FIO_NAME(FIO_STR_NAME, s) *src =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(src_);
  if (!dest || !src || !dest_ || !src_ || FIO_STR_IS_FROZEN(dest)) {
    return FIO_NAME(FIO_STR_NAME, info)(dest_);
  }
  fio_str_info_s src_state = FIO_NAME(FIO_STR_NAME, info)(src_);
  if (!src_state.len)
    return FIO_NAME(FIO_STR_NAME, info)(dest_);
  const size_t old_len = FIO_NAME(FIO_STR_NAME, len)(dest_);
  fio_str_info_s state =
      FIO_NAME(FIO_STR_NAME, resize)(dest_, src_state.len + old_len);
  FIO_MEMCPY(state.buf + old_len, src_state.buf, src_state.len);
  return state;
}

/**
 * Replaces the data in the String - replacing `old_len` bytes starting at
 * `start_pos`, with the data at `src` (`src_len` bytes long).
 *
 * Negative `start_pos` values are calculated backwards, `-1` == end of
 * String.
 *
 * When `old_len` is zero, the function will insert the data at `start_pos`.
 *
 * If `src_len == 0` than `src` will be ignored and the data marked for
 * replacement will be erased.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, replace)(FIO_STR_PTR s_,
                                                     intptr_t start_pos,
                                                     size_t old_len,
                                                     const void *src,
                                                     size_t src_len) {
  FIO_NAME(FIO_STR_NAME, s) *s = (FIO_NAME(FIO_STR_NAME, s) *)FIO_PTR_UNTAG(s_);
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!s_ || !s || FIO_STR_IS_FROZEN(s) || (!old_len && !src_len) ||
      (!src && src_len))
    return state;

  if (start_pos < 0) {
    /* backwards position indexing */
    start_pos += FIO_NAME(FIO_STR_NAME, len)(s_) + 1;
    if (start_pos < 0)
      start_pos = 0;
  }

  if (start_pos + old_len >= state.len) {
    /* old_len overflows the end of the String */
    if (FIO_STR_IS_SMALL(s)) {
      FIO_STR_SMALL_LEN_SET(s, start_pos);
    } else {
      FIO_STR_BIG_LEN_SET(s, start_pos);
    }
    return FIO_NAME(FIO_STR_NAME, write)(s_, src, src_len);
  }

  /* data replacement is now always in the middle (or start) of the String */
  const size_t new_size = state.len + (src_len - old_len);

  if (old_len != src_len) {
    /* there's an offset requiring an adjustment */
    if (old_len < src_len) {
      /* make room for new data */
      const size_t offset = src_len - old_len;
      state = FIO_NAME(FIO_STR_NAME, resize)(s_, state.len + offset);
    }
    memmove(state.buf + start_pos + src_len,
            state.buf + start_pos + old_len,
            (state.len - start_pos) - old_len);
  }
  if (src_len) {
    FIO_MEMCPY(state.buf + start_pos, src, src_len);
  }

  return FIO_NAME(FIO_STR_NAME, resize)(s_, new_size);
}

/**
 * Writes to the String using a vprintf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s __attribute__((format(FIO___PRINTF_STYLE, 2, 0)))
FIO_NAME(FIO_STR_NAME,
         vprintf)(FIO_STR_PTR s_, const char *format, va_list argv) {
  va_list argv_cpy;
  va_copy(argv_cpy, argv);
  int len = vsnprintf(NULL, 0, format, argv_cpy);
  va_end(argv_cpy);
  if (len <= 0)
    return FIO_NAME(FIO_STR_NAME, info)(s_);
  fio_str_info_s state =
      FIO_NAME(FIO_STR_NAME, resize)(s_, len + FIO_NAME(FIO_STR_NAME, len)(s_));
  if (state.capa >= (size_t)len)
    vsnprintf(state.buf + (state.len - len), len + 1, format, argv);
  return state;
}

/**
 * Writes to the String using a printf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s __attribute__((format(FIO___PRINTF_STYLE, 2, 3)))
FIO_NAME(FIO_STR_NAME, printf)(FIO_STR_PTR s_, const char *format, ...) {
  va_list argv;
  va_start(argv, format);
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, vprintf)(s_, format, argv);
  va_end(argv);
  return state;
}

/* *****************************************************************************
String API - C / JSON escaping
***************************************************************************** */

/* constant time (non-branching) if statement used in a loop as a helper */
#define FIO_STR_WRITE_ESCAPED_CT_OR(cond, a, b)                                \
  ((b) ^                                                                       \
   ((0 - ((((cond) | (0 - (cond))) >> ((sizeof((cond)) << 3) - 1)) & 1)) &     \
    ((a) ^ (b))))

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_escape)(FIO_STR_PTR s,
                                                          const void *src_,
                                                          size_t len) {
  const uint8_t *src = (const uint8_t *)src_;
  size_t extra_len = 0;
  size_t at = 0;
  uint8_t set_at = 1;

  /* collect escaping requiremnents */
  for (size_t i = 0; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ')
      continue;
    /* skip valid UTF-8 */
    switch (fio__str_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__str_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 3:
      if (fio__str_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 2:
      if (fio__str_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      i += fio__str_utf8_map[src[i] >> 3] - 1;
      continue;
    }
    /* store first instance of character that needs escaping */
    at = FIO_STR_WRITE_ESCAPED_CT_OR(set_at, i, at);
    set_at = 0;

    /* count extra bytes */
    switch (src[i]) {
    case '\b': /* fall through */
    case '\f': /* fall through */
    case '\n': /* fall through */
    case '\r': /* fall through */
    case '\t': /* fall through */
    case '"':  /* fall through */
    case '\\': /* fall through */
    case '/': /* fall through */ ++extra_len; break;
    default:
      /* escaping all control charactes and non-UTF-8 characters */
      extra_len += 5;
    }
  }
  /* reserve space and copy any valid "head" */
  fio_str_info_s dest;
  {
    const size_t org_len = FIO_NAME(FIO_STR_NAME, len)(s);
#if !FIO_STR_OPTIMIZE4IMMUTABILITY
    /* often, after `write_escape` come quotes */
    FIO_NAME(FIO_STR_NAME, reserve)(s, org_len + extra_len + len + 4);
#endif
    dest = FIO_NAME(FIO_STR_NAME, resize)(s, org_len + extra_len + len);
    dest.len = org_len;
  }
  dest.buf += dest.len;
  /* is escaping required? - simple memcpy if we don't need to escape */
  if (set_at) {
    FIO_MEMCPY(dest.buf, src, len);
    dest.buf -= dest.len;
    dest.len += len;
    return dest;
  }
  /* simple memcpy until first char that needs escaping */
  if (at >= 8) {
    FIO_MEMCPY(dest.buf, src, at);
  } else {
    at = 0;
  }
  /* start escaping */
  for (size_t i = at; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ') {
      dest.buf[at++] = src[i];
      continue;
    }
    /* skip valid UTF-8 */
    switch (fio__str_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__str_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 3:
      if (fio__str_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fall through */
    case 2:
      if (fio__str_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      switch (fio__str_utf8_map[src[i] >> 3]) {
      case 4: dest.buf[at++] = src[i++]; /* fall through */
      case 3: dest.buf[at++] = src[i++]; /* fall through */
      case 2: dest.buf[at++] = src[i++]; dest.buf[at++] = src[i];
      }
      continue;
    }

    /* write escape sequence */
    dest.buf[at++] = '\\';
    switch (src[i]) {
    case '\b': dest.buf[at++] = 'b'; break;
    case '\f': dest.buf[at++] = 'f'; break;
    case '\n': dest.buf[at++] = 'n'; break;
    case '\r': dest.buf[at++] = 'r'; break;
    case '\t': dest.buf[at++] = 't'; break;
    case '"': dest.buf[at++] = '"'; break;
    case '\\': dest.buf[at++] = '\\'; break;
    case '/': dest.buf[at++] = '/'; break;
    default:
      /* escaping all control charactes and non-UTF-8 characters */
      if (src[i] < 127) {
        dest.buf[at++] = 'u';
        dest.buf[at++] = '0';
        dest.buf[at++] = '0';
        dest.buf[at++] = fio_i2c(src[i] >> 4);
        dest.buf[at++] = fio_i2c(src[i] & 15);
      } else {
        /* non UTF-8 data... encode as...? */
        dest.buf[at++] = 'x';
        dest.buf[at++] = fio_i2c(src[i] >> 4);
        dest.buf[at++] = fio_i2c(src[i] & 15);
      }
    }
  }
  return FIO_NAME(FIO_STR_NAME, resize)(s, dest.len + at);
}

/**
 * Writes an escaped data into the string after unescaping the data.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_unescape)(FIO_STR_PTR s,
                                                            const void *src_,
                                                            size_t len) {
  fio_str_info_s dest;
  {
    const size_t org_len = FIO_NAME(FIO_STR_NAME, len)(s);
    dest = FIO_NAME(FIO_STR_NAME, resize)(s, org_len + len);
    dest.len = org_len;
  }
  size_t at = 0;
  const uint8_t *src = (const uint8_t *)src_;
  const uint8_t *end = src + len;
  dest.buf += dest.len;
  while (src < end) {
#if 1 /* A/B performance at a later stage */
    if (*src != '\\') {
      const uint8_t *escape_pos = (const uint8_t *)memchr(src, '\\', end - src);
      if (!escape_pos)
        escape_pos = end;
      const size_t valid_len = escape_pos - src;
      if (valid_len) {
        memmove(dest.buf + at, src, valid_len);
        at += valid_len;
        src = escape_pos;
      }
    }
#else
#if __x86_64__ || __aarch64__
    /* levarege unaligned memory access to test and copy 8 bytes at a time */
    while (src + 8 <= end) {
      const uint64_t wanted1 = 0x0101010101010101ULL * '\\';
      const uint64_t eq1 =
          ~((*((uint64_t *)src)) ^ wanted1); /* 0 == eq. inverted, all bits 1 */
      const uint64_t t0 = (eq1 & 0x7f7f7f7f7f7f7f7fllu) + 0x0101010101010101llu;
      const uint64_t t1 = (eq1 & 0x8080808080808080llu);
      if ((t0 & t1)) {
        break; /* from 8 byte seeking algorithm */
      }
      *(uint64_t *)(dest.buf + at) = *(uint64_t *)src;
      src += 8;
      at += 8;
    }
#endif
    while (src < end && *src != '\\') {
      dest.buf[at++] = *(src++);
    }
#endif
    if (end - src == 1) {
      dest.buf[at++] = *(src++);
    }
    if (src >= end)
      break;
    /* escaped data - src[0] == '\\' */
    ++src;
    switch (src[0]) {
    case 'b':
      dest.buf[at++] = '\b';
      ++src;
      break; /* from switch */
    case 'f':
      dest.buf[at++] = '\f';
      ++src;
      break; /* from switch */
    case 'n':
      dest.buf[at++] = '\n';
      ++src;
      break; /* from switch */
    case 'r':
      dest.buf[at++] = '\r';
      ++src;
      break; /* from switch */
    case 't':
      dest.buf[at++] = '\t';
      ++src;
      break; /* from switch */
    case 'u': {
      /* test UTF-8 notation */
      if (fio_c2i(src[1]) < 16 && fio_c2i(src[2]) < 16 &&
          fio_c2i(src[3]) < 16 && fio_c2i(src[4]) < 16) {
        uint32_t u = (((fio_c2i(src[1]) << 4) | fio_c2i(src[2])) << 8) |
                     ((fio_c2i(src[3]) << 4) | fio_c2i(src[4]));
        if (((fio_c2i(src[1]) << 4) | fio_c2i(src[2])) == 0xD8U &&
            src[5] == '\\' && src[6] == 'u' && fio_c2i(src[7]) < 16 &&
            fio_c2i(src[8]) < 16 && fio_c2i(src[9]) < 16 &&
            fio_c2i(src[10]) < 16) {
          /* surrogate-pair */
          u = (u & 0x03FF) << 10;
          u |= (((((fio_c2i(src[7]) << 4) | fio_c2i(src[8])) << 8) |
                 ((fio_c2i(src[9]) << 4) | fio_c2i(src[10]))) &
                0x03FF);
          u += 0x10000;
          src += 6;
        }
        if (u <= 127) {
          dest.buf[at++] = u;
        } else if (u <= 2047) {
          dest.buf[at++] = 192 | (u >> 6);
          dest.buf[at++] = 128 | (u & 63);
        } else if (u <= 65535) {
          dest.buf[at++] = 224 | (u >> 12);
          dest.buf[at++] = 128 | ((u >> 6) & 63);
          dest.buf[at++] = 128 | (u & 63);
        } else {
          dest.buf[at++] = 240 | ((u >> 18) & 7);
          dest.buf[at++] = 128 | ((u >> 12) & 63);
          dest.buf[at++] = 128 | ((u >> 6) & 63);
          dest.buf[at++] = 128 | (u & 63);
        }
        src += 5;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case 'x': { /* test for hex notation */
      if (fio_c2i(src[1]) < 16 && fio_c2i(src[2]) < 16) {
        dest.buf[at++] = (fio_c2i(src[1]) << 4) | fio_c2i(src[2]);
        src += 3;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': { /* test for octal notation */
      if (src[0] >= '0' && src[0] <= '7' && src[1] >= '0' && src[1] <= '7') {
        dest.buf[at++] = ((src[0] - '0') << 3) | (src[1] - '0');
        src += 2;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '"':
    case '\\':
    case '/':
    /* fall through */
    default:
    invalid_escape:
      dest.buf[at++] = *(src++);
    }
  }
  return FIO_NAME(FIO_STR_NAME, resize)(s, dest.len + at);
}

#undef FIO_STR_WRITE_ESCAPED_CT_OR

/* *****************************************************************************
String - Base64 support
***************************************************************************** */

/**
 * Writes data at the end of the String, encoding the data as Base64 encoded
 * data.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_base64enc)(FIO_STR_PTR s_,
                                               const void *data,
                                               size_t len,
                                               uint8_t url_encoded) {
  if (!s_ || !FIO_PTR_UNTAG(s_) || !len ||
      FIO_NAME_BL(FIO_STR_NAME, frozen)(s_))
    return FIO_NAME(FIO_STR_NAME, info)(s_);

  /* the base64 encoding array */
  const char *encoding;
  if (url_encoded == 0) {
    encoding =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
  } else {
    encoding =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";
  }

  /* base64 length and padding information */
  int groups = len / 3;
  const int mod = len - (groups * 3);
  const int target_size = (groups + (mod != 0)) * 4;
  const uint32_t org_len = FIO_NAME(FIO_STR_NAME, len)(s_);
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, resize)(s_, org_len + target_size);
  char *writer = i.buf + org_len;
  const unsigned char *reader = (const unsigned char *)data;

  /* write encoded data */
  while (groups) {
    --groups;
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);
    const unsigned char tmp3 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15))];
    *(writer++) = encoding[((tmp2 & 15) << 2) | ((tmp3 >> 6) & 3)];
    *(writer++) = encoding[tmp3 & 63];
  }

  /* write padding / ending */
  switch (mod) {
  case 2: {
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15)];
    *(writer++) = encoding[((tmp2 & 15) << 2)];
    *(writer++) = '=';
  } break;
  case 1: {
    const unsigned char tmp1 = *(reader++);

    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(tmp1 & 3) << 4];
    *(writer++) = '=';
    *(writer++) = '=';
  } break;
  }
  return i;
}

/**
 * Writes decoded base64 data to the end of the String.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_base64dec)(FIO_STR_PTR s_,
                                               const void *encoded_,
                                               size_t len) {
  /*
  Base64 decoding array. Generation script (Ruby):

a = []; a[255] = 0
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 };
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 };
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 }; a.map!{ |i| i.to_i }; a

  */
  const uint8_t base64_decodes[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   125, 127,
      125, 0,   127, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 0,   0,
      0,   129, 0,   0,   0,   1,   3,   5,   7,   9,   11,  13,  15,  17,  19,
      21,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,  45,  47,  49,
      51,  0,   0,   0,   0,   127, 0,   53,  55,  57,  59,  61,  63,  65,  67,
      69,  71,  73,  75,  77,  79,  81,  83,  85,  87,  89,  91,  93,  95,  97,
      99,  101, 103, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0};
#define FIO_BASE64_BITVAL(x) ((base64_decodes[(x)] >> 1) & 63)

  if (!s_ || !FIO_PTR_UNTAG(s_) || !len || !encoded_ ||
      FIO_NAME_BL(FIO_STR_NAME, frozen)(s_))
    return FIO_NAME(FIO_STR_NAME, info)(s_);

  const uint8_t *encoded = (const uint8_t *)encoded_;

  /* skip unknown data at end */
  while (len && !base64_decodes[encoded[len - 1]]) {
    len--;
  }

  /* reserve memory space */
  const uint32_t org_len = FIO_NAME(FIO_STR_NAME, len)(s_);
  fio_str_info_s i =
      FIO_NAME(FIO_STR_NAME, resize)(s_, org_len + ((len >> 2) * 3) + 3);
  i.buf += org_len;

  /* decoded and count actual length */
  int32_t written = 0;
  uint8_t tmp1, tmp2, tmp3, tmp4;
  while (len >= 4) {
    if (isspace((*encoded))) {
      while (len && isspace((*encoded))) {
        len--;
        encoded++;
      }
      continue;
    }
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    tmp3 = *(encoded++);
    tmp4 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2] ||
        !base64_decodes[tmp3] || !base64_decodes[tmp4]) {
      return (fio_str_info_s){.buf = NULL};
    }
    *(i.buf++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 4);
    *(i.buf++) =
        (FIO_BASE64_BITVAL(tmp2) << 4) | (FIO_BASE64_BITVAL(tmp3) >> 2);
    *(i.buf++) = (FIO_BASE64_BITVAL(tmp3) << 6) | (FIO_BASE64_BITVAL(tmp4));
    /* make sure we don't loop forever */
    len -= 4;
    /* count written bytes */
    written += 3;
  }
  /* skip white spaces */
  while (len && isspace((*encoded))) {
    len--;
    encoded++;
  }
  /* decode "tail" - if any (mis-encoded, shouldn't happen) */
  tmp1 = 0;
  tmp2 = 0;
  tmp3 = 0;
  tmp4 = 0;
  switch (len) {
  case 1:
    tmp1 = *(encoded++);
    if (!base64_decodes[tmp1]) {
      return (fio_str_info_s){.buf = NULL};
    }
    *(i.buf++) = FIO_BASE64_BITVAL(tmp1);
    written += 1;
    break;
  case 2:
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2]) {
      return (fio_str_info_s){.buf = NULL};
    }
    *(i.buf++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 6);
    *(i.buf++) = (FIO_BASE64_BITVAL(tmp2) << 4);
    written += 2;
    break;
  case 3:
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    tmp3 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2] ||
        !base64_decodes[tmp3]) {
      return (fio_str_info_s){.buf = NULL};
    }
    *(i.buf++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 6);
    *(i.buf++) =
        (FIO_BASE64_BITVAL(tmp2) << 4) | (FIO_BASE64_BITVAL(tmp3) >> 2);
    *(i.buf++) = FIO_BASE64_BITVAL(tmp3) << 6;
    written += 3;
    break;
  }
#undef FIO_BASE64_BITVAL

  if (encoded[-1] == '=') {
    i.buf--;
    written--;
    if (encoded[-2] == '=') {
      i.buf--;
      written--;
    }
    if (written < 0)
      written = 0;
  }

  // if (FIO_STR_IS_SMALL(s_))
  //   FIO_STR_SMALL_LEN_SET(s_, (org_len + written));
  // else
  //   FIO_STR_BIG_LEN_SET(s_, (org_len + written));

  return FIO_NAME(FIO_STR_NAME, resize)(s_, org_len + written);
}

/* *****************************************************************************
String - read file
***************************************************************************** */

#if FIO_OS_WIN && _MSC_VER && !defined(fstat)
#define fstat           _fstat64
#define FIO_FSTAT_UNDEF 1
#endif /* FIO_OS_WIN && _MSC_VER */

/**
 * Reads data from a file descriptor `fd` at offset `start_at` and pastes it's
 * contents (or a slice of it) at the end of the String. If `limit == 0`, than
 * the data will be read until EOF.
 *
 * The file should be a regular file or the operation might fail (can't be used
 * for sockets).
 *
 * The file descriptor will remain open and should be closed manually.
 *
 * Currently implemented only on POSIX systems.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfd)(FIO_STR_PTR s_,
                                                    int fd,
                                                    intptr_t start_at,
                                                    intptr_t limit) {
  struct stat f_data;
  fio_str_info_s state = {.buf = NULL};
  if (fd == -1 || fstat(fd, &f_data) == -1) {
    return state;
  }

  if (f_data.st_size <= 0 || start_at >= f_data.st_size) {
    state = FIO_NAME(FIO_STR_NAME, info)(s_);
    return state;
  }

  if (limit <= 0 || f_data.st_size < (limit + start_at)) {
    limit = f_data.st_size - start_at;
  }

  const size_t org_len = FIO_NAME(FIO_STR_NAME, len)(s_);
  size_t write_pos = org_len;
  state = FIO_NAME(FIO_STR_NAME, resize)(s_, org_len + limit);
  if (state.capa < (org_len + limit) || !state.buf) {
    return state;
  }

  while (limit) {
    /* copy up to 128Mb at a time... why? because pread might fail */
    const size_t to_read =
        (limit & (((size_t)1 << 27) - 1)) | ((!!(limit >> 27)) << 27);
    if (pread(fd, state.buf + write_pos, to_read, start_at) !=
        (ssize_t)to_read) {
      goto error;
    }
    limit -= to_read;
    write_pos += to_read;
    start_at += to_read;
  }
  return state;

error:
  FIO_NAME(FIO_STR_NAME, resize)(s_, org_len);
  state.buf = NULL;
  state.len = state.capa = 0;
  return state;
}

/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 *
 * Currently implemented only on POSIX systems.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfile)(FIO_STR_PTR s_,
                                                      const char *filename,
                                                      intptr_t start_at,
                                                      intptr_t limit) {
  fio_str_info_s state = {.buf = NULL};
  /* POSIX implementations. */
  int fd = fio_filename_open(filename, O_RDONLY);
  if (fd == -1)
    return state;
  state = FIO_NAME(FIO_STR_NAME, readfd)(s_, fd, start_at, limit);
  close(fd);
  return state;
}

#ifdef FIO_FSTAT_UNDEF
#undef FIO_FSTAT_UNDEF
#undef fstat
#endif

/* *****************************************************************************


                                    String Test


***************************************************************************** */
#ifdef FIO_STR_WRITE_TEST_FUNC

/**
 * Tests the fio_str functionality.
 */
SFUNC void FIO_NAME_TEST(stl, FIO_STR_NAME)(void) {
  FIO_NAME(FIO_STR_NAME, s) str = {0}; /* test zeroed out memory */
#define FIO__STR_SMALL_CAPA FIO_STR_SMALL_CAPA(&str)
  fprintf(
      stderr,
      "* Testing core string features for " FIO_MACRO2STR(FIO_STR_NAME) ".\n");
  fprintf(stderr,
          "* String container size (without wrapper): %zu\n",
          sizeof(FIO_NAME(FIO_STR_NAME, s)));
  fprintf(stderr,
          "* Self-contained capacity (FIO_STR_SMALL_CAPA): %zu\n",
          FIO__STR_SMALL_CAPA);
  FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, frozen)(&str), "new string is frozen");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) == FIO__STR_SMALL_CAPA,
             "small string capacity returned %zu",
             FIO_NAME(FIO_STR_NAME, capa)(&str));
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) == 0,
             "small string length reporting error!");
  FIO_ASSERT(
      FIO_NAME2(FIO_STR_NAME, ptr)(&str) == ((char *)(&str) + 1),
      "small string pointer reporting error (%zd offset)!",
      (ssize_t)(((char *)(&str) + 1) - FIO_NAME2(FIO_STR_NAME, ptr)(&str)));
  FIO_NAME(FIO_STR_NAME, write)(&str, "World", 4);
  FIO_ASSERT(FIO_STR_IS_SMALL(&str),
             "small string writing error - not small on small write!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) == FIO__STR_SMALL_CAPA,
             "Small string capacity reporting error after write!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) == 4,
             "small string length reporting error after write!");
  FIO_ASSERT(FIO_NAME2(FIO_STR_NAME, ptr)(&str) == (char *)&str + 1,
             "small string pointer reporting error after write!");
  FIO_ASSERT(!FIO_NAME2(FIO_STR_NAME, ptr)(&str)[4] &&
                 strlen(FIO_NAME2(FIO_STR_NAME, ptr)(&str)) == 4,
             "small string NUL missing after write (%zu)!",
             strlen(FIO_NAME2(FIO_STR_NAME, ptr)(&str)));
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Worl"),
             "small string write error (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));
  FIO_ASSERT(FIO_NAME2(FIO_STR_NAME, ptr)(&str) ==
                 FIO_NAME(FIO_STR_NAME, info)(&str).buf,
             "small string `data` != `info.buf` (%p != %p)",
             (void *)FIO_NAME2(FIO_STR_NAME, ptr)(&str),
             (void *)FIO_NAME(FIO_STR_NAME, info)(&str).buf);

  FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)
  (&str, sizeof(FIO_NAME(FIO_STR_NAME, s)));
  FIO_ASSERT(!FIO_STR_IS_SMALL(&str),
             "Long String reporting as small after capacity update!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) >=
                 sizeof(FIO_NAME(FIO_STR_NAME, s)) - 1,
             "Long String capacity update error (%zu != %zu)!",
             FIO_NAME(FIO_STR_NAME, capa)(&str),
             FIO_STR_SMALL_CAPA(&str));

  FIO_ASSERT(FIO_NAME2(FIO_STR_NAME, ptr)(&str) ==
                 FIO_NAME(FIO_STR_NAME, info)(&str).buf,
             "Long String `ptr` !>= "
             "`cstr(s).buf` (%p != %p)",
             (void *)FIO_NAME2(FIO_STR_NAME, ptr)(&str),
             (void *)FIO_NAME(FIO_STR_NAME, info)(&str).buf);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
  /* immutable string length is updated after `reserve` to reflect new capa */
  FIO_NAME(FIO_STR_NAME, resize)(&str, 4);
#endif
  FIO_ASSERT(
      FIO_NAME(FIO_STR_NAME, len)(&str) == 4,
      "Long String length changed during conversion from small string (%zu)!",
      FIO_NAME(FIO_STR_NAME, len)(&str));
  FIO_ASSERT(FIO_NAME2(FIO_STR_NAME, ptr)(&str) == str.buf,
             "Long String pointer reporting error after capacity update!");
  FIO_ASSERT(strlen(FIO_NAME2(FIO_STR_NAME, ptr)(&str)) == 4,
             "Long String NUL missing after capacity update (%zu)!",
             strlen(FIO_NAME2(FIO_STR_NAME, ptr)(&str)));
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Worl"),
             "Long String value changed after capacity update (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_NAME(FIO_STR_NAME, write)(&str, "d!", 2);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "World!"),
             "Long String `write` error (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_NAME(FIO_STR_NAME, replace)(&str, 0, 0, "Hello ", 6);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello World!"),
             "Long String `insert` error (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_NAME(FIO_STR_NAME, resize)(&str, 6);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello "),
             "Long String `resize` clipping error (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_NAME(FIO_STR_NAME, replace)(&str, 6, 0, "My World!", 9);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello My World!"),
             "Long String `replace` error when testing overflow (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)
  (&str, FIO_NAME(FIO_STR_NAME, len)(&str)); /* may truncate */

  FIO_NAME(FIO_STR_NAME, replace)(&str, -10, 2, "Big", 3);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello Big World!"),
             "Long String `replace` error when testing splicing (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) ==
                     FIO_STR_CAPA2WORDS(strlen("Hello Big World!")) ||
                 !FIO_NAME_BL(FIO_STR_NAME, allocated)(&str),
             "Long String `replace` capacity update error "
             "(%zu >=? %zu)!",
             FIO_NAME(FIO_STR_NAME, capa)(&str),
             FIO_STR_CAPA2WORDS(strlen("Hello Big World!")));

  if (FIO_NAME(FIO_STR_NAME, len)(&str) < (sizeof(str) - 2)) {
    FIO_NAME(FIO_STR_NAME, compact)(&str);
    FIO_ASSERT(FIO_STR_IS_SMALL(&str),
               "Compacting didn't change String to small!");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) == strlen("Hello Big World!"),
               "Compacting altered String length! (%zu != %zu)!",
               FIO_NAME(FIO_STR_NAME, len)(&str),
               strlen("Hello Big World!"));
    FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello Big World!"),
               "Compact data error (%s)!",
               FIO_NAME2(FIO_STR_NAME, ptr)(&str));
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) == sizeof(str) - 2,
               "Compacted String capacity reporting error!");
  } else {
    fprintf(stderr, "* skipped `compact` test (irrelevant for type).\n");
  }

  {
    FIO_NAME(FIO_STR_NAME, freeze)(&str);
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, frozen)(&str),
               "Frozen String not flagged as frozen.");
    fio_str_info_s old_state = FIO_NAME(FIO_STR_NAME, info)(&str);
    FIO_NAME(FIO_STR_NAME, write)(&str, "more data to be written here", 28);
    FIO_NAME(FIO_STR_NAME, replace)
    (&str, 2, 1, "more data to be written here", 28);
    fio_str_info_s new_state = FIO_NAME(FIO_STR_NAME, info)(&str);
    FIO_ASSERT(old_state.len == new_state.len, "Frozen String length changed!");
    FIO_ASSERT(old_state.buf == new_state.buf,
               "Frozen String pointer changed!");
    FIO_ASSERT(
        old_state.capa == new_state.capa,
        "Frozen String capacity changed (allowed, but shouldn't happen)!");
    FIO_STR_THAW_(&str);
  }
  FIO_NAME(FIO_STR_NAME, printf)(&str, " %u", 42);
  FIO_ASSERT(!strcmp(FIO_NAME2(FIO_STR_NAME, ptr)(&str), "Hello Big World! 42"),
             "`printf` data error (%s)!",
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));

  {
    FIO_NAME(FIO_STR_NAME, s) str2 = FIO_STR_INIT;
    FIO_NAME(FIO_STR_NAME, concat)(&str2, &str);
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, eq)(&str, &str2),
               "`concat` error, strings not equal (%s != %s)!",
               FIO_NAME2(FIO_STR_NAME, ptr)(&str),
               FIO_NAME2(FIO_STR_NAME, ptr)(&str2));
    FIO_NAME(FIO_STR_NAME, write)(&str2, ":extra data", 11);
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, eq)(&str, &str2),
               "`write` error after copy, strings equal "
               "((%zu)%s == (%zu)%s)!",
               FIO_NAME(FIO_STR_NAME, len)(&str),
               FIO_NAME2(FIO_STR_NAME, ptr)(&str),
               FIO_NAME(FIO_STR_NAME, len)(&str2),
               FIO_NAME2(FIO_STR_NAME, ptr)(&str2));

    FIO_NAME(FIO_STR_NAME, destroy)(&str2);
  }

  FIO_NAME(FIO_STR_NAME, destroy)(&str);

  FIO_NAME(FIO_STR_NAME, write_i)(&str, -42);
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) == 3 &&
                 !memcmp("-42", FIO_NAME2(FIO_STR_NAME, ptr)(&str), 3),
             "write_i output error ((%zu) %s != -42)",
             FIO_NAME(FIO_STR_NAME, len)(&str),
             FIO_NAME2(FIO_STR_NAME, ptr)(&str));
  FIO_NAME(FIO_STR_NAME, destroy)(&str);

  {
    fprintf(stderr, "* Testing string `readfile`.\n");
    FIO_NAME(FIO_STR_NAME, s) *s = FIO_NAME(FIO_STR_NAME, new)();
    FIO_ASSERT(FIO_PTR_UNTAG(s),
               "error, string not allocated (%p)!",
               (void *)s);
    fio_str_info_s state = FIO_NAME(FIO_STR_NAME, readfile)(s, __FILE__, 0, 0);

    FIO_ASSERT(state.len && state.buf,
               "error, no data was read for file %s!",
               __FILE__);

    FIO_ASSERT(!memcmp(state.buf,
                       "/* "
                       "******************************************************"
                       "***********************",
                       80),
               "content error, header mismatch!\n %s",
               state.buf);
    fprintf(stderr, "* Testing UTF-8 validation and length.\n");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_valid)(s),
               "`utf8_valid` error, code in this file "
               "should be valid!");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_len)(s) &&
                   (FIO_NAME(FIO_STR_NAME, utf8_len)(s) <=
                    FIO_NAME(FIO_STR_NAME, len)(s)) &&
                   (FIO_NAME(FIO_STR_NAME, utf8_len)(s) >=
                    (FIO_NAME(FIO_STR_NAME, len)(s)) >> 1),
               "`utf8_len` error, invalid value (%zu / %zu!",
               FIO_NAME(FIO_STR_NAME, utf8_len)(s),
               FIO_NAME(FIO_STR_NAME, len)(s));

    if (1) {
      /* String content == whole file (this file) */
      intptr_t pos = -11;
      size_t len = 20;
      fprintf(stderr, "* Testing UTF-8 positioning.\n");

      FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_select)(s, &pos, &len) == 0,
                 "`select` returned error for negative "
                 "pos! (%zd, %zu)",
                 (ssize_t)pos,
                 len);
      FIO_ASSERT(pos ==
                     (intptr_t)state.len - 10, /* no UTF-8 bytes in this file */
                 "`utf8_select` error, negative position "
                 "invalid! (%zd)",
                 (ssize_t)pos);
      FIO_ASSERT(len == 10,
                 "`utf8_select` error, trancated length "
                 "invalid! (%zd)",
                 (ssize_t)len);
      pos = 10;
      len = 20;
      FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_select)(s, &pos, &len) == 0,
                 "`utf8_select` returned error! (%zd, %zu)",
                 (ssize_t)pos,
                 len);
      FIO_ASSERT(pos == 10,
                 "`utf8_select` error, position invalid! (%zd)",
                 (ssize_t)pos);
      FIO_ASSERT(len == 20,
                 "`utf8_select` error, length invalid! (%zd)",
                 (ssize_t)len);
    }
    FIO_NAME(FIO_STR_NAME, free)(s);
  }
  FIO_NAME(FIO_STR_NAME, destroy)(&str);
  if (1) {
    /* Testing UTF-8 */
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    FIO_NAME(FIO_STR_NAME, write)(&str, utf8_sample, strlen(utf8_sample));
    intptr_t pos = -2;
    size_t len = 2;
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_select)(&str, &pos, &len) == 0,
               "`utf8_select` returned error for negative pos on "
               "UTF-8 data! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == (intptr_t)FIO_NAME(FIO_STR_NAME, len)(&str) -
                          4, /* 4 byte emoji */
               "`utf8_select` error, negative position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 4, /* last utf-8 char is 4 byte long */
               "`utf8_select` error, trancated length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 20;
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_select)(&str, &pos, &len) == 0,
               "`utf8_select` returned error on UTF-8 data! "
               "(%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == 4,
               "`utf8_select` error, position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 10,
               "`utf8_select` error, length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 3;
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, utf8_select)(&str, &pos, &len) == 0,
               "`utf8_select` returned error on UTF-8 data "
               "(2)! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(len ==
                   10, /* 3 UTF-8 chars: 4 byte + 4 byte + 2 byte codes == 10 */
               "`utf8_select` error, length invalid on UTF-8 data! "
               "(%zd)",
               (ssize_t)len);
  }
  FIO_NAME(FIO_STR_NAME, destroy)(&str);
  if (1) {
    /* Testing Static initialization and writing */
#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)(&str, "Welcome", 7);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC("Welcome");
#endif
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) == 0 ||
                   FIO_STR_IS_SMALL(&str),
               "Static string capacity non-zero.");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) > 0,
               "Static string length should be automatically calculated.");
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, allocated)(&str),
               "Static strings shouldn't be dynamic.");
    FIO_NAME(FIO_STR_NAME, destroy)(&str);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)
    (&str,
     "Welcome to a very long static string that should not fit within a "
     "containing struct... hopefuly",
     95);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC(
        "Welcome to a very long static string that should not fit within a "
        "containing struct... hopefuly");
#endif
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(&str) == 0 ||
                   FIO_STR_IS_SMALL(&str),
               "Static string capacity non-zero.");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) > 0,
               "Static string length should be automatically calculated.");
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, allocated)(&str),
               "Static strings shouldn't be dynamic.");
    FIO_NAME(FIO_STR_NAME, destroy)(&str);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)(&str, "Welcome", 7);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC("Welcome");
#endif
    fio_str_info_s state = FIO_NAME(FIO_STR_NAME, write)(&str, " Home", 5);
    FIO_ASSERT(state.capa > 0, "Static string not converted to non-static.");
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, allocated)(&str) ||
                   FIO_STR_IS_SMALL(&str),
               "String should be dynamic after `write`.");

    char *cstr = FIO_NAME(FIO_STR_NAME, detach)(&str);
    FIO_ASSERT(cstr, "`detach` returned NULL");
    FIO_ASSERT(!memcmp(cstr, "Welcome Home\0", 13),
               "`detach` string error: %s",
               cstr);
    FIO_MEM_FREE(cstr, state.capa);
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&str) == 0,
               "`detach` data wasn't cleared.");
    FIO_NAME(FIO_STR_NAME, destroy)
    (&str); /* does nothing, but what the heck... */
  }
  {
    fprintf(stderr, "* Testing Base64 encoding / decoding.\n");
    FIO_NAME(FIO_STR_NAME, destroy)(&str); /* does nothing, but why not... */

    FIO_NAME(FIO_STR_NAME, s) b64message = FIO_STR_INIT;
    fio_str_info_s b64i = FIO_NAME(
        FIO_STR_NAME,
        write)(&b64message, "Hello World, this is the voice of peace:)", 41);
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      b64i = FIO_NAME(FIO_STR_NAME, write)(&b64message, &c, 1);
      FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(&b64message) == (size_t)(42 + i),
                 "Base64 message length error (%zu != %zu)",
                 FIO_NAME(FIO_STR_NAME, len)(&b64message),
                 (size_t)(42 + i));
      FIO_ASSERT(FIO_NAME2(FIO_STR_NAME, ptr)(&b64message)[41 + i] == (char)c,
                 "Base64 message data error");
    }
    fio_str_info_s encoded =
        FIO_NAME(FIO_STR_NAME, write_base64enc)(&str, b64i.buf, b64i.len, 1);
    /* prevent encoded data from being deallocated during unencoding */
    encoded = FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)(
        &str,
        encoded.len + ((encoded.len >> 2) * 3) + 8);
    fio_str_info_s decoded;
    {
      FIO_NAME(FIO_STR_NAME, s) tmps;
      FIO_NAME(FIO_STR_NAME, init_copy2)(&tmps, &str);
      decoded = FIO_NAME(FIO_STR_NAME,
                         write_base64dec)(&str,
                                          FIO_NAME2(FIO_STR_NAME, ptr)(&tmps),
                                          FIO_NAME(FIO_STR_NAME, len)(&tmps));
      FIO_NAME(FIO_STR_NAME, destroy)(&tmps);
      encoded.buf = decoded.buf;
    }
    FIO_ASSERT(encoded.len, "Base64 encoding failed");
    FIO_ASSERT(decoded.len > encoded.len,
               "Base64 decoding failed:\n%s",
               encoded.buf);
    FIO_ASSERT(b64i.len == decoded.len - encoded.len,
               "Base 64 roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
               b64i.len,
               decoded.len - encoded.len,
               decoded.len,
               encoded.len,
               decoded.buf);

    FIO_ASSERT(!memcmp(b64i.buf, decoded.buf + encoded.len, b64i.len),
               "Base 64 roundtrip failed:\n %s",
               decoded.buf);
    FIO_NAME(FIO_STR_NAME, destroy)(&b64message);
    FIO_NAME(FIO_STR_NAME, destroy)(&str);
  }
  {
    fprintf(stderr, "* Testing JSON style character escaping / unescaping.\n");
    FIO_NAME(FIO_STR_NAME, s) unescaped = FIO_STR_INIT;
    fio_str_info_s ue;
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    FIO_NAME(FIO_STR_NAME, write)(&unescaped, utf8_sample, strlen(utf8_sample));
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      ue = FIO_NAME(FIO_STR_NAME, write)(&unescaped, &c, 1);
    }
    fio_str_info_s encoded =
        FIO_NAME(FIO_STR_NAME, write_escape)(&str, ue.buf, ue.len);
    // fprintf(stderr, "* %s\n", encoded.buf);
    fio_str_info_s decoded;
    {
      FIO_NAME(FIO_STR_NAME, s) tmps;
      FIO_NAME(FIO_STR_NAME, init_copy2)(&tmps, &str);
      decoded = FIO_NAME(FIO_STR_NAME,
                         write_unescape)(&str,
                                         FIO_NAME2(FIO_STR_NAME, ptr)(&tmps),
                                         FIO_NAME(FIO_STR_NAME, len)(&tmps));
      FIO_NAME(FIO_STR_NAME, destroy)(&tmps);
      encoded.buf = decoded.buf;
    }
    FIO_ASSERT(!memcmp(encoded.buf, utf8_sample, strlen(utf8_sample)),
               "valid UTF-8 data shouldn't be escaped:\n%.*s\n%s",
               (int)encoded.len,
               encoded.buf,
               decoded.buf);
    FIO_ASSERT(encoded.len, "JSON encoding failed");
    FIO_ASSERT(decoded.len > encoded.len,
               "JSON decoding failed:\n%s",
               encoded.buf);
    FIO_ASSERT(ue.len == decoded.len - encoded.len,
               "JSON roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
               ue.len,
               decoded.len - encoded.len,
               decoded.len,
               encoded.len,
               decoded.buf);

    FIO_ASSERT(!memcmp(ue.buf, decoded.buf + encoded.len, ue.len),
               "JSON roundtrip failed:\n %s",
               decoded.buf);
    FIO_NAME(FIO_STR_NAME, destroy)(&unescaped);
    FIO_NAME(FIO_STR_NAME, destroy)(&str);
  }
}
#undef FIO__STR_SMALL_CAPA
#undef FIO_STR_WRITE_TEST_FUNC
#endif /* FIO_STR_WRITE_TEST_FUNC */

/* *****************************************************************************
String Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_STR_SMALL
#undef FIO_STR_SMALL_CAPA
#undef FIO_STR_SMALL_DATA
#undef FIO_STR_SMALL_LEN
#undef FIO_STR_SMALL_LEN_SET

#undef FIO_STR_BIG_CAPA
#undef FIO_STR_BIG_CAPA_SET
#undef FIO_STR_BIG_DATA
#undef FIO_STR_BIG_FREE_BUF
#undef FIO_STR_BIG_IS_DYNAMIC
#undef FIO_STR_BIG_LEN
#undef FIO_STR_BIG_LEN_SET
#undef FIO_STR_BIG_SET_STATIC

#undef FIO_STR_CAPA2WORDS
#undef FIO_STR_FREEZE_

#undef FIO_STR_IS_FROZEN
#undef FIO_STR_IS_SMALL
#undef FIO_STR_NAME

#undef FIO_STR_OPTIMIZE4IMMUTABILITY
#undef FIO_STR_OPTIMIZE_EMBEDDED
#undef FIO_STR_PTR
#undef FIO_STR_THAW_
#undef FIO_STR_WRITE_ESCAPED_CT_OR
#undef FIO_STR_RESERVE_NAME

#endif /* FIO_STR_NAME */
