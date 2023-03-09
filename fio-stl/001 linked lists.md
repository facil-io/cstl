## Doubly Linked Lists

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element 
typedef struct {
  FIO_LIST_NODE node;
  char * data;
} my_list_s;
```

Doubly Linked Lists are an incredibly common and useful data structure.

### Linked Lists Performance

Memory overhead (on 64bit machines) is 16 bytes per node (or 8 bytes on 32 bit machines) for the `next` and `prev` pointers.

Linked Lists use pointers in order to provide fast add/remove operations with O(1) speeds. This O(1) operation ignores the object allocation time and suffers from poor memory locality, but it's still very fast.

However, Linked Lists suffer from slow seek/find and iteration operations.

Seek/find has a worst case scenario O(n) cost and iteration suffers from a high likelihood of CPU cache misses, resulting in degraded performance.

### Linked Lists Macros

Linked List Macros (and arch-type) are always defined by the CSTL and can be used to manage linked lists without creating a dedicated type.

#### `FIO_LIST_NODE` / `FIO_LIST_HEAD`

```c
/** A linked list node type */
#define FIO_LIST_NODE fio_list_node_s
/** A linked list head type */
#define FIO_LIST_HEAD fio_list_node_s
/** A linked list arch-type */
typedef struct fio_list_node_s {
  struct fio_list_node_s *next;
  struct fio_list_node_s *prev;
} fio_list_node_s;

```

These are the basic core types for a linked list node used by the Linked List macros.

#### `FIO_LIST_INIT(head)`

```c
#define FIO_LIST_INIT(obj)                                                     \
  (FIO_LIST_HEAD){ .next = &(obj), .prev = &(obj) }
```

Initializes a linked list.

#### `FIO_LIST_PUSH`

```c
#define FIO_LIST_PUSH(head, n)                                                 \
  do {                                                                         \
    (n)->prev = (head)->prev;                                                  \
    (n)->next = (head);                                                        \
    (head)->prev->next = (n);                                                  \
    (head)->prev = (n);                                                        \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)                          \
  do {                                                                         \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));          \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);                                   \
  } while (0)
```

UNSAFE macro for popping a node from a list.

* `type` is the underlying `struct` type of the next list member.

* `node_name` is the field name in the `type` that is the `FIO_LIST_NODE` linking type.

* `dest_prt` is the pointer that will accept the next list member.

* `head` is the head of the list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

Note that using this macro with an empty list will produce **undefined behavior**.

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.


#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
    (n)->next = (n)->prev = (n);                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data so it links to itself.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

#### `FIO_LIST_EACH`

```c
#define FIO_LIST_EACH(type, node_name, head, pos)                              \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->next),          \
            *next____p_ls_##pos =                                              \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->next);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls_##pos),                                             \
            (next____p_ls_##pos =                                              \
                 FIO_PTR_FROM_FIELD(type,                                      \
                                    node_name,                                 \
                                    next____p_ls_##pos->node_name.next)))
```

Loops through every node in the linked list except the head.

This macro allows `pos` to point to the type that the linked list contains (rather than a pointer to the node type).

i.e.,

```c
typedef struct {
  void * data;
  FIO_LIST_HEAD node;
} ptr_list_s;

FIO_LIST_HEAD my_ptr_list = FIO_LIST_INIT(my_ptr_list);

/* ... */

FIO_LIST_EACH(ptr_list_s, node, &my_ptr_list, pos) {
  do_something_with(pos->data);
}
```

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head) (!(head) || (head)->next == (head)->prev)
```

Macro for testing if a list is empty.


### Indexed Linked Lists Macros (always defined):


Indexed linked lists are often used to either save memory or making it easier to reallocate the memory used for the whole list. This is performed by listing pointer offsets instead of the whole pointer, allowing the offsets to use smaller type sizes.

For example, an Indexed Linked List might be added to objects in a cache array in order to implement a "least recently used" eviction policy. If the cache holds less than 65,536 members, than a 16 bit index is all that's required, reducing the list's overhead from 2 pointers (16 bytes on 64 bit systems) to a 4 byte overhead per cache member.

#### `FIO_INDEXED_LIST##_HEAD` / `FIO_INDEXED_LIST##_NODE`

```c
/** A 32 bit indexed linked list node type */
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
/** A 16 bit indexed linked list node type */
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
/** An 8 bit indexed linked list node type */
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t

/** A 32 bit indexed linked list node type */
typedef struct fio_index32_node_s {
  uint32_t next;
  uint32_t prev;
} fio_index32_node_s;

/** A 16 bit indexed linked list node type */
typedef struct fio_index16_node_s {
  uint16_t next;
  uint16_t prev;
} fio_index16_node_s;

/** An 8 bit indexed linked list node type */
typedef struct fio_index8_node_s {
  uint8_t next;
  uint8_t prev;
} fio_index8_node_s;
```

#### `FIO_INDEXED_LIST_PUSH`

```c
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)                        \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;                \
    (root)[n__].node_name.next = (head);                                       \
    (root)[(root)[(head)].node_name.prev].node_name.next = n__;                \
    (root)[(head)].node_name.prev = n__;                                       \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

#### `FIO_INDEXED_LIST_REMOVE`

```c
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)                            \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
  } while (0)
```

UNSAFE macro for removing a node from a list.

#### `FIO_INDEXED_LIST_REMOVE_RESET`

```c
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)                            \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
    (root)[n__].node_name.next = (root)[n__].node_name.prev = n__;             \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data so it links to itself.

#### `FIO_INDEXED_LIST_EACH`

```c
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)                      \
  for (size_t pos = (head), stopper___ils___ = 0; !stopper___ils___;           \
       stopper___ils___ = ((pos = (root)[pos].node_name.next) == (head)))
```

Loops through every index in the indexed list, **assuming `head` is valid**.
