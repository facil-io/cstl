/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_GLOB_MATCH         /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            Globe Matching



Copyright and License: see header file (000 copyright.h) or top of file
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
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
    case '?': /* Wildcard: anything goes */ break;

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
    case '\\': d = *(uint8_t *)pat.buf++; pat.len--;
    /* fall through */
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
      // clang-format off
      /* test empty string */
      {.pat = (char *)"", .str = (char *)"", .expect = 1},
      /* test exact match */
      {.pat = (char *)"a", .str = (char *)"a", .expect = 1},
      /* test empty pattern */
      {.pat = (char *)"", .str = (char *)"a", .expect = 0},
      /* test longer pattern */
      {.pat = (char *)"a", .str = (char *)"", .expect = 0},
      /* test empty string with glob pattern */
      {.pat = (char *)"*", .str = (char *)"", .expect = 1},
      /* test glob pattern */
      {.pat = (char *)"*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern at end */
      {.pat = (char *)"W*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern as bookends */
      {.pat = (char *)"*Whatever*", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle */
      {.pat = (char *)"W*er", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle - empty match*/
      {.pat = (char *)"W*hatever", .str = (char *)"Whatever", .expect = 1},
      /* test glob pattern in the middle  - no match */
      {.pat = (char *)"W*htever", .str = (char *)"Whatever", .expect = 0},
      /* test partial match with glob at end */
      {.pat = (char *)"h*", .str = (char *)"Whatever", .expect = 0},
      /* test partial match with glob in the middle */
      {.pat = (char *)"h*er", .str = (char *)"Whatever", .expect = 0},
      /* test glob match with "?"  */
      {.pat = (char *)"?h*er", .str = (char *)"Whatever", .expect = 1},
      /* test "?" for length restrictions */
      {.pat = (char *)"?", .str = (char *)"Whatever", .expect = 0},
      /* test ? in the middle */
      {.pat = (char *)"What?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter list */
      {.pat = (char *)"[ASW]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter range */
      {.pat = (char *)"[A-Z]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test letter range (fail) */
      {.pat = (char *)"[a-z]hat?ver", .str = (char *)"Whatever", .expect = 0},
      /* test inverted letter range */
      {.pat = (char *)"[!a-z]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test inverted list */
      {.pat = (char *)"[!F]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test escaped range */
      {.pat = (char *)"[!a-z\\]]hat?ver", .str = (char *)"Whatever", .expect = 1},
      /* test "?" after range (no skip) */
      {.pat = (char *)"[A-Z]?at?ver", .str = (char *)"Whatever", .expect = 1},
      /* test error after range (no skip) */
      {.pat = (char *)"[A-Z]Fat?ver", .str = (char *)"Whatever", .expect = 0},
      /* end of test marker */
      {.pat = (char *)NULL, .str = (char *)NULL, .expect = 0},
      // clang-format on
  };
  fprintf(stderr, "* Testing glob matching.\n");
  for (size_t i = 0; t[i].pat; ++i) {
    fio_str_info_s p = FIO_STR_INFO1(t[i].pat);
    fio_str_info_s s = FIO_STR_INFO1(t[i].str);
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
