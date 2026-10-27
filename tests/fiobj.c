/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"
#define FIO_BASIC
#define FIO_FIOBJ
#include FIO_INCLUDE_FILE

#define FIOBJ_TEST_REPETITIONS 1024

FIO_SFUNC int FIO_NAME_TEST(stl, fiobj_task)(fiobj_each_s *e) {
  static size_t index = 0;
  if (!e) {
    index = 0;
    return -1;
  }
  int *expect = (int *)e->udata;
  FIO_ASSERT(e->key == FIOBJ_INVALID, "key is set in an Array loop?");
  if (expect[index] == -1) {
    FIO_ASSERT(FIOBJ_TYPE(e->value) == FIOBJ_T_ARRAY,
               "each2 ordering issue [%zu] (array).",
               index);
  } else {
    FIO_ASSERT(FIO_NAME2(fiobj, i)(e->value) == expect[index],
               "each2 ordering issue [%zu] (number) %ld != %d",
               index,
               FIO_NAME2(fiobj, i)(e->value),
               expect[index]);
  }
  ++index;
  return 0;
}

typedef struct fiobj_each_test_state_s {
  unsigned first;
  unsigned second;
  struct fiobj_each_test_state_s *next;
} fiobj_each_test_state_s;

FIO_SFUNC int fiobj_each_test_second(fiobj_each_s *e) {
  fiobj_each_test_state_s *state = (fiobj_each_test_state_s *)e->udata;
  ++state->second;
  FIO_ASSERT(e->key == FIOBJ_INVALID ||
                 FIOBJ_TYPE(e->key) == FIOBJ_T_STRING,
             "each1 key invalid for array/hash");
  return -1;
}

FIO_SFUNC int fiobj_each_test_first(fiobj_each_s *e) {
  fiobj_each_test_state_s *state = (fiobj_each_test_state_s *)e->udata;
  ++state->first;
  e->task = fiobj_each_test_second;
  e->udata = state->next;
  return 0;
}

FIO_SFUNC void fiobj_each_test_adapters(void) {
  FIOBJ a = fiobj_array_new();
  FIOBJ h = fiobj_hash_new();
  fiobj_each_test_state_s replacement = {0};
  fiobj_each_test_state_s state = {.next = &replacement};
  for (int i = 0; i < 3; ++i) {
    fiobj_array_push(a, fiobj_num_new(i));
  }
  fiobj_hash_set2(h, "a", 1, fiobj_num_new(1));
  fiobj_hash_set2(h, "b", 1, fiobj_num_new(2));
  fiobj_hash_set2(h, "c", 1, fiobj_num_new(3));
  FIO_ASSERT(fiobj_each1(a, fiobj_each_test_first, &state, 0) == 2 &&
                 state.first == 1 && replacement.second == 1,
             "array task replacement/stop position");
  state = (fiobj_each_test_state_s){.next = &replacement};
  replacement = (fiobj_each_test_state_s){0};
  FIO_ASSERT(fiobj_each1(h, fiobj_each_test_first, &state, 0) == 2 &&
                 state.first == 1 && replacement.second == 1,
             "hash task replacement/stop position");
  fiobj_free(a);
  fiobj_free(h);
}

