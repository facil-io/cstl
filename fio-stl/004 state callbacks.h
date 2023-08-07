/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_STATE              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      State Callback Management API



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_STATE) && !defined(H___FIO_STATE___H) &&                       \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_STATE___H
/* *****************************************************************************
State Callback API
***************************************************************************** */

/* *****************************************************************************
Startup / State Callbacks (fork, start up, idle, etc')
***************************************************************************** */

/** a callback type enum */
typedef enum {
  /** Called once during library initialization. */
  FIO_CALL_ON_INITIALIZE,
  /** Called once before starting up the IO reactor. */
  FIO_CALL_PRE_START,
  /** Called before each time the IO reactor forks a new worker. */
  FIO_CALL_BEFORE_FORK,
  /** Called after each fork (both parent and child), before FIO_CALL_IN_XXX */
  FIO_CALL_AFTER_FORK,
  /** Called by a worker process right after forking. */
  FIO_CALL_IN_CHILD,
  /** Called by the master process after spawning a worker (after forking). */
  FIO_CALL_IN_MASTER,
  /** Called every time a *Worker* process starts. */
  FIO_CALL_ON_START,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED1,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED2,
  /** User state event queue (unused, available for the user). */
  FIO_CALL_ON_USER1,
  /** User state event queue (unused, available for the user). */
  FIO_CALL_ON_USER2,
  /** Called when facil.io enters idling mode. */
  FIO_CALL_ON_IDLE,
  /** A reversed user state event queue (unused, available for the user). */
  FIO_CALL_ON_USER1_REVERSE,
  /** A reversed user state event queue (unused, available for the user). */
  FIO_CALL_ON_USER2_REVERSE,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED1_REVERSED,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED2_REVERSED,
  /** Called before starting the shutdown sequence. */
  FIO_CALL_ON_SHUTDOWN,
  /** Called by each worker the moment it detects the master process crashed. */
  FIO_CALL_ON_PARENT_CRUSH,
  /** Called by the parent (master) after a worker process crashed. */
  FIO_CALL_ON_CHILD_CRUSH,
  /** Called just before finishing up (both on child and parent processes). */
  FIO_CALL_ON_FINISH,
  /** An alternative to the system's at_exit. */
  FIO_CALL_AT_EXIT,
  /** used for testing and array allocation - must be last. */
  FIO_CALL_NEVER
} fio_state_event_type_e;

/** Adds a callback to the list of callbacks to be called for the event. */
SFUNC void fio_state_callback_add(fio_state_event_type_e,
                                  void (*func)(void *),
                                  void *arg);

/** Removes a callback from the list of callbacks to be called for the event. */
SFUNC int fio_state_callback_remove(fio_state_event_type_e,
                                    void (*func)(void *),
                                    void *arg);

/**
 * Forces all the existing callbacks to run, as if the event occurred.
 *
 * Callbacks for all initialization / idling tasks are called in order of
 * creation (where fio_state_event_type_e <= FIO_CALL_ON_IDLE).
 *
 * Callbacks for all cleanup oriented tasks are called in reverse order of
 * creation (where fio_state_event_type_e >= FIO_CALL_ON_SHUTDOWN).
 *
 * During an event, changes to the callback list are ignored (callbacks can't
 * add or remove other callbacks for the same event).
 */
SFUNC void fio_state_callback_force(fio_state_event_type_e);

/* *****************************************************************************
State Callback Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
State Callback Map - I'd use the global mapping types...
(Ordered Hash Map)   but we can't depend on types yet, possible collisions.
***************************************************************************** */

typedef struct {
  void (*func)(void *);
  void *arg;
} fio___state_task_s;

FIO_IFUNC uint64_t fio___state_callback_hash_fn(fio___state_task_s *t) {
  uint64_t hash = fio_risky_ptr((void *)(uintptr_t)(t->func));
  hash ^= hash + fio_risky_ptr(t->arg);
  return hash;
}

#define FIO_STATE_CALLBACK_IS_VALID(pobj) ((pobj)->func)
#define FIO_STATE_CALLBACK_CMP(a, b)                                           \
  ((a)->func == (b)->func && (a)->arg == (b)->arg)
