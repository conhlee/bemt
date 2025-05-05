#include "buffer.h"

#include <stdlib.h>

#include <string.h>

void BufferInit(ConsBuffer* buffer, u64 size) {
    if (buffer == NULL)
        return;

    if (size == 0)
        buffer->data_void = 0x00;
    else {
        buffer->data_void = malloc(size);
        memset(buffer->data_void, 0x00, size);
    }

    buffer->size = size;
}

void BufferInitCopy(ConsBuffer* buffer, const void* data, u64 size) {
    if (buffer == NULL)
        return;

    if (size == 0)
        buffer->data_void = 0x00;
    else {
        buffer->data_void = malloc(size);
        memcpy(buffer->data_void, data, size);
    }

    buffer->size = size;
}

void BufferInitCopyView(ConsBuffer* buffer, ConsBufferView view) {
    if (buffer == NULL || !BufferViewIsValid(&view))
        return;
    BufferInitCopy(buffer, view.data_void, view.size);
}

void BufferDestroy(ConsBuffer* buffer) {
    if (buffer == NULL)
        return;

    if (buffer->data_void)
        free(buffer->data_void);

    buffer->data_void = NULL;
    buffer->size = 0;
}

void BufferResize(ConsBuffer* buffer, u64 newSize) {
    if (buffer == NULL)
        return;

    if (buffer->data_void == NULL) {
        BufferInit(buffer, newSize);
        return;
    }

    if (buffer->size == newSize)
        return;
    if (newSize == 0) {
        BufferDestroy(buffer);
        return;
    }

    buffer->data_void = realloc(buffer->data_void, newSize);
    if (newSize > buffer->size) {
        memset(buffer->data_u8 + buffer->size, 0, newSize - buffer->size);
    }

    buffer->size = newSize;
}

void BufferGrow(ConsBuffer* buffer, s64 growBy) {
    if (!buffer || growBy == 0)
        return;

    if (!buffer->data_void) {
        BufferInit(buffer, growBy);
        return;
    }

    s64 newSize = buffer->size + growBy;

    if (newSize <= 0) {
        BufferDestroy(buffer);
        return;
    }

    buffer->data_void = realloc(buffer->data_void, newSize);
    if (newSize > buffer->size) {
        memset(buffer->data_u8 + buffer->size, 0, newSize - buffer->size);
    }

    buffer->size = newSize;
}

bool BufferViewCompare(ConsBufferView a, ConsBufferView b) {
    if (a.size != b.size)
        return false;
    if (a.size == 0)
        return true;

    if (a.data_void && b.data_void)
        return memcmp(a.data_void, b.data_void, a.size) == 0;
    
    return false;
}
