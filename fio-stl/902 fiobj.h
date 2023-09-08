/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_FIOBJ Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_FIOBJ_TEST___H)
#define H___FIO_FIOBJ_TEST___H
#ifndef H___FIO_FIOBJ___H
#define FIO_FIOBJ
#define FIOBJ_MALLOC
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

#define FIOBJ_TEST_REPETITIONS 4096

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

FIO_SFUNC void FIO_NAME_TEST(stl, fiobj)(void) {
  FIOBJ o = FIOBJ_INVALID;
  if (!FIOBJ_MARK_MEMORY_ENABLED) {
    FIO_LOG_WARNING("FIOBJ defined without allocation counter. "
                    "Tests might not be complete.");
  }
  /* primitives - (in)sanity */
  {
    fprintf(stderr, "* Testing FIOBJ primitives.\n");
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
    fprintf(stderr, "* Testing FIOBJ integers.\n");
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
    fprintf(stderr, "* Testing FIOBJ floats.\n");
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
  {
    fprintf(stderr, "* Testing FIOBJ each2.\n");
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
    fprintf(stderr, "* Testing FIOBJ JSON handling.\n");
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
               "JSON roundtrip failed (length error).");
    FIO_ASSERT(!memcmp(json + 61,
                       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(j),
                       FIO_STRLEN(json + 61)),
               "JSON roundtrip failed (data error).");
    fiobj_free(o);
    fiobj_free(j);
    o = FIOBJ_INVALID;
  }
  {
    fprintf(stderr, "* Testing FIOBJ array equality test (fiobj_is_eq).\n");
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
    fprintf(stderr, "* Testing FIOBJ array ownership.\n");
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
    fprintf(stderr, "* Testing FIOBJ array ownership after concat.\n");
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
    fprintf(stderr, "* Testing FIOBJ hash ownership.\n");
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    for (size_t i = 1; i <= FIOBJ_TEST_REPETITIONS; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(o, k, tmp);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set_if_missing2)
      (o, k, fiobj_dup(tmp));
      fiobj_free(k);
    }

    FIOBJ set = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove2)(o, k, &removed);
    fiobj_free(k);
    k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)
    (o, fiobj2hash(o, k), k, fiobj_true(), &set);
    fiobj_free(k);
    FIO_ASSERT(set, "fiobj_hash_set2 didn't copy information to old pointer?");
    FIO_ASSERT(removed,
               "fiobj_hash_remove2 didn't copy information to old pointer?");
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
    fprintf(stderr, "* Testing FIOBJ for memory leaks.\n");
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
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(h, key, a);
    FIO_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(h, key) == a,
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
      fprintf(stderr, "%s\n", FIO_NAME2(fiobj, cstr)(json).buf);
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
  fprintf(stderr, "* Passed.\n");
}
#undef FIOBJ_TEST_REPETITIONS
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