/* FIOBJ_T_DATA: a fio_data_s store spilling to a temporary file */
FIO_SFUNC void FIO_NAME_TEST(stl, fiobj_data)(void) {
  fprintf(stderr, "* Testing FIOBJ Data (FIOBJ_T_DATA)\n");
  FIOBJ a = fiobj_data_new(16); /* tiny RAM limit: spills quickly */
  FIOBJ b = fiobj_data_new(0);
  FIO_ASSERT(FIOBJ_TYPE(a) == FIOBJ_T_DATA && FIOBJ_TYPE_CLASS(a) == FIOBJ_T_OTHER,
             "fiobj_data_new type");
  FIO_ASSERT(fiobj_data_ptr(a) && !fiobj_data_ptr(fiobj_true()) &&
                 !fiobj_data_ptr(FIOBJ_INVALID),
             "fiobj_data_ptr type checks");
  FIO_ASSERT(!fiobj_data_filename(a).buf && !fiobj_data_type(a).buf &&
                 !fiobj_data_headers(a).buf,
             "unset upload metadata");
  fiobj_data_filename_set(a, FIO_STR_INFO1("upload.txt"));
  fiobj_data_type_set(a, FIO_STR_INFO1("text/plain"));
  fiobj_data_headers_set(a, FIO_STR_INFO1("X-Test: yes\r\n"));
  fio_str_info_s fn = fiobj_data_filename(a);
  fio_str_info_s ty = fiobj_data_type(a);
  fio_str_info_s hd = fiobj_data_headers(a);
  FIO_ASSERT(fn.buf && fn.len == 10 && !memcmp(fn.buf, "upload.txt", 10) &&
                 fn.buf[fn.len] == 0 && ty.buf && ty.len == 10 &&
                 !memcmp(ty.buf, "text/plain", 10) && ty.buf[ty.len] == 0 &&
                 hd.buf && hd.len == 13 &&
                 !memcmp(hd.buf, "X-Test: yes\r\n", 13) && hd.buf[hd.len] == 0,
             "upload metadata round trip and termination");
  fiobj_data_filename_set(a, (fio_str_info_s){.buf = "", .len = 0});
  fn = fiobj_data_filename(a);
  FIO_ASSERT(fn.buf && !fn.len && !fn.buf[0], "empty metadata is set");
  fiobj_data_filename_set(a, FIO_STR_INFO1("new.txt"));
  fn = fiobj_data_filename(a);
  FIO_ASSERT(fn.buf && fn.len == 7 && !memcmp(fn.buf, "new.txt", 7),
             "overwrite metadata");
  fiobj_data_filename_set(a, (fio_str_info_s){0});
  fiobj_data_type_set(a, (fio_str_info_s){0});
  fiobj_data_headers_set(a, (fio_str_info_s){0});
  FIO_ASSERT(!fiobj_data_filename(a).buf && !fiobj_data_type(a).buf &&
                 !fiobj_data_headers(a).buf,
             "clear metadata");
  FIOBJ str = fiobj_str_new_cstr("hi", 2);
  fiobj_data_filename_set(str, FIO_STR_INFO1("file"));
  fiobj_data_type_set(str, FIO_STR_INFO1("type"));
  fiobj_data_headers_set(str, FIO_STR_INFO1("headers"));
  FIO_ASSERT(!fiobj_data_filename(str).buf && !fiobj_data_type(str).buf &&
                 !fiobj_data_headers(str).buf,
             "non-Data metadata calls");
  fiobj_free(str);
  fiobj_data_filename_set(a, FIO_STR_INFO1("last"));
  const char *txt = "Hello \"Data\" store, spilling past the RAM limit.";
  size_t len = strlen(txt);
  FIO_ASSERT(!fio_data_write(fiobj_data_ptr(a), txt, len), "write a");
  FIO_ASSERT(!fio_data_write(fiobj_data_ptr(b), txt, len), "write b");
  FIO_ASSERT(fio_data_fd(fiobj_data_ptr(a)) != -1 &&
                 fio_data_fd(fiobj_data_ptr(b)) == -1,
             "a should spill to a file, b should stay in RAM");
  fio_str_info_s sa = fiobj2cstr(a);
  FIO_ASSERT(sa.len == len && !memcmp(sa.buf, txt, len) &&
                 fiobj_data_ptr(a) && fiobj_data_filename(a).len == 4,
             "fiobj2cstr and fiobj_data_ptr after metadata");
  FIO_ASSERT(fiobj2i(a) == (intptr_t)len, "fiobj2i is the length");
  FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(a, b), "equal content should be equal");
  fio_data_write(fiobj_data_ptr(b), "!", 1);
  FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(a, b), "different content");
  /* reading position is preserved by fiobj2cstr / eq */
  fio_data_seek(fiobj_data_ptr(a), 6);
  (void)fiobj2cstr(a);
  FIO_ASSERT(fio_data_pos(fiobj_data_ptr(a)) == 6, "position preserved");
  /* JSON output renders the content as a String; hashes hold Data objects */
  FIOBJ h = fiobj_hash_new();
  fiobj_hash_set2(h, "d", 1, fiobj_dup(a));
  FIOBJ json = fiobj2json(FIOBJ_INVALID, h, 0);
  fio_str_info_s js = fiobj2cstr(json);
  const char *expect =
      "{\"d\":\"Hello \\\"Data\\\" store, spilling past the RAM limit.\"}";
  FIO_ASSERT(js.len == strlen(expect) && !memcmp(js.buf, expect, js.len),
             "Data JSON: %s",
             js.buf);
  fiobj_free(json);
  fiobj_free(h);
  fiobj_free(a);
  fiobj_free(b);
  FIOBJ e = fiobj_data_new(0);
  FIO_ASSERT(fiobj2cstr(e).buf && !fiobj2cstr(e).len, "empty Data to_s");
  fiobj_free(e);
}

