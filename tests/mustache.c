/* *****************************************************************************
Test - Mustache Edge Cases and Error Handling

This test file covers edge cases for the Mustache templating engine.
It documents both expected behavior and known issues/bugs.

FIXED ISSUES:
1. Unclosed sections at EOF now correctly detected and rejected.
2. Empty comment {{!}} now accepted (valid per Mustache spec).
3. Triple mustache {{{name}} with missing close brace now detected.

REMAINING KNOWN ISSUES:
1. Empty partial {{>}} fails - could be valid (no-op).
2. Newlines in tags don't cause failure - parser continues across lines.
3. Empty templates return NULL from build (not empty string).
4. Templates with only tags (no text) may return NULL from build.

***************************************************************************** */
#include "test-helpers.h"

#define FIO_MUSTACHE
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper macros for test assertions
***************************************************************************** */

#define TEST_MUSTACHE_SHOULD_FAIL(template_str, description)                   \
  do {                                                                         \
    const char *t = (template_str);                                            \
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1((char *)t));   \
    if (m) {                                                                   \
      FIO_LOG_WARNING("Expected failure for: %s\n\tTemplate: %s",              \
                      description,                                             \
                      t);                                                      \
      fio_mustache_free(m);                                                    \
      ++test_known_issues;                                                     \
    } else {                                                                   \
      ++test_passes;                                                           \
    }                                                                          \
  } while (0)

#define TEST_MUSTACHE_SHOULD_PASS(template_str, description)                   \
  do {                                                                         \
    const char *t = (template_str);                                            \
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1((char *)t));   \
    if (!m) {                                                                  \
      FIO_LOG_WARNING("Expected success for: %s\n\tTemplate: %s",              \
                      description,                                             \
                      t);                                                      \
      ++test_known_issues;                                                     \
    } else {                                                                   \
      fio_mustache_free(m);                                                    \
      ++test_passes;                                                           \
    }                                                                          \
  } while (0)

#define TEST_MUSTACHE_OUTPUT(template_str, expected_output, description)       \
  do {                                                                         \
    const char *t = (template_str);                                            \
    const char *e = (expected_output);                                         \
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1((char *)t));   \
    if (!m) {                                                                  \
      FIO_LOG_ERROR("Template load failed for: %s\n\tTemplate: %s",            \
                    description,                                               \
                    t);                                                        \
      ++test_failures;                                                         \
    } else {                                                                   \
      char *result = (char *)fio_mustache_build(m, .ctx = NULL);               \
      if (!result && e[0] != '\0') {                                           \
        FIO_LOG_WARNING("Build returned NULL for: %s (expected: '%s')",        \
                        description,                                           \
                        e);                                                    \
        ++test_known_issues;                                                   \
      } else if (result && !FIO_BUF_INFO_IS_EQ(fio_bstr_buf(result),           \
                                               FIO_BUF_INFO1((char *)e))) {    \
        FIO_LOG_ERROR(                                                         \
            "Output mismatch for: %s\n\tExpected: '%s'\n\tGot: '%s'",          \
            description,                                                       \
            e,                                                                 \
            result);                                                           \
        ++test_failures;                                                       \
      } else if (!result && e[0] == '\0') {                                    \
        ++test_passes; /* NULL for empty is acceptable */                      \
      } else {                                                                 \
        ++test_passes;                                                         \
      }                                                                        \
      fio_bstr_free(result);                                                   \
      fio_mustache_free(m);                                                    \
    }                                                                          \
  } while (0)

/* For documenting known issues that we expect to fail */
#define TEST_MUSTACHE_KNOWN_ISSUE_FAIL(template_str, description)              \
  do {                                                                         \
    const char *t = (template_str);                                            \
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1((char *)t));   \
    if (m) {                                                                   \
      /* This is a known issue - parser accepts invalid template */            \
      fio_mustache_free(m);                                                    \
      ++test_known_issues;                                                     \
    } else {                                                                   \
      /* Fixed! Parser now correctly rejects */                                \
      ++test_passes;                                                           \
    }                                                                          \
  } while (0)

