/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ENTITY             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          ML Entity Decoding



Provides `fio_entity` for decoding a single Markup Language entity
(`&name;`, `&#digits;`, `&#xhex;`) into its UTF-8 representation.

The entity table currently supports ~40 common named entities. The table
shape is designed for a future upgrade to static imap lookup.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ENTITY) && !defined(H___FIO_ENTITY___H)
#define H___FIO_ENTITY___H

/* Dependency: fio_atol10u / fio_atol16u for numeric entity parsing. */
#ifndef H___FIO_ATOL___H
#define FIO_ATOL
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#endif

/**
 * Decodes a single ML entity from `src` (length `len`, starting at '&').
 *
 * Writes decoded UTF-8 bytes to `dest`, which MUST be at least 8 bytes.
 * A NUL terminator is written only if the decoded result is shorter than
 * 8 bytes.
 *
 * Returns the number of bytes written to `dest` (excluding NUL), or 0 if
 * `src` does not start with a valid entity.
 *
 * Supported forms:
 *   - Named:   `&name;`   (case-insensitive, ~40 common entities)
 *   - Decimal: `&#digits;`
 *   - Hex:     `&#xhex;` or `&#Xhex;`
 */
SFUNC size_t fio_entity(char *dest, const char *src, size_t len);

/* *****************************************************************************
ML Entity Decoding — Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/** Named entity entry. `name` is lowercase. `r` is UTF-8. */
typedef struct {
  char name[32];
  uint8_t nlen;
  uint8_t rlen;
  uint8_t r[8];
} fio___entity_s;

static const fio___entity_s fio___entity_table[] = {
#define FIO___ENTITY(n, result)                                                \
  {.name = n, .nlen = (uint8_t)(sizeof(n) - 1),                               \
   .rlen = (uint8_t)(sizeof(result) - 1), .r = result}
    FIO___ENTITY("lt", "<"),
    FIO___ENTITY("gt", ">"),
    FIO___ENTITY("amp", "&"),
    FIO___ENTITY("apos", "'"),
    FIO___ENTITY("quot", "\""),
    FIO___ENTITY("nbsp", "\xC2\xA0"),
    FIO___ENTITY("tab", "\t"),
    FIO___ENTITY("ge", "\xE2\x89\xA5"),
    FIO___ENTITY("le", "\xE2\x89\xA4"),
    FIO___ENTITY("ne", "\xE2\x89\xA0"),
    FIO___ENTITY("copy", "\xC2\xA9"),
    FIO___ENTITY("aelig", "\xC3\x86"),
    FIO___ENTITY("aacute", "\xC3\x81"),
    FIO___ENTITY("dcaron", "\xC4\x8E"),
    FIO___ENTITY("ouml", "\xC3\xB6"),
    FIO___ENTITY("fjlig", "fj"),
    FIO___ENTITY("hilbertspace", "\xE2\x84\x8B"),
    FIO___ENTITY("differentiald", "\xE2\x85\x86"),
    FIO___ENTITY("clockwisecontourintegral", "\xE2\x88\xB2"),
    FIO___ENTITY("nge", "\xE2\x89\xA7\xCC\xB8"),
    FIO___ENTITY("raquo", "\xC2\xBB"),
    FIO___ENTITY("laquo", "\xC2\xAB"),
    FIO___ENTITY("rdquo", "\xE2\x80\x9D"),
    FIO___ENTITY("ldquo", "\xE2\x80\x9C"),
    FIO___ENTITY("reg", "\xC2\xAE"),
    FIO___ENTITY("asymp", "\xE2\x89\x88"),
    FIO___ENTITY("bdquo", "\xE2\x80\x9E"),
    FIO___ENTITY("bull", "\xE2\x80\xA2"),
    FIO___ENTITY("cent", "\xC2\xA2"),
    FIO___ENTITY("euro", "\xE2\x82\xAC"),
    FIO___ENTITY("dagger", "\xE2\x80\xA0"),
    FIO___ENTITY("deg", "\xC2\xB0"),
    FIO___ENTITY("frac14", "\xC2\xBC"),
    FIO___ENTITY("frac12", "\xC2\xBD"),
    FIO___ENTITY("frac34", "\xC2\xBE"),
    FIO___ENTITY("hellip", "\xE2\x80\xA6"),
    FIO___ENTITY("lsquo", "\xE2\x80\x98"),
    FIO___ENTITY("mdash", "\xE2\x80\x94"),
    FIO___ENTITY("middot", "\xC2\xB7"),
    FIO___ENTITY("ndash", "\xE2\x80\x93"),
    FIO___ENTITY("para", "\xC2\xB6"),
    FIO___ENTITY("plusmn", "\xC2\xB1"),
    FIO___ENTITY("pound", "\xC2\xA3"),
    FIO___ENTITY("prime", "\xE2\x80\xB2"),
    FIO___ENTITY("rsquo", "\xE2\x80\x99"),
    FIO___ENTITY("sbquo", "\xE2\x80\x9A"),
    FIO___ENTITY("sect", "\xC2\xA7"),
    FIO___ENTITY("trade", "\xE2\x84\xA2"),
    FIO___ENTITY("yen", "\xC2\xA5"),
#undef FIO___ENTITY
};

#define FIO___ENTITY_TABLE_LEN                                                 \
  (sizeof(fio___entity_table) / sizeof(fio___entity_table[0]))

SFUNC size_t fio_entity(char *dest, const char *src, size_t len) {
  if (!dest || !src || len < 3 || *src != '&')
    return 0;
  const char *p = src + 1;
  const char *end = src + len;

  /* ── Numeric: &#digits; or &#xhex; ── */
  if (*p == '#') {
    ++p;
    if (p >= end)
      return 0;
    int is_hex = (*p == 'x' || *p == 'X');
    p += is_hex;
    char *digits = (char *)p;
    uint64_t codepoint =
        (is_hex ? fio_atol16u : fio_atol10u)(&digits);
    if (digits == (char *)p)
      return 0; /* no digits */
    p = digits;
    if (p >= end || *p != ';')
      return 0;
    if (codepoint == 0)
      codepoint = 0xFFFD; /* replacement char for &#0; */
    if (codepoint > 0x10FFFF)
      return 0;
    size_t written = fio_utf8_write((uint8_t *)dest, (uint32_t)codepoint);
    if (written < 8)
      dest[written] = '\0';
    return written;
  }

  /* ── Named: &name; ── */
  {
    const char *name_start = p;
    while (p < end && *p != ';' &&
           (size_t)(p - name_start) < sizeof(fio___entity_table[0].name))
      ++p;
    if (p >= end || *p != ';')
      return 0;
    uint8_t name_len = (uint8_t)(p - name_start);
    for (size_t i = 0; i < FIO___ENTITY_TABLE_LEN; ++i) {
      if (fio___entity_table[i].nlen != name_len)
        continue;
      int match = 1;
      for (uint8_t j = 0; j < name_len; ++j)
        match &= ((name_start[j] | 32) ==
                  (fio___entity_table[i].name[j] | 32));
      if (!match)
        continue;
      uint8_t rlen = fio___entity_table[i].rlen;
      FIO_MEMCPY(dest, fio___entity_table[i].r, rlen);
      if (rlen < 8)
        dest[rlen] = '\0';
      return rlen;
    }
    return 0; /* unknown named entity */
  }
}

#undef FIO___ENTITY_TABLE_LEN
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ENTITY */
