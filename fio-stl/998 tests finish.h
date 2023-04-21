/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************



                                Testing


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_TEST_ALL___H)
#define H___FIO_TEST_ALL___H

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, math_speed)(void) {
  uint64_t n = 0, d = 1;
  uint64_t start[2], end[2];
  start[0] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0, r = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      fio_math_div(&q, &r, &n, &d, 1);
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[0] = fio_time_nano();
  n = 0, d = 1;
  start[1] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      q = n / d;
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[1] = fio_time_nano();
  FIO_LOG_INFO("\t fio_math_div test took %zu us (vs. %zu us) for a single "
               "64 bit word.",
               (size_t)(end[0] - start[0]),
               (size_t)(end[1] - start[1]));
}

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */
#define FIO___LOCK_TEST_TASK    (1LU << 25)
#define FIO___LOCK_TEST_THREADS 32U
#define FIO___LOCK_TEST_REPEAT  1

FIO_SFUNC void fio___lock_speedtest_task_inner(void *s) {
  size_t *r = (size_t *)s;
  static size_t i;
  for (i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
    FIO_COMPILER_GUARD;
    ++r[0];
  }
}

static void *fio___lock_mytask_lock(void *s) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock(&lock);
  return NULL;
}

#ifdef H___FIO_LOCK2___H
static void *fio___lock_mytask_lock2(void *s) {
  static fio_lock2_s lock = {FIO_LOCK_INIT};
  fio_lock2(&lock, 1);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock2(&lock, 1);
  return NULL;
}
#endif

static void *fio___lock_mytask_mutex(void *s) {
#if FIO_OS_WIN
  static fio_thread_mutex_t mutex;
#else
  static fio_thread_mutex_t mutex = FIO_THREAD_MUTEX_INIT;
#endif
  fio_thread_mutex_lock(&mutex);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_thread_mutex_unlock(&mutex);
  return NULL;
}

FIO_SFUNC void FIO_NAME_TEST(stl, lock_speed)(void) {
  uint64_t start, end;
  fio_thread_t threads[FIO___LOCK_TEST_THREADS];

  struct {
    size_t type_size;
    const char *type_name;
    const char *name;
    void *(*task)(void *);
  } test_funcs[] = {
      {
          .type_size = sizeof(fio_lock_i),
          .type_name = "fio_lock_i",
          .name = "fio_lock      (spinlock)",
          .task = fio___lock_mytask_lock,
      },
#ifdef H___FIO_LOCK2___H
      {
          .type_size = sizeof(fio_lock2_s),
          .type_name = "fio_lock2_s",
          .name = "fio_lock2 (pause/resume)",
          .task = fio___lock_mytask_lock2,
      },
#endif
      {
          .type_size = sizeof(fio_thread_mutex_t),
          .type_name = "fio_thread_mutex_t",
          .name = "OS threads (pthread_mutex / Windows handle)",
          .task = fio___lock_mytask_mutex,
      },
      {
          .name = NULL,
          .task = NULL,
      },
  };
  fprintf(stderr, "* Speed testing The following types:\n");
  for (size_t fn = 0; test_funcs[fn].name; ++fn) {
    fprintf(stderr,
            "\t%s\t(%zu bytes)\n",
            test_funcs[fn].type_name,
            test_funcs[fn].type_size);
  }

  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
    FIO_COMPILER_GUARD;
  }
  end = fio_time_micro();
  fprintf(stderr,
          "\n* Speed testing locking schemes - no contention, short work (%zu "
          "mms):\n"
          "\t\t(%zu itterations)\n",
          (size_t)(end - start),
          (size_t)FIO___LOCK_TEST_TASK);

  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      test_funcs[fn].task(NULL); /* warmup */
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
        FIO_COMPILER_GUARD;
        test_funcs[fn].task(NULL);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms\n",
              test_funcs[fn].name,
              (size_t)(end - start));
    }
  }

  fprintf(stderr,
          "\n* Speed testing locking schemes - no contention, long work ");
  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
    size_t result = 0;
    FIO_COMPILER_GUARD;
    fio___lock_speedtest_task_inner(&result);
  }
  end = fio_time_micro();
  fprintf(stderr, " %zu mms\n", (size_t)(end - start));
  clock_t long_work = end - start;
  fprintf(stderr, "(%zu mms):\n", (size_t)long_work);
  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        FIO_COMPILER_GUARD;
        test_funcs[fn].task(&result);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK_TEST_TASK * FIO___LOCK_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }

  fprintf(stderr,
          "\n* Speed testing locking schemes - %zu threads, long work (%zu "
          "mms):\n",
          (size_t)FIO___LOCK_TEST_THREADS,
          (size_t)long_work);
  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        fio_thread_create(threads + i, test_funcs[fn].task, &result);
      }
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        fio_thread_join(threads + i);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK_TEST_TASK * FIO___LOCK_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }
}