FIO_TYPEDEF_IMAP_ARRAY(fio___state_map,
                       fio___state_task_s,
                       uint32_t,
                       fio___state_callback_hash_fn,
                       FIO_STATE_CALLBACK_CMP,
                       FIO_STATE_CALLBACK_IS_VALID)
#undef FIO_STATE_CALLBACK_CMP
#undef FIO_STATE_CALLBACK_IS_VALID

/* *****************************************************************************
State Callback Global State and Locks
***************************************************************************** */
/* use `weak` instead of `static` to make sure state callbacks are global. */
FIO_WEAK fio___state_map_s FIO___STATE_TASKS_ARRAY[FIO_CALL_NEVER + 1];
FIO_WEAK fio_lock_i FIO___STATE_TASKS_ARRAY_LOCK[FIO_CALL_NEVER + 1];

FIO_IFUNC void fio_state_callback_clear_all(void) {
  for (size_t i = 0; i < FIO_CALL_NEVER; ++i) {
    fio___state_map_destroy(FIO___STATE_TASKS_ARRAY + i);
  }
  FIO_LOG_DEBUG2("fio_state_callback maps have been cleared.");
}

/** Adds a callback to the list of callbacks to be called for the event. */
SFUNC void fio_state_callback_add(fio_state_event_type_e e,
                                  void (*func)(void *),
                                  void *arg) {
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  fio___state_task_s t = {.func = func, .arg = arg};
  fio_lock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  fio___state_map_set(FIO___STATE_TASKS_ARRAY + (uintptr_t)e, t, 0);
  fio_unlock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  if (e == FIO_CALL_ON_INITIALIZE &&
      FIO___STATE_TASKS_ARRAY_LOCK[FIO_CALL_NEVER]) {
    /* initialization tasks already performed, perform this without delay */
    func(arg);
  }
}

/** Removes a callback from the list of callbacks to be called for the event. */
SFUNC int fio_state_callback_remove(fio_state_event_type_e e,
                                    void (*func)(void *),
                                    void *arg) {
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return -1;
  int ret;
  fio___state_task_s t = {.func = func, .arg = arg};
  fio_lock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  ret = fio___state_map_remove(FIO___STATE_TASKS_ARRAY + (uintptr_t)e, t);
  fio_unlock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  return ret;
}

/** Clears all the existing callbacks for the event. */
SFUNC void fio_state_callback_clear(fio_state_event_type_e e) {
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  fio_lock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  fio___state_map_destroy(FIO___STATE_TASKS_ARRAY + (uintptr_t)e);
  fio_unlock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
}

FIO_SFUNC void fio_state_callback_force___task(void *fn_p, void *arg) {
  union {
    void *p;
    void (*fn)(void *);
  } u = {.p = fn_p};
  u.fn(arg);
}
/**
 * Forces all the existing callbacks to run, as if the event occurred.
 *
 * Callbacks are called from last to first (last callback executes first).
 *
 * During an event, changes to the callback list are ignored (callbacks can't
 * remove other callbacks for the same event).
 */
