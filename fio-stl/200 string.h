/* ************************************************************************** */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_STR_NAME fio       /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        Dynamic Strings (binary safe)



Copyright and License: see header file (000 copyright.h) or top of file
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
 * without additional memory allocation.
 *
 * For example, when using a reference counter wrapper on a 64bit system, it
 * would make sense to set this value to 1 - allowing the type size to fully
 * utilize a 16 byte memory allocation alignment.
 */
#define FIO_STR_OPTIMIZE_EMBEDDED 0
#endif

#ifndef FIO_STR_OPTIMIZE4IMMUTABILITY
/**
 * Minimizes the struct size, storing only string length and pointer.
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
/* enforce limit due to 6 bit embedded string length limit (assumes 64 bit) */
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
  {                                                                            \
    .special = 4, .capa = FIO_STRLEN((buffer)), .len = FIO_STRLEN((buffer)),   \
    .buf = (char *)(buffer)                                                    \
  }

/**
 * This macro allows the container to be initialized with existing static data,
 * that shouldn't be freed.
 *
 * NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
 * FIO_STR_OPTIMIZE4IMMUTABILITY optimization)
 */
#define FIO_STR_INIT_STATIC2(buffer, length)                                   \
  { .special = 4, .capa = (length), .len = (length), .buf = (char *)(buffer) }

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
 * Frees the String's resources and re-initializes the container.
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
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, dealloc)(void *ptr);

/* *****************************************************************************
String API - String state (data pointers, length, capacity, etc')
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, info)(const FIO_STR_PTR s);

/** Returns the String's partial state (length and pointer).  */
FIO_IFUNC fio_buf_info_s FIO_NAME(FIO_STR_NAME, buf)(const FIO_STR_PTR s);

/** Returns a pointer (`char *`) to the String's content. */
FIO_IFUNC char *FIO_NAME(FIO_STR_NAME, ptr)(FIO_STR_PTR s);

/** Returns the String's length in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, len)(FIO_STR_PTR s);

/** Returns the String's existing capacity (total used & available memory). */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, capa)(FIO_STR_PTR s);

/** Prevents further manipulations to the String's content. */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, freeze)(FIO_STR_PTR s);

/** Returns true if the string is frozen. */
FIO_IFUNC uint8_t FIO_NAME_BL(FIO_STR_NAME, frozen)(FIO_STR_PTR s);

/** Returns 1 if memory was allocated (and the String must be destroyed). */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, allocated)(const FIO_STR_PTR s);

/** Binary comparison returns `1` if both strings are equal and `0` if not. */
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
 * Note: When optimized for immutability (`FIO_STR_SMALL`), this may corrupt the
 * string length data.
 *
 * Make sure to call `resize` with the updated information once the editing is
 * done.
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

/** Writes data at the end of the String. See `fio_string_write2`. */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              __write2)(FIO_STR_PTR s,
                                        const fio_string_write_s srcs[]);

#ifndef FIO_STR_WRITE2
#define FIO_STR_WRITE2(str_name, dest, ...)                                    \
  FIO_NAME(str_name, __write2)(dest, (fio_string_write_s[]){__VA_ARGS__, {0}})
#endif
/* *****************************************************************************
String API - Numerals
***************************************************************************** */

/** Writes a number at the end of the String using normal base 10 notation. */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_i)(FIO_STR_PTR s,
                                                     int64_t num);

/** Writes a number at the end of the String using Hex (base 16) notation. */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_hex)(FIO_STR_PTR s,
                                                       int64_t num);

/* Writes a binary representation of `i` to the String */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_bin)(FIO_STR_PTR s,
                                                       int64_t num);

/* *****************************************************************************
String API - printf style support
***************************************************************************** */

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
String API - HTML escaping support
***************************************************************************** */

/** Writes HTML escaped data to a String. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_html_escape)(FIO_STR_PTR s,
                                                               const void *raw,
                                                               size_t len);

/** Writes HTML un-escaped data to a String - incomplete and minimal. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_html_unescape)(FIO_STR_PTR s,
                                                   const void *escaped,
                                                   size_t len);

/* *****************************************************************************
String API - writing data from files to the String
***************************************************************************** */

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

/* used here, but declared later (reference counter is static / global). */

SFUNC FIO_NAME(FIO_STR_NAME, s) * FIO_NAME(FIO_STR_NAME, __object_new)(void);
SFUNC void FIO_NAME(FIO_STR_NAME, __object_free)(FIO_NAME(FIO_STR_NAME, s) * s);
SFUNC int FIO_NAME(FIO_STR_NAME, __default_reallocate)(fio_str_info_s *dest,
                                                       size_t new_capa);
SFUNC int FIO_NAME(FIO_STR_NAME,
                   __default_copy_and_reallocate)(fio_str_info_s *dest,
                                                  size_t new_capa);
SFUNC void FIO_NAME(FIO_STR_NAME, __default_free)(void *ptr, size_t capa);

/* *****************************************************************************
String Macro Helpers
***************************************************************************** */

#define FIO_STR_IS_SMALL(s)  ((((s)->special & 1) | !(s)->buf))
#define FIO_STR_SMALL_LEN(s) ((size_t)((s)->special >> 2))
#define FIO_STR_SMALL_LEN_SET(s, l)                                            \
  ((s)->special = (((s)->special & 2) | ((uint8_t)(l) << 2) | 1))
