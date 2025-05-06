#ifndef CONS_BUFFER_H
#define CONS_BUFFER_H

#include "type.h"

// Owning.
typedef struct ConsBuffer {
    union {
        s8* data_s8;
        u8* data_u8;
        void* data_void;
    };
    u64 size;
} ConsBuffer;
_Static_assert(
    sizeof(ConsBuffer) <= 16,
    "sizeof ConsBuffer must be 16 bytes or less to be passed "
    "through a register on modern systems"
);

static inline bool BufferIsValid(const ConsBuffer* buffer) {
    if (buffer == (ConsBuffer*)0)
        return false;
    return buffer->data_void != ((void*)0) && buffer->size > 0;
}

// Non-owning.
typedef union ConsBufferView {
    ConsBuffer as_buffer;
    struct {
        union {
            s8* data_s8;
            u8* data_u8;
            void* data_void;
        };
        u64 size;
    };
} ConsBufferView;
_Static_assert(
    sizeof(ConsBufferView) == sizeof(ConsBuffer),
    "ConsBufferView & ConsBuffer must share the same structure"
);

static inline bool BufferViewIsValid(const ConsBufferView* view) {
    return BufferIsValid(&view->as_buffer);
}

static inline ConsBufferView BufferViewFromPtr(void* data, u64 size) {
    ConsBufferView view;
    view.data_void = data;
    view.size = size;

    return view;
}
static inline ConsBufferView BufferViewFromPtrs(void* dataStart, void* dataEnd) {
    ConsBufferView view;
    view.data_void = dataStart;
    view.size = (u64)((u8*)dataEnd - (u8*)dataStart);

    return view;
}

#define BUFFER_TO_VIEW(buffer) ((ConsBufferView){ .as_buffer = (buffer) })

// Creates a zero-initialized buffer of the given size.
void BufferInit(ConsBuffer* buffer, u64 size);
// Creates a buffer by copying data from a raw pointer.
void BufferInitCopy(ConsBuffer* buffer, const void* data, u64 size);
// Creates a buffer by copying data from a buffer view.
void BufferInitCopyView(ConsBuffer* buffer, ConsBufferView view);

// Destroy a buffer. It's safe to pass in a uninitialized buffer.
void BufferDestroy(ConsBuffer* buffer);

// Resize a buffer. It's safe to pass in a uninitialized buffer.
void BufferResize(ConsBuffer* buffer, u64 newSize);

// Grow a buffer (increase it's size by growBy). Supports negative values.
// It's safe to pass in a uninitialized buffer.
void BufferGrow(ConsBuffer* buffer, s64 growBy);

static inline void* BufferGetEnd(ConsBuffer* buffer) {
    if (buffer == (ConsBuffer*)0)
        return (void*)0;
    return (void*)(buffer->data_u8 + buffer->size);
}

bool BufferViewCompare(ConsBufferView a, ConsBufferView b);

#endif // CONS_BUFFER_H