SFUNC void fio_state_callback_force(fio_state_event_type_e e) {
  /** a type-to-string map for callback types */
  static const char *fio___state_tasks_names[FIO_CALL_NEVER + 1] = {
      [FIO_CALL_ON_INITIALIZE] = "ON_INITIALIZE",
      [FIO_CALL_PRE_START] = "PRE_START",
      [FIO_CALL_BEFORE_FORK] = "BEFORE_FORK",
      [FIO_CALL_AFTER_FORK] = "AFTER_FORK",
      [FIO_CALL_IN_CHILD] = "IN_CHILD",
      [FIO_CALL_IN_MASTER] = "IN_MASTER",
      [FIO_CALL_ON_START] = "ON_START",
      [FIO_CALL_RESERVED1] = "RESERVED1",
      [FIO_CALL_RESERVED2] = "RESERVED2",
      [FIO_CALL_ON_USER1] = "ON_USER1",
      [FIO_CALL_ON_USER2] = "ON_USER2",
      [FIO_CALL_ON_IDLE] = "ON_IDLE",
      [FIO_CALL_ON_USER1_REVERSE] = "ON_USER1_REVERSE",
      [FIO_CALL_ON_USER2_REVERSE] = "ON_USER2_REVERSE",
      [FIO_CALL_RESERVED1_REVERSED] = "RESERVED1_REVERSED",
      [FIO_CALL_RESERVED2_REVERSED] = "RESERVED2_REVERSED",
      [FIO_CALL_ON_SHUTDOWN] = "ON_SHUTDOWN",
      [FIO_CALL_ON_PARENT_CRUSH] = "ON_PARENT_CRUSH",
      [FIO_CALL_ON_CHILD_CRUSH] = "ON_CHILD_CRUSH",
      [FIO_CALL_ON_FINISH] = "ON_FINISH",
      [FIO_CALL_AT_EXIT] = "AT_EXIT",
      [FIO_CALL_NEVER] = "NEVER",
  };

  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  if (e == FIO_CALL_AFTER_FORK) {
    /* make sure the `after_fork` events re-initializes all locks. */
    for (size_t i = 0; i < FIO_CALL_NEVER; ++i) {
      FIO___STATE_TASKS_ARRAY_LOCK[i] = FIO_LOCK_INIT;
    }
  }
  if (e == FIO_CALL_IN_CHILD)
    fio_rand_reseed(); /* re-seed random state in child processes */
  fio___state_task_s *ary = NULL;
  size_t ary_capa = (sizeof(*ary) * FIO___STATE_TASKS_ARRAY[e].count);
  size_t len = 0;
  if (e == FIO_CALL_ON_INITIALIZE) {
    fio_trylock(FIO___STATE_TASKS_ARRAY_LOCK + FIO_CALL_NEVER);
  }

  FIO_LOG_DDEBUG2("%d scheduling %s callbacks.",
                  (int)(fio_thread_getpid()),
                  fio___state_tasks_names[e]);

  /* copy task queue */
  fio_lock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);
  if (FIO___STATE_TASKS_ARRAY[e].w) {
    ary = (fio___state_task_s *)FIO_MEM_REALLOC(NULL, 0, ary_capa, 0);
    FIO_ASSERT_ALLOC(ary);
    for (size_t i = 0; i < FIO___STATE_TASKS_ARRAY[e].w; ++i) {
      if (!FIO___STATE_TASKS_ARRAY[e].ary[i].func)
        continue;
      ary[len++] = FIO___STATE_TASKS_ARRAY[e].ary[i];
    }
  }
  fio_unlock(FIO___STATE_TASKS_ARRAY_LOCK + (uintptr_t)e);

  if (e <= FIO_CALL_PRE_START) {
    /* perform copied tasks immediately within system thread */
    for (size_t i = 0; i < len; ++i)
      ary[i].func(ary[i].arg);
  } else if (e <= FIO_CALL_ON_IDLE) {
    /* perform tasks in order */
    for (size_t i = 0; i < len; ++i) {
      ary[i].func(ary[i].arg);
    }
  } else {
    /* perform tasks in reverse */
    while (len--) {
      ary[len].func(ary[len].arg);
    }
  }
  /* cleanup */
  FIO_MEM_FREE(ary, ary_capa);
  (void)fio___state_tasks_names; /* if unused */
}
/* *****************************************************************************
State constructor / destructor
***************************************************************************** */

FIO_SFUNC void fio___state_cleanup_task_at_exit(void *ignr_) {
  fio_state_callback_clear_all();
  (void)ignr_;
}

FIO_CONSTRUCTOR(fio___state_constructor) {
  FIO_LOG_DEBUG2("fio_state_callback maps are now active.");
  fio_state_callback_force(FIO_CALL_ON_INITIALIZE);
  fio_state_callback_add(FIO_CALL_AT_EXIT,
                         fio___state_cleanup_task_at_exit,
                         NULL);
}

FIO_DESTRUCTOR(fio___state_at_exit_hook) {
  fio_state_callback_force(FIO_CALL_AT_EXIT);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STATE
#endif /* FIO_STATE */
