/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_STATE Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_STATE_TEST___H)
#define H___FIO_STATE_TEST___H
#ifndef H___FIO_STATE___H
#define FIO_STATE
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

static size_t FIO_NAME_TEST(stl, state_task_counter) = 0;
FIO_SFUNC void FIO_NAME_TEST(stl, state_task)(void *arg) {
  size_t *i = (size_t *)arg;
  ++i[0];
}
FIO_SFUNC void FIO_NAME_TEST(stl, state_task_global)(void *arg) {
  (void)arg;
  ++FIO_NAME_TEST(stl, state_task_counter);
}
FIO_SFUNC void FIO_NAME_TEST(stl, state)(void) {
  fprintf(stderr, "* Testing state callback API.\n");
  size_t count = 0;
  for (size_t i = 0; i < 1024; ++i) {
    fio_state_callback_add(FIO_CALL_RESERVED1,
                           FIO_NAME_TEST(stl, state_task),
                           &count);
    fio_state_callback_add(FIO_CALL_RESERVED1,
                           FIO_NAME_TEST(stl, state_task_global),
                           (void *)i);
  }
  FIO_ASSERT(!count && !FIO_NAME_TEST(stl, state_task_counter),
             "callbacks should NOT have been called yet");
  fio_state_callback_force(FIO_CALL_RESERVED1);
  FIO_ASSERT(count == 1, "count error for local counter callback (%zu)", count);
  FIO_ASSERT(FIO_NAME_TEST(stl, state_task_counter) == 1024,
             "count error for global counter callback (%zu)",
             FIO_NAME_TEST(stl, state_task_counter));
  for (size_t i = 0; i < 1024; ++i) {
    fio_state_callback_remove(FIO_CALL_RESERVED1,
                              FIO_NAME_TEST(stl, state_task),
                              &count);
    fio_state_callback_remove(FIO_CALL_RESERVED1,
                              FIO_NAME_TEST(stl, state_task_global),
                              (void *)i);
  }
  fio_state_callback_force(FIO_CALL_RESERVED1);
  FIO_ASSERT(count == 1,
             "count error for local counter callback (%zu) - not removed?",
             count);
  FIO_ASSERT(FIO_NAME_TEST(stl, state_task_counter) == 1024,
             "count error for global counter callback (%zu) - not removed?",
             FIO_NAME_TEST(stl, state_task_counter));
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