#define FIO_STR_SMALL_CAPA(s) ((sizeof(*(s)) - 2) & 63)
#define FIO_STR_SMALL_DATA(s) ((char *)((s)->reserved))

#define FIO_STR_BIG_DATA(s)       ((s)->buf)
#define FIO_STR_BIG_IS_DYNAMIC(s) (!((s)->special & 4))
#define FIO_STR_BIG_SET_STATIC(s) ((s)->special |= 4)
#define FIO_STR_BIG_FREE_BUF(s)                                                \
  (FIO_NAME(FIO_STR_NAME, __default_free)((s)->buf, FIO_STR_BIG_CAPA((s))))

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
        (s)->reserved[0] = (l)&0xFF;                                           \
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
#define FIO_STR_BIG_CAPA(s) fio_string_capa4len(FIO_STR_BIG_LEN((s)))
#define FIO_STR_BIG_CAPA_SET(s, capa)
#else
#define FIO_STR_BIG_LEN(s)            ((s)->len)
#define FIO_STR_BIG_LEN_SET(s, l)     ((s)->len = (l))
#define FIO_STR_BIG_CAPA(s)           ((s)->capa)
#define FIO_STR_BIG_CAPA_SET(s, capa) (FIO_STR_BIG_CAPA(s) = (capa))
#endif

/* *****************************************************************************
String Information Round-tripping
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, info)(const FIO_STR_PTR s_) {
  fio_str_info_s r = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(s_, r);
  FIO_NAME(FIO_STR_NAME, s) *s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  if (FIO_STR_IS_SMALL(s))
    r = (fio_str_info_s){
        .buf = FIO_STR_SMALL_DATA(s),
        .len = FIO_STR_SMALL_LEN(s),
        .capa = FIO_STR_SMALL_CAPA(s),
    };
  else
    r = (fio_str_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = FIO_STR_BIG_LEN(s),
        .capa = FIO_STR_BIG_CAPA(s),
    };
  r.capa &= ((size_t)0ULL - (!FIO_STR_IS_FROZEN(s)));
  return r;
}

/** Returns the String's partial state (length and pointer).  */
FIO_IFUNC fio_buf_info_s FIO_NAME(FIO_STR_NAME, buf)(const FIO_STR_PTR s_) {
  fio_buf_info_s r = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(s_, r);
  FIO_NAME(FIO_STR_NAME, s) *s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  if (FIO_STR_IS_SMALL(s))
    r = (fio_buf_info_s){
        .buf = FIO_STR_SMALL_DATA(s),
        .len = FIO_STR_SMALL_LEN(s),
    };
  else
    r = (fio_buf_info_s){
        .buf = FIO_STR_BIG_DATA(s),
        .len = FIO_STR_BIG_LEN(s),
    };
  return r;
}

/* Internal(!): updated String data according to `info`.  */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, __info_update)(const FIO_STR_PTR s_,
                                                     fio_str_info_s info) {
  /* internally used function, tagging already validated. */
  FIO_NAME(FIO_STR_NAME, s) *s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  if (info.buf == FIO_STR_SMALL_DATA(s)) {
    s->special |= 1;
    FIO_STR_SMALL_LEN_SET(s, info.len);
    return;
  }
  s->special = 0;
  FIO_STR_BIG_LEN_SET(s, info.len);
  FIO_STR_BIG_CAPA_SET(s, info.capa);
  s->buf = info.buf;
}

/* Internal(!): updated String data according to `info`.  */
FIO_IFUNC fio_string_realloc_fn FIO_NAME(FIO_STR_NAME,
                                         __realloc_func)(const FIO_STR_PTR s_) {
  fio_string_realloc_fn options[] = {
      FIO_NAME(FIO_STR_NAME, __default_reallocate),
      FIO_NAME(FIO_STR_NAME, __default_copy_and_reallocate),
  };
  /* internally used function, tagging already validated. */
  FIO_NAME(FIO_STR_NAME, s) *s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  return options[FIO_STR_IS_SMALL(s) | !FIO_STR_BIG_IS_DYNAMIC(s)];
}

/* *****************************************************************************
String Constructors (inline)
***************************************************************************** */
#ifndef FIO_REF_CONSTRUCTOR_ONLY

/** Allocates a new String object on the heap. */
FIO_IFUNC FIO_STR_PTR FIO_NAME(FIO_STR_NAME, new)(void) {
  FIO_NAME(FIO_STR_NAME, s) *const s = FIO_NAME(FIO_STR_NAME, __object_new)();
  if (!FIO_MEM_REALLOC_IS_SAFE_ && s) {
    *s = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT;
  }
#ifdef DEBUG
  {
    FIO_NAME(FIO_STR_NAME, s) tmp = {0};
    FIO_ASSERT(!FIO_MEMCMP(&tmp, s, sizeof(tmp)),
               "new " FIO_MACRO2STR(
                   FIO_NAME(FIO_STR_NAME, s)) " object not initialized!");
  }
#endif
  return (FIO_STR_PTR)FIO_PTR_TAG(s);
}

/** Destroys the string and frees the container (if allocated with `new`). */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, free)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(s_);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  if (!FIO_STR_IS_SMALL(s) && FIO_STR_BIG_IS_DYNAMIC(s)) {
    FIO_STR_BIG_FREE_BUF(s);
  }
  FIO_NAME(FIO_STR_NAME, __object_free)(s);
}

