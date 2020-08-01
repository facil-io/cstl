## Linked Lists

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element 
typedef struct {
  char * data;
  FIO_LIST_NODE node;
} my_list_s;
// create linked list helper functions
#define FIO_LIST_NAME my_list
#include "fio-stl.h"
```

Doubly Linked Lists are an incredibly common and useful data structure.

### Linked Lists Performance

Memory overhead (on 64bit machines) is 16 bytes per node (or 8 bytes on 32 bit machines) for the `next` and `prev` pointers.

Linked Lists use pointers in order to provide fast add/remove operations with O(1) speeds. This O(1) operation ignores the object allocation time and suffers from poor memory locality, but it's still very fast.

However, Linked Lists suffer from slow seek/find and iteration operations.

Seek/find has a worst case scenario O(n) cost and iteration suffers from a high likelihood of CPU cache misses, resulting in degraded performance.

### Linked Lists Overview

Before creating linked lists, the library header should be included at least once.

To create a linked list type, create a `struct` that includes a `FIO_LIST_NODE` typed element somewhere within the structure. For example:

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element
typedef struct {
  long l;
  FIO_LIST_NODE node;
  int i;
  FIO_LIST_NODE node2;
  double d;
} my_list_s;
```

Next define the `FIO_LIST_NAME` macro. The linked list helpers and types will all be prefixed by this name. i.e.:

```c
#define FIO_LIST_NAME my_list /* defines list functions (example): my_list_push(...) */
```

Optionally, define the `FIO_LIST_TYPE` macro to point at the correct linked-list structure type. By default, the type for linked lists will be `<FIO_LIST_NAME>_s`.

An example were we need to define the `FIO_LIST_TYPE` macro will follow later on.

Optionally, define the `FIO_LIST_NODE_NAME` macro to point the linked list's node. By default, the node for linked lists will be `node`.

Finally, include the `fio-stl.h` header to create the linked list helpers.

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element 
typedef struct {
  long l;
  FIO_LIST_NODE node;
  int i;
  FIO_LIST_NODE node2;
  double d;
} my_list_s;
// create linked list helper functions
#define FIO_LIST_NAME my_list
#include "fio-stl.h"

void example(void) {
  FIO_LIST_HEAD list = FIO_LIST_INIT(list);
  for (int i = 0; i < 10; ++i) {
    my_list_s *n = malloc(sizeof(*n));
    n->i = i;
    my_list_push(&list, n);
  }
  int i = 0;
  while (my_list_any(&list)) {
    my_list_s *n = my_list_shift(&list);
    if (i != n->i) {
      fprintf(stderr, "list error - value mismatch\n"), exit(-1);
    }
    free(n);
    ++i;
  }
  if (i != 10) {
    fprintf(stderr, "list error - count error\n"), exit(-1);
  }
}
```

**Note**:

Each node is limited to a single list (an item can't belong to more then one list, unless it's a list of pointers to that item).

Items with more then a single node can belong to more then one list. i.e.:

```c
// list element 
typedef struct {
  long l;
  FIO_LIST_NODE node;
  int i;
  FIO_LIST_NODE node2;
  double d;
} my_list_s;
// list 1 
#define FIO_LIST_NAME my_list
#include "fio-stl.h"
// list 2 
#define FIO_LIST_NAME my_list2
#define FIO_LIST_TYPE my_list_s
#define FIO_LIST_NODE_NAME node2
#include "fio-stl.h"
```

### Linked Lists (embeded) - API


#### `FIO_LIST_INIT(head)`

```c
#define FIO_LIST_INIT(obj)                                                     \
  { .next = &(obj), .prev = &(obj) }
```

This macro initializes an uninitialized node (assumes the data in the node is junk). 

#### `LIST_any`

```c
int LIST_any(FIO_LIST_HEAD *head)
```
Returns a non-zero value if there are any linked nodes in the list. 

#### `LIST_is_empty`

```c
int LIST_is_empty(FIO_LIST_HEAD *head)
```
Returns a non-zero value if the list is empty. 

#### `LIST_remove`

```c
FIO_LIST_TYPE *LIST_remove(FIO_LIST_TYPE *node)
```
Removes a node from the list, Returns NULL if node isn't linked. 

#### `LIST_push`

```c
FIO_LIST_TYPE *LIST_push(FIO_LIST_HEAD *head, FIO_LIST_TYPE *node)
```
Pushes an existing node to the end of the list. Returns node. 

#### `LIST_pop`

```c
FIO_LIST_TYPE *LIST_pop(FIO_LIST_HEAD *head)
```
Pops a node from the end of the list. Returns NULL if list is empty. 

#### `LIST_unshift`

```c
FIO_LIST_TYPE *LIST_unshift(FIO_LIST_HEAD *head, FIO_LIST_TYPE *node)
```
Adds an existing node to the beginning of the list. Returns node.

#### `LIST_shift`

```c
FIO_LIST_TYPE *LIST_shift(FIO_LIST_HEAD *head)
```
Removed a node from the start of the list. Returns NULL if list is empty. 

#### `LIST_root`

```c
FIO_LIST_TYPE *LIST_root(FIO_LIST_HEAD *ptr)
```
Returns a pointer to a list's element, from a pointer to a node. 

#### `FIO_LIST_EACH`

_Note: macro, name unchanged, works for all lists_

```c
#define FIO_LIST_EACH(type, node_name, head, pos)                              \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->next),          \
            *next____p_ls =                                                    \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->next);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls),                                                   \
            (next____p_ls = FIO_PTR_FROM_FIELD(type, node_name,                \
                                               next____p_ls->node_name.next)))
```

Loops through every node in the linked list (except the head).

The `type` name should reference the list type.

`node_name` should indicate which node should be used for iteration.

`head` should point at the head of the list (usually a `FIO_LIST_HEAD` variable).

`pos` can be any temporary variable name that will contain the current position in the iteration.

The list **can** be mutated during the loop, but this is not recommended. Specifically, removing `pos` is safe, but pushing elements ahead of `pos` might result in an endless loop.

_Note: this macro won't work with pointer tagging_

-------------------------------------------------------------------------------