/* FIOBJ_T_DATA: larger content (many 4096 byte compare chunks), RAM vs file */
FIO_SFUNC void FIO_NAME_TEST(stl, fiobj_data_large)(void) {
  fprintf(stderr, "* Testing FIOBJ Data equality / fiobj2cstr (RAM vs file)\n");
  const size_t len = (4096 * 5) + 123;
  char *src = (char *)malloc(len);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len; ++i)
    src[i] = (char)('a' + ((i * 7) % 26));
  FIOBJ ram = fiobj_data_new(len);   /* stays in RAM */
  FIOBJ file = fiobj_data_new(1000); /* spills */
  fio_data_s *dr = fiobj_data_ptr(ram), *df = fiobj_data_ptr(file);
  for (size_t i = 0; i < len; i += 1000) {
    size_t l = (len - i) < 1000 ? (len - i) : 1000;
    FIO_ASSERT(!fio_data_write(dr, src + i, l) &&
                   !fio_data_write(df, src + i, l),
               "Data write failed");
  }
  FIO_ASSERT(fio_data_fd(dr) == -1 && fio_data_fd(df) != -1, "Data modes");
  fio_data_seek(dr, 17);
  fio_data_seek(df, -100);
  const size_t pf = fio_data_pos(df);
  FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(ram, file) &&
                 FIO_NAME_BL(fiobj, eq)(file, ram),
             "RAM and spilled Data with equal content should be equal");
  FIO_ASSERT(fio_data_pos(dr) == 17 && fio_data_pos(df) == pf,
             "eq should preserve reading positions");
  fio_str_info_s s = fiobj2cstr(file);
  FIO_ASSERT(s.len == len && !memcmp(s.buf, src, len),
             "fiobj2cstr of spilled Data (len %zu)",
             s.len);
  FIO_ASSERT(fio_data_pos(df) == pf, "fiobj2cstr should preserve position");
#if FIO___DATA_MMAP
  FIO_ASSERT(df->map == 1 && s.buf == df->buf,
             "fiobj2cstr of spilled Data should point into the mapping");
