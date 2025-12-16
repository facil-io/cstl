/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ASN1               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          ASN.1 DER Parser for X.509
                        (RFC 5280 Certificate Parsing)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ASN1) && !defined(H___FIO_ASN1___H)
#define H___FIO_ASN1___H

/* *****************************************************************************
ASN.1 Tag Types (Universal Class)
***************************************************************************** */

/** ASN.1 Universal Tag Types */
typedef enum {
  FIO_ASN1_EOC = 0x00,               /**< End-of-contents */
  FIO_ASN1_BOOLEAN = 0x01,           /**< Boolean */
  FIO_ASN1_INTEGER = 0x02,           /**< Integer */
  FIO_ASN1_BIT_STRING = 0x03,        /**< Bit String */
  FIO_ASN1_OCTET_STRING = 0x04,      /**< Octet String */
  FIO_ASN1_NULL = 0x05,              /**< Null */
  FIO_ASN1_OID = 0x06,               /**< Object Identifier */
  FIO_ASN1_OBJECT_DESCRIPTOR = 0x07, /**< Object Descriptor */
  FIO_ASN1_EXTERNAL = 0x08,          /**< External */
  FIO_ASN1_REAL = 0x09,              /**< Real (float) */
  FIO_ASN1_ENUMERATED = 0x0A,        /**< Enumerated */
  FIO_ASN1_EMBEDDED_PDV = 0x0B,      /**< Embedded PDV */
  FIO_ASN1_UTF8_STRING = 0x0C,       /**< UTF-8 String */
  FIO_ASN1_RELATIVE_OID = 0x0D,      /**< Relative OID */
  FIO_ASN1_SEQUENCE = 0x10,         /**< Sequence (0x30 with constructed bit) */
  FIO_ASN1_SET = 0x11,              /**< Set (0x31 with constructed bit) */
  FIO_ASN1_NUMERIC_STRING = 0x12,   /**< Numeric String */
  FIO_ASN1_PRINTABLE_STRING = 0x13, /**< Printable String */
  FIO_ASN1_T61_STRING = 0x14,       /**< T61 String (Teletex) */
  FIO_ASN1_VIDEOTEX_STRING = 0x15,  /**< Videotex String */
  FIO_ASN1_IA5_STRING = 0x16,       /**< IA5 String (ASCII) */
  FIO_ASN1_UTC_TIME = 0x17,         /**< UTC Time */
  FIO_ASN1_GENERALIZED_TIME = 0x18, /**< Generalized Time */
  FIO_ASN1_GRAPHIC_STRING = 0x19,   /**< Graphic String */
  FIO_ASN1_VISIBLE_STRING = 0x1A,   /**< Visible String */
  FIO_ASN1_GENERAL_STRING = 0x1B,   /**< General String */
  FIO_ASN1_UNIVERSAL_STRING = 0x1C, /**< Universal String */
  FIO_ASN1_BMP_STRING = 0x1E,       /**< BMP String (UCS-2) */
  /* Context-specific tags (0x80 | tag_number) with constructed bit (0x20) */
  FIO_ASN1_CONTEXT_0 = 0xA0, /**< [0] EXPLICIT/IMPLICIT */
  FIO_ASN1_CONTEXT_1 = 0xA1, /**< [1] EXPLICIT/IMPLICIT */
  FIO_ASN1_CONTEXT_2 = 0xA2, /**< [2] EXPLICIT/IMPLICIT */
  FIO_ASN1_CONTEXT_3 = 0xA3, /**< [3] EXPLICIT/IMPLICIT */
} fio_asn1_tag_e;

/** ASN.1 Tag Class (bits 7-6 of tag byte) */
typedef enum {
  FIO_ASN1_CLASS_UNIVERSAL = 0,   /**< Universal (built-in types) */
  FIO_ASN1_CLASS_APPLICATION = 1, /**< Application-specific */
  FIO_ASN1_CLASS_CONTEXT = 2,     /**< Context-specific */
  FIO_ASN1_CLASS_PRIVATE = 3,     /**< Private */
} fio_asn1_class_e;

/* *****************************************************************************
Common OID Constants for X.509 and TLS
***************************************************************************** */