#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/**
 * Frees the String's resources and reinitializes the container.
 *
 * Note: if the container isn't allocated on the stack, it should be freed
 * separately using the appropriate `free` function.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, destroy)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(s_);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
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
  FIO_PTR_TAG_VALID_OR_RETURN(s_, data);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);

  if (FIO_STR_IS_SMALL(s)) {
    if (FIO_STR_SMALL_LEN(s)) { /* keep these ifs apart */
      fio_str_info_s cpy = {.buf = FIO_STR_SMALL_DATA(s),
                            .len = FIO_STR_SMALL_LEN(s)};
      FIO_NAME(FIO_STR_NAME, __default_copy_and_reallocate)(&cpy, cpy.len);
      data = cpy.buf;
    }
  } else {
    if (FIO_STR_BIG_IS_DYNAMIC(s)) {
      data = FIO_STR_BIG_DATA(s);
    } else if (FIO_STR_BIG_LEN(s)) {
      fio_str_info_s cpy = {.buf = FIO_STR_BIG_DATA(s),
                            .len = FIO_STR_BIG_LEN(s)};
      FIO_NAME(FIO_STR_NAME, __default_copy_and_reallocate)(&cpy, cpy.len);
      data = cpy.buf;
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
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(s_);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  if (FIO_STR_IS_SMALL(s) || !FIO_STR_BIG_IS_DYNAMIC(s) ||
      fio_string_capa4len(FIO_NAME(FIO_STR_NAME, len)(s_)) >=
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
  FIO_PTR_TAG_VALID_OR_RETURN(s_, i);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
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
  FIO_STR_BIG_CAPA_SET(s, len);
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
  FIO_PTR_TAG_VALID_OR_RETURN(s_, i);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
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
  i = (fio_str_info_s){.buf = (char *)str, .len = len};
  FIO_NAME(FIO_STR_NAME, __default_copy_and_reallocate)(&i, len);
  FIO_STR_BIG_CAPA_SET(s, i.capa);
  FIO_STR_BIG_DATA(s) = i.buf;
  FIO_STR_BIG_LEN_SET(s, len);
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

/** Returns a pointer (`char *`) to the String's content. */
FIO_IFUNC char *FIO_NAME(FIO_STR_NAME, ptr)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, NULL);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  char *results[] = {(FIO_STR_BIG_DATA(s)), (FIO_STR_SMALL_DATA(s))};
  return results[FIO_STR_IS_SMALL(s)];
}

/** Returns the String's length in bytes. */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, len)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, 0);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  size_t results[] = {(FIO_STR_BIG_LEN(s)), (FIO_STR_SMALL_LEN(s))};
  return results[FIO_STR_IS_SMALL(s)];
}

/** Returns the String's existing capacity (total used & available memory). */
FIO_IFUNC size_t FIO_NAME(FIO_STR_NAME, capa)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, 0);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
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
  fio_str_info_s i = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(s_, i);
  i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa) {
    return i;
  }
  /* resize may be used to reserve memory in advance while setting size  */
  if (i.capa > size) {
    i.len = size;
    i.buf[i.len] = 0;
  } else {
    fio_string_write(&i,
                     FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                     NULL,
                     size - i.len);
  }
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);

  return i;
}

/**
 * Prevents further manipulations to the String's content.
 */
FIO_IFUNC void FIO_NAME(FIO_STR_NAME, freeze)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN_VOID(s_);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  FIO_STR_FREEZE_(s);
}

/**
 * Returns true if the string is frozen.
 */
FIO_IFUNC uint8_t FIO_NAME_BL(FIO_STR_NAME, frozen)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, 1);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  return FIO_STR_IS_FROZEN(s);
}

/** Returns 1 if memory was allocated and (the String must be destroyed). */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, allocated)(const FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, 0);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  return (!FIO_STR_IS_SMALL(s) & FIO_STR_BIG_IS_DYNAMIC(s));
}

/**
 * Binary comparison returns `1` if both strings are equal and `0` if not.
 */
FIO_IFUNC int FIO_NAME_BL(FIO_STR_NAME, eq)(const FIO_STR_PTR str1_,
                                            const FIO_STR_PTR str2_) {
  if (str1_ == str2_)
    return 1;
  FIO_PTR_TAG_VALID_OR_RETURN(str1_, 0);
  FIO_PTR_TAG_VALID_OR_RETURN(str2_, 0);
  fio_buf_info_s s1 = FIO_NAME(FIO_STR_NAME, buf)(str1_);
  fio_buf_info_s s2 = FIO_NAME(FIO_STR_NAME, buf)(str2_);
  return FIO_BUF_INFO_IS_EQ(s1, s2);
}

/**
 * Returns the string's Risky Hash value.
 *
 * Note: Hash algorithm might change without notice.
 */
FIO_IFUNC uint64_t FIO_NAME(FIO_STR_NAME, hash)(const FIO_STR_PTR s_,
                                                uint64_t seed) {
  fio_buf_info_s i = FIO_NAME(FIO_STR_NAME, buf)(s_);
  return fio_risky_hash((void *)i.buf, i.len, seed);
}

/* *****************************************************************************
String API - Content Manipulation and Review (inline)
***************************************************************************** */

