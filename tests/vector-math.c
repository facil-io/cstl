/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define VECTOR_TYPE   float
#define VECTOR_LENGTH 65536

#define FAIL_IF_NON_EQ(v1, v2)                                                 \
  do {                                                                         \
    for (size_t i = 0; i < VECTOR_LENGTH; ++i) {                               \
      FIO_ASSERT((v1)[i] == (v2)[i],                                           \
                 "(%zu) unexpected value, %zu != %zu",                         \
                 (size_t)__LINE__,                                             \
                 (size_t)((v1)[i]),                                            \
                 (size_t)((v2)[i]));                                           \
    }                                                                          \
  } while (0)

#define FAIL_IF_EQ(v1, v2)                                                     \
  do {                                                                         \
    for (size_t i = 0; i < VECTOR_LENGTH; ++i) {                               \
      FIO_ASSERT((v1)[i] != (v2)[i],                                           \
                 "(%zu) unexpected value, %zu == %zu",                         \
                 (size_t)__LINE__,                                             \
                 (size_t)((v1)[i]),                                            \
                 (size_t)((v2)[i]));                                           \
    }                                                                          \
  } while (0)

int main(void) {
  static VECTOR_TYPE buffer[VECTOR_LENGTH * 4];
  VECTOR_TYPE *v[4] = {buffer + (VECTOR_LENGTH * 0),
                       buffer + (VECTOR_LENGTH * 1),
                       buffer + (VECTOR_LENGTH * 2),
                       buffer + (VECTOR_LENGTH * 3)};
  VECTOR_TYPE result = 0;

  for (size_t i = 0; i < VECTOR_LENGTH; ++i) {
    v[0][i] = (VECTOR_TYPE)0;
    v[1][i] = (VECTOR_TYPE)1;
    v[2][i] = (VECTOR_TYPE)2;
    v[3][i] = (VECTOR_TYPE)4;
  }
  FAIL_IF_EQ(v[1], v[2]);
  FAIL_IF_NON_EQ(v[1], v[1]);

  FIO_VEC_REDUCE_ADD(result, v[1], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH,
             "FIO_VEC_REDUCE_ADD failed? got %zu",
             (size_t)result);
  result = 0;
  FIO_VEC_REDUCE_ADD(result, v[2], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH * 2,
             "FIO_VEC_REDUCE_ADD failed? got %zu",
             (size_t)result);
  result = 0;
  FIO_VEC_DOT(result, v[1], v[2], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH * 2,
             "FIO_VEC_DOT failed? got %zu",
             (size_t)result);

  FIO_VEC_MUL(v[0], v[2], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[2]);
  FAIL_IF_EQ(v[0], v[1]);
  FIO_VEC_SUB(v[0], v[0], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[1]);
  FAIL_IF_EQ(v[0], v[2]);
  FIO_VEC_ADD(v[0], v[1], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[2]);
  FIO_VEC_MUL(v[0], v[0], v[2], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[3]);

  { /* speed test? */
    uint64_t start, end;
    const size_t repetitions = 8192;
    const size_t vector_length = VECTOR_LENGTH - (fio_rand64() & 31);
    result = (VECTOR_TYPE)0;

    fprintf(stderr,
            "\t* multiply two %zu dimension vectors %zu times:\n",
            (size_t)vector_length,
            repetitions);
    for (size_t i = 0; i < VECTOR_LENGTH; ++i)
      v[0][i] = (VECTOR_TYPE)1;
    for (size_t i = 0; i < VECTOR_LENGTH; ++i)
      v[1][i] = (VECTOR_TYPE)3;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      for (size_t i = 0; i < vector_length; ++i)
        v[0][i] *= v[1][i];
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr, "\t\tnaive:\t%zu us\n", (size_t)(end - start));

    for (size_t i = 0; i < VECTOR_LENGTH; ++i)
      v[0][i] = (VECTOR_TYPE)1;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      FIO_VEC_MUL(v[0], v[0], v[1], vector_length);
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr, "\t\tmacro:\t%zu us\n", (size_t)(end - start));

    fprintf(stderr,
            "\t* reduce mul %zu dimension vector %zu times:\n",
            (size_t)vector_length,
            repetitions);

    result = (VECTOR_TYPE)1;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      for (size_t i = 0; i < vector_length; ++i)
        result *= v[1][i];
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tnaive:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);

    result = (VECTOR_TYPE)1;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      FIO_VEC_REDUCE_MUL(result, v[1], vector_length);
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tmacro:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);

    fprintf(stderr,
            "\t* reduce add %zu dimension vector %zu times:\n",
            (size_t)vector_length,
            repetitions);

    result = (VECTOR_TYPE)0;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      for (size_t i = 0; i < vector_length; ++i)
        result += v[1][i];
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tnaive:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);

    result = (VECTOR_TYPE)0;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      FIO_VEC_REDUCE_ADD(result, v[1], vector_length);
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tmacro:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);

    fprintf(stderr,
            "\t* dot product of two %zu dimension vectors %zu times:\n",
            (size_t)vector_length,
            repetitions);

    for (size_t i = 0; i < VECTOR_LENGTH; ++i)
      v[0][i] = (VECTOR_TYPE)7;
    for (size_t i = 0; i < VECTOR_LENGTH; ++i)
      v[1][i] = (VECTOR_TYPE)3;

    result = (VECTOR_TYPE)0;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      for (size_t i = 0; i < vector_length; ++i)
        result += v[0][i] * v[1][i];
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tnaive:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);

    result = (VECTOR_TYPE)0;
    start = fio_time_micro();
    for (size_t attempt = 0; attempt < repetitions; ++attempt) {
      FIO_VEC_DOT(result, v[0], v[1], vector_length);
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr,
            "\t\tmacro:\t%zu us\t (result: %.3g)\n",
            (size_t)(end - start),
            (float)result);
  }
  return 0;
}
