/* URL-encoded parser: heap fallback, early stop and allocation failures. */
#include "fio-stl/include.h"

static struct {
  void *ptr[2];
  size_t size[2];
  size_t attempts;
  size_t fail_at;
  size_t live;
  size_t pairs;
  size_t errors;
  size_t field_len;
  int stop;
} test_state;

static void *test_allocate(void *ptr, size_t size) {
  FIO_ASSERT(!ptr, "Stack fallback must allocate without freeing stack memory");
  if (++test_state.attempts == test_state.fail_at)
    return NULL;
  void *p = malloc(size);
  FIO_ASSERT(p, "Test allocation failed");
  FIO_ASSERT(test_state.live < 2, "Unexpected outstanding allocations");
  test_state.ptr[test_state.live] = p;
  test_state.size[test_state.live++] = size;
  return p;
}

static void test_deallocate(void *ptr, size_t size) {
  for (size_t i = 0; i < test_state.live; ++i) {
    if (test_state.ptr[i] != ptr)
      continue;
    FIO_ASSERT(size == test_state.size[i],
               "Free must receive allocated capacity");
    free(ptr);
    --test_state.live;
    test_state.ptr[i] = test_state.ptr[test_state.live];
    test_state.size[i] = test_state.size[test_state.live];
    return;
  }
  FIO_ASSERT(0, "Free of untracked memory (possibly a stack buffer)");
}

/* Keep this allocator active through recursive inclusion of the string core. */
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  test_allocate((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) test_deallocate((ptr), (size))
#define FIO_MEM_REALLOC_IS_SAFE_ 0
#define FIO_URL_ENCODED
#include FIO_INCLUDE_FILE

static void *test_on_pair(void *udata,
                          fio_buf_info_s name,
                          fio_buf_info_s value) {
  ++test_state.pairs;
  FIO_ASSERT(name.len == test_state.field_len &&
                 value.len == test_state.field_len,
             "Decoded pair has incorrect lengths");
  for (size_t i = 0; i + 1 < test_state.field_len; ++i) {
    FIO_ASSERT(name.buf[i] == 'n' && value.buf[i] == 'v',
               "Decoded pair has incorrect bytes");
  }
  FIO_ASSERT(name.buf[name.len - 1] == '+' && value.buf[value.len - 1] == ' ',
             "Pair was not decoded exactly once");
  FIO_ASSERT(test_state.live == (test_state.field_len > 4096 ? 2 : 0),
             "Unexpected buffer allocation lifetime");
  return test_state.stop ? NULL : udata;
}

static void test_on_error(void *udata) {
  FIO_ASSERT(udata == &test_state, "Error callback context changed");
  FIO_ASSERT(!test_state.live, "Error callback must run after buffer cleanup");
  ++test_state.errors;
}

int main(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
      .on_pair = test_on_pair,
      .on_error = test_on_error,
  };
  char input[2 * (8192 + 3) + 2];
  FIO_MEMSET(input, 'n', 8192);
  FIO_MEMCPY(input + 8192, "%2B=", 4);
  FIO_MEMSET(input + 8196, 'v', 8192);
  FIO_MEMCPY(input + 16388, "%20", 3);
  const size_t len = 16391;
  /* Repeat the pair to exercise independent cleanup on each iteration. */
  char repeated[2 * 16391 + 1];
  FIO_MEMCPY(repeated, input, len);
  repeated[len] = '&';
  FIO_MEMCPY(repeated + len + 1, input, len);

  test_state.field_len = 3;
  fio_url_encoded_result_s small =
      fio_url_encoded_parse(&callbacks, &test_state, "nn%2B=vv+", 9);
  FIO_ASSERT(!small.err && small.count == 1 && small.consumed == 9 &&
                 !test_state.attempts && !test_state.live,
             "Short pairs must use stack storage only");

  for (size_t scenario = 0; scenario < 6; ++scenario) {
    FIO_MEMSET(&test_state, 0, sizeof(test_state));
    test_state.field_len = 8193;
    test_state.stop = (scenario == 1);
    test_state.fail_at = scenario >= 2 ? scenario - 1 : 0;
    fio_url_encoded_result_s r = fio_url_encoded_parse(&callbacks,
                                                       &test_state,
                                                       repeated,
                                                       sizeof(repeated));
    FIO_ASSERT(!test_state.live, "Parser leaked decoded buffers");
    if (test_state.fail_at) {
      const size_t prior_pairs = (test_state.fail_at - 1) / 2;
      FIO_ASSERT(
          r.err && r.count == prior_pairs &&
              r.consumed == (prior_pairs ? len + 1 : 0) &&
              test_state.pairs == prior_pairs && test_state.errors == 1,
          "Allocation failure must stop before reporting a partial pair");
      FIO_ASSERT(test_state.attempts == test_state.fail_at,
                 "Parser allocated after decoding failed");
    } else {
      const size_t expected_count = test_state.stop ? 1 : 2;
      FIO_ASSERT(r.count == expected_count &&
                     test_state.pairs == expected_count &&
                     r.err == test_state.stop && !test_state.errors &&
                     r.consumed == (test_state.stop ? len : sizeof(repeated)),
                 "Long-pair parse result mismatch");
      FIO_ASSERT(test_state.attempts == 2 * expected_count,
                 "Both long buffers must use heap fallback");
    }
  }
  return 0;
}
