# Linked Lists

```c
#include "fio-stl.h"
```

Small, sharp linked-list macros from [`./000 core.h`](./000%20core.h). There are two families:

- pointer-based circular doubly linked lists, using `next` / `prev` pointers;
- indexed circular doubly linked lists, using integer offsets into an array.

The macros do not allocate memory, do not own stored objects, and do not check that nodes are initialized. Fast tools, pointy edges.

### Types

#### `fio_list_node_s`

```c
typedef struct fio_list_node_s {
  struct fio_list_node_s *next;
  struct fio_list_node_s *prev;
} fio_list_node_s;
```

The pointer-based list link. Embed it inside the struct you want to place in a list.

#### `FIO_LIST_NODE`

```c
#define FIO_LIST_NODE fio_list_node_s
```

Semantic alias for an embedded node field.

#### `FIO_LIST_HEAD`

```c
#define FIO_LIST_HEAD fio_list_node_s
```

Semantic alias for a list head/root. A list head is also a node, and an empty list points back to itself.

#### `fio_index32_node_s`

```c
typedef struct fio_index32_node_s {
  uint32_t next;
  uint32_t prev;
} fio_index32_node_s;
```

A 32-bit indexed-list link. It stores indexes into a caller-owned array.

#### `fio_index16_node_s`

```c
typedef struct fio_index16_node_s {
  uint16_t next;
  uint16_t prev;
} fio_index16_node_s;
```

A 16-bit indexed-list link.

#### `fio_index8_node_s`

```c
typedef struct fio_index8_node_s {
  uint8_t next;
  uint8_t prev;
} fio_index8_node_s;
```

An 8-bit indexed-list link.

#### `FIO_INDEXED_LIST32_NODE`, `FIO_INDEXED_LIST32_HEAD`

```c
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
```

32-bit indexed-list node and head index types.

#### `FIO_INDEXED_LIST16_NODE`, `FIO_INDEXED_LIST16_HEAD`

```c
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
```

16-bit indexed-list node and head index types.

#### `FIO_INDEXED_LIST8_NODE`, `FIO_INDEXED_LIST8_HEAD`

```c
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t
```

8-bit indexed-list node and head index types.

### API Functions

#### `FIO_LIST_INIT`

```c
#define FIO_LIST_INIT(obj)                                                     \
  (fio_list_node_s) { .next = &(obj), .prev = &(obj) }
```

Initializer for a pointer-list node or head. The node points to itself.

Use it for heads:

```c
FIO_LIST_HEAD list = FIO_LIST_INIT(list);
```

and for standalone nodes before insertion:

```c
item_s item = {.node = FIO_LIST_INIT(item.node)};
```

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

Iterates forward over every object after `head`. `pos` is declared by the macro as `type *`.

The next pointer is cached before the loop body, so removing the current `pos` is supported.

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

Attempts to iterate backward over every object before `head`. `pos` is declared by the macro as `type *`.

**Note:** the current implementation caches the next pointer from `(head)->next->prev`, which equals `head` for a normal list, so the loop stops after visiting the first reversed node. Treat full reverse iteration as limited/buggy until the source macro is fixed.

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

Pushes node `n` to the end of the circular list, just before `head`.

Both `head` and `n` must point to initialized `FIO_LIST_NODE` / `FIO_LIST_HEAD` objects. The macro does not check whether `n` already belongs to another list.

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
  } while (0)
```

Removes `n` from its current list. The node's own `next` and `prev` fields are left pointing at the old neighbors.

#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)                                               \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
    (n)->next = (n)->prev = (n);                                               \
  } while (0)
```

Removes `n`, then resets it to a self-linked node.

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)                          \
  do {                                                                         \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));          \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);                                   \
  } while (0)
```

Removes the first object after `head` and stores the containing object pointer in `dest_ptr`.

Do not call this on an empty list. It will treat the head as a stored object and wander into undefined behavior.

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head)                                                \
  ((!(head)) || ((!(head)->next) | ((head)->next == (head))))
```

Returns non-zero if `head` is `NULL`, `head->next` is `NULL`, or the head points to itself.

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

Adds index `i` to the end of the indexed circular list, just before `head`. `head` is not changed.

`root` is the array containing all elements, and `node_name` is the embedded indexed-list node field.

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

Adds index `i` to the beginning of the list and assigns `head = i`. The `head` argument must be an assignable lvalue.

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

