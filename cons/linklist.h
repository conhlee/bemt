#ifndef CONS_LINKLIST_H
#define CONS_LINKLIST_H

// CONS -- linked list implementation

#include "type.h"

typedef struct ConsLLNode {
    struct ConsLLNode* prev;
    struct ConsLLNode* next;
    u64 data;
} ConsLLNode;

// Create a new linked list with 'length' nodes initialized to 'initData'.
ConsLLNode* LinkListNew(u64 length, u64 initData);

// Allocate & initialize a single node.
ConsLLNode* LinkListNewNode(u64 data);

// Insert a node at a specific index (0-indexed). Returns new head.
ConsLLNode* LinkListInsertNode(ConsLLNode** head, ConsLLNode* node, s64 index);

// Insert a node at the head.
ConsLLNode* LinkListInsertHead(ConsLLNode** head, u64 data);

// Insert a node at the tail.
ConsLLNode* LinkListInsertTail(ConsLLNode** head, u64 data);

// Delete the specified node.
void LinkListDeleteNode(ConsLLNode** head, ConsLLNode* node);

// Delete all nodes in a linked list (including the head).
void LinkListDelete(ConsLLNode** head);

// Index may be negative. Returns NULL if index is out of range.
ConsLLNode* LinkListIndex(ConsLLNode* node, s64 index);

// Stops counting on cycles (circular linked lists).
s64 LinkListCountNodes(ConsLLNode* head);

// Find a node by it's value.
ConsLLNode* LinkListFind(ConsLLNode* head, u64 data);

// Remove a node by it's value (removes only the first occurrence).
void LinkListRemoveByValue(ConsLLNode** head, u64 data);

// ConsLLNode* node
#define LINK_LIST_INCR(node) do { \
    if (node) node = node->next; \
} while (0)

// ConsLLNode* node
#define LINK_LIST_DECR(node) do { \
    if (node) node = node->prev; \
} while (0)

#endif // CONS_LINKLIST_H