/* Signature Algorithms */
#define FIO_OID_SHA256_WITH_RSA   "1.2.840.113549.1.1.11"
#define FIO_OID_SHA384_WITH_RSA   "1.2.840.113549.1.1.12"
#define FIO_OID_SHA512_WITH_RSA   "1.2.840.113549.1.1.13"
#define FIO_OID_RSA_PSS           "1.2.840.113549.1.1.10"
#define FIO_OID_ECDSA_WITH_SHA256 "1.2.840.10045.4.3.2"
#define FIO_OID_ECDSA_WITH_SHA384 "1.2.840.10045.4.3.3"
#define FIO_OID_ECDSA_WITH_SHA512 "1.2.840.10045.4.3.4"
#define FIO_OID_ED25519           "1.3.101.112"
#define FIO_OID_ED448             "1.3.101.113"

/* Public Key Algorithms */
#define FIO_OID_RSA_ENCRYPTION "1.2.840.113549.1.1.1"
#define FIO_OID_EC_PUBLIC_KEY  "1.2.840.10045.2.1"

/* Elliptic Curves */
#define FIO_OID_SECP256R1 "1.2.840.10045.3.1.7"
#define FIO_OID_SECP384R1 "1.3.132.0.34"
#define FIO_OID_SECP521R1 "1.3.132.0.35"
#define FIO_OID_X25519    "1.3.101.110"
#define FIO_OID_X448      "1.3.101.111"

/* X.509 Extensions */
#define FIO_OID_SUBJECT_KEY_ID    "2.5.29.14"
#define FIO_OID_KEY_USAGE         "2.5.29.15"
#define FIO_OID_SUBJECT_ALT_NAME  "2.5.29.17"
#define FIO_OID_BASIC_CONSTRAINTS "2.5.29.19"
#define FIO_OID_CRL_DIST_POINTS   "2.5.29.31"
#define FIO_OID_CERT_POLICIES     "2.5.29.32"
#define FIO_OID_AUTH_KEY_ID       "2.5.29.35"
#define FIO_OID_EXT_KEY_USAGE     "2.5.29.37"

/* X.509 Distinguished Name Attributes */
#define FIO_OID_COMMON_NAME  "2.5.4.3"
#define FIO_OID_COUNTRY      "2.5.4.6"
#define FIO_OID_LOCALITY     "2.5.4.7"
#define FIO_OID_STATE        "2.5.4.8"
#define FIO_OID_ORGANIZATION "2.5.4.10"
#define FIO_OID_ORG_UNIT     "2.5.4.11"

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
} fio_asn1_element_s;

/** Iterator for SEQUENCE or SET contents */
typedef struct {
  const uint8_t *pos; /**< Current position */
  const uint8_t *end; /**< End of sequence */
} fio_asn1_iterator_s;

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
SFUNC const uint8_t *fio_asn1_parse(fio_asn1_element_s *elem,
                                    const uint8_t *data,
                                    size_t data_len);

/**
 * Get the total encoded length of an ASN.1 element (tag + length + content).
 *
 * @param elem Parsed element
 * @param data Original data pointer where element was parsed from
 * @return Total bytes used by the element encoding
 */
