/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___       /* Development inclusion - ignore line */
#define FIO_LIST_NAME list /* Development inclusion - ignore line */
#include "./include.h"     /* Development inclusion - ignore line */
#endif                     /* Development inclusion - ignore line */
/* *****************************************************************************




                            Linked Lists (embeded)








Copyright and License: see header file (000 header.h) or top of file
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
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/** Returns a non-zero value if there are any linked nodes in the list. */
IFUNC int FIO_NAME(FIO_LIST_NAME, any)(const FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, 0);
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
  return head->next != head;
}

/** Returns a non-zero value if the list is empty. */
IFUNC int FIO_NAME_BL(FIO_LIST_NAME, empty)(const FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, 0);
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
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
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
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
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_PTR_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->prev));
}

/** Adds an existing node to the beginning of the list. Returns node or NULL. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME,
                                 unshift)(FIO_LIST_HEAD *restrict head,
                                          FIO_LIST_TYPE_PTR restrict node) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  FIO_PTR_TAG_VALID_OR_RETURN(node, (FIO_LIST_TYPE_PTR)NULL);
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
  return FIO_NAME(FIO_LIST_NAME, push)(head->next, node);
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, shift)(FIO_LIST_HEAD *head) {
  FIO_PTR_TAG_VALID_OR_RETURN(head, (FIO_LIST_TYPE_PTR)NULL);
  head = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, head);
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_PTR_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->next));
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_PTR FIO_NAME(FIO_LIST_NAME, root)(FIO_LIST_HEAD *ptr) {
  FIO_PTR_TAG_VALID_OR_RETURN(ptr, (FIO_LIST_TYPE_PTR)NULL);
  ptr = FIO_PTR_TAG_GET_UNTAGGED(FIO_LIST_HEAD, ptr);
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