/** Writes data at the end of the String. */
FIO_IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write)(FIO_STR_PTR s_,
                                                       const void *src,
                                                       size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write(&i, FIO_NAME(FIO_STR_NAME, __realloc_func)(s_), src, len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/* *****************************************************************************


                             String Implementation

                               IMPLEMENTATION


***************************************************************************** */

/* *****************************************************************************
External functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_STR_NAME, s))
FIO___LEAK_COUNTER_DEF(FIO_NAME(FIO_STR_NAME, destroy))

/* *****************************************************************************
String Core Callbacks - Memory management
***************************************************************************** */
SFUNC FIO_NAME(FIO_STR_NAME, s) * FIO_NAME(FIO_STR_NAME, __object_new)(void) {
  FIO_NAME(FIO_STR_NAME, s) *r =
      (FIO_NAME(FIO_STR_NAME, s) *)FIO_MEM_REALLOC_(NULL, 0, (sizeof(*r)), 0);
  if (r)
    FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_STR_NAME, s));
  return r;
}
SFUNC void FIO_NAME(FIO_STR_NAME,
                    __object_free)(FIO_NAME(FIO_STR_NAME, s) * s) {
  if (!s)
    return;
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_STR_NAME, s));
  FIO_MEM_FREE_(s, sizeof(*s));
}

SFUNC int FIO_NAME(FIO_STR_NAME, __default_reallocate)(fio_str_info_s *dest,
                                                       size_t new_capa) {
  new_capa = fio_string_capa4len(new_capa);
  void *tmp = FIO_MEM_REALLOC_(dest->buf, dest->capa, new_capa, dest->len);
  if (!tmp)
    return -1;
  if (!dest->buf)
    FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_STR_NAME, destroy));
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
  return 0;
}
SFUNC int FIO_NAME(FIO_STR_NAME,
                   __default_copy_and_reallocate)(fio_str_info_s *dest,
                                                  size_t new_capa) {
  if (dest->len && new_capa < dest->len)
    new_capa = dest->len;
  new_capa = fio_string_capa4len(new_capa);
  void *tmp = FIO_MEM_REALLOC_(NULL, 0, new_capa, 0);
  if (!tmp)
    return -1;
  FIO___LEAK_COUNTER_ON_ALLOC(FIO_NAME(FIO_STR_NAME, destroy));
  if (dest->len)
    FIO_MEMCPY(tmp, dest->buf, dest->len);
  ((char *)tmp)[dest->len] = 0;
  dest->capa = new_capa;
  dest->buf = (char *)tmp;
  return 0;
}
SFUNC void FIO_NAME(FIO_STR_NAME, __default_free)(void *ptr, size_t capa) {
  if (!ptr)
    return;
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_STR_NAME, destroy));
  FIO_MEM_FREE_(ptr, capa);
  (void)capa; /* if unused */
}
SFUNC void FIO_NAME(FIO_STR_NAME, __default_free_noop)(void *str) { (void)str; }
SFUNC void FIO_NAME(FIO_STR_NAME, __default_free_noop2)(fio_str_info_s str) {
  (void)str;
}

/* *****************************************************************************
String Implementation - Memory management
***************************************************************************** */

/** Frees the pointer returned by `detach`. */
SFUNC void FIO_NAME(FIO_STR_NAME, dealloc)(void *ptr) {
  if (!ptr)
    return;
  FIO___LEAK_COUNTER_ON_FREE(FIO_NAME(FIO_STR_NAME, destroy));
  FIO_MEM_FREE_(ptr, -1);
}

/**
 * Reserves at least `amount` of bytes for the string's data.
 *
 * Returns the current state of the String.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              FIO_STR_RESERVE_NAME)(FIO_STR_PTR s_,
                                                    size_t amount) {
  fio_str_info_s state = {0};
  FIO_PTR_TAG_VALID_OR_RETURN(s_, state);
  FIO_NAME(FIO_STR_NAME, s) *const s =
      FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s_);
  state = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (FIO_STR_IS_FROZEN(s))
    return state;
  amount += state.len;
  if (state.capa <= amount) {
    FIO_NAME(FIO_STR_NAME, __realloc_func)(s_)(&state, amount);
    state.buf[state.len] = 0;
    FIO_NAME(FIO_STR_NAME, __info_update)(s_, state);
  } else if (state.capa > FIO_STR_SMALL_CAPA(s) &&
             amount <= FIO_STR_SMALL_CAPA(s) &&
             state.len <= FIO_STR_SMALL_CAPA(s)) {
    FIO_NAME(FIO_STR_NAME, s) tmp;
    state = FIO_NAME(FIO_STR_NAME, init_copy)((FIO_STR_PTR)FIO_PTR_TAG(&tmp),
                                              state.buf,
                                              state.len);
    FIO_NAME(FIO_STR_NAME, destroy)(s_);
    *s = tmp;
  }
  return state;
}

/* *****************************************************************************
String Implementation - UTF-8 State
***************************************************************************** */

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_valid)(FIO_STR_PTR s_) {
  FIO_PTR_TAG_VALID_OR_RETURN(s_, 0);
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  return fio_string_utf8_len(state);
}

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t FIO_NAME(FIO_STR_NAME, utf8_len)(FIO_STR_PTR s_) {
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  return fio_string_utf8_len(state);
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
  FIO_PTR_TAG_VALID_OR_RETURN(s_, -1);
  fio_str_info_s state = FIO_NAME(FIO_STR_NAME, info)(s_);
  return fio_string_utf8_select(state, pos, len);
}

