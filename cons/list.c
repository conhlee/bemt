#include "list.h"

#include <stdlib.h> // malloc, realloc

#include <string.h> // memset

void ListInit(ConsList* list, u32 elementSize, u64 initialCapacity) {
    if (list == NULL)
        return;

    list->elementSize = elementSize;
    list->_capacity = initialCapacity;
    list->elementCount = 0;
    list->data = malloc(elementSize * initialCapacity);
}

void* ListGet(ConsList* list, u64 index) {
    if (list == NULL)
        return NULL;
    if (index >= list->elementCount)
        return NULL;

    return (void*)(list->data_u8 + (index * list->elementSize));
}

void ListAdd(ConsList* list, void* element) {
    if (list == NULL)
        return;

    // Double capacity.
    if (list->elementCount == list->_capacity) {
        if (list->_capacity == 0)
            list->_capacity = 1;
        else
            list->_capacity *= 2;

        list->data = realloc(list->data, list->elementSize * list->_capacity);
    }

    u64 position = list->elementCount * list->elementSize;
    memcpy(list->data_u8 + position, element, list->elementSize);

    list->elementCount++;
}

void ListRemove(ConsList* list, u64 index) {
    if (list == NULL)
        return;

    if (index >= list->elementCount)
        return;

    memmove(
        list->data_u8 + (index * list->elementSize), 
        list->data_u8 + ((index + 1) * list->elementSize), 
        (list->elementCount - index - 1) * list->elementSize
    );
    list->elementCount--;
}

void ListAddRange(ConsList* list, void* elements, u64 count) {
    if (list == NULL)
        return;

    if (list->elementCount + count > list->_capacity) {
        list->_capacity = list->elementCount + count;

        list->data = realloc(list->data, list->elementSize * list->_capacity);
    }

    u64 position = list->elementCount * list->elementSize;
    memcpy(list->data_u8 + position, elements, count * list->elementSize);

    list->elementCount += count;
}

void ListSet(ConsList* list, u64 index, void* element) {
    if (list == NULL)
        return;

    if (index < list->elementCount) {
        u64 position = index * list->elementSize;
        memcpy(list->data_u8 + position, element, list->elementSize);
    }
}

u64 ListSize(ConsList* list) {
    if (list == NULL)
        return 0;

    return list->elementCount;
}

bool ListIsEmpty(ConsList* list) {
    if (list == NULL)
        return true;
    return list->elementCount == 0;
}

void ListResize(ConsList* list, u64 newCount) {
    if (list == NULL)
        return;

    if (newCount > list->_capacity) {
        list->_capacity = newCount;
        list->data = realloc(list->data, list->elementSize * list->_capacity);
    }

    u64 startPosition = list->elementCount * list->elementSize;
    u64 endPosition = newCount * list->elementSize;

    memset(list->data_u8 + startPosition, 0x00, endPosition - startPosition);

    list->elementCount = newCount;
}

bool ListContains(ConsList* list, void* element) {
    if (list == NULL)
        return false;

    void* dataEnd = (void*)(list->data_u8 + (list->elementCount * list->elementSize));
    void* currentData = list->data;

    while (currentData < dataEnd) {
        if (memcmp(currentData, element, list->elementSize) == 0)
            return true;
        currentData = (void*)((u8*)currentData + list->elementSize);
    }

    return false;
}

s64 ListIndexOf(ConsList* list, void* element) {
    if (list == NULL)
        return -1;

    void* currentData = list->data;
    for (s64 i = 0; i < (s64)list->elementCount; i++) {
        if (memcmp(currentData, element, list->elementSize) == 0)
            return i;
        currentData = (void*)((u8*)currentData + list->elementSize);
    }

    return -1;
}

void ListCopy(ConsList* dest, ConsList* src) {
    if (dest == NULL || src == NULL)
        return;

    u64 srcDataSize = src->elementCount * src->elementSize;

    // Uninitialized dest
    if (dest->data == NULL) {
        dest->data = malloc(srcDataSize);
    }
    // Mismatched element size
    else if (dest->elementSize != src->elementSize) {
        dest->data = realloc(dest->data, srcDataSize);
    }
    // Matching element size
    else {
        if (dest->_capacity < src->elementCount) {
            dest->data = realloc(dest->data, srcDataSize);
            dest->_capacity = src->elementCount;
        }
        memcpy(dest->data, src->data, srcDataSize);
        dest->elementCount = src->elementCount;
        return;
    }

    memcpy(dest->data, src->data, srcDataSize);

    dest->elementCount = src->elementCount;
    dest->elementSize = src->elementSize;
    dest->_capacity = src->elementCount;
}

void ListClear(ConsList* list) {
    if (list == NULL)
        return;

    list->elementCount = 0;
}

void ListDestroy(ConsList* list) {
    if (list == NULL)
        return;

    if (list->data)
        free(list->data);
    list->elementCount = 0;
    list->elementSize = 0;
    list->_capacity = 0;
}
