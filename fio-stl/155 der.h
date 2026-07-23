/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_DER                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          ASN.1 DER Parser for X.509
                        (RFC 5280 Certificate Parsing)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_DER) && !defined(H___FIO_DER___H)
#define H___FIO_DER___H

/* *****************************************************************************
ASN.1 Tag Types (Universal Class)
***************************************************************************** */

/** ASN.1 Universal Tag Types */
typedef enum {
  FIO_DER_EOC = 0x00,               /**< End-of-contents */
  FIO_DER_BOOLEAN = 0x01,           /**< Boolean */
  FIO_DER_INTEGER = 0x02,           /**< Integer */
  FIO_DER_BIT_STRING = 0x03,        /**< Bit String */
  FIO_DER_OCTET_STRING = 0x04,      /**< Octet String */
  FIO_DER_NULL = 0x05,              /**< Null */
  FIO_DER_OID = 0x06,               /**< Object Identifier */
  FIO_DER_OBJECT_DESCRIPTOR = 0x07, /**< Object Descriptor */
  FIO_DER_EXTERNAL = 0x08,          /**< External */
  FIO_DER_REAL = 0x09,              /**< Real (float) */
  FIO_DER_ENUMERATED = 0x0A,        /**< Enumerated */
  FIO_DER_EMBEDDED_PDV = 0x0B,      /**< Embedded PDV */
  FIO_DER_UTF8_STRING = 0x0C,       /**< UTF-8 String */
  FIO_DER_RELATIVE_OID = 0x0D,      /**< Relative OID */
  FIO_DER_SEQUENCE = 0x10,          /**< Sequence (0x30 with constructed bit) */
  FIO_DER_SET = 0x11,               /**< Set (0x31 with constructed bit) */
  FIO_DER_NUMERIC_STRING = 0x12,    /**< Numeric String */
  FIO_DER_PRINTABLE_STRING = 0x13,  /**< Printable String */
  FIO_DER_T61_STRING = 0x14,        /**< T61 String (Teletex) */
  FIO_DER_VIDEOTEX_STRING = 0x15,   /**< Videotex String */
  FIO_DER_IA5_STRING = 0x16,        /**< IA5 String (ASCII) */
  FIO_DER_UTC_TIME = 0x17,          /**< UTC Time */
  FIO_DER_GENERALIZED_TIME = 0x18,  /**< Generalized Time */
  FIO_DER_GRAPHIC_STRING = 0x19,    /**< Graphic String */
  FIO_DER_VISIBLE_STRING = 0x1A,    /**< Visible String */
  FIO_DER_GENERAL_STRING = 0x1B,    /**< General String */
  FIO_DER_UNIVERSAL_STRING = 0x1C,  /**< Universal String */
  FIO_DER_BMP_STRING = 0x1E,        /**< BMP String (UCS-2) */
  /* Context-specific tags (0x80 | tag_number) with constructed bit (0x20) */
  FIO_DER_CONTEXT_0 = 0xA0, /**< [0] EXPLICIT/IMPLICIT */
  FIO_DER_CONTEXT_1 = 0xA1, /**< [1] EXPLICIT/IMPLICIT */
  FIO_DER_CONTEXT_2 = 0xA2, /**< [2] EXPLICIT/IMPLICIT */
  FIO_DER_CONTEXT_3 = 0xA3, /**< [3] EXPLICIT/IMPLICIT */
} fio_der_tag_e;

/** ASN.1 Tag Class (bits 7-6 of tag byte) */
typedef enum {
  FIO_DER_CLASS_UNIVERSAL = 0,   /**< Universal (built-in types) */
  FIO_DER_CLASS_APPLICATION = 1, /**< Application-specific */
  FIO_DER_CLASS_CONTEXT = 2,     /**< Context-specific */
  FIO_DER_CLASS_PRIVATE = 3,     /**< Private */
} fio_der_class_e;

/* *****************************************************************************
ASN.1 Parsed Element Structure
***************************************************************************** */

/** Parsed ASN.1 DER element */
typedef struct {
  const uint8_t *data;    /**< Pointer to element content (after tag+length) */
  size_t len;             /**< Length of content */
  uint8_t tag;            /**< Raw tag byte */
  uint8_t is_constructed; /**< 1 if constructed (contains other elements) */
  uint8_t tag_class;  /**< 0=Universal, 1=Application, 2=Context, 3=Private */
  uint8_t tag_number; /**< Tag number (bits 4-0, or extended) */
} fio_der_element_s;

/** Iterator for SEQUENCE or SET contents */
typedef struct {
  const uint8_t *pos; /**< Current position */
  const uint8_t *end; /**< End of sequence */
} fio_der_iterator_s;

/* *****************************************************************************
ASN.1 Parser API - Core Functions
***************************************************************************** */

/**
 * Parse one ASN.1 element from DER-encoded data.
 *
 * @param elem Output structure to fill with parsed element info
 * @param data Pointer to DER-encoded data
 * @param data_len Length of data buffer
 * @return Pointer to next element (after this one), or NULL on error
 */
SFUNC const uint8_t *fio_der_parse(fio_der_element_s *elem,
                                   const uint8_t *data,
                                   size_t data_len);

