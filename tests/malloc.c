#define FIO_MALLOC
#define FIO_LOG
#define FIO_TIME
#define FIO_ATOMIC
#include <fio-stl.h>
#define FIO_CLI
#define FIO_MALLOC_TMP_USE_SYSTEM 1
#include <fio-stl.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

#define TEST_CYCLES_START 128
#define TEST_CYCLES_END 256
#define TEST_CYCLES_REPEAT 3

/**
 * facil.io doesn't keep metadata in memory slices, panelizing realloc.
 *
 * To compensate, it provides fio_realloc2 that copies only the valid data.
 *
 * If thiis flag is set, the test will run using this compensation, assuming all
 * previously allocated memory should be copied (but no risk of copy overflow
 * performance hit).
 */
#define TEST_WITH_REALLOC2 0

/*
 * VALUES: 0 == 1 thread, no contention; 1 == 2 threads; etc'
 *
 *
 * NOTE:
 * Since facil.io makes a point of returning memory to the system (keeping only
 * a small rotating cache), a high contention rate will result in more system
 * calls to collect memory from the system and to return it to the system.
 */
#define TEST_THREAD_CONTENTION 0

#if TEST_WITH_REALLOC2
FIO_SFUNC void *sys_realloc2(void *ptr, size_t new_size, size_t copy_len) {
  return realloc(ptr, new_size);
  (void)copy_len;
}
typedef void *(*realloc_func_p)(void *, size_t, size_t);
#else
typedef void *(*realloc_func_p)(void *, size_t);
#endif

static size_t test_mem_functions(void *(*malloc_func)(size_t),
                                 void *(*calloc_func)(size_t, size_t),
                                 realloc_func_p realloc_func,
                                 void (*free_func)(void *)) {
  static size_t clock_alloc = 0, clock_realloc = 0, clock_free = 0,
                clock_free2 = 0, clock_calloc = 0, fio_optimized = 0,
                fio_optimized2 = 0, errors = 0, repetitions = 0;

  if (!malloc_func) {
    size_t total = clock_alloc + clock_realloc + clock_free + clock_free2 +
                   clock_calloc + fio_optimized + fio_optimized2;
    clock_alloc /= repetitions;
    clock_realloc /= repetitions;
    clock_free /= repetitions;
    clock_free2 /= repetitions;
    clock_calloc /= repetitions;
    fio_optimized /= repetitions;
    fio_optimized2 /= repetitions;
    fprintf(stderr, "* Avrg. micro-seconds per malloc: %zu\n", clock_alloc);
    fprintf(stderr, "* Avrg. micro-seconds per calloc: %zu\n", clock_calloc);
    fprintf(stderr, "* Avrg. micro-seconds per realloc: %zu\n", clock_realloc);
    fprintf(
        stderr, "* Avrg. micro-seconds per free (realloc): %zu\n", clock_free);
    fprintf(stderr,
            "* Avrg. micro-seconds per free (re-cycle): %zu\n",
            clock_free2);
    fprintf(stderr,
            "* Avrg. micro-seconds per a facil.io use-case round"
            " (medium-short life): %zu\n",
            fio_optimized);
    fprintf(stderr,
            "* Avrg. micro-seconds per a zero-life span"
            " (malloc-free): %zu\n",
            fio_optimized2);
    fprintf(stderr, "* Failed allocations: %zu\n", errors);
    fprintf(stderr, "Total CPU Time (micros): %zu\n", total);

    clock_alloc = 0;
    clock_realloc = 0;
    clock_free = 0;
    clock_free2 = 0;
    clock_calloc = 0;
    fio_optimized = 0;
    fio_optimized2 = 0;
    errors = 0;
    repetitions = 0;
    return 0;
  }
  fio_atomic_add(&repetitions,
                 (TEST_CYCLES_END - TEST_CYCLES_START) * TEST_CYCLES_REPEAT);

  for (int i = TEST_CYCLES_START; i < TEST_CYCLES_END; ++i) {
    for (int repeat = 0; repeat < TEST_CYCLES_REPEAT; ++repeat) {
      void **pointers = calloc_func(sizeof(*pointers), 4096);
      uint64_t start;

      /* malloc */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        pointers[j] = malloc_func(i << 4);
        if (i) {
          if (!pointers[j])
            ++errors;
          else
            ((char *)pointers[j])[0] = '1';
        }
      }
      fio_atomic_add(&clock_alloc, fio_time_micro() - start);

      /* realloc */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
#if TEST_WITH_REALLOC2
        void *tmp = realloc_func(pointers[j], i << 5, i << 4);
#else
        void *tmp = realloc_func(pointers[j], i << 5);
#endif
        if (tmp) {
          pointers[j] = tmp;
          ((char *)pointers[j])[0] = '1';
        } else if (i)
          ++errors;
      }
      fio_atomic_add(&clock_realloc, fio_time_micro() - start);

      /* free (bigger sizes, due to realloc) */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        free_func(pointers[j]);
        pointers[j] = NULL;
      }
      fio_atomic_add(&clock_free, fio_time_micro() - start);

      /* calloc */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        pointers[j] = calloc_func(16, i);
        if (i) {
          if (!pointers[j])
            ++errors;
          else
            ((char *)pointers[j])[0] = '1';
        }
      }
      fio_atomic_add(&clock_calloc, fio_time_micro() - start);

      /* free (smaller sizes, no realloc) */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        free_func(pointers[j]);
      }
      fio_atomic_add(&clock_free2, fio_time_micro() - start);

      /* facil.io use-case */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        pointers[j] = malloc_func(i << 4);
        if (i) {
          if (!pointers[j])
            ++errors;
          else
            ((char *)pointers[j])[0] = '1';
        }
      }
      for (int j = 0; j < 4096; ++j) {
        free_func(pointers[j]);
      }
      fio_atomic_add(&fio_optimized, fio_time_micro() - start);

      /* facil.io use-case */
      start = fio_time_micro();
      for (int j = 0; j < 4096; ++j) {
        pointers[j] = malloc_func(i << 4);
        if (i) {
          if (!pointers[j])
            ++errors;
          else
            ((char *)pointers[j])[(i << 4) - 1] = '1';
        }
        free_func(pointers[j]);
      }
      fio_atomic_add(&fio_optimized2, fio_time_micro() - start);

      free_func(pointers);
    }
  }
  return clock_alloc + clock_realloc + clock_free + clock_calloc + clock_free2;
}

