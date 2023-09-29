## State Callbacks

The state callback API, which is also used internally by stateful modules such as the memory allocator, allows callbacks to be registered for specific changes in the state of the app.

This allows modules to react to changes in the state of the program without requiring the functions that caused the change in state to know about each of the modules that wish to react, only requiting it to publish a notification by calling `fio_state_callback_force`.

When using this module it is better if it is used as a global `FIO_EXTERN` module, so state notifications are not limited to the scope of the C file (the translation unit).

By defining the `FIO_STATE` macro, the following are defined:

**Note:** this module depends on the `FIO_RAND`, `FIO_ATOMIC`, and `FIO_IMAP_CORE` modules which will be automatically included.

#### `fio_state_callback_add`

```c
void fio_state_callback_add(fio_state_event_type_e event,
                            void (*func)(void *),
                            void *arg);
```

Adds a callback to the list of callbacks to be called for the `event`.

The callback should accept a single `void *` as an argument.

Events are performed either in the order in which they were registered or in reverse order, depending on the context.

These are the possible `event` values, note that some of them are only relevant in the context of the `FIO_SERVER` module:

```c
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
  /** Called by each worker thread in a Server Async queue as it starts. */
  FIO_CALL_ON_WORKER_THREAD_START,
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

  /* the following events are performed in reverse (LIFO): */

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
  /** Called by each worker thread in a Server Async queue as it ends. */
  FIO_CALL_ON_WORKER_THREAD_END,
  /** Called just before finishing up (both on child and parent processes). */
  FIO_CALL_ON_FINISH,
  /** An alternative to the system's at_exit. */
  FIO_CALL_AT_EXIT,
  /** used for testing and array allocation - must be last. */
  FIO_CALL_NEVER
} fio_state_event_type_e;

```

#### `fio_state_callback_remove`

```c
int fio_state_callback_remove(fio_state_event_type_e,
                              void (*func)(void *),
                              void *arg);
```

Removes a callback from the list of callbacks to be called for the event.

See also [`fio_state_callback_add`](#fio_state_callback_add) for details of possible events.

#### `fio_state_callback_force`

```c
void fio_state_callback_force(fio_state_event_type_e);
```

Forces all the existing callbacks to run, as if the event occurred.

Callbacks for all initialization / idling tasks are called in order of creation (where `fio_state_event_type_e` <= `FIO_CALL_ON_IDLE`).

Callbacks for all cleanup oriented tasks are called in reverse order of creation (where `fio_state_event_type_e` >= `FIO_CALL_ON_USER1_REVERSE`).

During an event, changes to the callback list are ignored (callbacks can't add or remove other callbacks for the same event).

-------------------------------------------------------------------------------
