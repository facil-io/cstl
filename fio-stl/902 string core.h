/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_STR Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_STR_TEST___H)
#define H___FIO_STR_TEST___H
#ifndef H___FIO_STR___H
#define FIO_STR
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

FIO_SFUNC size_t FIO_NAME_TEST(stl, string_core_ltoa)(char *buf,
                                                      int64_t i,
                                                      uint8_t base) {
  fio_str_info_s s = FIO_STR_INFO3(buf, 0, 1024);
  if (base == 16) {
    fio_string_write_hex(&s, NULL, i);
    return s.len;
  }
  if (base == 2) {
    fio_string_write_bin(&s, NULL, i);
    return s.len;
  }
  fio_string_write_i(&s, NULL, i);
  return s.len;
}

FIO_SFUNC void FIO_NAME_TEST(stl, string_core_helpers)(void) {
  fprintf(stderr, "* Testing Core String API.\n");
  { /* test basic fio_string_write functions. */
    fprintf(stderr, "* Testing Core String writing functions.\n");
    char mem[16];
    fio_str_info_s buf = FIO_STR_INFO3(mem, 0, 16);
    FIO_ASSERT(!fio_string_write(&buf, NULL, "World", 5),
               "non-truncated return should be zero for fio_string_write");
    FIO_ASSERT(mem == buf.buf && buf.len == 5 && !memcmp(buf.buf, "World", 6),
               "fio_string_write failed!");
    FIO_ASSERT(!fio_string_replace(&buf, NULL, 0, 0, "Hello ", 6),
               "non-truncated return should be zero for fio_string_replace");
    FIO_ASSERT(mem == buf.buf && buf.len == 11 &&
                   !memcmp(buf.buf, "Hello World", 12),
               "fio_string_replace failed to perform insert (index[0])!");
    fio_string_write(&buf, NULL, "Hello World", 11);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "Hello WorldHell", 16),
               "fio_string_write failed to truncate!");
    fio_string_replace(&buf, NULL, 0, 5, "Hola", 4);
    FIO_ASSERT(mem == buf.buf && buf.len == 14 &&
                   !memcmp(buf.buf, "Hola WorldHell", 15),
               "fio_string_replace at index 0 failed!");
    FIO_ASSERT(!fio_string_replace(&buf, NULL, 5, 9, "World", 5),
               "non-truncated return should be zero for fio_string_replace");
    FIO_ASSERT(mem == buf.buf && buf.len == 10 &&
                   !memcmp(buf.buf, "Hola World", 11),
               "fio_string_replace end overwrite failed!");
    fio_string_replace(&buf, NULL, 5, 0, "my beautiful", 12);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "Hola my beautif", 16),
               "fio_string_replace failed to truncate!");
    FIO_ASSERT(fio_string_replace(&buf, NULL, -11, 2, "big", 3),
               "truncation should return non-zero on fio_string_replace.");
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "Hola big beauti", 16),
               "fio_string_replace failed to truncate (negative index)!");
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_printf(&buf, NULL, "I think %d is the best answer", 42);
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 42 is t", 16),
               "fio_string_printf failed to truncate!");

    FIO_MEMSET(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    FIO_ASSERT(
        fio_string_write2(&buf,
                          NULL,
                          FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                          FIO_STRING_WRITE_NUM(42),
                          FIO_STRING_WRITE_STR1((char *)" is the best answer")),
        "truncation return value should be non-zero for fio_string_write2.");
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 42 is t", 16),
               "fio_string_write2 failed to truncate!");
    FIO_MEMSET(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    FIO_ASSERT(
        fio_string_write2(&buf,
                          NULL,
                          FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                          FIO_STRING_WRITE_HEX(42),
                          FIO_STRING_WRITE_STR1((char *)" is the best answer")),
        "truncation return value should be non-zero for fio_string_write2.");
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I think 2A is t", 16),
               "fio_string_write2 failed to truncate (hex)!");
    FIO_MEMSET(mem, 0, 16);
    buf = FIO_STR_INFO3(mem, 0, 16);
    FIO_ASSERT(
        fio_string_write2(&buf,
                          NULL,
                          FIO_STRING_WRITE_STR2((char *)"I Think ", 8),
                          FIO_STRING_WRITE_FLOAT(42.42),
                          FIO_STRING_WRITE_STR1((char *)" is the best answer")),
        "truncation return value should be non-zero for fio_string_write2.");
    FIO_ASSERT(mem == buf.buf && buf.len == 15 &&
                   !memcmp(buf.buf, "I Think 42.42 i", 16),
               "fio_string_write2 failed to truncate (float)!");
    buf = FIO_STR_INFO3(mem, 0, 16);
    fio_string_write2(&buf,
                      NULL,
                      FIO_STRING_WRITE_STR2((char *)"I think ", 8),
                      FIO_STRING_WRITE_BIN(-1LL),
                      FIO_STRING_WRITE_STR1((char *)" is the best answer"));
    FIO_ASSERT(mem == buf.buf && buf.len == 8 &&
                   !memcmp(buf.buf, "I think ", 8),
               "fio_string_write2 failed to truncate (bin)!");
  }
  { /* test numeral fio_string_write functions. */
    char mem[32];
    fio_str_info_s buf = FIO_STR_INFO3(mem, 0, 32);
    FIO_ASSERT(!fio_string_write_i(&buf, NULL, 0),
               "fio_string_write_i returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 1 && !memcmp(buf.buf, "0", 2),
               "fio_string_write_i didn't print 0!");
    FIO_ASSERT(!fio_string_write_i(&buf, NULL, -42),
               "fio_string_write_i returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 4 && !memcmp(buf.buf, "0-42", 5),
               "fio_string_write_i didn't print -24!");
    buf = FIO_STR_INFO3(mem, 0, 32);
    FIO_ASSERT(!fio_string_write_u(&buf, NULL, 0),
               "fio_string_write_u returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 1 && !memcmp(buf.buf, "0", 2),
               "fio_string_write_u didn't print 0!");
    FIO_ASSERT(!fio_string_write_u(&buf, NULL, -42LL),
               "fio_string_write_u returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 21 &&
                   !memcmp(buf.buf, "018446744073709551574", 21),
               "fio_string_write_u didn't print -24!");
    buf = FIO_STR_INFO3(mem, 0, 32);
    FIO_ASSERT(!fio_string_write_hex(&buf, NULL, 0),
               "fio_string_write_hex returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 2 && !memcmp(buf.buf, "00", 3),
               "fio_string_write_hex didn't print 0!");
    FIO_ASSERT(!fio_string_write_hex(&buf, NULL, 42),
               "fio_string_write_hex returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 4 && !memcmp(buf.buf, "002A", 5),
               "fio_string_write_hex didn't print 2A!");
    buf = FIO_STR_INFO3(mem, 0, 32);
    FIO_ASSERT(!fio_string_write_bin(&buf, NULL, 0),
               "fio_string_write_bin returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 1 && !memcmp(buf.buf, "0", 2),
               "fio_string_write_bin didn't print 0!");
    FIO_ASSERT(!fio_string_write_bin(&buf, NULL, 16),
               "fio_string_write_bin returned error!");
    FIO_ASSERT(mem == buf.buf && buf.len == 7 && !memcmp(buf.buf, "0010000", 8),
               "fio_string_write_bin didn't print 16!");
  }
  { /* Testing UTF-8 */
    fprintf(stderr, "* Testing UTF-8 support.\n");
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    fio_str_info_s utf8 = FIO_STR_INFO1((char *)utf8_sample);
    intptr_t pos = -2;
    size_t len = 2;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error for negative pos on "
               "UTF-8 data! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == (intptr_t)utf8.len - 4, /* 4 byte emoji */
               "`fio_string_utf8_select` error, negative position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 4, /* last utf-8 char is 4 byte long */
               "`fio_string_utf8_select` error, truncated length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 20;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error on UTF-8 data! "
               "(%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(pos == 4,
               "`fio_string_utf8_select` error, position invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)pos);
    FIO_ASSERT(len == 10,
               "`fio_string_utf8_select` error, length invalid on "
               "UTF-8 data! (%zd)",
               (ssize_t)len);
    pos = 1;
    len = 3;
    FIO_ASSERT(fio_string_utf8_select(utf8, &pos, &len) == 0,
               "`fio_string_utf8_select` returned error on UTF-8 data "
               "(2)! (%zd, %zu)",
               (ssize_t)pos,
               len);
    FIO_ASSERT(len ==
                   10, /* 3 UTF-8 chars: 4 byte + 4 byte + 2 byte codes == 10 */
               "`fio_string_utf8_select` error, length invalid on UTF-8 data! "
               "(%zd)",
               (ssize_t)len);
  }
  { /* testing C / JSON style escaping */
    fprintf(stderr, "* Testing C / JSON style character (un)escaping.\n");
    char mem[2048];
    fio_str_info_s unescaped = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 1024);
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    FIO_ASSERT(!fio_string_write(&unescaped,
                                 NULL,
                                 utf8_sample,
                                 FIO_STRLEN(utf8_sample)),
               "Couldn't write UTF-8 example.");
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      FIO_ASSERT(!fio_string_write(&unescaped, NULL, &c, 1),
                 "write returned an error");
    }
    FIO_ASSERT(
        !fio_string_write_escape(&encoded, NULL, unescaped.buf, unescaped.len),
        "write escape returned an error");
    FIO_ASSERT(
        !fio_string_write_unescape(&decoded, NULL, encoded.buf, encoded.len),
        "write unescape returned an error");
    FIO_ASSERT(encoded.len, "JSON encoding failed");
    FIO_ASSERT(!memcmp(encoded.buf, utf8_sample, FIO_STRLEN(utf8_sample)),
               "valid UTF-8 data shouldn't be escaped:\n%.*s\n%s",
               (int)encoded.len,
               encoded.buf,
               decoded.buf);
    FIO_ASSERT(
        unescaped.len == decoded.len,
        "C escaping roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
        unescaped.len,
        decoded.len,
        decoded.len,
        encoded.len,
        decoded.buf);
    FIO_ASSERT(!memcmp(unescaped.buf, decoded.buf, unescaped.len),
               "C escaping round-trip failed:\n %s",
               decoded.buf);
  }
  { /* testing Base64 Support */
    fprintf(stderr, "* Testing Base64 encoding / decoding.\n");
    char mem[2048];
    fio_str_info_s original = FIO_STR_INFO3(mem, 0, 512);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 512, 0, 512);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 1024, 0, 512);
    fio_string_write(&original,
                     NULL,
                     "Hello World, this is the voice of peace:)",
                     41);
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      FIO_ASSERT(!fio_string_write(&original, NULL, &c, 1),
                 "write returned an error");
    }
    FIO_ASSERT(!fio_string_write_base64enc(&encoded,
                                           NULL,
                                           original.buf,
                                           original.len,
                                           1),
               "base64 write escape returned an error");
    FIO_ASSERT(
        !fio_string_write_base64dec(&decoded, NULL, encoded.buf, encoded.len),
        "base64 write unescape returned an error");

    FIO_ASSERT(encoded.len, "Base64 encoding failed");
    FIO_ASSERT(decoded.len < encoded.len,
               "Base64 decoding failed:\n%s",
               encoded.buf);
    FIO_ASSERT(original.len == decoded.len,
               "Base64 roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
               original.len,
               decoded.len,
               decoded.len,
               encoded.len,
               decoded.buf);
    FIO_ASSERT(!memcmp(original.buf, decoded.buf, original.len),
               "Base64 round-trip failed:\n %s",
               decoded.buf);
  }
  { /* testing Base64 Support */
    fprintf(stderr, "* Testing URL (percent) encoding / decoding.\n");
    char mem[2048];
    for (size_t i = 0; i < 256; ++i) {
      mem[i] = i;
    }
    fio_str_info_s original = FIO_STR_INFO3(mem, 256, 256);
    fio_str_info_s encoded = FIO_STR_INFO3(mem + 256, 0, 1024);
    fio_str_info_s decoded = FIO_STR_INFO3(mem + 1024 + 256, 0, 257);
    FIO_ASSERT(
        !fio_string_write_url_enc(&encoded, NULL, mem, 256),
        "fio_string_write_url_enc reported an error where none was expected!");
    FIO_ASSERT(encoded.len > 256, "fio_string_write_url_enc did nothing?");
    FIO_ASSERT(
        !fio_string_write_url_dec(&decoded, NULL, encoded.buf, encoded.len),
        "fio_string_write_url_dec reported an error where none was expected!");
    FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, decoded),
               "fio_string_write_url_enc/dec roundtrip failed!");
  }
  { /* testing HTML escaping / un-escaping Support */
    fprintf(stderr, "* Testing HTML escaping / un-escaping (basic support)\n");
    char mem[3072];
    fio_str_info_s original = FIO_STR_INFO3(mem, 127, 256);
    fio_str_info_s escaped = FIO_STR_INFO3(mem + 256, 0, 2048);
    fio_str_info_s unescaped = FIO_STR_INFO3(mem + 2560, 0, 512);
    for (size_t i = 0; i < 127; ++i)
      mem[i] = (char)i;
    FIO_ASSERT(!fio_string_write_html_escape(&escaped,
                                             NULL,
                                             original.buf,
                                             original.len),
               "fio_string_write_html_escape returned an error");
    for (size_t i = 0; i < 2; ++i) {
      FIO_ASSERT(!fio_string_write_html_unescape(&unescaped,
                                                 NULL,
                                                 escaped.buf,
                                                 escaped.len),
                 "fio_string_write_html_unescape returned an error");
      FIO_ASSERT(!FIO_STR_INFO_IS_EQ(original, escaped),
                 "fio_string_write_html_escape did nothing!");
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(original, unescaped),
                 "fio_string_write_html_(un)escape roundtrip failed!");
      original.len = 0;
      fio_string_write(&original, NULL, "ÿ", FIO_STRLEN("ÿ"));
      original.buf[original.len++] = (char)0xE2; /* euro sign (UTF-8) */
      original.buf[original.len++] = (char)0x82;
      original.buf[original.len++] = (char)0xAC;
      original.buf[original.len++] = (char)0xC2; /* pounds (UTF-8) */
      original.buf[original.len++] = (char)0xA3;
      original.buf[original.len++] = (char)0xC2; /* cents (UTF-8) */
      original.buf[original.len++] = (char)0xA2;
      original.buf[original.len++] = (char)0xC2; /* copyright (UTF-8) */
      original.buf[original.len++] = (char)0xA9;
      original.buf[original.len++] = (char)0xC2; /* trademark (UTF-8) */
      original.buf[original.len++] = (char)0xAE;
      original.buf[original.len++] = (char)0xC2; /* nbsp; (UTF-8) */
      original.buf[original.len++] = (char)0xA0;
      original.buf[original.len++] = (char)0x26; /* & */
      original.buf[original.len++] = (char)0x27; /* ' */
      original.buf[original.len++] = (char)0x22; /* " */
      original.buf[original.len] = 0;
      unescaped.len = escaped.len = 0;
      fio_string_write(
          &escaped,
          NULL,
          "&#255;&eUro;&pound;&cenT&Copy;&reg&nbsp;&amp;&apos;&quot",
          56);
    }
    original.buf[original.len] = 0;
    unescaped.len = escaped.len = 0;
    escaped.capa = 8;
    FIO_ASSERT(fio_string_write_html_escape(&escaped,
                                            NULL,
                                            original.buf,
                                            original.len),
               "fio_string_write_html_escape should error on capacity");
  }
  { /* Comparison testing */
    fprintf(stderr, "* Testing comparison\n");
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"A"),
                                     FIO_STR_INFO1((char *)"")),
               "fio_string_is_greater failed for A vs __");
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"hello world"),
                                     FIO_STR_INFO1((char *)"hello worl")),
               "fio_string_is_greater failed for hello worl(d)");
    FIO_ASSERT(fio_string_is_greater(FIO_STR_INFO1((char *)"01234567"),
                                     FIO_STR_INFO1((char *)"012345664")),
               "fio_string_is_greater failed for 01234567");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)""),
                                      FIO_STR_INFO1((char *)"A")),
               "fio_string_is_greater failed for A inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"hello worl"),
                                      FIO_STR_INFO1((char *)"hello world")),
               "fio_string_is_greater failed for hello worl(d) inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"012345664"),
                                      FIO_STR_INFO1((char *)"01234567")),
               "fio_string_is_greater failed for 01234567 inv");
    FIO_ASSERT(!fio_string_is_greater(FIO_STR_INFO1((char *)"Hzzzzzzzzzz"),
                                      FIO_STR_INFO1((char *)"hello world")),
               "fio_string_is_greater failed for Hello world");
  }
  { /* testing fio_bstr helpers */
    fprintf(stderr, "* Testing fio_bstr helpers (micro test).\n");
    char *str = fio_bstr_write(NULL, "Hello", 5);
    FIO_ASSERT(fio_bstr_info(str).len == 5 &&
                   !memcmp(str, "Hello", fio_bstr_info(str).len + 1),
               "fio_bstr_write failed!");
    FIO_ASSERT(fio_bstr_is_greater(str, NULL),
               "fio_bstr_is_greater failed vs a NULL String");
    str = fio_bstr_write2(str,
                          FIO_STRING_WRITE_STR1((char *)" "),
                          FIO_STRING_WRITE_STR1((char *)"World!"));
    FIO_ASSERT(fio_bstr_info(str).len == 12 &&
                   !memcmp(str, "Hello World!", fio_bstr_info(str).len + 1),
               "fio_bstr_write2 failed!");
    /* test copy-on-write for fio_bstr_copy */
    char *s_copy = fio_bstr_copy(str);
    FIO_ASSERT(s_copy == str, "fio_bstr_copy should only copy on write");
    str = fio_bstr_write(str, "!", 1);
    FIO_ASSERT(s_copy != str, "fio_bstr_s write after copy error!");
    FIO_ASSERT(fio_bstr_len(str) > fio_bstr_len(s_copy),
               "fio_bstr copy after write length error!");
    FIO_ASSERT(!memcmp(str, s_copy, fio_bstr_len(s_copy)),
               "fio_bstr copy after write copied data error!");
    FIO_ASSERT(FIO_BUF_INFO_IS_EQ(fio_bstr_buf(s_copy),
                                  FIO_BUF_INFO2((char *)"Hello World!", 12)),
               "fio_bstr old copy corrupted?");
    fio_bstr_free(s_copy);
    fio_bstr_free(str);
  }
  { /* testing readfile */
    char *s = fio_bstr_readfile(NULL, __FILE__, 0, 0);
    FIO_ASSERT(s && fio_bstr_len(s), "fio_bstr_readfile failed");
    FIO_LOG_DEBUG("readfile returned %zu bytes, starting with:\n%s",
                  fio_bstr_len(s),
                  s);
    char *find_z = (char *)FIO_MEMCHR(s, 'Z', fio_bstr_len(s));
    if (find_z) {
      int fd = open(__FILE__, 0, "r"); // fio_filename_open(__FILE__, 0);
      FIO_ASSERT(fd != -1, "couldn't open file for testing: " __FILE__);
      size_t z_index = fio_fd_find_next(fd, 'Z', 0);
      FIO_ASSERT(z_index != FIO_FD_FIND_EOF, "fio_fd_find_next returned EOF");
      FIO_ASSERT(z_index == (size_t)(find_z - s),
                 "fio_fd_find_next index error (%zu != %zu)",
                 z_index,
                 (size_t)(find_z - s));
      close(fd);
      char *s2 = fio_bstr_getdelim_file(NULL, __FILE__, 0, 'Z', 0);
      FIO_ASSERT(fio_bstr_len(s2) == z_index + 1,
                 "fio_bstr_getdelim_file length error (%zu != %zu)?",
                 fio_bstr_len(s2),
                 z_index + 1);
      FIO_ASSERT(s2[z_index] == 'Z',
                 "fio_bstr_getdelim_file copy error?\n%s",
                 s2);
      fio_bstr_free(s2);
    } else {
      FIO_LOG_WARNING("couldn't find 'Z' after reading file (bstr)");
    }
    fio_bstr_free(s);
  }

