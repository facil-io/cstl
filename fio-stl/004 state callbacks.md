# State Callbacks

```c
#define FIO_STATE
#include "fio-stl.h"
```

A small global event bus for lifecycle callbacks: initialize, fork, start, idle, shutdown, exit, and two user queues. Modules can listen without the caller knowing they exist. Sneaky, but polite. Implemented in [`./004 state callbacks.h`](./004%20state%20callbacks.h).

Use this as a global `FIO_EXTERN` module when multiple translation units should share the same callback registry.

### Types

#### `fio_state_event_type_e`

```c
typedef enum {
  FIO_CALL_ON_INITIALIZE,
  FIO_CALL_PRE_START,
  FIO_CALL_BEFORE_FORK,
  FIO_CALL_IN_CHILD,
  FIO_CALL_IN_MASTER,
  FIO_CALL_AFTER_FORK,
  FIO_CALL_ON_WORKER_THREAD_START,
  FIO_CALL_ON_START,
  FIO_CALL_RESERVED1,
  FIO_CALL_RESERVED2,
  FIO_CALL_ON_USER1,
  FIO_CALL_ON_USER2,
  FIO_CALL_ON_IDLE,
  FIO_CALL_ON_USER1_REVERSE,
  FIO_CALL_ON_USER2_REVERSE,
  FIO_CALL_RESERVED1_REVERSED,
  FIO_CALL_RESERVED2_REVERSED,
  FIO_CALL_ON_SHUTDOWN,
  FIO_CALL_ON_PARENT_CRUSH,
  FIO_CALL_ON_CHILD_CRUSH,
  FIO_CALL_ON_WORKER_THREAD_END,
  FIO_CALL_ON_STOP,
  FIO_CALL_AT_EXIT,
  FIO_CALL_AFTER_EXIT,
  FIO_CALL_NEVER
} fio_state_event_type_e;
```

Event queues. `FIO_CALL_NEVER` is the sentinel and should not be used as an event.

**Ordering:**
- events `<= FIO_CALL_ON_IDLE` run in registration order
- events `>= FIO_CALL_ON_SHUTDOWN` run in reverse registration order

`FIO_CALL_ON_USER1` / `FIO_CALL_ON_USER2` and their reverse variants are available for user code.

### Callback Management

#### `fio_state_callback_add`

```c
SFUNC void fio_state_callback_add(fio_state_event_type_e,
                                  void (*func)(void *),
                                  void *arg);
```

Adds `func(arg)` to an event queue. Adding the same function / argument pair replaces the existing map entry rather than creating a duplicate.

If `FIO_CALL_ON_INITIALIZE` already ran, adding another initialize callback calls it immediately.

#### `fio_state_callback_remove`

```c
SFUNC int fio_state_callback_remove(fio_state_event_type_e,
                                    void (*func)(void *),
                                    void *arg);
```

Removes `func(arg)` from an event queue.

**Returns:** the internal map removal result, or `-1` if the event is invalid.

#### `fio_state_callback_clear`

```c
SFUNC void fio_state_callback_clear(fio_state_event_type_e e);
```

Clears all callbacks for `e`. Invalid events are ignored.

This function is implemented in the header's implementation section; with `FIO_EXTERN`, expose the implementation in exactly one translation unit as usual.

#### `fio_state_callback_force`

```c
SFUNC void fio_state_callback_force(fio_state_event_type_e);
```

Runs the callbacks for an event as if the event happened.

During a forced event, the callback list is copied first. Additions and removals for the same event do not affect the current run.

Special behavior:
- `FIO_CALL_ON_INITIALIZE` runs only once and reserves internal maps
- `FIO_CALL_IN_CHILD` reinitializes internal locks and reseeds randomness
- `FIO_CALL_AFTER_FORK` reseeds randomness

### Debug Helpers

#### `fio_state_callback_print_state`

```c
FIO_IFUNC void fio_state_callback_print_state(void);
```

Prints callback counts and capacities for every event queue to `stderr`.

### Thread-Safety

Each event queue has its own lock. Adding, removing, clearing, and forcing callbacks are synchronized while touching the queue. Callback execution happens after the queue has been copied and unlocked.

### Example

```c
#define FIO_STATE
#include "fio-stl.h"
#include <stdio.h>

static void hello(void *arg) {
  printf("hello %s\n", (const char *)arg);
}

int main(void) {
  fio_state_callback_add(FIO_CALL_ON_USER1, hello, "callbacks");
  fio_state_callback_force(FIO_CALL_ON_USER1);
  fio_state_callback_remove(FIO_CALL_ON_USER1, hello, "callbacks");
  return 0;
}
```

---
