/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_STATE                   /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "010 riskyhash.h"          /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                      State Callback Management API




***************************************************************************** */
#if defined(FIO_STATE) && !defined(H__FIO_STATE__H) && !defined(FIO_STL_KEEP__)
#define H__FIO_STATE__H
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

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
State Callback Map - I'd use the global mapping types...
(Ordered Hash Map)   but we can't depend on types yet, possible collisions.
***************************************************************************** */

typedef struct {
  void (*func)(void *);
  void *arg;
} fio___state_task_s;

/* an ordered array and a poor-man's hash map mapping indexes */
typedef struct {
  size_t count;
  size_t w;
  size_t capa_bits;
  fio___state_task_s *ary;
  uintptr_t *imap;
} fio___state_map_s;

FIO_IFUNC uint64_t fio___state_callback_hash(void (*func)(void *), void *arg) {
  uint64_t hash = fio_risky_ptr((void *)(uintptr_t)func);
  hash ^= hash + fio_risky_ptr((void *)(uintptr_t)arg);
  return hash;
}

typedef struct {
  size_t i;
  uintptr_t *map;
  uint64_t hash;
} fio___state_map_pos_s;

FIO_SFUNC fio___state_map_pos_s fio___state_map_find(fio___state_map_s *map,
                                                     void (*func)(void *),
                                                     void *arg) {
  fio___state_map_pos_s pos = {.hash = fio___state_callback_hash(func, arg)};
  if (!map->ary)
    return pos;
  const uintptr_t hash_mask = ((~(size_t)0ULL) << map->capa_bits);
  const uintptr_t index_mask = ~hash_mask;
  pos.hash |= 0ULL - !(pos.hash & hash_mask); /* prevent false hole loop */
  size_t ipos = pos.hash;
  pos.hash &= hash_mask;
  for (size_t step = 0; step < 31; (++step, (ipos += 656959ULL))) {
    /* for each cuckoo step  */
    ipos &= index_mask;
    if (ipos == index_mask) /* reserved index - prevent false holes */
      continue;
    if (!map->imap[ipos]) { /* empty spot */
      pos.map = map->imap + ipos;
      pos.i = map->w;
      return pos;
    }
    if (map->imap[ipos] == (~(uintptr_t)0ULL)) { /* hole (item removed) */
      pos.map = map->imap + ipos;
      pos.i = map->w;
      continue;
    }
    if ((map->imap[ipos] & hash_mask) != pos.hash)
      continue;
    const size_t opos = map->imap[ipos] & index_mask;
    if (map->ary[opos].func == func &&
        map->ary[opos].arg == arg) { /* exact match */
      pos.map = map->imap + ipos;    /* ipos is the index in the index map */
      pos.i = opos; /* confusing, but pos.i is the index of the object itself */
      return pos;
    }
    /* this is a partial hash collision... we could count them and test for
     * attack, but this implementation assumes safe inputs and a good hash
     * function */
  }
  return pos;
}

FIO_SFUNC void fio___state_map_destroy(fio___state_map_s *map) {
  if (map->ary) {
    size_t capa = (1ULL << map->capa_bits);
    FIO_MEM_FREE(map->ary, (sizeof(*map->ary) + sizeof(*map->imap)) * capa);
    (void)capa; /* may be unused */
  }
  *map = (fio___state_map_s){0};
}

FIO_SFUNC void fio___state_map_alloc(fio___state_map_s *map) {
  size_t new_capa = 1ULL << map->capa_bits;
  map->ary = (fio___state_task_s *)FIO_MEM_REALLOC(
      NULL,
      0,
      new_capa * (sizeof(map->ary[0]) + sizeof(map->imap[0])),
      0);
  FIO_ASSERT_ALLOC(map->ary);
  map->imap = (uintptr_t *)(map->ary + new_capa);
  FIO_MEMSET(map->ary, 0, new_capa * sizeof(*map->ary));
  FIO_MEMSET(map->imap, 0, new_capa * sizeof(*map->imap));
}

FIO_SFUNC int fio___state_map_copy(fio___state_map_s *dest,
                                   fio___state_map_s *map) {
  for (size_t i = 0; i < map->count; ++i) {
    if (!map->ary[i].func)
      continue;
    fio___state_map_pos_s pos =
        fio___state_map_find(dest, map->ary[i].func, map->ary[i].arg);
    if (!pos.map) { /* dest not big enough to contain collisions! */
      return -1;
    }
    dest->ary[pos.i] = map->ary[i];
    pos.map[0] = pos.hash | pos.i;
    ++dest->w;
    ++dest->count;
  }
  return 0;
}