void *test_system_malloc(void *ignr) {
  (void)ignr;
#if TEST_WITH_REALLOC2
  uintptr_t result = test_mem_functions(malloc, calloc, sys_realloc2, free);
#else
  uintptr_t result = test_mem_functions(malloc, calloc, realloc, free);
#endif
  return (void *)result;
}
void *test_facil_malloc(void *ignr) {
  (void)ignr;
#if TEST_WITH_REALLOC2
  uintptr_t result =
      test_mem_functions(fio_malloc, fio_calloc, fio_realloc2, fio_free);
#else
  uintptr_t result =
      test_mem_functions(fio_malloc, fio_calloc, fio_realloc, fio_free);
#endif
  return (void *)result;
}

int main(int argc, char const *argv[]) {
  fio_cli_start(
      argc,
      argv,
      0,
      0,
      "This program tests malloc speed vs. fio_malloc. It also tests for "
      "failed allocations as reported to the user. It does not test the "
      "actual allocators, this is performed in the STL test unit.",
      FIO_CLI_INT(
          "--threads -t runs the test concurrently, adding contention."));

#if DEBUG
  fprintf(stderr,
          "\n=== WARNING: performance tests using the DEBUG mode are "
          "invalid. \n");
#endif
  fio_cli_set_default("-t", "1");
  const size_t thread_count = fio_cli_get_i("-t");
  fio_cli_end();
  pthread_t threads[thread_count];

  fprintf(stderr, "========================================\n");
  fprintf(stderr,
          "NOTE: The facil.io allocator always returns memory that was "
          "zeroed out.\n"
          "\n      In contrast, the system allocator may return (and retain) "
          "junk data.\n"
          "\n      Test allocation ranges: %zu - %zu bytes.\n",
          ((size_t)(TEST_CYCLES_START) << 4),
          ((size_t)(TEST_CYCLES_END) << 5));
  fprintf(stderr, "========================================\n");

  /* test system allocations */
  fprintf(stderr,
          "Performance Testing system memory allocator with %zu threads "
          "(please wait):\n\n",
          thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    FIO_ASSERT(pthread_create(threads + i, NULL, test_system_malloc, NULL) == 0,
               "Couldn't spawn thread.");
  }
  for (size_t i = 0; i < thread_count; ++i) {
    FIO_ASSERT(pthread_join(threads[i], NULL) == 0, "Couldn't join thread");
  }
  test_mem_functions(NULL, NULL, NULL, NULL);

  /* test facil.io allocations */
  fprintf(stderr, "========================================\n");
  fprintf(stderr,
          "Performance Testing facil.io memory allocator with %zu threads "
          "(please wait):\n\n",
          thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    FIO_ASSERT(pthread_create(threads + i, NULL, test_facil_malloc, NULL) == 0,
               "Couldn't spawn thread.");
  }
  for (size_t i = 0; i < thread_count; ++i) {
    FIO_ASSERT(pthread_join(threads[i], NULL) == 0, "Couldn't join thread");
  }
  test_mem_functions(NULL, NULL, NULL, NULL);

  return 0; // fio_cycles > sys_cycles;
}