#endif
  /* appending invalidates the mapping: the new content is visible */
  FIO_ASSERT(!fio_data_write(df, "+", 1), "Data append failed");
  s = fiobj2cstr(file);
  FIO_ASSERT(s.len == len + 1 && !memcmp(s.buf, src, len) && s.buf[len] == '+',
             "fiobj2cstr after append (len %zu)",
             s.len);
  FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(ram, file), "appended Data differs");
  FIO_ASSERT(!fio_data_write(dr, "+", 1) && FIO_NAME_BL(fiobj, eq)(ram, file),
             "equal again after the same append");
  s = fiobj2cstr(ram);
  FIO_ASSERT(s.len == len + 1 && !memcmp(s.buf, src, len), "fiobj2cstr RAM");
  s = fiobj2cstr(ram);
  FIO_ASSERT(s.len == len + 1 && !memcmp(s.buf, src, len),
             "fiobj2cstr RAM Data");
  /* a difference in the last chunk */
  FIOBJ other = fiobj_data_new(1000);
  FIO_ASSERT(!fio_data_write(fiobj_data_ptr(other), src, len) &&
                 !fio_data_write(fiobj_data_ptr(other), "#", 1),
             "Data write failed");
  FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(ram, other) &&
                 !FIO_NAME_BL(fiobj, eq)(other, file),
             "Data differing in the last byte should differ");
  fiobj_free(other);
  fiobj_free(ram);
  fiobj_free(file);
  free(src);
}

static void test_json_exact_input(void) {
  size_t bom_used = 0;
  FIOBJ bom = fiobj_json_parse2("\xEF\xBB\xBF", 3, &bom_used);
  FIO_ASSERT(bom == FIOBJ_INVALID && bom_used == 3,
             "BOM-only FIOBJ input must report progress without a root");
  static const char *const numbers[] = {"12345678901234567", "1.25", "0x123", "0b101", "-42"};
  for (size_t i = 0; i < sizeof(numbers) / sizeof(*numbers); ++i) {
    size_t len = strlen(numbers[i]);
    /* A readable numeric suffix is outside the supplied input slice. */
    char *buf = malloc(len + 9);
    FIO_ASSERT(buf, "allocation failed");
    memcpy(buf, numbers[i], len);
    memcpy(buf + len, "77777777", 9);
    size_t consumed = 0;
    FIOBJ obj = fiobj_json_parse(FIO_STR_INFO2(buf, len), &consumed);
    FIO_ASSERT(obj != FIOBJ_INVALID && consumed == len,
               "JSON read outside input: %zu != %zu", consumed, len);
    fiobj_free(obj);
    free(buf);
    buf = malloc(len);
    FIO_ASSERT(buf, "allocation failed");
    memcpy(buf, numbers[i], len);
    obj = fiobj_json_parse(FIO_STR_INFO2(buf, len), &consumed);
    FIO_ASSERT(obj != FIOBJ_INVALID && consumed == len,
               "exact JSON input failed");
    fiobj_free(obj);
    free(buf);
  }
}