/**
 * Get the total encoded length of an ASN.1 element (tag + length + content).
 *
 * @param elem Parsed element
 * @param data Original data pointer where element was parsed from
 * @return Total bytes used by the element encoding
 */
FIO_IFUNC size_t fio_der_element_total_len(const fio_der_element_s *elem,
                                           const uint8_t *data);

/* *****************************************************************************
ASN.1 Parser API - Type-Specific Parsers
***************************************************************************** */

/**
 * Parse an ASN.1 INTEGER element.
 *
 * For small integers (<= 64-bit), sets *value.
 * For large integers (RSA modulus), use elem->data/len directly.
 * Leading zero bytes for positive numbers are handled correctly.
 *
 * @param elem Parsed element (must be INTEGER type)
 * @param value Output for integer value (can be NULL for large integers)
 * @return 0 on success, -1 on error
 */
SFUNC int fio_der_parse_integer(const fio_der_element_s *elem, uint64_t *value);

/**
 * Parse an ASN.1 BIT STRING element.
 *
 * @param elem Parsed element (must be BIT STRING type)
 * @param bits Output pointer to bit data (first byte is unused bits count)
 * @param bit_len Output length of bit data in bytes
 * @param unused_bits Output number of unused bits in last byte (0-7)
 * @return 0 on success, -1 on error
 */
SFUNC int fio_der_parse_bit_string(const fio_der_element_s *elem,
                                   const uint8_t **bits,
                                   size_t *bit_len,
                                   uint8_t *unused_bits);

/**
 * Parse an ASN.1 OID into a dot-separated string.
 *
 * Example output: "1.2.840.113549.1.1.11"
 *
 * @param elem Parsed element (must be OID type)
 * @param buf Output buffer for string
 * @param buf_len Buffer size
 * @return Number of chars written (excluding NUL), or -1 on error
 */
SFUNC int fio_der_parse_oid(const fio_der_element_s *elem,
                            char *buf,
                            size_t buf_len);

/**
 * Parse an ASN.1 time (UTC Time or Generalized Time) to Unix timestamp.
 *
 * @param elem Parsed element (must be UTC_TIME or GENERALIZED_TIME type)
 * @param unix_time Output Unix timestamp (seconds since 1970-01-01 00:00:00
 * UTC)
 * @return 0 on success, -1 on error
 */
SFUNC int fio_der_parse_time(const fio_der_element_s *elem, int64_t *unix_time);

/**
 * Parse an ASN.1 string element.
 *
 * Supports UTF8String, PrintableString, IA5String, etc.
 * Returns pointer directly into the element data (no copy).
 *
 * @param elem Parsed element (must be a string type)
 * @param len Output length of string
 * @return Pointer to string data, or NULL on error
 */
FIO_IFUNC const char *fio_der_parse_string(const fio_der_element_s *elem,
                                           size_t *len);

/**
 * Parse an ASN.1 BOOLEAN element.
 *
 * @param elem Parsed element (must be BOOLEAN type)
 * @param value Output boolean value (0 = false, non-zero = true)
 * @return 0 on success, -1 on error
 */
FIO_IFUNC int fio_der_parse_boolean(const fio_der_element_s *elem, int *value);

/* *****************************************************************************
ASN.1 Parser API - Sequence/Set Iteration
***************************************************************************** */

/**
 * Initialize an iterator for a SEQUENCE or SET element.
 *
 * @param it Iterator to initialize
 * @param sequence Parsed element (must be SEQUENCE or SET)
 */
FIO_IFUNC void fio_der_iterator_init(fio_der_iterator_s *it,
                                     const fio_der_element_s *sequence);

/**
 * Get the next element from an iterator.
 *
 * @param it Iterator (updated to point to next element)
 * @param elem Output for parsed element
 * @return 0 if element available, -1 if end or error
 */
SFUNC int fio_der_iterator_next(fio_der_iterator_s *it,
                                fio_der_element_s *elem);

/**
 * Check if iterator has more elements.
 *
 * @param it Iterator
 * @return 1 if more elements available, 0 otherwise
 */
FIO_IFUNC int fio_der_iterator_has_next(const fio_der_iterator_s *it);

/* *****************************************************************************
ASN.1 Parser API - Helper Functions
***************************************************************************** */

/**
 * Check if an element is a specific tag type.
 *
 * @param elem Parsed element
 * @param tag Expected tag (e.g., FIO_DER_INTEGER)
 * @return 1 if match, 0 otherwise
 */
FIO_IFUNC int fio_der_is_tag(const fio_der_element_s *elem, uint8_t tag);

/**
 * Check if an element is a context-specific tag.
 *
 * @param elem Parsed element
 * @param tag_num Context tag number (0-31)
 * @return 1 if match, 0 otherwise
 */
FIO_IFUNC int fio_der_is_context_tag(const fio_der_element_s *elem,
                                     uint8_t tag_num);

/**
 * Get the tag number from an element.
 *
 * For universal tags, returns the tag value (0-30).
 * For context-specific tags, returns the context number.
 *
 * @param elem Parsed element
 * @return Tag number
 */
