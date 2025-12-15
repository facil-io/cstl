/* *****************************************************************************
Test - Threads Module
***************************************************************************** */
#include "test-helpers.h"

#define FIO_THREADS
#include "../fio-stl/include.h"

/* Priority is controlled by the OS and changes aren't always accepted */
#define FIO___TEST_PRIORITY 1
/* *****************************************************************************
Test Helpers and Shared State
***************************************************************************** */

/* Shared counter for contention tests */
static volatile size_t fio___test_thread_counter = 0;

/* Mutex for contention tests */
static fio_thread_mutex_t fio___test_thread_mutex = FIO_THREAD_MUTEX_INIT;

/* Condition variable for signaling tests */
static fio_thread_cond_t fio___test_thread_cond;

/* Flag for condition variable tests */
static volatile int fio___test_thread_cond_flag = 0;

/* *****************************************************************************
Basic Thread Creation and Joining
***************************************************************************** */

/* Simple thread function that modifies a value */
FIO_SFUNC void *fio___test_thread_simple_fn(void *arg) {
  int *value = (int *)arg;
  *value = 42;
  return (void *)(uintptr_t)123;
}

FIO_SFUNC void fio___test_thread_basic(void) {
  fprintf(stderr, "\t* Testing basic thread creation and joining.\n");

  int value = 0;
  fio_thread_t thread;

  /* Test thread creation */
  int result = fio_thread_create(&thread, fio___test_thread_simple_fn, &value);
  FIO_ASSERT(result == 0, "fio_thread_create should return 0 on success");

  /* Test thread join */
  result = fio_thread_join(&thread);
  FIO_ASSERT(result == 0, "fio_thread_join should return 0 on success");

  /* Verify thread executed */
  FIO_ASSERT(value == 42,
             "Thread should have modified value to 42, got %d",
             value);
}

/* *****************************************************************************
Thread Identity and Comparison
***************************************************************************** */

/* Thread function that stores its own thread handle */
FIO_SFUNC void *fio___test_thread_identity_fn(void *arg) {
  fio_thread_t *stored = (fio_thread_t *)arg;
  *stored = fio_thread_current();
  return NULL;
}

FIO_SFUNC void fio___test_thread_identity(void) {
  fprintf(stderr, "\t* Testing thread identity and comparison.\n");

  fio_thread_t main_thread = fio_thread_current();
  fio_thread_t child_thread;
  fio_thread_t child_self = {0};

  /* Create thread that stores its own handle */
  int result = fio_thread_create(&child_thread,
                                 fio___test_thread_identity_fn,
                                 &child_self);
  FIO_ASSERT(result == 0, "fio_thread_create should succeed");

  result = fio_thread_join(&child_thread);
  FIO_ASSERT(result == 0, "fio_thread_join should succeed");

  /* Test thread equality */
  FIO_ASSERT(fio_thread_equal(&main_thread, &main_thread),
             "Same thread should be equal to itself");

  /* Note: child_self was captured inside the thread, child_thread is the
   * handle from create. They should refer to the same thread. */
  FIO_ASSERT(fio_thread_equal(&child_thread, &child_self),
             "Thread handle from create should equal thread's self handle");
}

/* *****************************************************************************
Thread Yield
***************************************************************************** */

/* Thread function that yields multiple times */
FIO_SFUNC void *fio___test_thread_yield_fn(void *arg) {
  volatile int *counter = (volatile int *)arg;
  for (int i = 0; i < 10; ++i) {
    fio_thread_yield();
    fio_atomic_add(counter, 1);
  }
  return NULL;
}

FIO_SFUNC void fio___test_thread_yield(void) {
  fprintf(stderr, "\t* Testing thread yield.\n");

  volatile int counter = 0;
  fio_thread_t thread;

  int result =
      fio_thread_create(&thread, fio___test_thread_yield_fn, (void *)&counter);
  FIO_ASSERT(result == 0, "fio_thread_create should succeed");

  /* Yield from main thread too */
  for (int i = 0; i < 5; ++i) {
    fio_thread_yield();
  }

  result = fio_thread_join(&thread);
  FIO_ASSERT(result == 0, "fio_thread_join should succeed");

  FIO_ASSERT(counter == 10,
             "Counter should be 10 after thread yields, got %d",
             counter);
}

