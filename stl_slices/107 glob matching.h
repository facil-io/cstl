/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#define FIO_GLOB_MATCH              /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










      A packet based data stream for storing / buffering endless data.










***************************************************************************** */
#if defined(FIO_GLOB_MATCH) && !defined(H___FIO_GLOB_MATCH___H)
#define H___FIO_GLOB_MATCH___H

/* *****************************************************************************
Globe Matching API
***************************************************************************** */

/** A binary glob matching helper. Returns 1 on match, otherwise returns 0. */
SFUNC uint8_t fio_glob_match(fio_str_info_s pattern, fio_str_info_s string);

/* *****************************************************************************








                          Globe Matching Implementation








***************************************************************************** */

/* *****************************************************************************
Globe Matching Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
 * Glob Matching
 **************************************************************************** */

/** A binary glob matching helper. Returns 1 on match, otherwise returns 0. */
SFUNC uint8_t fio_glob_match(fio_str_info_s pat, fio_str_info_s str) {
  /* adapted and rewritten, with thankfulness, from the code at:
   * https://github.com/opnfv/kvmfornfv/blob/master/kernel/lib/glob.c
   *
   * Original version's copyright:
   * Copyright 2015 Open Platform for NFV Project, Inc. and its contributors
   * Under the MIT license.
   */

  /*
   * Backtrack to previous * on mismatch and retry starting one
   * character later in the string.  Because * matches all characters,
   * there's never a need to backtrack multiple levels.
   */
  uint8_t *back_pat = NULL, *back_str = (uint8_t *)str.buf;
  size_t back_pat_len = 0, back_str_len = str.len;

  /*
   * Loop over each token (character or class) in pat, matching
   * it against the remaining unmatched tail of str.  Return false
   * on mismatch, or true after matching the trailing nul bytes.
   */
  while (str.len && pat.len) {
    uint8_t c = *(uint8_t *)str.buf++;
    uint8_t d = *(uint8_t *)pat.buf++;
    str.len--;
    pat.len--;

    switch (d) {
    case '?': /* Wildcard: anything goes */
      break;

    case '*':       /* Any-length wildcard */
      if (!pat.len) /* Optimize trailing * case */
        return 1;
      back_pat = (uint8_t *)pat.buf;
      back_pat_len = pat.len;
      back_str = (uint8_t *)--str.buf; /* Allow zero-length match */
      back_str_len = ++str.len;
      break;

    case '[': { /* Character class */
      uint8_t match = 0, inverted = (*(uint8_t *)pat.buf == '^' ||
                                     *(uint8_t *)pat.buf == '!');
      uint8_t *cls = (uint8_t *)pat.buf + inverted;
      uint8_t a = *cls++;

      /*
       * Iterate over each span in the character class.
       * A span is either a single character a, or a
       * range a-b.  The first span may begin with ']'.
       */
      do {
        uint8_t b = a;
        if (a == '\\') { /* when escaped, next character is regular */
          b = a = *(cls++);
        } else if (cls[0] == '-' && cls[1] != ']') {
          b = cls[1];

          cls += 2;
          if (a > b) {
            uint8_t tmp = a;
            a = b;
            b = tmp;
          }
        }
        match |= (a <= c && c <= b);
      } while ((a = *cls++) != ']');

      if (match == inverted)
        goto backtrack;
      pat.len -= cls - (uint8_t *)pat.buf;
      pat.buf = (char *)cls;

    } break;
    case '\\':
      d = *(uint8_t *)pat.buf++;
      pat.len--;
    /* fallthrough */
    default: /* Literal character */
      if (c == d)
        break;
    backtrack:
      if (!back_pat)
        return 0; /* No point continuing */
      /* Try again from last *, one character later in str. */
      pat.buf = (char *)back_pat;
      str.buf = (char *)++back_str;
      str.len = --back_str_len;
      pat.len = back_pat_len;
    }
  }
  /* if the trailing pattern allows for empty data, skip it */
  while (pat.len && pat.buf[0] == '*') {
    ++pat.buf;
    --pat.len;
  }
  return !str.len && !pat.len;
}

/* *****************************************************************************
Globe Matching Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, glob_matching)(void) {
  struct {
    char *pat;
    char *str;
    uint8_t expect;
  } t[] = {
      /* test empty string */
      {.pat = "", .str = "", .expect = 1},
      /* test exact match */
      {.pat = "a", .str = "a", .expect = 1},
      /* test empty pattern */
      {.pat = "", .str = "a", .expect = 0},
      /* test longer pattern */
      {.pat = "a", .str = "", .expect = 0},
      /* test empty string with glob pattern */
      {.pat = "*", .str = "", .expect = 1},
      /* test glob pattern */
      {.pat = "*", .str = "Whatever", .expect = 1},
      /* test glob pattern at end */
      {.pat = "W*", .str = "Whatever", .expect = 1},
      /* test glob pattern as bookends */
      {.pat = "*Whatever*", .str = "Whatever", .expect = 1},
      /* test glob pattern in the middle */
      {.pat = "W*er", .str = "Whatever", .expect = 1},
      /* test glob pattern in the middle - empty match*/
      {.pat = "W*hatever", .str = "Whatever", .expect = 1},
      /* test glob pattern in the middle  - no match */
      {.pat = "W*htever", .str = "Whatever", .expect = 0},
      /* test partial match with glob at end */
      {.pat = "h*", .str = "Whatever", .expect = 0},
      /* test partial match with glob in the middle */
      {.pat = "h*er", .str = "Whatever", .expect = 0},
      /* test glob match with "?"  */
      {.pat = "?h*er", .str = "Whatever", .expect = 1},
      /* test "?" for length restrictions */
      {.pat = "?", .str = "Whatever", .expect = 0},
      /* test ? in the middle */
      {.pat = "What?ver", .str = "Whatever", .expect = 1},
      /* test letter list */
      {.pat = "[ASW]hat?ver", .str = "Whatever", .expect = 1},
      /* test letter range */
      {.pat = "[A-Z]hat?ver", .str = "Whatever", .expect = 1},
      /* test letter range (fail) */
      {.pat = "[a-z]hat?ver", .str = "Whatever", .expect = 0},
      /* test inverted letter range */
      {.pat = "[!a-z]hat?ver", .str = "Whatever", .expect = 1},
      /* test inverted list */
      {.pat = "[!F]hat?ver", .str = "Whatever", .expect = 1},
      /* test escaped range */
      {.pat = "[!a-z\\]]hat?ver", .str = "Whatever", .expect = 1},
      /* test "?" after range (no skip) */
      {.pat = "[A-Z]?at?ver", .str = "Whatever", .expect = 1},
      /* test error after range (no skip) */
      {.pat = "[A-Z]Fat?ver", .str = "Whatever", .expect = 0},
      /* end of test marker */
      {.pat = NULL, .str = NULL, .expect = 0},
  };
  fprintf(stderr, "* testing glob matching.\n");
  for (size_t i = 0; t[i].pat; ++i) {
    fio_str_info_s p, s;
    p.buf = t[i].pat;
    p.len = strlen(t[i].pat);
    s.buf = t[i].str;
    s.len = strlen(t[i].str);
    FIO_ASSERT(t[i].expect == fio_glob_match(p, s),
               "glob matching error for:\n\t String: %s\n\t Pattern: %s",
               s.buf,
               p.buf);
  }
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_GLOB_MATCH_MONITOR_MAX
#endif /* FIO_GLOB_MATCH */
#undef FIO_GLOB_MATCH