FIO_SFUNC void fio___state_map_expand(fio___state_map_s *map) {
  fio___state_map_s tmp = {0};
  tmp.capa_bits = (map->capa_bits + (!map->capa_bits) + (!map->capa_bits));
  for (;;) {
    tmp.capa_bits <<= 1;
    fio___state_map_alloc(&tmp);
    if (!fio___state_map_copy(&tmp, map)) {
      fio___state_map_destroy(map);
      *map = tmp;
      return;
    }
    fio___state_map_destroy(&tmp);
  }
}

FIO_IFUNC int fio___state_map_remove(fio___state_map_s *map,
                                     void (*func)(void *),
                                     void *arg) {
  if (!func)
    return -1;
  fio___state_map_pos_s pos = fio___state_map_find(map, func, arg);
  if (!pos.map || pos.i == map->w)
    return -1;
  pos.map[0] = (~(uintptr_t)0ULL); /* mark hole */
  map->ary[pos.i].func = NULL;
  map->ary[pos.i].arg = NULL;
  --map->count;
  while (map->w && !map->ary[map->w - 1].func)
    --map->w;
  return 0;
}

FIO_IFUNC void fio___state_map_add(fio___state_map_s *map,
                                   void (*func)(void *),
                                   void *arg) {
  if (!func)
    return;
  if (map->count != map->w && map->w + 1 == (1ULL << map->capa_bits)) {
    /* array is full, but there are holes in the array, so we can compress it */
    fio___state_map_s tmp = {0};
    tmp.capa_bits = map->capa_bits;
    fio___state_map_alloc(&tmp);
    if (!fio___state_map_copy(&tmp, map)) {
      FIO_LOG_ERROR("impossible map copy failed in state map...");
    }
    fio___state_map_destroy(map);
    *map = tmp;
  }
  for (;;) {
    fio___state_map_pos_s pos = fio___state_map_find(map, func, arg);
    if (!pos.map) {
      fio___state_map_expand(map);
      continue;
    }
    if (pos.i != map->w)
      return; /* exists */
    map->ary[pos.i].func = func;
    map->ary[pos.i].arg = arg;
    pos.map[0] = pos.hash | pos.i;
    ++map->w;
    ++map->count;
  }
}
FIO_IFUNC int fio___state_map_exists(fio___state_map_s *map,
                                     void (*func)(void *),
                                     void *arg) {
  if (!func)
    return -1;
  fio___state_map_pos_s pos = fio___state_map_find(map, func, arg);
  if (!pos.map || pos.i == map->w)
    return 0;
  return 1;
}

/* *****************************************************************************
State Callback Global State and Locks
***************************************************************************** */
static fio___state_map_s fio___state_tasks_array[FIO_CALL_NEVER];
static fio_lock_i fio___state_tasks_array_lock[FIO_CALL_NEVER + 1];