/* *****************************************************************************
Testing function
***************************************************************************** */

FIO_SFUNC void fio____test_dynamic_types__stack_poisoner(void) {
#define FIO___STACK_POISON_LENGTH (1ULL << 18)
  uint8_t buf[FIO___STACK_POISON_LENGTH];
  FIO_COMPILER_GUARD;
  FIO_MEMSET(buf, (int)(0xA0U), FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_rand_bytes(buf, FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_trylock(buf);
#undef FIO___STACK_POISON_LENGTH
}

FIO_SFUNC void fio_test_dynamic_types(void) {
  char *filename = (char *)FIO_INCLUDE_FILE;
  while (filename[0] == '.' && filename[1] == '/')
    filename += 2;
  fio____test_dynamic_types__stack_poisoner();
  fprintf(stderr, "===============\n");
  fprintf(stderr, "Testing facil.io CSTL (%s)\n", filename);
  fprintf(
      stderr,
      "Version: \x1B[1m" FIO_VERSION_STRING "\x1B[0m\n"
      "The facil.io library was originally coded by \x1B[1mBoaz Segev\x1B[0m.\n"
      "Please give credit where credit is due.\n"
      "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
      "(code contributions / donations)\n\n");
  fprintf(stderr, "===============\n");
  FIO_LOG_DEBUG("example FIO_LOG_DEBUG message.");
  FIO_LOG_DEBUG2("example FIO_LOG_DEBUG2 message.");
  FIO_LOG_INFO("example FIO_LOG_INFO message.");
  FIO_LOG_WARNING("example FIO_LOG_WARNING message.");
  FIO_LOG_SECURITY("example FIO_LOG_SECURITY message.");
  FIO_LOG_ERROR("example FIO_LOG_ERROR message.");
  FIO_LOG_FATAL("example FIO_LOG_FATAL message.");
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, type_sizes)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, random)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, atomics)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, core)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, atol)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, math)();
  FIO_NAME_TEST(stl, math_speed)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sort)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, url)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, glob_matching)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, imap_core)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, state)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, string_core_helpers)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  FIO_NAME_TEST(stl, ary3____test)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, uset___test_size_t)();
  FIO_NAME_TEST(stl, umap___test_size)();
  FIO_NAME_TEST(stl, omap___test_size_t)();
  FIO_NAME_TEST(stl, omap___test_size_lru)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, fio_big_str)();
  FIO_NAME_TEST(stl, fio_small_str)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, mustache)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, time)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, queue)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, cli)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, stream)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, poll)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, files)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, memalt)();
  fprintf(stderr, "===============\n");
  /* test memory allocator that initializes memory to zero */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_safe), mem)();
  fprintf(stderr, "===============\n");
  /* test memory allocator that allows junk data in allocations */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_unsafe), mem)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sock)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, fiobj)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, server)();
  FIO_NAME_TEST(stl, pubsub)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, http_s)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, risky)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sha1)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sha2)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, chacha)();
#if !DEBUG
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, lock_speed)();
#endif
  fprintf(stderr, "===============\n");
  {
    char timebuf[64];
    fio_time2rfc7231(timebuf, fio_time_real().tv_sec);
    fprintf(stderr,
            "On %s\n"
            "Testing \x1B[1mPASSED\x1B[0m "
            "for facil.io core version: "
            "\x1B[1m" FIO_VERSION_STRING "\x1B[0m"
            "\n",
            timebuf);
  }
  fprintf(stderr,
          "\nThe facil.io library was originally coded by \x1B[1mBoaz "
          "Segev\x1B[0m.\n"
          "Please give credit where credit is due.\n"
          "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
          "(code contributions / donations)\n\n");
}

/* *****************************************************************************
Testing cleanup
***************************************************************************** */
#undef FIO_TEST_ALL
#undef FIO_TEST_REPEAT
/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif
/* *****************************************************************************
Finish testing segment
***************************************************************************** */
#endif /* FIO_TEST_ALL / H___TESTS_FINISH___H */

#if defined(FIO___TEST_ALL_RECURSION)
#undef FIO___TEST_ALL_RECURSION
#define FIO_TEST_ALL
#endif
