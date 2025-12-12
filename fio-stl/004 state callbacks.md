## State Callbacks

```c
#define FIO_STATE
#include "fio-stl.h"
```

The state callback API allows callbacks to be registered for specific changes in the state of the application. This is also used internally by stateful modules such as the memory allocator.

This allows modules to react to changes in the state of the program without requiring the functions that caused the change in state to know about each of the modules that wish to react, only requiring it to publish a notification by calling `fio_state_callback_force`.

When using this module it is better if it is used as a global `FIO_EXTERN` module, so state notifications are not limited to the scope of the C file (the translation unit).

**Note**: this module depends on the `FIO_RAND`, `FIO_ATOMIC`, and `FIO_IMAP_CORE` modules which will be automatically included.

### Event Types

#### `fio_state_event_type_e`

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
  /** Called when either a *Worker* or *Master* stopped. */
  FIO_CALL_ON_STOP,
  /** An alternative to the system's at_exit. */
  FIO_CALL_AT_EXIT,
  /** used for testing and array allocation - must be last. */
  FIO_CALL_NEVER
} fio_state_event_type_e;
```

An enumeration of event types that can be used with the state callback API.

Events are performed either in the order in which they were registered (FIFO) or in reverse order (LIFO), depending on the event type:

- Events where `event <= FIO_CALL_ON_IDLE` are called in order of registration (FIFO)
- Events where `event >= FIO_CALL_ON_SHUTDOWN` are called in reverse order (LIFO)

**Note**: some events are only relevant in the context of the `FIO_SERVER` module and were designed for the server's use.

### State Callback API

#### `fio_state_callback_add`

```c
void fio_state_callback_add(fio_state_event_type_e event,
                            void (*func)(void *),
                            void *arg);
```

Adds a callback to the list of callbacks to be called for the `event`.

The callback should accept a single `void *` as an argument.

**Parameters:**
- `event` - the event type to register the callback for
- `func` - the callback function to be called when the event occurs
- `arg` - an opaque pointer that will be passed to the callback

**Note**: if `FIO_CALL_ON_INITIALIZE` callbacks have already been performed and a new callback is added for that event, the callback will be executed immediately.

#### `fio_state_callback_remove`

```c
int fio_state_callback_remove(fio_state_event_type_e event,
                              void (*func)(void *),
                              void *arg);
```

Removes a callback from the list of callbacks to be called for the event.

**Parameters:**
- `event` - the event type to remove the callback from
- `func` - the callback function to remove
- `arg` - the opaque pointer that was passed when adding the callback

**Returns:** `0` on success, `-1` if the callback was not found or the event type is invalid.

#### `fio_state_callback_clear`

```c
void fio_state_callback_clear(fio_state_event_type_e event);
```

Clears all the existing callbacks for the specified event.

**Parameters:**
- `event` - the event type to clear all callbacks for

#### `fio_state_callback_force`

```c
void fio_state_callback_force(fio_state_event_type_e event);
```

Forces all the existing callbacks to run, as if the event occurred.

Callbacks for all initialization / idling tasks are called in order of creation (where `event <= FIO_CALL_ON_IDLE`).

Callbacks for all cleanup oriented tasks are called in reverse order of creation (where `event >= FIO_CALL_ON_SHUTDOWN`).

**Note**: during an event, changes to the callback list are ignored (callbacks can't add or remove other callbacks for the same event).

**Note**: when `FIO_CALL_AFTER_FORK` is forced, all internal locks are re-initialized. When `FIO_CALL_IN_CHILD` is forced, the random generator is re-seeded.

### Example

```c
#define FIO_STATE
#define FIO_LOG
#include "fio-stl.h"

static void my_cleanup(void *arg) {
  FIO_LOG_INFO("Cleanup called with arg: %p", arg);
}

static void my_startup(void *arg) {
  FIO_LOG_INFO("Startup called with arg: %p", arg);
}

int main(void) {
  /* Register a callback for when the application starts */
  fio_state_callback_add(FIO_CALL_ON_START, my_startup, (void *)0x1234);
  
  /* Register a cleanup callback for when the application exits */
  fio_state_callback_add(FIO_CALL_AT_EXIT, my_cleanup, (void *)0x5678);
  
  /* Force the ON_START event to run all registered callbacks */
  fio_state_callback_force(FIO_CALL_ON_START);
  
  /* Remove a specific callback */
  fio_state_callback_remove(FIO_CALL_ON_START, my_startup, (void *)0x1234);
  
  /* Clear all callbacks for a specific event */
  fio_state_callback_clear(FIO_CALL_ON_USER1);
  
  return 0;
  /* AT_EXIT callbacks will be called automatically on program exit */
}
```

-------------------------------------------------------------------------------