/* *****************************************************************************
String Implementation - Content Manipulation and Review
***************************************************************************** */

/**
 * Writes a number at the end of the String using normal base 10 notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_i)(FIO_STR_PTR s_,
                                                     int64_t num) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_i(&i, FIO_NAME(FIO_STR_NAME, __realloc_func)(s_), num);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes a number at the end of the String using Hex (base 16) notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_hex)(FIO_STR_PTR s_,
                                                       int64_t num) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_hex(&i, FIO_NAME(FIO_STR_NAME, __realloc_func)(s_), num);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes a number at the end of the String using binary notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_bin)(FIO_STR_PTR s_,
                                                       int64_t num) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_bin(&i, FIO_NAME(FIO_STR_NAME, __realloc_func)(s_), num);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Appends the `src` String to the end of the `dest` String.
 *
 * If `dest` is empty, the resulting Strings will be equal.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, concat)(FIO_STR_PTR dest_,
                                                    FIO_STR_PTR const src_) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(dest_);
  if (!i.capa)
    return i;
  FIO_PTR_TAG_VALID_OR_RETURN(src_, i);
  fio_str_info_s src = FIO_NAME(FIO_STR_NAME, info)(src_);
  if (!src.len)
    return i;
  fio_string_write(&i,
                   FIO_NAME(FIO_STR_NAME, __realloc_func)(dest_),
                   src.buf,
                   src.len);
  FIO_NAME(FIO_STR_NAME, __info_update)(dest_, i);
  return i;
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
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_replace(&i,
                     FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                     start_pos,
                     old_len,
                     src,
                     src_len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes a number at the end of the String using binary notation.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              __write2)(FIO_STR_PTR s_,
                                        const fio_string_write_s srcs[]) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write2 FIO_NOOP(&i,
                             FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                             srcs);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes to the String using a vprintf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s FIO___PRINTF_STYLE(2, 0)
    FIO_NAME(FIO_STR_NAME,
             vprintf)(FIO_STR_PTR s_, const char *format, va_list argv) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_vprintf(&i,
                     FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                     format,
                     argv);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes to the String using a printf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_str_info_s FIO___PRINTF_STYLE(2, 3)
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

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_escape)(FIO_STR_PTR s_,
                                                          const void *src,
                                                          size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_escape(&i,
                          FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                          src,
                          len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes an escaped data into the string after unescaping the data.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_unescape)(FIO_STR_PTR s_,
                                                            const void *src,
                                                            size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_unescape(&i,
                            FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                            src,
                            len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

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
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_base64enc(&i,
                             FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                             data,
                             len,
                             url_encoded);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Writes decoded base64 data to the end of the String.
 */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_base64dec)(FIO_STR_PTR s_,
                                               const void *encoded_,
                                               size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_base64dec(&i,
                             FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                             encoded_,
                             len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/* *****************************************************************************
String API - HTML escaping support
***************************************************************************** */

/** Writes HTML escaped data to a String. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, write_html_escape)(FIO_STR_PTR s_,
                                                               const void *data,
                                                               size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_html_escape(&i,
                               FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                               data,
                               len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/** Writes HTML un-escaped data to a String - incomplete and minimal. */
IFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME,
                              write_html_unescape)(FIO_STR_PTR s_,
                                                   const void *data,
                                                   size_t len) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_write_html_unescape(&i,
                                 FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                                 data,
                                 len);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/* *****************************************************************************
String - read file
***************************************************************************** */

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
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfd)(FIO_STR_PTR s_,
                                                    int fd,
                                                    intptr_t start_at,
                                                    intptr_t limit) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_readfd(&i,
                    FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                    fd,
                    start_at,
                    limit);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 */