FIO_IFUNC size_t fio_asn1_element_total_len(const fio_asn1_element_s *elem,
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
SFUNC int fio_asn1_parse_integer(const fio_asn1_element_s *elem,
                                 uint64_t *value);

/**
 * Parse an ASN.1 BIT STRING element.
 *
 * @param elem Parsed element (must be BIT STRING type)
 * @param bits Output pointer to bit data (first byte is unused bits count)
 * @param bit_len Output length of bit data in bytes
 * @param unused_bits Output number of unused bits in last byte (0-7)
 * @return 0 on success, -1 on error
 */
SFUNC int fio_asn1_parse_bit_string(const fio_asn1_element_s *elem,
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
SFUNC int fio_asn1_parse_oid(const fio_asn1_element_s *elem,
                             char *buf,
                             size_t buf_len);

/**
 * Compare an ASN.1 OID element to a known OID string.
 *
 * @param elem Parsed element (must be OID type)
 * @param oid_string OID in dot notation (e.g., "1.2.840.113549.1.1.11")
 * @return 1 if match, 0 if no match
 */
SFUNC int fio_asn1_oid_eq(const fio_asn1_element_s *elem,
                          const char *oid_string);

/**
 * Parse an ASN.1 time (UTC Time or Generalized Time) to Unix timestamp.
 *
 * @param elem Parsed element (must be UTC_TIME or GENERALIZED_TIME type)
 * @param unix_time Output Unix timestamp (seconds since 1970-01-01 00:00:00
 * UTC)
 * @return 0 on success, -1 on error
 */
SFUNC int fio_asn1_parse_time(const fio_asn1_element_s *elem,
                              int64_t *unix_time);

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
FIO_IFUNC const char *fio_asn1_parse_string(const fio_asn1_element_s *elem,
                                            size_t *len);

/**
 * Parse an ASN.1 BOOLEAN element.
 *
 * @param elem Parsed element (must be BOOLEAN type)
 * @param value Output boolean value (0 = false, non-zero = true)
 * @return 0 on success, -1 on error
 */
FIO_IFUNC int fio_asn1_parse_boolean(const fio_asn1_element_s *elem,
                                     int *value);

/* *****************************************************************************
ASN.1 Parser API - Sequence/Set Iteration
***************************************************************************** */

/**
 * Initialize an iterator for a SEQUENCE or SET element.
 *
 * @param it Iterator to initialize
 * @param sequence Parsed element (must be SEQUENCE or SET)
 */
FIO_IFUNC void fio_asn1_iterator_init(fio_asn1_iterator_s *it,
                                      const fio_asn1_element_s *sequence);

/**
 * Get the next element from an iterator.
 *
 * @param it Iterator (updated to point to next element)
 * @param elem Output for parsed element
 * @return 0 if element available, -1 if end or error
 */
SFUNC int fio_asn1_iterator_next(fio_asn1_iterator_s *it,
                                 fio_asn1_element_s *elem);

/**
 * Check if iterator has more elements.
 *
 * @param it Iterator
 * @return 1 if more elements available, 0 otherwise
 */
FIO_IFUNC int fio_asn1_iterator_has_next(const fio_asn1_iterator_s *it);

/* *****************************************************************************
ASN.1 Parser API - Helper Functions
***************************************************************************** */

/**
 * Check if an element is a specific tag type.
 *
 * @param elem Parsed element
 * @param tag Expected tag (e.g., FIO_ASN1_INTEGER)
 * @return 1 if match, 0 otherwise
 */
FIO_IFUNC int fio_asn1_is_tag(const fio_asn1_element_s *elem, uint8_t tag);

/**
 * Check if an element is a context-specific tag.
 *
 * @param elem Parsed element
 * @param tag_num Context tag number (0-31)
 * @return 1 if match, 0 otherwise
 */
FIO_IFUNC int fio_asn1_is_context_tag(const fio_asn1_element_s *elem,
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
FIO_IFUNC uint8_t fio_asn1_tag_number(const fio_asn1_element_s *elem);

/* *****************************************************************************
Implementation - Inline Functions
***************************************************************************** */

/** Get total encoded length of element */
FIO_IFUNC size_t fio_asn1_element_total_len(const fio_asn1_element_s *elem,
                                            const uint8_t *data) {
  if (!elem || !data || !elem->data)
    return 0;
  return (size_t)(elem->data - data) + elem->len;
}

/** Parse string types - returns pointer to data */
FIO_IFUNC const char *fio_asn1_parse_string(const fio_asn1_element_s *elem,
                                            size_t *len) {
  if (!elem || !len)
    return NULL;
  /* Accept various string types */
  uint8_t tag_num = elem->tag & 0x1F;
  if (elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL)
    return NULL;
  switch (tag_num) {
  case FIO_ASN1_UTF8_STRING:
  case FIO_ASN1_PRINTABLE_STRING:
  case FIO_ASN1_IA5_STRING:
  case FIO_ASN1_T61_STRING:
  case FIO_ASN1_VISIBLE_STRING:
  case FIO_ASN1_GENERAL_STRING:
  case FIO_ASN1_UNIVERSAL_STRING:
  case FIO_ASN1_BMP_STRING:
  case FIO_ASN1_NUMERIC_STRING:
    *len = elem->len;
    return (const char *)elem->data;
  default: return NULL;
  }
}

/** Parse boolean value */
FIO_IFUNC int fio_asn1_parse_boolean(const fio_asn1_element_s *elem,
                                     int *value) {
  if (!elem || !value)
    return -1;
  if ((elem->tag & 0x1F) != FIO_ASN1_BOOLEAN ||
      elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL)
    return -1;
  if (elem->len != 1)
    return -1;
  *value = (elem->data[0] != 0);
  return 0;
}

/** Initialize iterator for sequence/set */
FIO_IFUNC void fio_asn1_iterator_init(fio_asn1_iterator_s *it,
                                      const fio_asn1_element_s *sequence) {
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
FIO_IFUNC int fio_asn1_iterator_has_next(const fio_asn1_iterator_s *it) {
  return (it && it->pos && it->end && it->pos < it->end);
}

/** Check if element matches tag */
FIO_IFUNC int fio_asn1_is_tag(const fio_asn1_element_s *elem, uint8_t tag) {
  if (!elem)
    return 0;
  /* For universal tags, compare the tag number */
  if (elem->tag_class == FIO_ASN1_CLASS_UNIVERSAL)
    return (elem->tag & 0x1F) == (tag & 0x1F);
  /* For other classes, compare full tag byte */
  return elem->tag == tag;
}

/** Check if element is context-specific tag */
FIO_IFUNC int fio_asn1_is_context_tag(const fio_asn1_element_s *elem,
                                      uint8_t tag_num) {
  if (!elem)
    return 0;
  return (elem->tag_class == FIO_ASN1_CLASS_CONTEXT) &&
         ((elem->tag & 0x1F) == (tag_num & 0x1F));
}

/** Get tag number from element */
FIO_IFUNC uint8_t fio_asn1_tag_number(const fio_asn1_element_s *elem) {
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
FIO_SFUNC const uint8_t *fio___asn1_parse_length(const uint8_t *data,
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
SFUNC const uint8_t *fio_asn1_parse(fio_asn1_element_s *elem,
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
  p = fio___asn1_parse_length(p, end, &content_len);
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

SFUNC int fio_asn1_parse_integer(const fio_asn1_element_s *elem,
                                 uint64_t *value) {
  if (!elem)
    return -1;

  /* Verify it's an INTEGER */
  if (elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_ASN1_INTEGER)
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

SFUNC int fio_asn1_parse_bit_string(const fio_asn1_element_s *elem,
                                    const uint8_t **bits,
                                    size_t *bit_len,
                                    uint8_t *unused_bits) {
  if (!elem || !bits || !bit_len || !unused_bits)
    return -1;

  /* Verify it's a BIT STRING */
  if (elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_ASN1_BIT_STRING)
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
FIO_SFUNC const uint8_t *fio___asn1_parse_oid_component(const uint8_t *p,
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
FIO_SFUNC int fio___asn1_write_uint(char *buf, size_t buf_len, uint64_t value) {
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

SFUNC int fio_asn1_parse_oid(const fio_asn1_element_s *elem,
                             char *buf,
                             size_t buf_len) {
  if (!elem || !buf || buf_len < 4)
    return -1;

  /* Verify it's an OID */
  if (elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL ||
      (elem->tag & 0x1F) != FIO_ASN1_OID)
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

  int len = fio___asn1_write_uint(out, (size_t)(out_end - out), c1);
  if (len < 0)
    return -1;
  out += len;
  total += len;

  if (out >= out_end)
    return -1;
  *out++ = '.';
  total++;

  len = fio___asn1_write_uint(out, (size_t)(out_end - out), c2);
  if (len < 0)
    return -1;
  out += len;
  total += len;

  /* Remaining components */
  while (p < end) {
    uint64_t component;
    p = fio___asn1_parse_oid_component(p, end, &component);
    if (!p)
      return -1;

    if (out >= out_end)
      return -1;
    *out++ = '.';
    total++;

    len = fio___asn1_write_uint(out, (size_t)(out_end - out), component);
    if (len < 0)
      return -1;
    out += len;
    total += len;
  }

  *out = '\0';
  return total;
}

SFUNC int fio_asn1_oid_eq(const fio_asn1_element_s *elem,
                          const char *oid_string) {
  if (!elem || !oid_string)
    return 0;

  /* Parse OID to string and compare */
  char buf[128];
  int len = fio_asn1_parse_oid(elem, buf, sizeof(buf));
  if (len < 0)
    return 0;

  /* Compare strings */
  size_t oid_len = FIO_STRLEN(oid_string);
  if ((size_t)len != oid_len)
    return 0;

  return FIO_MEMCMP(buf, oid_string, oid_len) == 0;
}

/* *****************************************************************************
Implementation - Time Parser
***************************************************************************** */

/** Parse 2-digit number from string */
FIO_SFUNC int fio___asn1_parse_2digits(const char *p, int *value) {
  if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9')
    return -1;
  *value = (p[0] - '0') * 10 + (p[1] - '0');
  return 0;
}

/** Days in each month (non-leap year) */
static const int fio___days_in_month[] =
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/** Check if year is leap year */
FIO_SFUNC int fio___is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/** Convert broken-down time to Unix timestamp */
FIO_SFUNC int64_t
fio___asn1_mktime(int year, int month, int day, int hour, int min, int sec) {
  /* Validate ranges */
  if (month < 1 || month > 12)
    return -1;
  if (day < 1 || day > 31)
    return -1;
  if (hour < 0 || hour > 23)
    return -1;
  if (min < 0 || min > 59)
    return -1;
  if (sec < 0 || sec > 60) /* Allow leap second */
    return -1;

  /* Calculate days since Unix epoch (1970-01-01) */
  int64_t days = 0;

  /* Years from 1970 to year-1 */
  for (int y = 1970; y < year; ++y)
    days += fio___is_leap_year(y) ? 366 : 365;
  for (int y = year; y < 1970; ++y)
    days -= fio___is_leap_year(y) ? 366 : 365;

  /* Months in current year */
  for (int m = 1; m < month; ++m) {
    days += fio___days_in_month[m - 1];
    if (m == 2 && fio___is_leap_year(year))
      days += 1;
  }

  /* Days in current month */
  days += day - 1;

  /* Convert to seconds */
  int64_t ts = days * 86400LL + hour * 3600LL + min * 60LL + sec;
  return ts;
}

SFUNC int fio_asn1_parse_time(const fio_asn1_element_s *elem,
                              int64_t *unix_time) {
  if (!elem || !unix_time)
    return -1;

  uint8_t tag = elem->tag & 0x1F;
  if (elem->tag_class != FIO_ASN1_CLASS_UNIVERSAL)
    return -1;

  const char *p = (const char *)elem->data;
  size_t len = elem->len;

  int year, month, day, hour, min, sec = 0;

  if (tag == FIO_ASN1_UTC_TIME) {
    /* UTCTime: YYMMDDhhmmZ or YYMMDDhhmmssZ */
    if (len != 11 && len != 13)
      return -1;

    int yy;
    if (fio___asn1_parse_2digits(p, &yy) < 0)
      return -1;
    /* RFC 5280: years 00-49 = 2000-2049, 50-99 = 1950-1999 */
    year = (yy < 50) ? 2000 + yy : 1900 + yy;
    p += 2;

    if (fio___asn1_parse_2digits(p, &month) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &day) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &hour) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &min) < 0)
      return -1;
    p += 2;

    if (len == 13) {
      if (fio___asn1_parse_2digits(p, &sec) < 0)
        return -1;
      p += 2;
    }

    if (*p != 'Z')
      return -1;
  } else if (tag == FIO_ASN1_GENERALIZED_TIME) {
    /* GeneralizedTime: YYYYMMDDhhmmssZ */
    if (len < 15)
      return -1;

    int yy1, yy2;
    if (fio___asn1_parse_2digits(p, &yy1) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &yy2) < 0)
      return -1;
    p += 2;
    year = yy1 * 100 + yy2;

    if (fio___asn1_parse_2digits(p, &month) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &day) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &hour) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &min) < 0)
      return -1;
    p += 2;
    if (fio___asn1_parse_2digits(p, &sec) < 0)
      return -1;
    p += 2;

    /* Skip optional fractional seconds */
    if (*p == '.') {
      p++;
      while (*p >= '0' && *p <= '9')
        p++;
    }

    if (*p != 'Z')
      return -1;
  } else {
    return -1;
  }

  *unix_time = fio___asn1_mktime(year, month, day, hour, min, sec);
  return (*unix_time == -1) ? -1 : 0;
}

/* *****************************************************************************
Implementation - Iterator
***************************************************************************** */

SFUNC int fio_asn1_iterator_next(fio_asn1_iterator_s *it,
                                 fio_asn1_element_s *elem) {
  if (!it || !elem || !it->pos || !it->end || it->pos >= it->end)
    return -1;

  size_t remaining = (size_t)(it->end - it->pos);
  const uint8_t *next = fio_asn1_parse(elem, it->pos, remaining);
  if (!next)
    return -1;

  it->pos = next;
  return 0;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ASN1 */
#undef FIO_ASN1