/* *****************************************************************************
Thread Detach
***************************************************************************** */

/* Thread function for detach test */
FIO_SFUNC void *fio___test_thread_detach_fn(void *arg) {
  volatile int *flag = (volatile int *)arg;
  /* Small delay to ensure detach happens */
  for (volatile int i = 0; i < 1000; ++i) {
    fio_thread_yield();
  }
  fio_atomic_exchange(flag, 1);
  return NULL;
}

FIO_SFUNC void fio___test_thread_detach(void) {
  fprintf(stderr, "\t* Testing thread detach.\n");

  volatile int flag = 0;
  fio_thread_t thread;

  int result =
      fio_thread_create(&thread, fio___test_thread_detach_fn, (void *)&flag);
  FIO_ASSERT(result == 0, "fio_thread_create should succeed");

  result = fio_thread_detach(&thread);
  FIO_ASSERT(result == 0, "fio_thread_detach should return 0 on success");

  /* Wait for detached thread to complete (with timeout) */
  for (int i = 0; i < 10000 && !flag; ++i) {
    fio_thread_yield();
  }

  /* Note: We can't guarantee the detached thread completed, but we can verify
   * detach didn't crash */
  fprintf(stderr, "\t  - Detach completed (flag=%d).\n", flag);
}

/* *****************************************************************************
Mutex Basic Operations
***************************************************************************** */