SFUNC fio_str_info_s FIO_NAME(FIO_STR_NAME, readfile)(FIO_STR_PTR s_,
                                                      const char *filename,
                                                      intptr_t start_at,
                                                      intptr_t limit) {
  fio_str_info_s i = FIO_NAME(FIO_STR_NAME, info)(s_);
  if (!i.capa)
    return i;
  fio_string_readfile(&i,
                      FIO_NAME(FIO_STR_NAME, __realloc_func)(s_),
                      filename,
                      start_at,
                      limit);
  FIO_NAME(FIO_STR_NAME, __info_update)(s_, i);
  return i;
}

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
  FIO_STR_PTR pstr = FIO_PTR_TAG((&str));
  fprintf(
      stderr,
      "* Testing core string features for " FIO_MACRO2STR(FIO_STR_NAME) ".\n");
  fprintf(stderr,
          "* String container size (without wrapper): %zu\n",
          sizeof(FIO_NAME(FIO_STR_NAME, s)));
  fprintf(stderr,
          "* Self-contained capacity (FIO_STR_SMALL_CAPA): %zu\n",
          FIO__STR_SMALL_CAPA);
  FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, frozen)(pstr), "new string is frozen");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) == FIO__STR_SMALL_CAPA,
             "small string capacity returned %zu",
             FIO_NAME(FIO_STR_NAME, capa)(pstr));
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) == 0,
             "small string length reporting error!");
  FIO_ASSERT(
      FIO_NAME(FIO_STR_NAME, ptr)(pstr) == ((char *)(&str) + 1),
      "small string pointer reporting error (%zd offset)!",
      (ssize_t)(((char *)(&str) + 1) - FIO_NAME(FIO_STR_NAME, ptr)(pstr)));
  FIO_NAME(FIO_STR_NAME, write)(pstr, "World", 4);
  FIO_ASSERT(FIO_STR_IS_SMALL(&str),
             "small string writing error - not small on small write!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) == FIO__STR_SMALL_CAPA,
             "Small string capacity reporting error after write!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) == 4,
             "small string length reporting error after write!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, ptr)(pstr) == (char *)&str + 1,
             "small string pointer reporting error after write!");
  FIO_ASSERT(!FIO_NAME(FIO_STR_NAME, ptr)(pstr)[4] &&
                 FIO_STRLEN(FIO_NAME(FIO_STR_NAME, ptr)(pstr)) == 4,
             "small string NUL missing after write (%zu)!",
             FIO_STRLEN(FIO_NAME(FIO_STR_NAME, ptr)(pstr)));
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Worl"),
             "small string write error (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, ptr)(pstr) ==
                 FIO_NAME(FIO_STR_NAME, info)(pstr).buf,
             "small string `data` != `info.buf` (%p != %p)",
             (void *)FIO_NAME(FIO_STR_NAME, ptr)(pstr),
             (void *)FIO_NAME(FIO_STR_NAME, info)(pstr).buf);

  FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)
  (pstr, sizeof(FIO_NAME(FIO_STR_NAME, s)));
  FIO_ASSERT(!FIO_STR_IS_SMALL(&str),
             "Long String reporting as small after capacity update!");
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) >=
                 sizeof(FIO_NAME(FIO_STR_NAME, s)) - 1,
             "Long String capacity update error (%zu != %zu)!",
             FIO_NAME(FIO_STR_NAME, capa)(pstr),
             FIO_STR_SMALL_CAPA(&str));

  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, ptr)(pstr) ==
                 FIO_NAME(FIO_STR_NAME, info)(pstr).buf,
             "Long String `ptr` !>= "
             "`cstr(s).buf` (%p != %p)",
             (void *)FIO_NAME(FIO_STR_NAME, ptr)(pstr),
             (void *)FIO_NAME(FIO_STR_NAME, info)(pstr).buf);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
  /* immutable string length is updated after `reserve` to reflect new capa */
  FIO_NAME(FIO_STR_NAME, resize)(pstr, 4);