/** a type-to-string map for callback types */
FIO_SFUNC const char *fio___state_tasks_names[FIO_CALL_NEVER + 1] = {
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

FIO_IFUNC void fio_state_callback_clear_all(void) {
  for (size_t i = 0; i < FIO_CALL_NEVER; ++i) {
    fio___state_map_destroy(fio___state_tasks_array + i);
  }
}

/** Adds a callback to the list of callbacks to be called for the event. */
SFUNC void fio_state_callback_add(fio_state_event_type_e e,
                                  void (*func)(void *),
                                  void *arg) {
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  fio_lock(fio___state_tasks_array_lock + (uintptr_t)e);
  fio___state_map_add(fio___state_tasks_array + (uintptr_t)e, func, arg);
  fio_unlock(fio___state_tasks_array_lock + (uintptr_t)e);
  if (e == FIO_CALL_ON_INITIALIZE &&
      fio___state_tasks_array_lock[FIO_CALL_NEVER]) {
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
  fio_lock(fio___state_tasks_array_lock + (uintptr_t)e);
  ret =
      fio___state_map_remove(fio___state_tasks_array + (uintptr_t)e, func, arg);
  fio_unlock(fio___state_tasks_array_lock + (uintptr_t)e);
  return ret;
}

/** Clears all the existing callbacks for the event. */
SFUNC void fio_state_callback_clear(fio_state_event_type_e e) {
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  fio_lock(fio___state_tasks_array_lock + (uintptr_t)e);
  fio___state_map_destroy(fio___state_tasks_array + (uintptr_t)e);
  fio_unlock(fio___state_tasks_array_lock + (uintptr_t)e);
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
  if ((uintptr_t)e >= FIO_CALL_NEVER)
    return;
  if (e == FIO_CALL_AFTER_FORK) {
    /* make sure the `after_fork` events re-initializes all locks. */
    for (size_t i = 0; i < FIO_CALL_NEVER; ++i) {
      fio___state_tasks_array_lock[i] = FIO_LOCK_INIT;
    }
  }
  fio___state_task_s *ary = NULL;
  size_t ary_capa = (sizeof(*ary) * fio___state_tasks_array[e].count);
  size_t len = 0;
  if (e == FIO_CALL_ON_INITIALIZE) {
    fio_trylock(fio___state_tasks_array_lock + FIO_CALL_NEVER);
  }

  FIO_LOG_DDEBUG2("(%d) Scheduling %s callbacks.",
                  (int)getpid(),
                  fio___state_tasks_names[e]);

  /* copy task queue */
  fio_lock(fio___state_tasks_array_lock + (uintptr_t)e);
  if (fio___state_tasks_array[e].w) {
    ary = (fio___state_task_s *)FIO_MEM_REALLOC(NULL, 0, ary_capa, 0);
    FIO_ASSERT_ALLOC(ary);
    for (size_t i = 0; i < fio___state_tasks_array[e].w; ++i) {
      if (!fio___state_tasks_array[e].ary[i].func)
        continue;
      ary[len++] = fio___state_tasks_array[e].ary[i];
    }
  }
  fio_unlock(fio___state_tasks_array_lock + (uintptr_t)e);

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
}
/* *****************************************************************************
State constructor / destructor
***************************************************************************** */

FIO_CONSTRUCTOR(fio___state_constructor) {
  FIO_LOG_DEBUG2("fio_state_callback maps are now active.");
  fio_state_callback_force(FIO_CALL_ON_INITIALIZE);
}

FIO_DESTRUCTOR(fio___state_cleanup) {
  fio_state_callback_force(FIO_CALL_AT_EXIT);
  fio_state_callback_clear_all();
  FIO_LOG_DEBUG2("fio_state_callback maps have been cleared.");
}

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, state)(void) {
  /*
   * TODO: test module here
   */
  fprintf(stderr, "* testing state callback map\n");
  {
    fio___state_map_s map = {0}, map2 = {0};
    fio___state_map_add(&map, (void (*)(void *))1, (void *)1);
    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))1, (void *)1),
               "add failed? (exists is negative)");
    FIO_ASSERT(map.w == 1 && map.count == 1 && map.ary &&
                   map.ary[0].arg == (void *)1,
               "map state error");
    fio___state_map_add(&map, (void (*)(void *))1, (void *)1);
    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))1, (void *)1),
               "double add failed? (exists is negative)");
    FIO_ASSERT(map.w == 1 && map.count == 1 && map.ary &&
                   map.ary[0].arg == (void *)1,
               "double add should be a no-op");
    fio___state_map_add(&map, (void (*)(void *))2, (void *)2);
    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))2, (void *)2),
               "add failed? (exists is negative)");
    FIO_ASSERT(map.w == 2 && map.count == 2 && map.ary[1].arg == (void *)2,
               "map state error");
    fio___state_map_add(&map, (void (*)(void *))3, (void *)3);
    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))3, (void *)3),
               "add failed? (exists is negative)");
    FIO_ASSERT(map.w == 3 && map.count == 3 && map.ary[2].arg == (void *)3,
               "map state error");

    map2.capa_bits = map.capa_bits;
    fio___state_map_alloc(&map2);
    FIO_ASSERT(map2.ary && map2.imap && map2.w == 0 && map2.count == 0 &&
                   map2.ary[2].arg == (void *)0,
               "map state error (alloc)");
    FIO_ASSERT(!fio___state_map_copy(&map2, &map), "map copy failed?");

    fio___state_map_remove(&map, (void (*)(void *))2, (void *)2);

    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))1, (void *)1),
               "remove failed? (exists is negative)");
    FIO_ASSERT(!fio___state_map_exists(&map, (void (*)(void *))2, (void *)2),
               "remove failed? (exists is positive)");
    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))3, (void *)3),
               "remove failed? (exists is negative)");

    FIO_ASSERT(map.w == 3 && map.count == 2 && map.ary[1].arg == (void *)0,
               "map state error (remove)");
    fio___state_map_remove(&map, (void (*)(void *))3, (void *)3);
    FIO_ASSERT(map.w == 1 && map.count == 1 && map.ary[2].arg == (void *)0,
               "map state error (remove)");

    FIO_ASSERT(fio___state_map_exists(&map, (void (*)(void *))1, (void *)1),
               "remove failed? (exists is negative)");
    FIO_ASSERT(!fio___state_map_exists(&map, (void (*)(void *))2, (void *)2),
               "remove failed? (exists is positive)");
    FIO_ASSERT(!fio___state_map_exists(&map, (void (*)(void *))3, (void *)3),
               "remove failed? (exists is positive)");

    FIO_ASSERT(map2.w == 3 && map2.count == 3 && map2.ary[2].arg == (void *)3,
               "map state error (copy)");

    fio___state_map_destroy(&map);
    fio___state_map_destroy(&map2);
  }
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_STATE
#endif /* FIO_STATE */