Removes index `i` from its current indexed list. The removed node keeps its previous links.

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

Removes index `i`, then resets its indexed node so `next == prev == i`.

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

Iterates forward over an indexed circular list, including `head`. `pos` is declared by the macro as `size_t`.

`head` must be a valid index. Empty-list sentinels are a caller convention, not something this macro handles.

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

Iterates backward over an indexed circular list, including the tail and eventually the head.

### Examples

#### Pointer list

```c
#include "fio-stl.h"

typedef struct item_s {
  FIO_LIST_NODE node;
  int value;
} item_s;

int main(void) {
  FIO_LIST_HEAD list = FIO_LIST_INIT(list);
  item_s a = {.node = FIO_LIST_INIT(a.node), .value = 1};
  item_s b = {.node = FIO_LIST_INIT(b.node), .value = 2};

  FIO_LIST_PUSH(&list, &a.node);
  FIO_LIST_PUSH(&list, &b.node);

  int sum = 0;
  FIO_LIST_EACH(item_s, node, &list, pos) {
    sum += pos->value;
  }

  item_s *first = 0;
  FIO_LIST_POP(item_s, node, first, &list);

  return (sum == 3 && first == &a) ? 0 : 1;
}
```

#### Safe removal while iterating

```c
#include "fio-stl.h"

typedef struct item_s {
  FIO_LIST_NODE node;
  int value;
} item_s;

int main(void) {
  FIO_LIST_HEAD list = FIO_LIST_INIT(list);
  item_s a = {.node = FIO_LIST_INIT(a.node), .value = 1};
  item_s b = {.node = FIO_LIST_INIT(b.node), .value = 2};

  FIO_LIST_PUSH(&list, &a.node);
  FIO_LIST_PUSH(&list, &b.node);

  FIO_LIST_EACH(item_s, node, &list, pos) {
    if (pos->value == 1)
      FIO_LIST_REMOVE_RESET(&pos->node);
  }

  return FIO_LIST_IS_EMPTY(&list) ? 1 : 0;
}
```

#### Indexed list

```c
#include "fio-stl.h"

typedef struct slot_s {
  FIO_INDEXED_LIST16_NODE node;
  int value;
} slot_s;

int main(void) {
  slot_s slots[3] = {{{0}}};
  FIO_INDEXED_LIST16_HEAD head = 0;

  slots[0].node.next = slots[0].node.prev = 0;
  slots[0].value = 10;

  slots[1].node.next = slots[1].node.prev = 1;
  slots[1].value = 20;

  slots[2].node.next = slots[2].node.prev = 2;
  slots[2].value = 30;

  FIO_INDEXED_LIST_PUSH(slots, node, head, 1);
  FIO_INDEXED_LIST_PUSH(slots, node, head, 2);

  int sum = 0;
  FIO_INDEXED_LIST_EACH(slots, node, head, pos) {
    sum += slots[pos].value;
  }

  FIO_INDEXED_LIST_REMOVE_RESET(slots, node, 1);

  return (sum == 60 && slots[1].node.next == 1) ? 0 : 1;
}
```

### Safety Notes

#### Initialization

Every head or node must be initialized before use. Pointer lists have `FIO_LIST_INIT`; indexed lists do not have a dedicated init macro, so initialize an indexed node with:

```c
root[i].node.next = root[i].node.prev = i;
```

#### Empty lists

Pointer lists have a natural empty state: `head->next == head`. `FIO_LIST_IS_EMPTY` checks it.

Indexed lists usually reserve an out-of-range value, often `(type)-1`, to mean empty. The indexed macros do not check for this sentinel; call them only when `head` is a valid index.

#### Index width

The indexed macros use `size_t` temporaries, then store into `uint32_t`, `uint16_t`, or `uint8_t` node fields. Keep every index representable by the chosen node type.

#### Removal and iteration

The iteration macros cache the next node/index before running the loop body, so removing the current non-head item is supported. If you remove or reset the current head in an indexed list, update your `head` value deliberately.

#### Memory ownership

Lists own no memory. Removing or popping unlinks nodes only; it does not destroy containing objects.

#### Thread safety

Mutating a list is not atomic. Multiple threads may read immutable lists, but concurrent mutation needs external synchronization.

#### Macro expansion

All APIs here are macros. Arguments may be evaluated more than once, and helper variable names are introduced inside loops. Pass simple lvalues/pointers, not expressions with side effects.