int main(void) {
  test_json_exact_input();
  {
    FIOBJ s = fiobj_str_new_cstr("abcdef", 6);
    FIOBJ out = fiobj2json(s, s, 1);
    FIO_ASSERT(out == s && !memcmp(fiobj2cstr(s).buf + 6, "\"abcdef\"", 8),
               "JSON destination aliases source");
    fiobj_free(s);
    FIOBJ h = fiobj_hash_new();
    FIOBJ input = fiobj_str_new_cstr("{\"a\":1,\"tail\":2}", 16);
    fiobj_hash_set2(h, "a", 1, input);
    size_t consumed = fiobj_hash_update_json(h, fiobj2cstr(input));
    FIO_ASSERT(consumed == 16 && fiobj2i(fiobj_hash_get2(h, "tail", 4)) == 2,
               "JSON update must retain aliased input through overwrite");
    fiobj_free(h);
    char digit = '7';
    FIOBJ_STR_TEMP_VAR_STATIC(view, &digit, 1);
    FIO_ASSERT(fiobj2i(view) == 7 && fiobj2f(view) == 7,
               "exact-sized numeric view");
    FIOBJ_STR_TEMP_DESTROY(view);
    FIOBJ base = fiobj_object_new();
    FIO_ASSERT(base, "base constructor");
    fiobj_free(base);
    FIOBJ boxed = fiobj_num_new(INTPTR_MAX);
    FIO_ASSERT(boxed && fiobj_each1(boxed, NULL, NULL, 0) == 0,
               "boxed number generic iteration");
    fiobj_free(boxed);
    FIOBJ array = fiobj_array_new();
    fiobj_array_push(array, fiobj_dup(array));
    (void)fiobj2hash(array);
    /* Break the cycle before releasing the array. */
    FIOBJ child = FIOBJ_INVALID;
    fiobj_array_pop(array, &child);
    fiobj_free(child);
    fiobj_free(array);
  }
  {
    FIOBJ f = fiobj_float_new(NAN);
    FIO_ASSERT(f && fiobj_float2i(f) == 0, "NaN integer conversion");
    fiobj_free(f);
    f = fiobj_float_new(INFINITY);
    FIO_ASSERT(f && fiobj_float2i(f) == INTPTR_MAX,
               "positive infinity integer conversion");
    fiobj_free(f);
    f = fiobj_float_new(-INFINITY);
    FIO_ASSERT(f && fiobj_float2i(f) == INTPTR_MIN,
               "negative infinity integer conversion");
    fiobj_free(f);
    f = fiobj_float_new((double)INTPTR_MIN);
    FIO_ASSERT(f && fiobj_float2i(f) == INTPTR_MIN,
               "minimum integer float conversion");
    fiobj_free(f);
    f = fiobj_float_new(-((double)INTPTR_MIN));
    FIO_ASSERT(f && fiobj_float2i(f) == INTPTR_MAX,
               "upper bound integer float conversion");
    fiobj_free(f);
    FIOBJ a = fiobj_float_new(1.0);
    FIOBJ b = fiobj_float_new(nextafter(1.0, 2.0));
    FIO_ASSERT(a && b && !fiobj_is_eq(a, b),
               "adjacent float keys cannot compare approximately equal");
    fiobj_free(a);
    fiobj_free(b);
    a = fiobj_float_new(0.42);
    b = fiobj_float_new(0.42);
    FIO_ASSERT(a && b && fiobj_is_eq(a, b) && fiobj2hash(a) == fiobj2hash(b),
               "equal boxed floats must have matching hashes");
    fiobj_free(a);
    fiobj_free(b);
    a = fiobj_float_new(0.0);
    b = fiobj_float_new(-0.0);
    FIO_ASSERT(a && b && !fiobj_is_eq(a, b),
               "signed zero equality must match bitwise hashing");
    fiobj_free(a);
    fiobj_free(b);
    FIO_ASSERT(fiobj_str_new_buf(SIZE_MAX) == FIOBJ_INVALID,
               "oversized string capacity must fail");
  }
  FIOBJ o = FIOBJ_INVALID;
  FIO_NAME_TEST(stl, fiobj_data)();
  FIO_NAME_TEST(stl, fiobj_data_large)();
  if (!FIOBJ_MARK_MEMORY_ENABLED) {
    FIO_LOG_WARNING("FIOBJ defined without allocation counter. "
                    "Tests might not be complete.");
  }
  /* primitives - (in)sanity */
  {
    FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_NULL,
               "invalid FIOBJ type should be FIOBJ_T_NULL.");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(o, FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "invalid FIOBJ is NOT a fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(fiobj_true(),
                                       FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_true() is NOT fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(fiobj_false(),
                                       FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_false() is NOT fiobj_null().");
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(fiobj_false(), fiobj_true()),
               "fiobj_false() is NOT fiobj_true().");
    FIO_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_NULL)()) == FIOBJ_T_NULL,
               "fiobj_null() type should be FIOBJ_T_NULL.");
    FIO_ASSERT(FIOBJ_TYPE(fiobj_true()) == FIOBJ_T_TRUE,
               "fiobj_true() type should be FIOBJ_T_TRUE.");
    FIO_ASSERT(FIOBJ_TYPE(fiobj_false()) == FIOBJ_T_FALSE,
               "fiobj_false() type should be FIOBJ_T_FALSE.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_NULL)(),
                                      FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
               "fiobj_null() should be equal to self.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(fiobj_true(), fiobj_true()),
               "fiobj_true() should be equal to self.");
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(fiobj_false(), fiobj_false()),
               "fiobj_false() should be equal to self.");
  }
  {
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8) - 4; ++bit) {
      uintptr_t i = ((uintptr_t)1 << bit) + 1;
      uintptr_t m = (uintptr_t)0 - i;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i);
      FIO_ASSERT(FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER,
                 "FIOBJ integer allocation wasn't supposed to happen for %zd",
                 (size_t)i);
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)m);
      FIO_ASSERT(FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER,
                 "FIOBJ integer allocation wasn't supposed to happen for %zd",
                 (size_t)m);
    }
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i);
      FIO_ASSERT(FIO_NAME2(fiobj, i)(o) == (intptr_t)i,
                 "Number not reversible at bit %d (%zd != %zd)!",
                 (int)bit,
                 (ssize_t)FIO_NAME2(fiobj, i)(o),
                 (ssize_t)i);
      fio_str_info_s str = FIO_NAME2(fiobj, cstr)(o);
      char *str_buf = str.buf;
      FIO_ASSERT(fio_atol(&str_buf) == (intptr_t)i,
                 "Number atol not reversible at bit %d (%s != %zd)!",
                 (int)bit,
                 str.buf,
                 (ssize_t)i);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_ASSERT(allocation_flags == 3,
               "no bits are allocated / no allocations optimized away (%d)",
               (int)allocation_flags);
  }
  {
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(double) * 8); ++bit) {
      union {
        double d;
        uint64_t i;
      } punned;
      punned.i = (uint64_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(punned.d);
      FIO_ASSERT(FIO_NAME2(fiobj, f)(o) == punned.d,
                 "Float not reversible at bit %d (%lf != %lf)!",
                 (int)bit,
                 FIO_NAME2(fiobj, f)(o),
                 punned.d);

      fio_str_info_s str = FIO_NAME2(fiobj, cstr)(o);
      char buf_tmp[32];
      FIO_ASSERT(fio_ftoa(buf_tmp, FIO_NAME2(fiobj, f)(o), 10) == str.len,
                 "fio_atof length didn't match Float's fiobj2cstr length.");
      FIO_ASSERT(!memcmp(str.buf, buf_tmp, str.len),
                 "fio_atof string didn't match Float's fiobj2cstr.");
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_FLOAT) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_ASSERT(allocation_flags == 3,
               "no bits are allocated / no allocations optimized away (%d)",
               (int)allocation_flags);
  }
  fiobj_each_test_adapters();
  {
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
    for (size_t i = 1; i < 10; ++i) // 1, 2, 3 ... 10
    {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i));
      if (i % 3 == 0) {
        a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
      }
    }
    int expectation[] =
        {-1 /* array */, -1, 1, 2, 3, -1, 4, 5, 6, -1, 7, 8, 9, -1};
    size_t c =
        fiobj_each2(o, FIO_NAME_TEST(stl, fiobj_task), (void *)expectation);
    FIO_ASSERT(c == FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o) +
                        9 + 1,
               "each2 repetition count error");
    fiobj_free(o);
    FIO_NAME_TEST(stl, fiobj_task)(NULL);
  }
  {
    char json[] =
        "                    "
        "\n# comment 1"
        "\n// comment 2"
        "\n/* comment 3 */"
        "{\"true\":true,\"false\":false,\"null\":null,\"array\":[1,2,3,4.2,"
        "\"five\"],"
        "\"string\":\"hello\\tjson\\bworld!\\r\\n\",\"hash\":{\"true\":true,"
        "\"false\":false},\"array2\":[1,2,3,4.2,\"five\",{\"hash\":true},[{"
        "\"hash\":{\"true\":true}}]]}";
    o = fiobj_json_parse2(json, FIO_STRLEN(json), NULL);
    FIO_ASSERT(o, "JSON parsing failed - no data returned.");
    FIO_ASSERT(fiobj_json_find2(o, (char *)"array2[6][0].hash.true", 22) ==
                   fiobj_true(),
               "fiobj_json_find2 failed");
    FIOBJ j = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, o, 0);
