/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                            Linked Lists (embeded)








Example:

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element
typedef struct {
  long l;
  FIO_LIST_NODE node;
  int i;
  double d;
} my_list_s;
// create linked list helper functions
#define FIO_LIST_NAME my_list
#include "fio-stl.h"

void example(void) {
  FIO_LIST_HEAD FIO_LIST_INIT(list);
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

***************************************************************************** */

/* *****************************************************************************
Linked Lists (embeded) - Type
***************************************************************************** */

#if defined(FIO_LIST_NAME)

#ifndef FIO_LIST_TYPE
/** Name of the list type and function prefix, defaults to FIO_LIST_NAME_s */
#define FIO_LIST_TYPE FIO_NAME(FIO_LIST_NAME, s)
#endif

#ifndef FIO_LIST_NODE_NAME
/** List types must contain at least one node element, defaults to `node`. */
#define FIO_LIST_NODE_NAME node
#endif

#ifdef FIO_PTR_TAG_TYPE
#define FIO_LIST_TYPE_PTR FIO_PTR_TAG_TYPE
#else
#define FIO_LIST_TYPE_PTR FIO_LIST_TYPE *
#endif

/* *****************************************************************************
Linked Lists (embeded) - API
***************************************************************************** */

/** Initialize FIO_LIST_HEAD objects - already defined. */
/* FIO_LIST_INIT(obj) */

/** Returns a non-zero value if there are any linked nodes in the list. */
IFUNC int FIO_NAME(FIO_LIST_NAME, any)(const FIO_LIST_HEAD *head);

/** Returns a non-zero value if the list is empty. */
IFUNC int FIO_NAME_BL(FIO_LIST_NAME, empty)(const FIO_LIST_HEAD *head);

/** Removes a node from the list, Returns NULL if node isn't linked. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, remove)(FIO_LIST_TYPE_PTR node);

/** Pushes an existing node to the end of the list. Returns node. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 push)(FIO_LIST_HEAD *restrict head,
                                       FIO_LIST_TYPE_PTR restrict node);

/** Pops a node from the end of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, pop)(FIO_LIST_HEAD *head);

/** Adds an existing node to the beginning of the list. Returns node. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 unshift)(FIO_LIST_HEAD *restrict head,
                                          FIO_LIST_TYPE_PTR restrict node);

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, shift)(FIO_LIST_HEAD *head);

/** Returns a pointer to a list's element, from a pointer to a node. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, root)(FIO_LIST_HEAD *ptr);

/* *****************************************************************************
Linked Lists (embeded) - Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/** Returns a non-zero value if there are any linked nodes in the list. */
IFUNC int FIO_NAME(FIO_LIST_NAME, any)(const FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, 0);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  return head->next != head;
}

/** Returns a non-zero value if the list is empty. */
IFUNC int FIO_NAME_BL(FIO_LIST_NAME, empty)(const FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, 0);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  return head->next == head;
}

/** Removes a node from the list, always returning the node. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 remove)(FIO_LIST_TYPE_PTR node_) {
  FIO_PTR_TAG_VALID_OR_RETURN(node_, (FIO_LIST_TYPE_PTR)0);
  FIO_LIST_TYPE *node = (FIO_LIST_TYPE *)(FIO_PTR_UNTAG(node_));
  if (node->FIO_LIST_NODE_NAME.next == &node->FIO_LIST_NODE_NAME)
    return NULL;
  node->FIO_LIST_NODE_NAME.prev->next = node->FIO_LIST_NODE_NAME.next;
  node->FIO_LIST_NODE_NAME.next->prev = node->FIO_LIST_NODE_NAME.prev;
  node->FIO_LIST_NODE_NAME.next = node->FIO_LIST_NODE_NAME.prev =
      &node->FIO_LIST_NODE_NAME;
  return node_;
}

/** Pushes an existing node to the end of the list. Returns node or NULL. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 push)(FIO_LIST_HEAD *restrict head,
                                       FIO_LIST_TYPE_PTR restrict node_) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  FIO_PTR_TAG_VALID_OR_RETURN(node_, (FIO_LIST_TYPE_PTR)NULL);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  FIO_LIST_TYPE *restrict node = (FIO_LIST_TYPE *)(FIO_PTR_UNTAG(node_));
  node->FIO_LIST_NODE_NAME.prev = head->prev;
  node->FIO_LIST_NODE_NAME.next = head;
  head->prev->next = &node->FIO_LIST_NODE_NAME;
  head->prev = &node->FIO_LIST_NODE_NAME;
  return node_;
}

/** Pops a node from the end of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, pop)(FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_PTR_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->prev));
}

/** Adds an existing node to the beginning of the list. Returns node or NULL. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 unshift)(FIO_LIST_HEAD *restrict head,
                                          FIO_LIST_TYPE_PTR restrict node) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  FIO_PTR_TAG_VALID_OR_RETURN(node, (FIO_LIST_TYPE_PTR)NULL);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, push)(head->next, node);
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, shift)(FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  head = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_PTR_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->next));
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, root)(FIO_LIST_HEAD *ptr) {
  FIO_PTR_TAG_VALID_OR_RETURN(ptr, (FIO_LIST_TYPE_PTR)NULL);
  ptr = (FIO_LIST_HEAD *)(FIO_PTR_UNTAG(ptr));
  return FIO_PTR_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, ptr);
}

/* *****************************************************************************
Linked Lists (embeded) - cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_LIST_NAME
#undef FIO_LIST_TYPE
#undef FIO_LIST_NODE_NAME
#undef FIO_LIST_TYPE_PTR
#endif
