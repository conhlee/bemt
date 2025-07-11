#ifndef CONS_ALIGN_H
#define CONS_ALIGN_H

// CONS -- common macros

// Alignment

#define ALIGN_DOWN_2(value) ( (value) & ~(2 - 1) )
#define ALIGN_UP_2(value)   ( ((value) + 2 - 1) & ~(2 - 1) )

#define ALIGN_DOWN_4(value) ( (value) & ~(4 - 1) )
#define ALIGN_UP_4(value)   ( ((value) + 4 - 1) & ~(4 - 1) )

#define ALIGN_DOWN_8(value) ( (value) & ~(8 - 1) )
#define ALIGN_UP_8(value)   ( ((value) + 8 - 1) & ~(8 - 1) )

#define ALIGN_DOWN_16(value) ( (value) & ~(16 - 1) )
#define ALIGN_UP_16(value)   ( ((value) + 16 - 1) & ~(16 - 1) )

#define ALIGN_DOWN_32(value) ( (value) & ~(32 - 1) )
#define ALIGN_UP_32(value)   ( ((value) + 32 - 1) & ~(32 - 1) )

#define ALIGN_DOWN_64(value) ( (value) & ~(64 - 1) )
#define ALIGN_UP_64(value)   ( ((value) + 64 - 1) & ~(64 - 1) )

// Identifiers

#define IDENTIFIER_TO_U32(char1, char2, char3, char4) ( \
    ((u32)(char4) << 24) | ((u32)(char3) << 16) | \
    ((u32)(char2) <<  8) | ((u32)(char1) <<  0) \
)
#define IDENTIFIER_TO_U16(char1, char2) ( \
    ((u16)(char2) << 8) | ((u16)(char1) << 0) \
)

// Maxmin

#define MAX(a, b) ( (a) > (b) ? a : b )
#define MIN(a, b) ( (a) < (b) ? a : b )

// Literal string & array length

// No (const) char* please!!
#define STR_LIT_LEN(lstring) ( sizeof((lstring)) - 1 )

#define ARR_LIT_LEN(larray) \
    (sizeof(larray) / sizeof((larray)[0]) + \
     0 * sizeof(char[1 - 2 * !!(sizeof(larray) / sizeof((larray)[0]) == 0)]))

// Assertion

#define STRUCT_SIZE_ASSERT(struct, expectSize) _Static_assert(sizeof(struct) == expectSize, "sizeof " #struct " is mismatched")

#endif // CONS_ALIGN_H