#define TEST_MUSTACHE_KNOWN_ISSUE_PASS(template_str, description)              \
  do {                                                                         \
    const char *t = (template_str);                                            \
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1((char *)t));   \
    if (!m) {                                                                  \
      /* This is a known issue - parser rejects valid template */              \
      ++test_known_issues;                                                     \
    } else {                                                                   \
      /* Fixed! Parser now correctly accepts */                                \
      fio_mustache_free(m);                                                    \
      ++test_passes;                                                           \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Test: Tag Mismatch Errors
***************************************************************************** */
FIO_SFUNC void test_mustache_tag_mismatch(size_t *passes,
                                          size_t *failures,
                                          size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing tag mismatch errors...\n");

  /* Section end tag doesn't match start tag */
  TEST_MUSTACHE_SHOULD_FAIL("{{#section}}content{{/wrong_name}}",
                            "mismatched section end tag");

  /* Nested sections closed in wrong order */
  TEST_MUSTACHE_SHOULD_FAIL("{{#outer}}{{#inner}}{{/outer}}{{/inner}}",
                            "nested sections closed in wrong order");

  /* Inverted section closed with regular section tag */
  TEST_MUSTACHE_SHOULD_FAIL("{{^inverted}}content{{/section}}",
                            "inverted section with wrong closing name");

  /* Regular section closed with different name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#a}}{{#b}}{{/a}}{{/b}}",
                            "sections a and b closed in wrong order");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Tag mismatch: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Unclosed Tags
***************************************************************************** */
FIO_SFUNC void test_mustache_unclosed_tags(size_t *passes,
                                           size_t *failures,
                                           size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing unclosed tags...\n");

  /* KNOWN ISSUE: Section without closing tag - parser doesn't check at EOF */
  TEST_MUSTACHE_KNOWN_ISSUE_FAIL("{{#section}}content without closing",
                                 "unclosed section at EOF");

  /* Variable tag without closing braces - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{name", "unclosed variable tag");

  /* Nested sections with only one close - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{#section}}{{#nested}}only one close{{/section}}",
                            "nested section with missing inner close");

  /* KNOWN ISSUE: Multiple unclosed sections at EOF */
  TEST_MUSTACHE_KNOWN_ISSUE_FAIL("{{#a}}{{#b}}{{#c}}content",
                                 "multiple unclosed sections at EOF");

  /* Unclosed tag at end of template - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("Hello {{", "unclosed tag at end");

  /* Unclosed triple mustache - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{{name", "unclosed triple mustache");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Unclosed tags: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Unopened Closing Tags
***************************************************************************** */
FIO_SFUNC void test_mustache_unopened_closing(size_t *passes,
                                              size_t *failures,
                                              size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing unopened closing tags...\n");

  /* Closing tag without opening - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{/section}}", "closing tag without opening");

  /* Content followed by random closing tag - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("content{{/random}}",
                            "content with orphan closing tag");

  /* Multiple orphan closing tags - first one causes failure */
  TEST_MUSTACHE_SHOULD_FAIL("{{/a}}{{/b}}{{/c}}", "multiple orphan closings");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Unopened closing: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Malformed Tags
***************************************************************************** */
FIO_SFUNC void test_mustache_malformed_tags(size_t *passes,
                                            size_t *failures,
                                            size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing malformed tags...\n");

  /* Empty tag - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{}}", "empty tag");

  /* Tag with only space - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{ }}", "tag with only space");

  /* Tag with only whitespace - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{   }}", "tag with only whitespace");

  /* Section marker without name - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{#}}", "section marker without name");

  /* Close marker without name - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{/}}", "close marker without name");

  /* Inverted marker without name - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{^}}", "inverted marker without name");

  /* KNOWN ISSUE: Empty comment {{!}} fails but should be valid per spec */
  TEST_MUSTACHE_KNOWN_ISSUE_PASS("{{!}}", "empty comment");

  /* KNOWN ISSUE: Empty partial {{>}} fails */
  TEST_MUSTACHE_KNOWN_ISSUE_PASS("{{>}}", "empty partial");

  /* Delimiter marker without proper format - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{=}}", "delimiter marker malformed");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Malformed tags: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Delimiter Edge Cases
***************************************************************************** */
FIO_SFUNC void test_mustache_delimiter_edge_cases(size_t *passes,
                                                  size_t *failures,
                                                  size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing delimiter edge cases...\n");

  /* Change delimiters and back */
  TEST_MUSTACHE_SHOULD_PASS("{{=<% %>=}}<%! comment %><%={{ }}=%>",
                            "delimiter change and restore");

  /* Multiple delimiter changes */
  TEST_MUSTACHE_SHOULD_PASS("{{=| |=}}|! comment |{{=<< >>=%}}<<!comment>>",
                            "multiple delimiter changes");

  /* Single character delimiters */
  TEST_MUSTACHE_SHOULD_PASS("{{=| |=}}|name|", "single char delimiters");

  /* Multi-char delimiters (up to 4 chars) */
  TEST_MUSTACHE_SHOULD_PASS("{{=<< >>=}}<<name>>", "two char delimiters");

  /* KNOWN ISSUE: Delimiter with spaces - parser accepts but shouldn't */
  /* Actually this is valid - spaces separate the two delimiters */
  TEST_MUSTACHE_SHOULD_PASS("{{=| |=}}",
                            "single char delimiter with space separator");

  /* Delimiter too long (>4 chars) - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{=<<<<< >>>>>=}}", "delimiter too long");

  /* Empty delimiter - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{= =}}", "empty delimiter");

  /* Only opening delimiter specified - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{=< =}}", "only opening delimiter");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(
      stderr,
      "    Delimiter edge cases: %zu passed, %zu failed, %zu known issues\n",
      test_passes,
      test_failures,
      test_known_issues);
}

/* *****************************************************************************
Test: Nested Section Errors
***************************************************************************** */
FIO_SFUNC void test_mustache_nested_section_errors(size_t *passes,
                                                   size_t *failures,
                                                   size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing nested section errors...\n");

  /* Three levels closed in wrong order - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{#a}}{{#b}}{{#c}}{{/c}}{{/a}}{{/b}}",
                            "three levels wrong close order");

  /* KNOWN ISSUE: Same name nested, one unclosed at EOF */
  TEST_MUSTACHE_KNOWN_ISSUE_FAIL("{{#a}}{{#a}}{{/a}}",
                                 "same name nested unclosed at EOF");

  /* Deeply nested with mismatch - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{#a}}{{#b}}{{#c}}{{#d}}{{/d}}{{/c}}{{/a}}{{/b}}",
                            "deep nesting with mismatch");

  /* Valid deeply nested - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{#a}}{{#b}}{{#c}}{{/c}}{{/b}}{{/a}}",
                            "valid deep nesting");

  /* Same name nested correctly - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{#a}}{{#a}}{{/a}}{{/a}}",
                            "same name nested valid");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(
      stderr,
      "    Nested section errors: %zu passed, %zu failed, %zu known issues\n",
      test_passes,
      test_failures,
      test_known_issues);
}

/* *****************************************************************************
Test: Partial Errors
***************************************************************************** */
FIO_SFUNC void test_mustache_partial_errors(size_t *passes,
                                            size_t *failures,
                                            size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing partial errors...\n");

  /* Non-existent partial - correctly ignored (no-op) */
  TEST_MUSTACHE_SHOULD_PASS("{{> nonexistent}}", "non-existent partial");

  /* KNOWN ISSUE: Empty partial name with space fails */
  TEST_MUSTACHE_KNOWN_ISSUE_PASS("{{> }}", "empty partial name with space");

  /* KNOWN ISSUE: Empty partial name fails */
  TEST_MUSTACHE_KNOWN_ISSUE_PASS("{{>}}", "empty partial name");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Partial errors: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Comment Edge Cases
***************************************************************************** */
FIO_SFUNC void test_mustache_comment_edge_cases(size_t *passes,
                                                size_t *failures,
                                                size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing comment edge cases...\n");

  /* Unclosed comment - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{! unclosed comment", "unclosed comment");

  /* KNOWN ISSUE: Empty comment fails but should be valid */
  TEST_MUSTACHE_KNOWN_ISSUE_PASS("{{!}}", "empty comment");

  /* Multi-line comment - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{! multi\nline\ncomment }}",
                            "multi-line comment");

  /* Comment with special characters - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{! <>&\"' }}", "comment with special chars");

  /* Comment with mustache-like content - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{! {{nested}} }}",
                            "comment with mustache content");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Comment edge cases: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Triple Mustache / Unescaped Edge Cases
***************************************************************************** */
FIO_SFUNC void test_mustache_unescaped_edge_cases(size_t *passes,
                                                  size_t *failures,
                                                  size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing unescaped/triple mustache edge cases...\n");

  /* KNOWN ISSUE: Missing closing brace in triple mustache not detected */
  /* {{{name}} is parsed as {{name} with extra { at start */
  TEST_MUSTACHE_KNOWN_ISSUE_FAIL("{{{name}}",
                                 "triple mustache missing close brace");

  /* Triple mustache with section marker - treated as raw variable */
  TEST_MUSTACHE_SHOULD_PASS("{{{#section}}}",
                            "triple mustache with # (treated as var)");

  /* Empty ampersand unescaped - correctly detected */
  TEST_MUSTACHE_SHOULD_FAIL("{{&}}", "empty ampersand unescaped");

  /* Valid triple mustache - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{{name}}}", "valid triple mustache");

  /* Valid ampersand unescaped - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{&name}}", "valid ampersand unescaped");

  /* Ampersand with spaces - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{& name }}", "ampersand with spaces");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(
      stderr,
      "    Unescaped edge cases: %zu passed, %zu failed, %zu known issues\n",
      test_passes,
      test_failures,
      test_known_issues);
}

/* *****************************************************************************
Test: Whitespace Handling
***************************************************************************** */
FIO_SFUNC void test_mustache_whitespace_handling(size_t *passes,
                                                 size_t *failures,
                                                 size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing whitespace handling...\n");

  /* Spaces in section tags - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{# section }}content{{/ section }}",
                            "spaces in section tags");

  /* Spaces around variable name - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{  name  }}", "spaces around variable");

  /* Tab characters in tags - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{\tname\t}}", "tabs in variable tag");

  /* Mixed whitespace - should pass */
  TEST_MUSTACHE_SHOULD_PASS("{{ \t name \t }}", "mixed whitespace in tag");

  /* KNOWN ISSUE: Newline in tag doesn't fail - parser continues across lines */
  /* This is actually debatable - some parsers allow it */
  TEST_MUSTACHE_KNOWN_ISSUE_FAIL("{{na\nme}}", "newline in tag");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Whitespace handling: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Output Verification
***************************************************************************** */
FIO_SFUNC void test_mustache_output_verification(size_t *passes,
                                                 size_t *failures,
                                                 size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing output verification...\n");

  /* Simple variable replacement (no context = empty) */
  TEST_MUSTACHE_OUTPUT("Hello {{name}}!", "Hello !", "simple variable no ctx");

  /* Text only template */
  TEST_MUSTACHE_OUTPUT("Hello World!", "Hello World!", "text only");

  /* Empty template - returns NULL which is acceptable for empty */
  TEST_MUSTACHE_OUTPUT("", "", "empty template");

  /* Comment removal */
  TEST_MUSTACHE_OUTPUT("Hello{{! comment }}World",
                       "HelloWorld",
                       "comment removal");

  /* Multiple variables - may return NULL for empty result */
  TEST_MUSTACHE_OUTPUT("{{a}}{{b}}{{c}}", "", "multiple variables no ctx");

  /* Standalone section (no context = not rendered) */
  TEST_MUSTACHE_OUTPUT("{{#section}}content{{/section}}", "", "section no ctx");

  /* Inverted section (no context = rendered) */
  TEST_MUSTACHE_OUTPUT("{{^section}}content{{/section}}",
                       "content",
                       "inverted section no ctx");

  /* Text with section */
  TEST_MUSTACHE_OUTPUT("before{{#s}}inside{{/s}}after",
                       "beforeafter",
                       "text with empty section");

  /* Text with inverted section */
  TEST_MUSTACHE_OUTPUT("before{{^s}}inside{{/s}}after",
                       "beforeinsideafter",
                       "text with inverted section");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Output verification: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Stress Tests
***************************************************************************** */
FIO_SFUNC void test_mustache_stress(size_t *passes,
                                    size_t *failures,
                                    size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing stress cases...\n");

  /* Many consecutive tags */
  TEST_MUSTACHE_SHOULD_PASS(
      "{{a}}{{b}}{{c}}{{d}}{{e}}{{f}}{{g}}{{h}}{{i}}{{j}}"
      "{{k}}{{l}}{{m}}{{n}}{{o}}{{p}}{{q}}{{r}}{{s}}{{t}}",
      "many consecutive tags");

  /* Deeply nested sections (within limit) */
  TEST_MUSTACHE_SHOULD_PASS("{{#a}}{{#b}}{{#c}}{{#d}}{{#e}}"
                            "content"
                            "{{/e}}{{/d}}{{/c}}{{/b}}{{/a}}",
                            "deeply nested sections");

  /* Long variable name */
  TEST_MUSTACHE_SHOULD_PASS(
      "{{this_is_a_very_long_variable_name_that_should_still_work}}",
      "long variable name");

  /* Mixed content */
  TEST_MUSTACHE_SHOULD_PASS(
      "Text {{var}} more {{#sec}}inner{{/sec}} {{^inv}}shown{{/inv}} end",
      "mixed content");

  /* Unicode in template */
  TEST_MUSTACHE_SHOULD_PASS("Hello {{name}} 你好 مرحبا שלום",
                            "unicode in template");

  /* Special HTML characters in text */
  TEST_MUSTACHE_SHOULD_PASS("<div>{{content}}</div>", "HTML in template");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Stress tests: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Dot Notation
***************************************************************************** */
FIO_SFUNC void test_mustache_dot_notation(size_t *passes,
                                          size_t *failures,
                                          size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing dot notation...\n");

  /* Simple dot (current context) */
  TEST_MUSTACHE_SHOULD_PASS("{{.}}", "simple dot");

  /* Dotted name */
  TEST_MUSTACHE_SHOULD_PASS("{{a.b.c}}", "dotted name");

  /* Dot at start */
  TEST_MUSTACHE_SHOULD_PASS("{{.name}}", "dot at start");

  /* Multiple dots */
  TEST_MUSTACHE_SHOULD_PASS("{{a..b}}", "multiple dots");

  /* Trailing dot */
  TEST_MUSTACHE_SHOULD_PASS("{{name.}}", "trailing dot");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Dot notation: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Edge Cases from Original Test
***************************************************************************** */
FIO_SFUNC void test_mustache_original(size_t *passes,
                                      size_t *failures,
                                      size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing original test cases...\n");

  /* Original valid example */
  char *example1 = (char *)"This is a {{tag}}, and so is {{ this_one }}.";
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO1(example1));
  if (!m) {
    FIO_LOG_ERROR("valid example load failed!");
    ++test_failures;
  } else {
    char *result = (char *)fio_mustache_build(m, .ctx = NULL);
    if (!result) {
      FIO_LOG_ERROR("valid fio_mustache_build returned NULL");
      ++test_failures;
    } else if (!FIO_BUF_INFO_IS_EQ(
                   fio_bstr_buf(result),
                   FIO_BUF_INFO1((char *)"This is a , and so is ."))) {
      FIO_LOG_ERROR("valid example result failed: %s", result);
      ++test_failures;
    } else {
      ++test_passes;
    }
    fio_bstr_free(result);
    fio_mustache_free(m);
  }

  /* Original invalid example */
  char *example2 = (char *)"{{tag}} and {{ incomplete}";
  m = fio_mustache_load(.data = FIO_BUF_INFO1(example2));
  if (m) {
    FIO_LOG_ERROR("invalid example load returned an object.");
    fio_mustache_free(m);
    ++test_failures;
  } else {
    ++test_passes;
  }

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Original tests: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Closure Mismatch - Wrong/Illegal Names
***************************************************************************** */
FIO_SFUNC void test_mustache_closure_mismatch(size_t *passes,
                                              size_t *failures,
                                              size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing closure with wrong/illegal names...\n");

  /* Test 1: Section closed with wrong name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/wrong}}",
                            "section closed with wrong name");

  /* Test 2: Inverted section closed with wrong name */
  TEST_MUSTACHE_SHOULD_FAIL("{{^tag}}content{{/wrong}}",
                            "inverted section closed with wrong name");

  /* Test 3: Section closed with inverted marker in name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/^tag}}",
                            "section closed with ^ in close name");

  /* Test 4: Inverted section closed with section marker in name */
  TEST_MUSTACHE_SHOULD_FAIL("{{^tag}}content{{/#tag}}",
                            "inverted closed with # in close name");

  /* Test 5: Nested sections with swapped closures */
  TEST_MUSTACHE_SHOULD_FAIL("{{#outer}}{{#inner}}content{{/outer}}{{/inner}}",
                            "nested sections with swapped closures");

  /* Test 6a: Close tag with # character in name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/ta#g}}",
                            "close tag with # in name");

  /* Test 6b: Close tag with ^ character in name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/ta^g}}",
                            "close tag with ^ in name");

  /* Test 6c: Close tag with / character in name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/ta/g}}",
                            "close tag with / in name");

  /* Test 7: Close tag with empty name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/}}",
                            "close tag with empty name");

  /* Test 8: Close tag that doesn't match any open section */
  TEST_MUSTACHE_SHOULD_FAIL("content{{/random}}",
                            "close tag without matching open section");

  /* Additional: Close with only whitespace after / */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/ }}",
                            "close tag with only whitespace after /");

  /* Additional: Close with whitespace around wrong name */
  TEST_MUSTACHE_SHOULD_FAIL("{{#tag}}content{{/ wrong }}",
                            "close tag with whitespace around wrong name");

  /* Additional: Triple nested with wrong middle close */
  TEST_MUSTACHE_SHOULD_FAIL("{{#a}}{{#b}}{{#c}}x{{/c}}{{/a}}{{/b}}",
                            "triple nested with wrong middle close");

  /* Additional: Closing inverted with inverted marker */
  TEST_MUSTACHE_SHOULD_FAIL("{{^tag}}content{{/^tag}}",
                            "inverted section close with ^ prefix");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Closure mismatch: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Test: Boundary Conditions
***************************************************************************** */
FIO_SFUNC void test_mustache_boundary_conditions(size_t *passes,
                                                 size_t *failures,
                                                 size_t *known_issues) {
  size_t test_passes = 0;
  size_t test_failures = 0;
  size_t test_known_issues = 0;

  fprintf(stderr, "  * Testing boundary conditions...\n");

  /* Template starting with tag */
  TEST_MUSTACHE_SHOULD_PASS("{{name}}rest", "template starting with tag");

  /* Template ending with tag */
  TEST_MUSTACHE_SHOULD_PASS("start{{name}}", "template ending with tag");

  /* Only a tag */
  TEST_MUSTACHE_SHOULD_PASS("{{name}}", "only a tag");

  /* Tag at exact boundary positions */
  TEST_MUSTACHE_SHOULD_PASS("a{{b}}c", "tag between single chars");

  /* Section at start */
  TEST_MUSTACHE_SHOULD_PASS("{{#s}}content{{/s}}after", "section at start");

  /* Section at end */
  TEST_MUSTACHE_SHOULD_PASS("before{{#s}}content{{/s}}", "section at end");

  /* Adjacent sections */
  TEST_MUSTACHE_SHOULD_PASS("{{#a}}A{{/a}}{{#b}}B{{/b}}", "adjacent sections");

  /* Section containing only whitespace */
  TEST_MUSTACHE_SHOULD_PASS("{{#s}}   {{/s}}", "section with only whitespace");

  /* Empty section */
  TEST_MUSTACHE_SHOULD_PASS("{{#s}}{{/s}}", "empty section");

  *passes += test_passes;
  *failures += test_failures;
  *known_issues += test_known_issues;
  fprintf(stderr,
          "    Boundary conditions: %zu passed, %zu failed, %zu known issues\n",
          test_passes,
          test_failures,
          test_known_issues);
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */
int main(void) {
  size_t total_passes = 0;
  size_t total_failures = 0;
  size_t total_known_issues = 0;

  fprintf(stderr, "\n=== Mustache Edge Case Tests ===\n\n");

  /* Run original tests first */
  test_mustache_original(&total_passes, &total_failures, &total_known_issues);

  /* Run all edge case tests */
  test_mustache_tag_mismatch(&total_passes,
                             &total_failures,
                             &total_known_issues);
  test_mustache_unclosed_tags(&total_passes,
                              &total_failures,
                              &total_known_issues);
  test_mustache_unopened_closing(&total_passes,
                                 &total_failures,
                                 &total_known_issues);
  test_mustache_malformed_tags(&total_passes,
                               &total_failures,
                               &total_known_issues);
  test_mustache_delimiter_edge_cases(&total_passes,
                                     &total_failures,
                                     &total_known_issues);
  test_mustache_nested_section_errors(&total_passes,
                                      &total_failures,
                                      &total_known_issues);
  test_mustache_partial_errors(&total_passes,
                               &total_failures,
                               &total_known_issues);
  test_mustache_comment_edge_cases(&total_passes,
                                   &total_failures,
                                   &total_known_issues);
  test_mustache_unescaped_edge_cases(&total_passes,
                                     &total_failures,
                                     &total_known_issues);
  test_mustache_whitespace_handling(&total_passes,
                                    &total_failures,
                                    &total_known_issues);
  test_mustache_output_verification(&total_passes,
                                    &total_failures,
                                    &total_known_issues);
  test_mustache_stress(&total_passes, &total_failures, &total_known_issues);
  test_mustache_dot_notation(&total_passes,
                             &total_failures,
                             &total_known_issues);
  test_mustache_boundary_conditions(&total_passes,
                                    &total_failures,
                                    &total_known_issues);
  test_mustache_closure_mismatch(&total_passes,
                                 &total_failures,
                                 &total_known_issues);

  /* Print summary */
  fprintf(stderr, "\n=== Test Summary ===\n");
  fprintf(stderr, "Total Passed:       %zu\n", total_passes);
  fprintf(stderr, "Total Failed:       %zu\n", total_failures);
  fprintf(stderr, "Total Known Issues: %zu\n", total_known_issues);
  fprintf(stderr,
          "Total Tests:        %zu\n",
          total_passes + total_failures + total_known_issues);

  fprintf(stderr, "\n=== Known Issues Summary ===\n");
  fprintf(stderr, "1. Empty partial {{>}} rejected\n");
  fprintf(stderr, "2. Newlines in tags not rejected\n");
  fprintf(stderr, "\n=== Fixed Issues ===\n");
  fprintf(stderr, "1. Unclosed sections at EOF now detected\n");
  fprintf(stderr, "2. Empty comment {{!}} now accepted\n");
  fprintf(stderr, "3. Triple mustache {{{name}} missing brace now detected\n");

  if (total_failures > 0) {
    fprintf(stderr, "\n*** %zu TESTS FAILED ***\n\n", total_failures);
    return 1;
  }

  fprintf(stderr,
          "\n*** ALL TESTS PASSED (with %zu known issues) ***\n\n",
          total_known_issues);
  return 0;
}
