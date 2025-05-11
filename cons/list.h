#ifndef CONS_LIST_H
#define CONS_LIST_H

// CONS -- list implementation

#include "type.h"

typedef struct ConsList {
    union {
        void* data;
        u8* data_u8;
    };

    u64 elementCount;
    u32 elementSize;

    u64 _capacity;
} ConsList;

// Initialize a list.
void ListInit(ConsList* list, u32 elementSize, u64 initialCapacity);

// Get a pointer to the specified element.
void* ListGet(ConsList* list, u64 index);

// Push an element into the list.
void ListAdd(ConsList* list, void* element);

// Remove the element at the specified index.
void ListRemove(ConsList* list, u64 index);

// Push a range of elements into the list.
void ListAddRange(ConsList* list, void* elements, u64 count);

// Set an element at the specified index.
void ListSet(ConsList* list, u64 index, void* element);

// Get the amount of elements contained in the list.
u64 ListSize(ConsList* list);

// Check if the list is empty.
bool ListIsEmpty(ConsList* list);

// Modify the element count of the list.
void ListResize(ConsList* list, u64 newCount);

// Check if the list contains an element at least once.
bool ListContains(ConsList* list, void* element);

// Get the index of the first occourence of an element.
s64 ListIndexOf(ConsList* list, void* element);

// Copy a list to another list.
void ListCopy(ConsList* dest, ConsList* src);

// Clear a list (does not change capacity).
void ListClear(ConsList* list);

// Destroy a list (free all elements).
void ListDestroy(ConsList* list);

#endif // CONS_LIST_H