#ifdef DEBUG
    fprintf(stderr, "JSON: %s\n", FIO_NAME2(fiobj, cstr)(j).buf);
#endif
    FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(j) ==
                   FIO_STRLEN(json + 61),
               "JSON roundtrip failed (length error %zu != %zu).\n%s\n%s",
               (size_t)FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(j),
               (size_t)FIO_STRLEN(json + 61),
               FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(j),
               json + 61);
    FIO_ASSERT(!memcmp(json + 61,
                       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(j),
                       FIO_STRLEN(json + 61)),
               "JSON roundtrip failed (data error).");
    fiobj_free(o);
    fiobj_free(j);
    o = FIOBJ_INVALID;
  }
  {
    /* Hash updates pass the Hash to fio_json_parse as the explicit root. */
    FIOBJ h = fiobj_hash_new();
    FIOBJ items = fiobj_array_new();
    fiobj_array_push(items, fiobj_num_new(1));
    fiobj_hash_set2(h, "items", 5, items);
    fiobj_hash_set2(h, "keep", 4, fiobj_str_new_cstr("yes", 3));
    char update[] = "{\"items\":[2],\"nested\":{\"x\":3}}";
    size_t used = fiobj_hash_update_json2(h, update, FIO_STRLEN(update));
    FIO_ASSERT(used == FIO_STRLEN(update) && FIOBJ_TYPE(h) == FIOBJ_T_HASH,
               "JSON Hash update failed or consumed the root");
    items = fiobj_hash_get2(h, "items", 5);
    FIO_ASSERT(FIOBJ_TYPE(items) == FIOBJ_T_ARRAY &&
                   fiobj_array_count(items) == 2 &&
                   fiobj2i(fiobj_array_get(items, 1)) == 2,
               "JSON Hash update must append to an existing Array");
    FIOBJ nested = fiobj_hash_get2(h, "nested", 6);
    FIO_ASSERT(FIOBJ_TYPE(nested) == FIOBJ_T_HASH &&
                   fiobj2i(fiobj_hash_get2(nested, "x", 1)) == 3,
               "JSON Hash update must merge nested maps");

    char malformed[] = "{\"partial\":4,\"broken\":";
    used = fiobj_hash_update_json2(h, malformed, FIO_STRLEN(malformed));
    FIO_ASSERT(used && FIOBJ_TYPE(h) == FIOBJ_T_HASH &&
                   fiobj2i(fiobj_hash_get2(h, "partial", 7)) == 4 &&
                   fiobj_hash_get2(h, "keep", 4) != FIOBJ_INVALID,
               "JSON Hash update error must retain partial caller data");
    char scalar[] = "42";
    FIO_ASSERT(fiobj_hash_update_json2(h, scalar, 2) == 2 &&
                   FIOBJ_TYPE(h) == FIOBJ_T_HASH,
               "scalar JSON update must not consume the Hash");
    char array[] = "[9]";
    fiobj_hash_update_json2(h, array, 3);
    FIO_ASSERT(FIOBJ_TYPE(h) == FIOBJ_T_HASH &&
                   fiobj_hash_get2(h, "keep", 4) != FIOBJ_INVALID,
               "Array JSON update must not consume or replace the Hash");
    fiobj_free(h);
  }
  {
    FIOBJ a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, n1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, n2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a1, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a2, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_ASSERT(FIO_NAME_BL(fiobj, eq)(a1, a2), "equal arrays aren't equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_false());
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n1, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n2, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a1, 0, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a2, -1, NULL);
    FIO_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    fiobj_free(a1);
    fiobj_free(a2);
  }
  {
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (size_t i = 1; i <= FIOBJ_TEST_REPETITIONS; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, tmp);
    }
    FIOBJ shifted = FIOBJ_INVALID;
    FIOBJ popped = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ set = FIOBJ_INVALID;
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), shift)(a, &shifted);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), pop)(a, &popped);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), set)
    (a, 1, fiobj_true(), &set);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a, 2, &removed);
    fiobj_free(a);
    if (1) {
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped) ==
                  FIO_STRLEN(
                      "number: " FIO_MACRO2STR(FIOBJ_TEST_REPETITIONS)) &&
              !memcmp(
                  "number: " FIO_MACRO2STR(FIOBJ_TEST_REPETITIONS),
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped),
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped)),
          "Object popped from Array lost it's value %s",
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped));
      FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(shifted) ==
                         9 &&
                     !memcmp("number: 1",
                             FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                      ptr)(shifted),
                             9),
                 "Object shifted from Array lost it's value %s",
                 FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(shifted));
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) == 9 &&
              !memcmp("number: 3",
                      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                      9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) == 9 &&
              !memcmp(
                  "number: 4",
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                  9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    }
    fiobj_free(shifted);
    fiobj_free(popped);
    fiobj_free(set);
    fiobj_free(removed);
  }
  {
    FIOBJ a1, a2;
    a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (size_t i = 0; i < FIOBJ_TEST_REPETITIONS; ++i) {
      FIOBJ str = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(str, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, str);
    }
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), concat)(a2, a1);
    fiobj_free(a1);
    for (size_t i = 0; i < FIOBJ_TEST_REPETITIONS; ++i) {
      FIOBJ_STR_TEMP_VAR(tmp);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)) ==
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp),
          "string length zeroed out - string freed?");
      FIO_ASSERT(
          !memcmp(
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(tmp),
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)),
              FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp)),
          "string data error - string freed?");
      FIOBJ_STR_TEMP_DESTROY(tmp);
    }
    fiobj_free(a2);
  }
  {
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    for (size_t i = 1; i <= FIOBJ_TEST_REPETITIONS; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)
      (o, k, fiobj_dup(tmp), NULL);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set_if_missing)(o, k, tmp);
      fiobj_free(k);
    }

    FIOBJ set = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove)(o, k, &removed);
    fiobj_free(k);
    k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)(o, k, fiobj_true(), &set);
    fiobj_free(k);
    FIO_ASSERT(set, "fiobj_hash_set didn't copy information to old pointer?");
    FIO_ASSERT(removed,
               "fiobj_hash_remove didn't copy information to old pointer?");
    // fiobj_hash_set(o, uintptr_t hash, FIOBJ key, FIOBJ value, FIOBJ *old)
    FIO_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) ==
                FIO_STRLEN("number: 1") &&
            !memcmp(
                "number: 1",
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    FIO_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) ==
                FIO_STRLEN("number: 2") &&
            !memcmp("number: 2",
                    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));

    fiobj_free(removed);
    fiobj_free(set);
    fiobj_free(o);
  }

#if FIOBJ_MARK_MEMORY
  {
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), reserve)(a, 64);
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i));
    }
    FIOBJ h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    FIOBJ key = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(key, "array", 5);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)(h, key, a);
    FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get)(h, key) == a,
               "FIOBJ Hash retrieval failed");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, key);
    if (0) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, fiobj_true());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, fiobj_false());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(0.42));

      FIOBJ json = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, h, 0);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(json, "\n", 1);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), reserve)
      (json,
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json)
           << 1); /* prevent memory realloc */
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
      (json,
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(json),
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json) - 1);
      fprintf(stderr, "\t%s\n", FIO_NAME2(fiobj, cstr)(json).buf);
      fiobj_free(json);
    }
    fiobj_free(h);
    FIOBJ_MARK_MEMORY_PRINT();
    FIO_ASSERT(FIOBJ_MARK_MEMORY_ALLOC_COUNTER ==
                   FIOBJ_MARK_MEMORY_FREE_COUNTER,
               "FIOBJ leak detected (freed %zu/%zu)",
               FIOBJ_MARK_MEMORY_FREE_COUNTER,
               FIOBJ_MARK_MEMORY_ALLOC_COUNTER);
  }
#endif
}
#undef FIOBJ_TEST_REPETITIONS