FIO_IFUNC uint8_t fio_der_tag_number(const fio_der_element_s *elem);

/* *****************************************************************************
Implementation - Inline Functions
***************************************************************************** */

/** Get total encoded length of element */
FIO_IFUNC size_t fio_der_element_total_len(const fio_der_element_s *elem,
                                           const uint8_t *data) {
  if (!elem || !data || !elem->data)
    return 0;
  return (size_t)(elem->data - data) + elem->len;
}

/** Parse string types - returns pointer to data */
FIO_IFUNC const char *fio_der_parse_string(const fio_der_element_s *elem,
                                           size_t *len) {
  if (!elem || !len)
    return NULL;
  /* Accept various string types */
  uint8_t tag_num = elem->tag & 0x1F;
  if (elem->tag_class != FIO_DER_CLASS_UNIVERSAL)
    return NULL;
  switch (tag_num) {
  case FIO_DER_UTF8_STRING:
  case FIO_DER_PRINTABLE_STRING:
  case FIO_DER_IA5_STRING:
  case FIO_DER_T61_STRING:
  case FIO_DER_VISIBLE_STRING:
  case FIO_DER_GENERAL_STRING:
  case FIO_DER_UNIVERSAL_STRING:
  case FIO_DER_BMP_STRING:
  case FIO_DER_NUMERIC_STRING:
    *len = elem->len;
    return (const char *)elem->data;
  default: return NULL;
  }
}

/** Parse boolean value */
FIO_IFUNC int fio_der_parse_boolean(const fio_der_element_s *elem, int *value) {
  if (!elem || !value)
    return -1;
  if ((elem->tag & 0x1F) != FIO_DER_BOOLEAN ||
      elem->tag_class != FIO_DER_CLASS_UNIVERSAL)
    return -1;
  if (elem->len != 1)
    return -1;
  *value = (elem->data[0] != 0);
  return 0;
}

/* *****************************************************************************
OID Value Helpers (internal) - fio_u128 byte-15 design
*****************************************************************************

An OID value is a plain fio_u128 where bytes 0-14 hold the DER content bytes
(zero-padded) and byte 15 holds the content length. The length is folded in
because 0x00 is a legal OID content byte (arc 0), so padding alone cannot
distinguish {..0B} from {..0B 00} - different OIDs. Oversized OIDs (content
> 15 bytes) simply never match, which is safe: AlgorithmIdentifier OIDs are
attacker-controlled and RFC 5280 requires exact match.
***************************************************************************** */

/** Build OID value from element. Build ONCE per element, then test
    against any number of constants. Invalid/oversized -> all 0xFF (never
    matches a valid value: byte 15 would be 0xFF, not <= 15). */
FIO_IFUNC fio_u128 fio___der_oid_value(const fio_der_element_s *elem) {
  fio_u128 r;
  if (!elem || elem->len > 15) {
    r.u64[0] = r.u64[1] = ~(uint64_t)0;
    return r;
  }
  r.u64[0] = r.u64[1] = 0;
  for (uint8_t i = 0; i < elem->len; ++i)
    r.u8[i] = elem->data[i];
  r.u8[15] = (uint8_t)elem->len;
  return r;
}

/** Exact compare: two u64 lane ==. No len field, no memcmp, no call.
    Endianness-agnostic (both sides built from identical byte layouts). */
