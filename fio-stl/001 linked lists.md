## Doubly Linked Lists

```c
#include "fio-stl.h"
```

Doubly Linked Lists are an incredibly common and useful data structure. The facil.io C STL provides macros and types for managing both pointer-based linked lists and indexed linked lists.

These macros are always defined by the CSTL core and can be used without defining any additional macros.

### Linked Lists Performance

Memory overhead (on 64bit machines) is 16 bytes per node (or 8 bytes on 32 bit machines) for the `next` and `prev` pointers.

Linked Lists use pointers in order to provide fast add/remove operations with O(1) speeds. This O(1) operation ignores the object allocation time and suffers from poor memory locality, but it's still very fast.

However, Linked Lists suffer from slow seek/find and iteration operations.

Seek/find has a worst case scenario O(n) cost and iteration suffers from a high likelihood of CPU cache misses, resulting in degraded performance.

-------------------------------------------------------------------------------

### Linked List Types

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

`FIO_LIST_NODE` and `FIO_LIST_HEAD` are both aliases for `fio_list_node_s`. The distinction is semantic - use `FIO_LIST_HEAD` for the list's head/root and `FIO_LIST_NODE` for nodes embedded in your data structures.

Example:

```c
typedef struct {
  FIO_LIST_NODE node;
  char *data;
} my_list_s;

FIO_LIST_HEAD my_list = FIO_LIST_INIT(my_list);
```

-------------------------------------------------------------------------------

### Linked List Macros

#### `FIO_LIST_INIT`

```c
#define FIO_LIST_INIT(obj) (fio_list_node_s){ .next = &(obj), .prev = &(obj) }
```

Initializes a linked list head so it points to itself (indicating an empty list).

Example:

```c
FIO_LIST_HEAD my_list = FIO_LIST_INIT(my_list);
```

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

UNSAFE macro for pushing a node to the end of a list (before the head).

**Parameters:**
- `head` - pointer to the list head (`FIO_LIST_HEAD *`)
- `n` - pointer to the node to push (`FIO_LIST_NODE *`)

**Note**: this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)                          \
  do {                                                                         \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));          \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);                                   \
  } while (0)
```

UNSAFE macro for popping a node from the beginning of a list.

**Parameters:**
- `type` - the underlying `struct` type of the list member
- `node_name` - the field name in `type` that is the `FIO_LIST_NODE` linking type
- `dest_ptr` - the pointer that will receive the popped list member
- `head` - pointer to the list head

**Note**: this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

**Note**: using this macro with an empty list will produce **undefined behavior**.

Example:

```c
typedef struct {
  FIO_LIST_NODE node;
  int value;
} item_s;

FIO_LIST_HEAD list = FIO_LIST_INIT(list);
// ... add items to list ...

item_s *popped;
FIO_LIST_POP(item_s, node, popped, &list);
// popped now points to the first item, which has been removed from the list
```

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list.

**Parameters:**
- `n` - pointer to the node to remove (`FIO_LIST_NODE *`)

**Note**: this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

**Note**: after removal, the node's `next` and `prev` pointers still point to their old neighbors. Use `FIO_LIST_REMOVE_RESET` if you need the node to be self-referential after removal.

#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)                                               \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
    (n)->next = (n)->prev = (n);                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list and resetting its pointers to point to itself.

**Parameters:**
- `n` - pointer to the node to remove (`FIO_LIST_NODE *`)

**Note**: this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

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

**Parameters:**
- `type` - the underlying `struct` type of the list members
- `node_name` - the field name in `type` that is the `FIO_LIST_NODE`
- `head` - pointer to the list head
- `pos` - the variable name to use for the current position in the loop

**Note**: it is safe to remove the current node (`pos`) during iteration.

Example:

```c
typedef struct {
  FIO_LIST_NODE node;
  void *data;
} ptr_list_s;

FIO_LIST_HEAD my_ptr_list = FIO_LIST_INIT(my_ptr_list);

// ... add items to list ...