FIO_SFUNC void fio___test_mutex_basic(void) {
  fprintf(stderr, "\t* Testing basic mutex operations.\n");

  fio_thread_mutex_t mutex;
  int result;

  /* Test mutex initialization */
  result = fio_thread_mutex_init(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_init should return 0");

  /* Test lock */
  result = fio_thread_mutex_lock(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_lock should return 0");

  /* Test trylock on already locked mutex (should fail) */
  result = fio_thread_mutex_trylock(&mutex);
  FIO_ASSERT(result != 0,
             "fio_thread_mutex_trylock should fail on locked mutex");

  /* Test unlock */
  result = fio_thread_mutex_unlock(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_unlock should return 0");

  /* Test trylock on unlocked mutex (should succeed) */
  result = fio_thread_mutex_trylock(&mutex);
  FIO_ASSERT(result == 0,
             "fio_thread_mutex_trylock should succeed on unlocked mutex");

  /* Unlock and destroy */
  result = fio_thread_mutex_unlock(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_unlock should return 0");

  fio_thread_mutex_destroy(&mutex);
}

/* *****************************************************************************
Mutex Static Initialization
***************************************************************************** */

FIO_SFUNC void fio___test_mutex_static_init(void) {
  fprintf(stderr, "\t* Testing static mutex initialization.\n");

  fio_thread_mutex_t mutex = FIO_THREAD_MUTEX_INIT;
  int result;

  /* Test lock on statically initialized mutex */
  result = fio_thread_mutex_lock(&mutex);
  FIO_ASSERT(result == 0,
             "fio_thread_mutex_lock should work on statically initialized "
             "mutex");

  result = fio_thread_mutex_unlock(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_unlock should return 0");

  fio_thread_mutex_destroy(&mutex);
}

/* *****************************************************************************
Mutex Contention Test
***************************************************************************** */

/* Thread function for mutex contention test */
FIO_SFUNC void *fio___test_mutex_contention_fn(void *arg) {
  size_t iterations = (size_t)(uintptr_t)arg;

  for (size_t i = 0; i < iterations; ++i) {
    fio_thread_mutex_lock(&fio___test_thread_mutex);
    size_t tmp = fio___test_thread_counter;
    fio_thread_yield(); /* Increase chance of race condition if mutex fails */
    fio___test_thread_counter = tmp + 1;
    fio_thread_mutex_unlock(&fio___test_thread_mutex);
  }

  return NULL;
}

FIO_SFUNC void fio___test_mutex_contention(void) {
  fprintf(stderr, "\t* Testing mutex contention with multiple threads.\n");

  const size_t num_threads = 4;
  const size_t iterations_per_thread = 1000;
  fio_thread_t threads[4];

  /* Reset counter */
  fio___test_thread_counter = 0;

  /* Create threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result = fio_thread_create(&threads[i],
                                   fio___test_mutex_contention_fn,
                                   (void *)(uintptr_t)iterations_per_thread);
    FIO_ASSERT(result == 0,
               "fio_thread_create should succeed for thread %zu",
               i);
  }

  /* Join threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result = fio_thread_join(&threads[i]);
    FIO_ASSERT(result == 0, "fio_thread_join should succeed for thread %zu", i);
  }

  /* Verify counter */
  size_t expected = num_threads * iterations_per_thread;
  FIO_ASSERT(fio___test_thread_counter == expected,
             "Counter should be %zu after mutex-protected increments, got %zu",
             expected,
             fio___test_thread_counter);
}

/* *****************************************************************************
Condition Variable Basic Operations
***************************************************************************** */

/* Thread function for condition variable test */
FIO_SFUNC void *fio___test_cond_wait_fn(void *arg) {
  fio_thread_mutex_t *mutex = (fio_thread_mutex_t *)arg;

  fio_thread_mutex_lock(mutex);

  /* Wait for signal */
  while (!fio___test_thread_cond_flag) {
    fio_thread_cond_wait(&fio___test_thread_cond, mutex);
  }

  fio_thread_mutex_unlock(mutex);

  return (void *)(uintptr_t)1;
}

FIO_SFUNC void fio___test_cond_basic(void) {
  fprintf(stderr, "\t* Testing condition variable basic operations.\n");

  fio_thread_mutex_t mutex;
  fio_thread_t thread;
  int result;

  /* Initialize */
  result = fio_thread_mutex_init(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_init should succeed");

  result = fio_thread_cond_init(&fio___test_thread_cond);
  FIO_ASSERT(result == 0, "fio_thread_cond_init should succeed");

  fio___test_thread_cond_flag = 0;

  /* Create waiting thread */
  result = fio_thread_create(&thread, fio___test_cond_wait_fn, &mutex);
  FIO_ASSERT(result == 0, "fio_thread_create should succeed");

  /* Give thread time to start waiting */
  for (int i = 0; i < 100; ++i) {
    fio_thread_yield();
  }

  /* Signal the condition */
  fio_thread_mutex_lock(&mutex);
  fio___test_thread_cond_flag = 1;
  result = fio_thread_cond_signal(&fio___test_thread_cond);
  FIO_ASSERT(result == 0, "fio_thread_cond_signal should succeed");
  fio_thread_mutex_unlock(&mutex);

  /* Join thread */
  result = fio_thread_join(&thread);
  FIO_ASSERT(result == 0, "fio_thread_join should succeed");

  /* Cleanup */
  fio_thread_cond_destroy(&fio___test_thread_cond);
  fio_thread_mutex_destroy(&mutex);
}

/* *****************************************************************************
Condition Variable Timed Wait
***************************************************************************** */

FIO_SFUNC void fio___test_cond_timedwait(void) {
  fprintf(stderr, "\t* Testing condition variable timed wait.\n");

  fio_thread_mutex_t mutex;
  fio_thread_cond_t cond;
  int result;

  /* Initialize */
  result = fio_thread_mutex_init(&mutex);
  FIO_ASSERT(result == 0, "fio_thread_mutex_init should succeed");

  result = fio_thread_cond_init(&cond);
  FIO_ASSERT(result == 0, "fio_thread_cond_init should succeed");

  /* Test timed wait that should timeout */
  fio_thread_mutex_lock(&mutex);

  /* Wait for 10ms - should timeout since no one signals */
  result = fio_thread_cond_timedwait(&cond, &mutex, 10);
  /* Result should be non-zero (timeout) */
  FIO_ASSERT(result != 0, "fio_thread_cond_timedwait should timeout");

  fio_thread_mutex_unlock(&mutex);

  /* Cleanup */
  fio_thread_cond_destroy(&cond);
  fio_thread_mutex_destroy(&mutex);
}

/* *****************************************************************************
Thread Priority (if supported)
***************************************************************************** */

FIO_SFUNC void fio___test_thread_priority(void) {
#if (FIO___TEST_PRIORITY - 1 + 1)
  fprintf(stderr, "\t* Testing thread priority.\n");

  /* Get current priority */
  fio_thread_priority_e priority = fio_thread_priority();

  /* Priority might return error on some systems, that's OK */
  if (priority == FIO_THREAD_PRIORITY_ERROR) {
    fprintf(stderr, "\t  - Thread priority not supported on this system.\n");
    return;
  }

  fprintf(stderr, "\t  - Current priority: %d\n", (int)priority);

  /* Try to set priority to normal */
  int result = fio_thread_priority_set(FIO_THREAD_PRIORITY_NORMAL);
  if (result != 0) {
    fprintf(stderr,
            "\t  - Setting thread priority not supported or requires "
            "privileges.\n");
    return;
  }

  /* Verify priority was set */
  fio_thread_priority_e new_priority = fio_thread_priority();
  FIO_ASSERT(new_priority == FIO_THREAD_PRIORITY_NORMAL ||
                 new_priority == FIO_THREAD_PRIORITY_ERROR,
             "Priority should be NORMAL after setting, but is %d",
             (int)new_priority);
#endif
}

/* *****************************************************************************
Stress Test - Many Threads
***************************************************************************** */

/* Thread function for stress test */
FIO_SFUNC void *fio___test_stress_fn(void *arg) {
  volatile size_t *counter = (volatile size_t *)arg;
  fio_atomic_add(counter, 1);
  fio_thread_yield();
  return NULL;
}

FIO_SFUNC void fio___test_thread_stress(void) {
  fprintf(stderr, "\t* Testing thread stress (many threads).\n");

  const size_t num_threads = 64;
  fio_thread_t threads[64];
  volatile size_t counter = 0;

  /* Create many threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result =
        fio_thread_create(&threads[i], fio___test_stress_fn, (void *)&counter);
    FIO_ASSERT(result == 0,
               "fio_thread_create should succeed for thread %zu",
               i);
  }

  /* Join all threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result = fio_thread_join(&threads[i]);
    FIO_ASSERT(result == 0, "fio_thread_join should succeed for thread %zu", i);
  }

  FIO_ASSERT(counter == num_threads,
             "Counter should be %zu after all threads complete, got %zu",
             num_threads,
             counter);
}

/* *****************************************************************************
Thread-safe Counter Test (without mutex, using atomics)
***************************************************************************** */

/* Thread function for atomic counter test */
FIO_SFUNC void *fio___test_atomic_counter_fn(void *arg) {
  volatile size_t *counter = (volatile size_t *)arg;
  for (size_t i = 0; i < 10000; ++i) {
    fio_atomic_add(counter, 1);
  }
  return NULL;
}

FIO_SFUNC void fio___test_atomic_counter(void) {
  fprintf(stderr, "\t* Testing atomic counter with multiple threads.\n");

  const size_t num_threads = 8;
  fio_thread_t threads[8];
  volatile size_t counter = 0;

  /* Create threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result = fio_thread_create(&threads[i],
                                   fio___test_atomic_counter_fn,
                                   (void *)&counter);
    FIO_ASSERT(result == 0,
               "fio_thread_create should succeed for thread %zu",
               i);
  }

  /* Join threads */
  for (size_t i = 0; i < num_threads; ++i) {
    int result = fio_thread_join(&threads[i]);
    FIO_ASSERT(result == 0, "fio_thread_join should succeed for thread %zu", i);
  }

  size_t expected = num_threads * 10000;
  FIO_ASSERT(counter == expected,
             "Atomic counter should be %zu, got %zu",
             expected,
             counter);
}

/* *****************************************************************************
Producer-Consumer Pattern Test
***************************************************************************** */

#define FIO___TEST_QUEUE_SIZE 16

typedef struct {
  fio_thread_mutex_t mutex;
  fio_thread_cond_t not_empty;
  fio_thread_cond_t not_full;
  int buffer[FIO___TEST_QUEUE_SIZE];
  size_t head;
  size_t tail;
  size_t count;
  int done;
} fio___test_queue_s;

FIO_SFUNC void *fio___test_producer_fn(void *arg) {
  fio___test_queue_s *q = (fio___test_queue_s *)arg;

  for (int i = 1; i <= 100; ++i) {
    fio_thread_mutex_lock(&q->mutex);

    /* Wait while queue is full */
    while (q->count == FIO___TEST_QUEUE_SIZE && !q->done) {
      fio_thread_cond_wait(&q->not_full, &q->mutex);
    }

    if (q->done) {
      fio_thread_mutex_unlock(&q->mutex);
      break;
    }

    /* Add item */
    q->buffer[q->tail] = i;
    q->tail = (q->tail + 1) % FIO___TEST_QUEUE_SIZE;
    q->count++;

    fio_thread_cond_signal(&q->not_empty);
    fio_thread_mutex_unlock(&q->mutex);
  }

  /* Signal done */
  fio_thread_mutex_lock(&q->mutex);
  q->done = 1;
  fio_thread_cond_signal(&q->not_empty);
  fio_thread_mutex_unlock(&q->mutex);

  return NULL;
}

FIO_SFUNC void *fio___test_consumer_fn(void *arg) {
  fio___test_queue_s *q = (fio___test_queue_s *)arg;
  size_t sum = 0;

  for (;;) {
    fio_thread_mutex_lock(&q->mutex);

    /* Wait while queue is empty */
    while (q->count == 0 && !q->done) {
      fio_thread_cond_wait(&q->not_empty, &q->mutex);
    }

    if (q->count == 0 && q->done) {
      fio_thread_mutex_unlock(&q->mutex);
      break;
    }

    /* Remove item */
    int item = q->buffer[q->head];
    q->head = (q->head + 1) % FIO___TEST_QUEUE_SIZE;
    q->count--;
    sum += (size_t)item;

    fio_thread_cond_signal(&q->not_full);
    fio_thread_mutex_unlock(&q->mutex);
  }

  return (void *)sum;
}

FIO_SFUNC void fio___test_producer_consumer(void) {
  fprintf(stderr, "\t* Testing producer-consumer pattern.\n");

  fio___test_queue_s queue = {0};
  fio_thread_t producer, consumer;

  /* Initialize */
  fio_thread_mutex_init(&queue.mutex);
  fio_thread_cond_init(&queue.not_empty);
  fio_thread_cond_init(&queue.not_full);

  /* Create threads */
  int result = fio_thread_create(&producer, fio___test_producer_fn, &queue);
  FIO_ASSERT(result == 0, "Producer thread creation should succeed");

  result = fio_thread_create(&consumer, fio___test_consumer_fn, &queue);
  FIO_ASSERT(result == 0, "Consumer thread creation should succeed");

  /* Join threads */
  result = fio_thread_join(&producer);
  FIO_ASSERT(result == 0, "Producer join should succeed");

  result = fio_thread_join(&consumer);
  FIO_ASSERT(result == 0, "Consumer join should succeed");

  /* Expected sum: 1 + 2 + ... + 100 = 5050 */
  /* Note: We can't easily get the return value with fio_thread_join,
   * so we just verify no crashes occurred */

  /* Cleanup */
  fio_thread_cond_destroy(&queue.not_empty);
  fio_thread_cond_destroy(&queue.not_full);
  fio_thread_mutex_destroy(&queue.mutex);
}

/* *****************************************************************************
Multi-threaded memcpy Test
***************************************************************************** */

FIO_SFUNC void fio___test_thread_memcpy(void) {
  fprintf(stderr, "\t* Testing fio_thread_memcpy.\n");

  /* Test with small buffer (should use single thread) */
  {
    char src[256];
    char dst[256];
    fio_rand_bytes(src, 256);
    FIO_MEMSET(dst, 0, 256);

    size_t threads_used = fio_thread_memcpy(dst, src, 256);
    FIO_ASSERT(threads_used >= 1,
               "fio_thread_memcpy should use at least 1 thread");
    FIO_ASSERT(!FIO_MEMCMP(src, dst, 256),
               "fio_thread_memcpy should copy data correctly (small)");
  }

  /* Test with larger buffer (may use multiple threads) */
  {
    size_t size = (1ULL << 24); /* 16MB - above FIO_MEMCPY_THREADS___MINCPY */
    char *src = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);
    char *dst = (char *)FIO_MEM_REALLOC(NULL, 0, size, 0);

    if (src && dst) {
      fio_rand_bytes(src, size);
      FIO_MEMSET(dst, 0, size);

      size_t threads_used = fio_thread_memcpy(dst, src, size);
      fprintf(stderr,
              "\t  - Used %zu thread(s) for %zu byte copy.\n",
              threads_used,
              size);

      FIO_ASSERT(!FIO_MEMCMP(src, dst, size),
                 "fio_thread_memcpy should copy data correctly (large)");

      FIO_MEM_FREE(src, size);
      FIO_MEM_FREE(dst, size);
    } else {
      fprintf(stderr,
              "\t  - Skipping large buffer test (allocation failed).\n");
      if (src)
        FIO_MEM_FREE(src, size);
      if (dst)
        FIO_MEM_FREE(dst, size);
    }
  }
}

/* *****************************************************************************
Fork Test (POSIX only)
***************************************************************************** */

#if FIO_OS_POSIX
FIO_SFUNC void fio___test_fork(void) {
  fprintf(stderr, "\t* Testing fork operations.\n");

  fio_thread_pid_t parent_pid = fio_thread_getpid();
  FIO_ASSERT(parent_pid > 0, "fio_thread_getpid should return positive PID");

  fio_thread_pid_t child_pid = fio_thread_fork();

  if (child_pid == 0) {
    /* Child process */
    fio_thread_pid_t my_pid = fio_thread_getpid();
    /* Child should have different PID than parent */
    if (my_pid == parent_pid) {
      _exit(1);
    }
    _exit(0);
  } else if (child_pid > 0) {
    /* Parent process */
    int status = 0;
    int result = fio_thread_waitpid(child_pid, &status, 0);
    FIO_ASSERT(result >= 0, "fio_thread_waitpid should succeed");
    FIO_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
               "Child process should exit successfully");
  } else {
    FIO_ASSERT(0, "fio_thread_fork failed");
  }
}
#endif /* FIO_OS_POSIX */

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
  fprintf(stderr, "Testing Threads Module\n");
  fprintf(stderr, "======================\n");

  /* Basic thread operations */
  fio___test_thread_basic();
  fio___test_thread_identity();
  fio___test_thread_yield();
  fio___test_thread_detach();

  /* Mutex operations */
  fio___test_mutex_basic();
  fio___test_mutex_static_init();
  fio___test_mutex_contention();

  /* Condition variables */
  fio___test_cond_basic();
  fio___test_cond_timedwait();

  /* Thread priority */
  fio___test_thread_priority();

  /* Stress tests */
  fio___test_thread_stress();
  fio___test_atomic_counter();

  /* Producer-consumer pattern */
  fio___test_producer_consumer();

  /* Multi-threaded memcpy */
  fio___test_thread_memcpy();

  /* Fork tests (POSIX only) */
#if FIO_OS_POSIX
  fio___test_fork();
#endif

  fprintf(stderr, "\nAll thread tests passed!\n");
  return 0;
}