#endif
  FIO_ASSERT(
      FIO_NAME(FIO_STR_NAME, len)(pstr) == 4,
      "Long String length changed during conversion from small string (%zu)!",
      FIO_NAME(FIO_STR_NAME, len)(pstr));
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, ptr)(pstr) == str.buf,
             "Long String pointer reporting error after capacity update!");
  FIO_ASSERT(FIO_STRLEN(FIO_NAME(FIO_STR_NAME, ptr)(pstr)) == 4,
             "Long String NUL missing after capacity update (%zu)!",
             FIO_STRLEN(FIO_NAME(FIO_STR_NAME, ptr)(pstr)));
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Worl"),
             "Long String value changed after capacity update (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_NAME(FIO_STR_NAME, write)(pstr, "d!", 2);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "World!"),
             "Long String `write` error (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_NAME(FIO_STR_NAME, replace)(pstr, 0, 0, "Hello ", 6);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello World!"),
             "Long String `insert` error (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_NAME(FIO_STR_NAME, resize)(pstr, 6);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello "),
             "Long String `resize` clipping error (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_NAME(FIO_STR_NAME, replace)(pstr, 6, 0, "My World!", 9);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello My World!"),
             "Long String `replace` error when testing overflow (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)
  (pstr, FIO_NAME(FIO_STR_NAME, len)(pstr)); /* may truncate */

  FIO_NAME(FIO_STR_NAME, replace)(pstr, -10, 2, "Big", 3);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello Big World!"),
             "Long String `replace` error when testing splicing (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) ==
                     fio_string_capa4len(FIO_STRLEN("Hello Big World!")) ||
                 !FIO_NAME_BL(FIO_STR_NAME, allocated)(pstr),
             "Long String `replace` capacity update error "
             "(%zu >=? %zu)!",
             FIO_NAME(FIO_STR_NAME, capa)(pstr),
             fio_string_capa4len(FIO_STRLEN("Hello Big World!")));

  if (FIO_NAME(FIO_STR_NAME, len)(pstr) < (sizeof(str) - 2)) {
    FIO_NAME(FIO_STR_NAME, compact)(pstr);
    FIO_ASSERT(FIO_STR_IS_SMALL(&str),
               "Compacting didn't change String to small!");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) ==
                   FIO_STRLEN("Hello Big World!"),
               "Compacting altered String length! (%zu != %zu)!",
               FIO_NAME(FIO_STR_NAME, len)(pstr),
               FIO_STRLEN("Hello Big World!"));
    FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello Big World!"),
               "Compact data error (%s)!",
               FIO_NAME(FIO_STR_NAME, ptr)(pstr));
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) == sizeof(str) - 2,
               "Compacted String capacity reporting error!");
  } else {
    FIO_LOG_DEBUG2("* Skipped `compact` test (irrelevant for type).");
  }

  {
    FIO_NAME(FIO_STR_NAME, freeze)(pstr);
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, frozen)(pstr),
               "Frozen String not flagged as frozen.");
    fio_str_info_s old_state = FIO_NAME(FIO_STR_NAME, info)(pstr);
    FIO_NAME(FIO_STR_NAME, write)(pstr, "more data to be written here", 28);
    FIO_NAME(FIO_STR_NAME, replace)
    (pstr, 2, 1, "more data to be written here", 28);
    fio_str_info_s new_state = FIO_NAME(FIO_STR_NAME, info)(pstr);
    FIO_ASSERT(old_state.len == new_state.len, "Frozen String length changed!");
    FIO_ASSERT(old_state.buf == new_state.buf,
               "Frozen String pointer changed!");
    FIO_ASSERT(
        old_state.capa == new_state.capa,
        "Frozen String capacity changed (allowed, but shouldn't happen)!");
    FIO_STR_THAW_(&str);
  }
  FIO_NAME(FIO_STR_NAME, printf)(pstr, " %u", 42);
  FIO_ASSERT(!strcmp(FIO_NAME(FIO_STR_NAME, ptr)(pstr), "Hello Big World! 42"),
             "`printf` data error (%s)!",
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));

  {
    FIO_NAME(FIO_STR_NAME, s) str2 = FIO_STR_INIT;
    FIO_STR_PTR pstr2 = FIO_PTR_TAG(&str2);
    FIO_NAME(FIO_STR_NAME, concat)(pstr2, pstr);
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, eq)(pstr, pstr2),
               "`concat` error, strings not equal (%s != %s)!",
               FIO_NAME(FIO_STR_NAME, ptr)(pstr),
               FIO_NAME(FIO_STR_NAME, ptr)(pstr2));
    FIO_NAME(FIO_STR_NAME, write)(pstr2, ":extra data", 11);
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, eq)(pstr, pstr2),
               "`write` error after copy, strings equal "
               "((%zu)%s == (%zu)%s)!",
               FIO_NAME(FIO_STR_NAME, len)(pstr),
               FIO_NAME(FIO_STR_NAME, ptr)(pstr),
               FIO_NAME(FIO_STR_NAME, len)(pstr2),
               FIO_NAME(FIO_STR_NAME, ptr)(pstr2));

    FIO_NAME(FIO_STR_NAME, destroy)(pstr2);
  }

  FIO_NAME(FIO_STR_NAME, destroy)(pstr);

  FIO_NAME(FIO_STR_NAME, write_i)(pstr, -42);
  FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) == 3 &&
                 !memcmp("-42", FIO_NAME(FIO_STR_NAME, ptr)(pstr), 3),
             "write_i output error ((%zu) %s != -42)",
             FIO_NAME(FIO_STR_NAME, len)(pstr),
             FIO_NAME(FIO_STR_NAME, ptr)(pstr));
  FIO_NAME(FIO_STR_NAME, destroy)(pstr);
  {
    fprintf(stderr, "* Testing string `readfile`.\n");
    FIO_NAME(FIO_STR_NAME, s) *s = FIO_NAME(FIO_STR_NAME, new)();
    FIO_ASSERT(FIO_PTR_TAG_GET_UNTAGGED(FIO_NAME(FIO_STR_NAME, s), s),
               "error, string not allocated (%p)!",
               (void *)s);
    fio_str_info_s state = FIO_NAME(FIO_STR_NAME, readfile)(s, __FILE__, 0, 0);

    FIO_ASSERT(state.len && state.buf,
               "error, no data was read for file %s!",
               __FILE__);
#if defined(H___FIO_CSTL_COMBINED___H)
    FIO_ASSERT(!memcmp(state.buf,
                       "/* "
                       "******************************************************"
                       "***********************",
                       80),
               "content error, header mismatch!\n %s",
               state.buf);
#endif /* H___FIO_CSTL_COMBINED___H */
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
  FIO_NAME(FIO_STR_NAME, destroy)(pstr);
  if (1) {
    /* Testing Static initialization and writing */
#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)(pstr, "Welcome", 7);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC("Welcome");
#endif
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) == 0 ||
                   FIO_STR_IS_SMALL(&str),
               "Static string capacity non-zero.");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) > 0,
               "Static string length should be automatically calculated.");
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, allocated)(pstr),
               "Static strings shouldn't be dynamic.");
    FIO_NAME(FIO_STR_NAME, destroy)(pstr);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)
    (pstr,
     "Welcome to a very long static string that should not fit within a "
     "containing struct... hopefuly",
     95);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC(
        "Welcome to a very long static string that should not fit within a "
        "containing struct... hopefuly");
#endif
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, capa)(pstr) == 0 ||
                   FIO_STR_IS_SMALL(&str),
               "Static string capacity non-zero.");
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) > 0,
               "Static string length should be automatically calculated.");
    FIO_ASSERT(!FIO_NAME_BL(FIO_STR_NAME, allocated)(pstr),
               "Static strings shouldn't be dynamic.");
    FIO_NAME(FIO_STR_NAME, destroy)(pstr);

#if FIO_STR_OPTIMIZE4IMMUTABILITY
    FIO_NAME(FIO_STR_NAME, init_const)(pstr, "Welcome", 7);
#else
    str = (FIO_NAME(FIO_STR_NAME, s))FIO_STR_INIT_STATIC("Welcome");