#if !defined(DEBUG) || defined(NODEBUG)
  { /* speed testing comparison */
    char mem[4096];
    fio_str_info_s sa = FIO_STR_INFO3(mem, 0, 2047);
    fio_str_info_s sb = FIO_STR_INFO3(mem + 2048, 0, 2047);
    fio_string_readfile(&sa, NULL, __FILE__, 0, 0);
    fio_string_write(&sb, NULL, sa.buf, sa.len);
    sa.buf[sa.len - 1] += 1;
    fio_buf_info_s sa_buf = FIO_STR2BUF_INFO(sa);
    fio_buf_info_s sb_buf = FIO_STR2BUF_INFO(sb);

    const size_t test_repetitions = (1ULL << 19);
    const size_t positions[] = {(sa.len - 1), ((sa.len >> 1) - 1), 30, 0};
    for (const size_t *ppos = positions; *ppos; ++ppos) {
      sa.buf[*ppos] += 1;
      sa.len = *ppos + 1;
      sb.len = *ppos + 1;
      fprintf(stderr,
              "* Testing comparison speeds (%zu tests of %zu bytes):\n",
              test_repetitions,
              *ppos);
      clock_t start = clock();
      for (size_t i = 0; i < test_repetitions; ++i) {
        FIO_COMPILER_GUARD;
        int r = fio_string_is_greater_buf(sa_buf, sb_buf);
        FIO_ASSERT(r > 0, "fio_string_is_greater error?!");
      }
      clock_t end = clock();
      fprintf(stderr,
              "\t* fio_string_is_greater test cycles:   %zu\n",
              (size_t)(end - start));
      start = clock();
      for (size_t i = 0; i < test_repetitions; ++i) {
        FIO_COMPILER_GUARD;
        int r = memcmp(sa.buf, sb.buf, sa.len > sb.len ? sb.len : sa.len);
        if (!r)
          r = sa.len > sb.len;
        FIO_ASSERT(r > 0, "memcmp error?!");
      }
      end = clock();
      fprintf(stderr,
              "\t* memcmp libc test cycles:            %zu\n",
              (size_t)(end - start));
      start = clock();
      for (size_t i = 0; i < test_repetitions; ++i) {
        FIO_COMPILER_GUARD;
        int r = strcmp(sa.buf, sb.buf);
        FIO_ASSERT(r > 0, "strcmp error?!");
      }
      end = clock();
      fprintf(stderr,
              "\t* strcmp libc test cycles:            %zu\n",
              (size_t)(end - start));
    }

    fprintf(stderr, "* Testing fio_string_write_(i|u|hex|bin) speeds:\n");
    FIO_NAME_TEST(stl, atol_speed)
    ("fio_string_write/fio_atol",
     fio_atol,
     FIO_NAME_TEST(stl, string_core_ltoa));
  }
#endif /* DEBUG */
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