FIO_IFUNC int fio___der_oid_eq(fio_u128 a, fio_u128 b) {
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

/** Initialize iterator for sequence/set */
FIO_IFUNC void fio_der_iterator_init(fio_der_iterator_s *it,
                                     const fio_der_element_s *sequence) {
  if (!it)
    return;
  if (!sequence || !sequence->data) {
    it->pos = NULL;
    it->end = NULL;
    return;
  }
  it->pos = sequence->data;
  it->end = sequence->data + sequence->len;
}

/** Check if iterator has more elements */
FIO_IFUNC int fio_der_iterator_has_next(const fio_der_iterator_s *it) {
  return (it && it->pos && it->end && it->pos < it->end);
}

/** Check if element matches tag */
FIO_IFUNC int fio_der_is_tag(const fio_der_element_s *elem, uint8_t tag) {
  if (!elem)
    return 0;
  /* For universal tags, compare the tag number */
  if (elem->tag_class == FIO_DER_CLASS_UNIVERSAL)
    return (elem->tag & 0x1F) == (tag & 0x1F);
  /* For other classes, compare full tag byte */
  return elem->tag == tag;
}

/** Check if element is context-specific tag */
FIO_IFUNC int fio_der_is_context_tag(const fio_der_element_s *elem,
                                     uint8_t tag_num) {
  if (!elem)
    return 0;
  return (elem->tag_class == FIO_DER_CLASS_CONTEXT) &&
         ((elem->tag & 0x1F) == (tag_num & 0x1F));
}

/** Get tag number from element */
FIO_IFUNC uint8_t fio_der_tag_number(const fio_der_element_s *elem) {
  if (!elem)
    return 0;
  return elem->tag & 0x1F;
}

/* *****************************************************************************
Implementation - Possibly Externed Functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - Core Parser
***************************************************************************** */

/**
 * Parse DER length field.
 * Returns pointer past length field, or NULL on error.
 * Sets *out_len to the parsed length value.
 */
FIO_SFUNC const uint8_t *fio___der_parse_length(const uint8_t *data,
                                                const uint8_t *end,
                                                size_t *out_len) {
  if (!data || !end || !out_len || data >= end)
    return NULL;

  uint8_t first = *data++;

  /* Short form: 0x00-0x7F = length directly */
  if (!(first & 0x80)) {
    *out_len = first;
    return data;
  }

  /* Long form: first byte & 0x7F = number of length bytes */
  size_t num_bytes = first & 0x7F;

  /* Indefinite length (0x80) not allowed in DER */
  if (num_bytes == 0)
    return NULL;

  /* Too many length bytes (protect against overflow) */
  if (num_bytes > sizeof(size_t) || (size_t)(end - data) < num_bytes)
    return NULL;

  size_t len = 0;
  for (size_t i = 0; i < num_bytes; ++i) {
    /* Check for overflow before shifting */
    if (len > (SIZE_MAX >> 8))
      return NULL;
    len = (len << 8) | *data++;
  }

  *out_len = len;
  return data;
}

/** Parse one ASN.1 element from DER data */
SFUNC const uint8_t *fio_der_parse(fio_der_element_s *elem,
                                   const uint8_t *data,
                                   size_t data_len) {
  if (!elem || !data || data_len == 0)
    return NULL;

  const uint8_t *end = data + data_len;
  const uint8_t *p = data;

  /* Parse tag byte */
  if (p >= end)
    return NULL;

  uint8_t tag = *p++;
  elem->tag = tag;
  elem->tag_class = (tag >> 6) & 0x03;
  elem->is_constructed = (tag >> 5) & 0x01;
  elem->tag_number = tag & 0x1F;

  /* Handle long-form tags (tag number >= 31) */
  if (elem->tag_number == 0x1F) {
    /* Multi-byte tag number - accumulate until high bit clear */
    uint32_t tag_num = 0;
    while (p < end) {
      uint8_t b = *p++;
      if (tag_num > (0xFFFFFFFF >> 7))
        return NULL; /* Overflow */
      tag_num = (tag_num << 7) | (b & 0x7F);
      if (!(b & 0x80))
        break;
    }
    /* Store as much as fits in tag_number (used for context tags mainly) */
    elem->tag_number = (uint8_t)(tag_num & 0xFF);
  }

  /* Parse length */
  size_t content_len;
  p = fio___der_parse_length(p, end, &content_len);
  if (!p)
    return NULL;

  /* Validate content fits in remaining data */
  if ((size_t)(end - p) < content_len)
    return NULL;

  elem->data = p;
  elem->len = content_len;

  return p + content_len;
}

/* *****************************************************************************
Implementation - Integer Parser
***************************************************************************** */

SFUNC int fio_der_parse_integer(const fio_der_element_s *elem,
                                uint64_t *value) {
  if (!elem)
    return -1;

  /* Verify it's an INTEGER */
  if (elem->tag_class != FIO_DER_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_DER_INTEGER)
    return -1;

  if (elem->len == 0)
    return -1;

  const uint8_t *p = elem->data;
  size_t len = elem->len;

  /* Skip leading zero byte used for positive numbers */
  if (len > 1 && p[0] == 0x00 && (p[1] & 0x80)) {
    p++;
    len--;
  }

  /* If caller wants the value and it fits in uint64_t */
  if (value) {
    if (len > 8)
      return -1; /* Too large for uint64_t */

    uint64_t v = 0;
    for (size_t i = 0; i < len; ++i)
      v = (v << 8) | p[i];
    *value = v;
  }

  return 0;
}

/* *****************************************************************************
Implementation - Bit String Parser
***************************************************************************** */

SFUNC int fio_der_parse_bit_string(const fio_der_element_s *elem,
                                   const uint8_t **bits,
                                   size_t *bit_len,
                                   uint8_t *unused_bits) {
  if (!elem || !bits || !bit_len || !unused_bits)
    return -1;

  /* Verify it's a BIT STRING */
  if (elem->tag_class != FIO_DER_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_DER_BIT_STRING)
    return -1;

  if (elem->len < 1)
    return -1;

  /* First byte is unused bits count (0-7) */
  uint8_t unused = elem->data[0];
  if (unused > 7)
    return -1;

  /* Empty bit string */
  if (elem->len == 1) {
    if (unused != 0)
      return -1;
    *bits = NULL;
    *bit_len = 0;
    *unused_bits = 0;
    return 0;
  }

  *bits = elem->data + 1;
  *bit_len = elem->len - 1;
  *unused_bits = unused;
  return 0;
}

/* *****************************************************************************
Implementation - OID Parser
***************************************************************************** */

/** Parse OID component from base-128 encoding */
FIO_SFUNC const uint8_t *fio___der_parse_oid_component(const uint8_t *p,
                                                       const uint8_t *end,
                                                       uint64_t *value) {
  uint64_t v = 0;
  size_t count = 0;

  while (p < end) {
    uint8_t b = *p++;
    /* Protect against overflow - OID components shouldn't be this large */
    if (count++ > 9)
      return NULL;
    v = (v << 7) | (b & 0x7F);
    if (!(b & 0x80)) {
      *value = v;
      return p;
    }
  }
  return NULL; /* Incomplete encoding */
}

/** Write unsigned integer to string */
FIO_SFUNC int fio___der_write_uint(char *buf, size_t buf_len, uint64_t value) {
  if (!buf || buf_len == 0)
    return -1;

  /* Count digits */
  char tmp[24];
  int len = 0;
  uint64_t v = value;
  do {
    tmp[len++] = '0' + (char)(v % 10);
    v /= 10;
  } while (v > 0);

  if ((size_t)len >= buf_len)
    return -1;

  /* Reverse into output buffer */
  for (int i = 0; i < len; ++i)
    buf[i] = tmp[len - 1 - i];
  buf[len] = '\0';

  return len;
}

SFUNC int fio_der_parse_oid(const fio_der_element_s *elem,
                            char *buf,
                            size_t buf_len) {
  if (!elem || !buf || buf_len < 4)
    return -1;

  /* Verify it's an OID */
  if (elem->tag_class != FIO_DER_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_DER_OID)
    return -1;

  if (elem->len == 0)
    return -1;

  const uint8_t *p = elem->data;
  const uint8_t *end = elem->data + elem->len;

  char *out = buf;
  char *out_end = buf + buf_len - 1;
  int total = 0;

  /* First byte encodes first two components: X.Y where byte = X*40 + Y */
  uint8_t first = *p++;
  uint64_t c1 = first / 40;
  uint64_t c2 = first % 40;

  /* First component can only be 0, 1, or 2 */
  if (c1 > 2) {
    c1 = 2;
    c2 = first - 80;
  }

  int len = fio___der_write_uint(out, (size_t)(out_end - out), c1);
  if (len < 0)
    return -1;
  out += len;
  total += len;

  if (out >= out_end)
    return -1;
  *out++ = '.';
  total++;

  len = fio___der_write_uint(out, (size_t)(out_end - out), c2);
  if (len < 0)
    return -1;
  out += len;
  total += len;

  /* Remaining components */
  while (p < end) {
    uint64_t component;
    p = fio___der_parse_oid_component(p, end, &component);
    if (!p)
      return -1;

    if (out >= out_end)
      return -1;
    *out++ = '.';
    total++;

    len = fio___der_write_uint(out, (size_t)(out_end - out), component);
    if (len < 0)
      return -1;
    out += len;
    total += len;
  }

  *out = '\0';
  return total;
}

/* *****************************************************************************
Implementation - Time Parser
***************************************************************************** */

SFUNC int fio_der_parse_time(const fio_der_element_s *elem,
                             int64_t *unix_time) {
  if (!elem || !unix_time)
    return -1;

  char *p = (char *)elem->data;
  uint64_t digits;
  uint64_t divisor = 100000000ULL;
  size_t consumed = 0;
  int year, month, day, hour, min, sec = 0;
  uint8_t tag = elem->tag & 0x1F;

  if (elem->tag_class != FIO_DER_CLASS_UNIVERSAL ||
      (tag != FIO_DER_UTC_TIME && tag != FIO_DER_GENERALIZED_TIME) ||
      elem->len < 10 || fio_buf2u32u(p) == (0x01010101UL * '0'))
    return -1;

  digits = fio_atol10u(&p);
  consumed = (size_t)(p - (const char *)elem->data);

  /* Skip optional fractional seconds */
  if (*p == '.') {
    p++;
    while (*p >= '0' && *p <= '9')
      p++;
  }
  if (*p != 'Z')
    return -1;

  if (tag == FIO_DER_UTC_TIME) {
    if (consumed == 12)
      divisor = 10000000000ULL; /*   YYMMDDhhmmss */
    else if (consumed != 10)
      return -1;
  } else { /* GENERALIZED_TIME */
    if (consumed == 14)
      divisor = 10000000000ULL; /* YYYYMMDDhhmmss */
    else if (consumed != 12)
      return -1;
  }

  year = (int)(digits / divisor);
  digits -= year * divisor;
  divisor /= 100;
  month = (int)(digits / divisor);
  digits -= month * divisor;
  divisor /= 100;
  day = (int)(digits / divisor);
  digits -= day * divisor;
  divisor /= 100;
  hour = (int)(digits / divisor);
  digits -= hour * divisor;
  divisor /= 100;
  min = (int)(digits / divisor);
  digits -= min * divisor;
  divisor /= 100;
  sec = (int)(digits);

  if (tag == FIO_DER_UTC_TIME) {
    year += (year < 50 ? 2000 : 1900);
  }

  {
    struct tm tm = {
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
        .tm_isdst = 0,
    };
    *unix_time = (int64_t)fio_gm2time(tm);
  }
  return 0;
}

/* *****************************************************************************
Implementation - Iterator
*****************************************************************************
*/

SFUNC int fio_der_iterator_next(fio_der_iterator_s *it,
                                fio_der_element_s *elem) {
  if (!it || !elem || !it->pos || !it->end || it->pos >= it->end)
    return -1;

  size_t remaining = (size_t)(it->end - it->pos);
  const uint8_t *next = fio_der_parse(elem, it->pos, remaining);
  if (!next)
    return -1;

  it->pos = next;
  return 0;
}

/* *****************************************************************************
ASN.1 DER Encoding API
*****************************************************************************
*/

/**
 * Encode DER length field.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param len Length value to encode
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_length(uint8_t *buf, size_t len);

/**
 * Encode an ASN.1 INTEGER.
 *
 * Handles positive integers with proper leading zero byte when needed.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param data Integer data in big-endian format
 * @param data_len Length of integer data
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_integer(uint8_t *buf,
                                    const uint8_t *data,
                                    size_t data_len);

/**
 * Encode an ASN.1 INTEGER from a small value.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param value Integer value (up to 64-bit)
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_integer_small(uint8_t *buf, uint64_t value);

/**
 * Encode an ASN.1 OID TLV from an OID value (internal).
 *
 * The OID value holds the DER content bytes in bytes 0-14 and the content
 * length in byte 15 (see fio___der_oid_value). No dot-string parsing.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param oid OID value (fio_u128, length folded into byte 15)
 * @return Number of bytes written/needed, or 0 on error
 */
SFUNC size_t fio___der_encode_oid(uint8_t *buf, fio_u128 oid);

/**
 * Encode an ASN.1 UTF8String.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param str String data
 * @param str_len String length
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_utf8_string(uint8_t *buf,
                                        const char *str,
                                        size_t str_len);

/**
 * Encode an ASN.1 PrintableString.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param str String data
 * @param str_len String length
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_printable_string(uint8_t *buf,
                                             const char *str,
                                             size_t str_len);

/**
 * Encode an ASN.1 BIT STRING.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param bits Bit data
 * @param bit_len Length of bit data in bytes
 * @param unused_bits Number of unused bits in last byte (0-7)
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_bit_string(uint8_t *buf,
                                       const uint8_t *bits,
                                       size_t bit_len,
                                       uint8_t unused_bits);

/**
 * Encode an ASN.1 OCTET STRING.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param data Octet data
 * @param data_len Length of data
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_octet_string(uint8_t *buf,
                                         const uint8_t *data,
                                         size_t data_len);

/**
 * Encode an ASN.1 SEQUENCE wrapper around existing content.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param content_len Length of content that will follow
 * @return Number of bytes written/needed for tag+length only
 */
SFUNC size_t fio_der_encode_sequence_header(uint8_t *buf, size_t content_len);

/**
 * Encode an ASN.1 SET wrapper around existing content.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param content_len Length of content that will follow
 * @return Number of bytes written/needed for tag+length only
 */
SFUNC size_t fio_der_encode_set_header(uint8_t *buf, size_t content_len);

/**
 * Encode an ASN.1 context-specific tag wrapper.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param tag_num Context tag number (0-30)
 * @param content_len Length of content that will follow
 * @param constructed 1 if constructed (contains other elements), 0 if
 * primitive
 * @return Number of bytes written/needed for tag+length only
 */
SFUNC size_t fio_der_encode_context_header(uint8_t *buf,
                                           uint8_t tag_num,
                                           size_t content_len,
                                           int constructed);

/**
 * Encode an ASN.1 NULL.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @return Number of bytes written/needed (always 2)
 */
SFUNC size_t fio_der_encode_null(uint8_t *buf);

/**
 * Encode an ASN.1 BOOLEAN.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param value Boolean value (0 = false, non-zero = true)
 * @return Number of bytes written/needed (always 3)
 */
SFUNC size_t fio_der_encode_boolean(uint8_t *buf, int value);

/**
 * Encode an ASN.1 UTCTime from Unix timestamp.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param unix_time Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_utc_time(uint8_t *buf, int64_t unix_time);

/**
 * Encode an ASN.1 GeneralizedTime from Unix timestamp.
 *
 * @param buf Output buffer (can be NULL to calculate length only)
 * @param unix_time Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
 * @return Number of bytes written/needed
 */
SFUNC size_t fio_der_encode_generalized_time(uint8_t *buf, int64_t unix_time);

/* *****************************************************************************
Implementation - ASN.1 Encoding Functions
*****************************************************************************
*/

/** Encode DER length field */
SFUNC size_t fio_der_encode_length(uint8_t *buf, size_t len) {
  if (len < 128) {
    /* Short form */
    if (buf)
      buf[0] = (uint8_t)len;
    return 1;
  }

  /* Long form - count bytes needed */
  size_t num_bytes = 0;
  size_t tmp = len;
  while (tmp > 0) {
    ++num_bytes;
    tmp >>= 8;
  }

  if (buf) {
    buf[0] = (uint8_t)(0x80 | num_bytes);
    for (size_t i = num_bytes; i > 0; --i)
      buf[i] = (uint8_t)(len >> ((num_bytes - i) * 8));
  }

  return 1 + num_bytes;
}

/** Encode ASN.1 INTEGER */
SFUNC size_t fio_der_encode_integer(uint8_t *buf,
                                    const uint8_t *data,
                                    size_t data_len) {
  if (!data || data_len == 0)
    return 0;

  /* Skip leading zeros (but keep at least one byte) */
  while (data_len > 1 && data[0] == 0) {
    ++data;
    --data_len;
  }

  /* Check if we need a leading zero (positive number with high bit set) */
  int need_zero = (data[0] & 0x80) ? 1 : 0;
  size_t content_len = data_len + need_zero;

  /* Calculate total length */
  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes + content_len;

  if (buf) {
    buf[0] = FIO_DER_INTEGER;
    fio_der_encode_length(buf + 1, content_len);
    size_t offset = 1 + len_bytes;
    if (need_zero)
      buf[offset++] = 0x00;
    FIO_MEMCPY(buf + offset, data, data_len);
  }

  return total;
}

/** Encode ASN.1 INTEGER from small value */
SFUNC size_t fio_der_encode_integer_small(uint8_t *buf, uint64_t value) {
  uint8_t tmp[9];
  size_t len = 0;

  /* Convert to big-endian bytes */
  if (value == 0) {
    tmp[0] = 0;
    len = 1;
  } else {
    /* Find number of bytes needed */
    uint64_t v = value;
    while (v > 0) {
      ++len;
      v >>= 8;
    }
    /* Write big-endian */
    for (size_t i = 0; i < len; ++i)
      tmp[i] = (uint8_t)(value >> ((len - 1 - i) * 8));
  }

  return fio_der_encode_integer(buf, tmp, len);
}

/** Encode ASN.1 OID TLV from an OID value (internal) */
SFUNC size_t fio___der_encode_oid(uint8_t *buf, fio_u128 oid) {
  size_t content_len = oid.u8[15];
  if (content_len > 15)
    return 0; /* invalid OID value (e.g., the all-0xFF never-match value) */

  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes + content_len;

  if (buf) {
    buf[0] = FIO_DER_OID;
    fio_der_encode_length(buf + 1, content_len);
    size_t offset = 1 + len_bytes;
    for (size_t i = 0; i < content_len; ++i)
      buf[offset + i] = oid.u8[i];
  }

  return total;
}

/** Encode ASN.1 UTF8String */
SFUNC size_t fio_der_encode_utf8_string(uint8_t *buf,
                                        const char *str,
                                        size_t str_len) {
  size_t len_bytes = fio_der_encode_length(NULL, str_len);
  size_t total = 1 + len_bytes + str_len;

  if (buf) {
    buf[0] = FIO_DER_UTF8_STRING;
    fio_der_encode_length(buf + 1, str_len);
    if (str_len > 0)
      FIO_MEMCPY(buf + 1 + len_bytes, str, str_len);
  }

  return total;
}

/** Encode ASN.1 PrintableString */
SFUNC size_t fio_der_encode_printable_string(uint8_t *buf,
                                             const char *str,
                                             size_t str_len) {
  size_t len_bytes = fio_der_encode_length(NULL, str_len);
  size_t total = 1 + len_bytes + str_len;

  if (buf) {
    buf[0] = FIO_DER_PRINTABLE_STRING;
    fio_der_encode_length(buf + 1, str_len);
    if (str_len > 0)
      FIO_MEMCPY(buf + 1 + len_bytes, str, str_len);
  }

  return total;
}

/** Encode ASN.1 BIT STRING */
SFUNC size_t fio_der_encode_bit_string(uint8_t *buf,
                                       const uint8_t *bits,
                                       size_t bit_len,
                                       uint8_t unused_bits) {
  size_t content_len = 1 + bit_len; /* unused bits byte + data */
  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes + content_len;

  if (buf) {
    buf[0] = FIO_DER_BIT_STRING;
    fio_der_encode_length(buf + 1, content_len);
    buf[1 + len_bytes] = unused_bits;
    if (bit_len > 0)
      FIO_MEMCPY(buf + 2 + len_bytes, bits, bit_len);
  }

  return total;
}

/** Encode ASN.1 OCTET STRING */
SFUNC size_t fio_der_encode_octet_string(uint8_t *buf,
                                         const uint8_t *data,
                                         size_t data_len) {
  size_t len_bytes = fio_der_encode_length(NULL, data_len);
  size_t total = 1 + len_bytes + data_len;

  if (buf) {
    buf[0] = FIO_DER_OCTET_STRING;
    fio_der_encode_length(buf + 1, data_len);
    if (data && data_len > 0)
      FIO_MEMCPY(buf + 1 + len_bytes, data, data_len);
  }

  return total;
}

/** Encode ASN.1 SEQUENCE header */
SFUNC size_t fio_der_encode_sequence_header(uint8_t *buf, size_t content_len) {
  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes;

  if (buf) {
    buf[0] = 0x30; /* SEQUENCE tag (constructed) */
    fio_der_encode_length(buf + 1, content_len);
  }

  return total;
}

/** Encode ASN.1 SET header */
SFUNC size_t fio_der_encode_set_header(uint8_t *buf, size_t content_len) {
  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes;

  if (buf) {
    buf[0] = 0x31; /* SET tag (constructed) */
    fio_der_encode_length(buf + 1, content_len);
  }

  return total;
}

/** Encode ASN.1 context-specific tag header */
SFUNC size_t fio_der_encode_context_header(uint8_t *buf,
                                           uint8_t tag_num,
                                           size_t content_len,
                                           int constructed) {
  size_t len_bytes = fio_der_encode_length(NULL, content_len);
  size_t total = 1 + len_bytes;

  if (buf) {
    buf[0] = (uint8_t)(0x80 | (constructed ? 0x20 : 0) | (tag_num & 0x1F));
    fio_der_encode_length(buf + 1, content_len);
  }

  return total;
}

/** Encode ASN.1 NULL */
SFUNC size_t fio_der_encode_null(uint8_t *buf) {
  if (buf) {
    buf[0] = FIO_DER_NULL;
    buf[1] = 0x00;
  }
  return 2;
}

/** Encode ASN.1 BOOLEAN */
SFUNC size_t fio_der_encode_boolean(uint8_t *buf, int value) {
  if (buf) {
    buf[0] = FIO_DER_BOOLEAN;
    buf[1] = 0x01;
    buf[2] = value ? 0xFF : 0x00;
  }
  return 3;
}

/** Helper: convert Unix timestamp to broken-down time */
FIO_SFUNC void fio___der_gmtime(int64_t unix_time,
                                int *year,
                                int *month,
                                int *day,
                                int *hour,
                                int *min,
                                int *sec) {
  const time_t t = (time_t)unix_time;
  const struct tm tm = fio_time2gm(t);

  *year = tm.tm_year + 1900;
  *month = tm.tm_mon + 1;
  *day = tm.tm_mday;
  *hour = tm.tm_hour;
  *min = tm.tm_min;
  *sec = tm.tm_sec;
}

/** Encode ASN.1 UTCTime */
SFUNC size_t fio_der_encode_utc_time(uint8_t *buf, int64_t unix_time) {
  /* UTCTime format: YYMMDDhhmmssZ (13 bytes) */
  size_t content_len = 13;
  size_t total = 1 + 1 + content_len; /* tag + length + content */

  if (buf) {
    int year, month, day, hour, min, sec;
    fio___der_gmtime(unix_time, &year, &month, &day, &hour, &min, &sec);

    buf[0] = FIO_DER_UTC_TIME;
    buf[1] = (uint8_t)content_len;

    /* Year (2 digits) */
    int yy = year % 100;
    buf[2] = (uint8_t)('0' + yy / 10);
    buf[3] = (uint8_t)('0' + yy % 10);
    /* Month */
    buf[4] = (uint8_t)('0' + month / 10);
    buf[5] = (uint8_t)('0' + month % 10);
    /* Day */
    buf[6] = (uint8_t)('0' + day / 10);
    buf[7] = (uint8_t)('0' + day % 10);
    /* Hour */
    buf[8] = (uint8_t)('0' + hour / 10);
    buf[9] = (uint8_t)('0' + hour % 10);
    /* Minute */
    buf[10] = (uint8_t)('0' + min / 10);
    buf[11] = (uint8_t)('0' + min % 10);
    /* Second */
    buf[12] = (uint8_t)('0' + sec / 10);
    buf[13] = (uint8_t)('0' + sec % 10);
    /* Zulu */
    buf[14] = 'Z';
  }

  return total;
}

/** Encode ASN.1 GeneralizedTime */
SFUNC size_t fio_der_encode_generalized_time(uint8_t *buf, int64_t unix_time) {
  /* GeneralizedTime format: YYYYMMDDhhmmssZ (15 bytes) */
  size_t content_len = 15;
  size_t total = 1 + 1 + content_len; /* tag + length + content */

  if (buf) {
    int year, month, day, hour, min, sec;
    fio___der_gmtime(unix_time, &year, &month, &day, &hour, &min, &sec);

    buf[0] = FIO_DER_GENERALIZED_TIME;
    buf[1] = (uint8_t)content_len;

    /* Year (4 digits) */
    buf[2] = (uint8_t)('0' + (year / 1000) % 10);
    buf[3] = (uint8_t)('0' + (year / 100) % 10);
    buf[4] = (uint8_t)('0' + (year / 10) % 10);
    buf[5] = (uint8_t)('0' + year % 10);
    /* Month */
    buf[6] = (uint8_t)('0' + month / 10);
    buf[7] = (uint8_t)('0' + month % 10);
    /* Day */
    buf[8] = (uint8_t)('0' + day / 10);
    buf[9] = (uint8_t)('0' + day % 10);
    /* Hour */
    buf[10] = (uint8_t)('0' + hour / 10);
    buf[11] = (uint8_t)('0' + hour % 10);
    /* Minute */
    buf[12] = (uint8_t)('0' + min / 10);
    buf[13] = (uint8_t)('0' + min % 10);
    /* Second */
    buf[14] = (uint8_t)('0' + sec / 10);
    buf[15] = (uint8_t)('0' + sec % 10);
    /* Zulu */
    buf[16] = 'Z';
  }

  return total;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_DER */
#undef FIO_DER