#endif
    fio_str_info_s state = FIO_NAME(FIO_STR_NAME, write)(pstr, " Home", 5);
    FIO_ASSERT(state.capa > 0, "Static string not converted to non-static.");
    FIO_ASSERT(FIO_NAME_BL(FIO_STR_NAME, allocated)(pstr) ||
                   FIO_STR_IS_SMALL(&str),
               "String should be dynamic after `write`.");

    char *cstr = FIO_NAME(FIO_STR_NAME, detach)(pstr);
    FIO_ASSERT(cstr, "`detach` returned NULL");
    FIO_ASSERT(!memcmp(cstr, "Welcome Home\0", 13),
               "`detach` string error: %s",
               cstr);
    FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(pstr) == 0,
               "`detach` data wasn't cleared.");
    FIO_NAME(FIO_STR_NAME, destroy)(pstr); /*not really needed... detached... */
    FIO_NAME(FIO_STR_NAME, dealloc)(cstr);
  }
  {
    fprintf(stderr, "* Testing Base64 encoding / decoding.\n");
    FIO_NAME(FIO_STR_NAME, destroy)(pstr); /* does nothing, but why not... */

    FIO_NAME(FIO_STR_NAME, s) b64message = FIO_STR_INIT;
    fio_str_info_s b64i = FIO_NAME(FIO_STR_NAME, write)(
        FIO_PTR_TAG(&b64message),
        "Hello World, this is the voice of peace:)",
        41);
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      b64i = FIO_NAME(FIO_STR_NAME, write)(FIO_PTR_TAG(&b64message), &c, 1);
      FIO_ASSERT(FIO_NAME(FIO_STR_NAME, len)(FIO_PTR_TAG(&b64message)) ==
                     (size_t)(42 + i),
                 "Base64 message length error (%zu != %zu)",
                 FIO_NAME(FIO_STR_NAME, len)(FIO_PTR_TAG(&b64message)),
                 (size_t)(42 + i));
      FIO_ASSERT(FIO_NAME(FIO_STR_NAME,
                          ptr)(FIO_PTR_TAG(&b64message))[41 + i] == (char)c,
                 "Base64 message data error");
    }
    fio_str_info_s encoded =
        FIO_NAME(FIO_STR_NAME, write_base64enc)(pstr, b64i.buf, b64i.len, 1);
    /* prevent encoded data from being deallocated during unencoding */
    encoded = FIO_NAME(FIO_STR_NAME, FIO_STR_RESERVE_NAME)(
        pstr,
        encoded.len + ((encoded.len >> 2) * 3) + 8);
    fio_str_info_s decoded;
    {
      FIO_NAME(FIO_STR_NAME, s) tmps;
      FIO_NAME(FIO_STR_NAME, init_copy2)(FIO_PTR_TAG(&tmps), pstr);
      decoded = FIO_NAME(FIO_STR_NAME, write_base64dec)(
          pstr,
          FIO_NAME(FIO_STR_NAME, ptr)(FIO_PTR_TAG(&tmps)),
          FIO_NAME(FIO_STR_NAME, len)(FIO_PTR_TAG(&tmps)));
      FIO_NAME(FIO_STR_NAME, destroy)(FIO_PTR_TAG(&tmps));
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
    FIO_NAME(FIO_STR_NAME, destroy)(FIO_PTR_TAG(&b64message));
    FIO_NAME(FIO_STR_NAME, destroy)(pstr);
  }
  {
    fprintf(stderr, "* Testing JSON style character escaping / unescaping.\n");
    FIO_NAME(FIO_STR_NAME, s) unescaped = FIO_STR_INIT;
    fio_str_info_s ue;
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    FIO_NAME(FIO_STR_NAME, write)
    (FIO_PTR_TAG(&unescaped), utf8_sample, FIO_STRLEN(utf8_sample));
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      ue = FIO_NAME(FIO_STR_NAME, write)(FIO_PTR_TAG(&unescaped), &c, 1);
    }
    fio_str_info_s encoded =
        FIO_NAME(FIO_STR_NAME, write_escape)(pstr, ue.buf, ue.len);
    // fprintf(stderr, "* %s\n", encoded.buf);
    fio_str_info_s decoded;
    {
      FIO_NAME(FIO_STR_NAME, s) tmps;
      FIO_NAME(FIO_STR_NAME, init_copy2)(&tmps, pstr);
      decoded = FIO_NAME(FIO_STR_NAME,
                         write_unescape)(pstr,
                                         FIO_NAME(FIO_STR_NAME, ptr)(&tmps),
                                         FIO_NAME(FIO_STR_NAME, len)(&tmps));
      FIO_NAME(FIO_STR_NAME, destroy)(&tmps);
      encoded.buf = decoded.buf;
    }
    FIO_ASSERT(!memcmp(encoded.buf, utf8_sample, FIO_STRLEN(utf8_sample)),
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
    FIO_NAME(FIO_STR_NAME, destroy)(FIO_PTR_TAG(&unescaped));
    FIO_NAME(FIO_STR_NAME, destroy)(pstr);
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

#undef FIO_STR_FREEZE_

#undef FIO_STR_IS_FROZEN
#undef FIO_STR_IS_SMALL
#undef FIO_STR_NAME

#undef FIO_STR_OPTIMIZE4IMMUTABILITY
#undef FIO_STR_OPTIMIZE_EMBEDDED
#undef FIO_STR_PTR
#undef FIO_STR_THAW_
#undef FIO_STR_RESERVE_NAME

#endif /* FIO_STR_NAME */