FIO_LIST_EACH(ptr_list_s, node, &my_ptr_list, pos) {
  do_something_with(pos->data);
}
```

#### `FIO_LIST_EACH_REVERSED`

```c
#define FIO_LIST_EACH_REVERSED(type, node_name, head, pos)                     \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->prev),          \
            *next____p_ls_##pos =                                              \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->prev);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls_##pos),                                             \
            (next____p_ls_##pos =                                              \
                 FIO_PTR_FROM_FIELD(type,                                      \
                                    node_name,                                 \
                                    next____p_ls_##pos->node_name.prev)))
```

Loops through every node in the linked list in reverse order (from tail to head).

**Parameters:**
- `type` - the underlying `struct` type of the list members
- `node_name` - the field name in `type` that is the `FIO_LIST_NODE`
- `head` - pointer to the list head
- `pos` - the variable name to use for the current position in the loop

**Note**: it is safe to remove the current node (`pos`) during iteration.

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head)                                                \
  ((!(head)) || ((!(head)->next) | ((head)->next == (head))))
```

Macro for testing if a list is empty.

**Parameters:**
- `head` - pointer to the list head

**Returns:** non-zero (true) if the list is empty or `head` is NULL, zero (false) otherwise.

-------------------------------------------------------------------------------

### Indexed Linked Lists

Indexed linked lists are often used to either save memory or make it easier to reallocate the memory used for the whole list. This is performed by storing index offsets instead of full pointers, allowing the offsets to use smaller type sizes.

For example, an Indexed Linked List might be added to objects in a cache array in order to implement a "least recently used" eviction policy. If the cache holds less than 65,536 members, then a 16 bit index is all that's required, reducing the list's overhead from 2 pointers (16 bytes on 64 bit systems) to a 4 byte overhead per cache member.

The "head" index is usually validated by reserving the value of `-1` (or the maximum value for the type) to indicate an empty list.

-------------------------------------------------------------------------------

### Indexed Linked List Types

#### `FIO_INDEXED_LIST32_NODE` / `FIO_INDEXED_LIST32_HEAD`

```c
/** A 32 bit indexed linked list node type */
typedef struct fio_index32_node_s {
  uint32_t next;
  uint32_t prev;
} fio_index32_node_s;

#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
```

A 32 bit indexed linked list node type, supporting up to 4,294,967,295 elements.

#### `FIO_INDEXED_LIST16_NODE` / `FIO_INDEXED_LIST16_HEAD`

```c
/** A 16 bit indexed linked list node type */
typedef struct fio_index16_node_s {
  uint16_t next;
  uint16_t prev;
} fio_index16_node_s;

#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
```

A 16 bit indexed linked list node type, supporting up to 65,535 elements.

#### `FIO_INDEXED_LIST8_NODE` / `FIO_INDEXED_LIST8_HEAD`

```c
/** An 8 bit indexed linked list node type */
typedef struct fio_index8_node_s {
  uint8_t next;
  uint8_t prev;
} fio_index8_node_s;

#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t
```

An 8 bit indexed linked list node type, supporting up to 255 elements.

-------------------------------------------------------------------------------

### Indexed Linked List Macros

#### `FIO_INDEXED_LIST_PUSH`

```c
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)                        \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;                \
    (root)[n__].node_name.next = (head);                                       \
    (root)[(root)[(head)].node_name.prev].node_name.next = (n__);              \
    (root)[(head)].node_name.prev = (n__);                                     \
  } while (0)
```

UNSAFE macro for pushing a node to the end of an indexed list (before the head).

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `head` - the index of the list head
- `i` - the index of the element to push

#### `FIO_INDEXED_LIST_UNSHIFT`

```c
#define FIO_INDEXED_LIST_UNSHIFT(root, node_name, head, i)                     \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.next = (root)[(head)].node_name.next;                \
    (root)[n__].node_name.prev = (head);                                       \
    (root)[(root)[(head)].node_name.next].node_name.prev = (n__);              \
    (root)[(head)].node_name.next = (n__);                                     \
    (head) = (n__);                                                            \
  } while (0)
```

UNSAFE macro for adding a node to the beginning of an indexed list (making it the new head).

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `head` - the index of the list head (will be updated to the new head)
- `i` - the index of the element to add

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

UNSAFE macro for removing a node from an indexed list.

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `i` - the index of the element to remove

#### `FIO_INDEXED_LIST_REMOVE_RESET`

```c
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)                      \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
    (root)[n__].node_name.next = (root)[n__].node_name.prev = (n__);           \
  } while (0)
```

UNSAFE macro for removing a node from an indexed list and resetting its links to point to itself.

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `i` - the index of the element to remove

#### `FIO_INDEXED_LIST_EACH`

```c
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)                      \
  for (size_t pos = (head),                                                    \
              stooper___hd = (head),                                           \
              stopper___ils___ = 0,                                            \
              pos##___nxt = (root)[(head)].node_name.next;                     \
       !stopper___ils___;                                                      \
       (stopper___ils___ = ((pos = pos##___nxt) == stooper___hd)),             \
              pos##___nxt = (root)[pos].node_name.next)
```

Loops through every index in the indexed list, **assuming `head` is valid**.

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `head` - the index of the list head
- `pos` - the variable name to use for the current index in the loop

**Note**: it is safe to remove the current element during iteration.

Example:

```c
typedef struct {
  FIO_INDEXED_LIST32_NODE node;
  int value;
} indexed_item_s;

indexed_item_s items[100];
uint32_t head = 0;

// Initialize head to point to itself
items[0].node.next = items[0].node.prev = 0;

// ... add items to list ...

FIO_INDEXED_LIST_EACH(items, node, head, pos) {
  printf("Item at index %zu has value %d\n", pos, items[pos].value);
}
```

#### `FIO_INDEXED_LIST_EACH_REVERSED`

```c
#define FIO_INDEXED_LIST_EACH_REVERSED(root, node_name, head, pos)             \
  for (size_t pos = ((root)[(head)].node_name.prev),                           \
              pos##___nxt =                                                    \
                  ((root)[((root)[(head)].node_name.prev)].node_name.prev),    \
              stooper___hd = (head),                                           \
              stopper___ils___ = 0;                                            \
       !stopper___ils___;                                                      \
       ((stopper___ils___ = (pos == stooper___hd)),                            \
        (pos = pos##___nxt),                                                   \
        (pos##___nxt = (root)[pos##___nxt].node_name.prev)))
```

Loops through every index in the indexed list in reverse order, **assuming `head` is valid**.

**Parameters:**
- `root` - pointer to the array containing the list elements
- `node_name` - the field name in the element type that is the indexed list node
- `head` - the index of the list head
- `pos` - the variable name to use for the current index in the loop

**Note**: it is safe to remove the current element during iteration.

-------------------------------------------------------------------------------
