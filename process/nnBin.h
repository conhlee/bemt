#ifndef NN_BIN_H
#define NN_BIN_H

#include "../cons/type.h"
#include "../cons/macro.h"

#include <stdlib.h>

#define NN_BOM_FOREIGN (0xFFFE)
#define NN_BOM_NATIVE (0xFEFF)

#define NN__DIC_MAGIC IDENTIFIER_TO_U32('_','D','I','C')
#define NN__STR_MAGIC IDENTIFIER_TO_U32('_','S','T','R')
#define NN__RLT_MAGIC IDENTIFIER_TO_U32('_','R','L','T')

typedef struct __attribute__((packed)) {
    u32 identifier;
    u32 signature;

    u8  versionBugfix;
    u8  versionMinor;
    u16 versionMajor;

    u16 byteOrderMark; // Compare to NN_BOM_NATIVE and NN_BOM_FOREIGN.

    u8 alignmentShift; // Alignment is 1 << alignmentShift.
    u8 targetAddrSize; // In bits; usually 64. Might be unused.

    u32 filenameOffset; // Offset to a null-terminated string containing the filename. Might be unused.

    u16 flags; // Bit 0: relocated

    u16 firstBlockOffset;
    u32 relocationTableOffset;

    // The size of this file that is loaded into memory.
    //     - Note that in some cases, this won't be the total file size.
    u32 memoryLoadSize;
} NnFileHeader;
STRUCT_SIZE_ASSERT(NnFileHeader, 0x20);

// Returns true if the version is matching, false if not. Accounts for foreign endianness.
bool NnFileHeaderCheckVer(
    const NnFileHeader* fileHeader, u16 versionMajor, u8 versionMinor, u8 versionBugfix
);

static inline u64 NnFileHeaderGetAlignment(const NnFileHeader* fileHeader) {
    return 1ull << (fileHeader->alignmentShift & 63);
}

typedef struct __attribute__((packed)) {
    u32 signature;

    u32 offsetToNextBlock; // Relative to the start of this block. If zero, then no block follows.
    u32 blockSize;
    u32 _reserved;
} NnBlockHeader;
STRUCT_SIZE_ASSERT(NnBlockHeader, 0x10);

static inline NnBlockHeader* NnBlockGetNext(NnBlockHeader* block) {
    if (block == NULL || block->offsetToNextBlock == 0)
        return NULL;
    else
        return (NnBlockHeader*)((u8*)block + block->offsetToNextBlock);
}

typedef struct __attribute__((packed)) {
    u16 len;
    char str[0]; // Must be null-terminated.
} NnString;
STRUCT_SIZE_ASSERT(NnString, 0x02);

typedef struct __attribute__((packed)) {
    // Why is the string pool is considered a block ..??
    NnBlockHeader _00; // Compare signature to NN__STR_MAGIC.

    u32 stringCount;

    // The first string. All strings are aligned by 2 to ensure an aligned
    // 16-bit read on the length field.
    NnString firstString[0];
} NnStringPool;
STRUCT_SIZE_ASSERT(NnStringPool, 0x14);

typedef struct __attribute__((packed)) {
    s32 refBitPos; // Root node always has the value -1 (npos).
    u16 leftIndex; // Followed when bit is unset. Always followed on root node.
    u16 rightIndex; // Followed when bit is set.

    u64 namePtr; // Relocated offset to a NnString containing the name.
} NnDicNode;
STRUCT_SIZE_ASSERT(NnDicNode, 0x10);

typedef struct __attribute__((packed)) {
    u32 signature; // Compare to NN__DIC_MAGIC.

    s32 nodeCount; // Exclusive of root node.
    NnDicNode nodes[1];
} NnDic;
STRUCT_SIZE_ASSERT(NnDic, 0x18);

// set baseData to NULL if dictionary is relocated.
const NnDicNode* NnDicFind(void* baseData, const NnDic* dic, const char* key);

static inline u32 NnDicNodeGetIndex(const NnDic* dic, const NnDicNode* node) {
    if (dic == NULL || node == NULL)
        return (u32)-1;
    return node - (dic->nodes + 1);
}

// Pointers with a value of zero are not relocated.
typedef struct __attribute__((packed)) {
    u32 offsetToPointerList; // Relative to the start of the file.
    u16 pointerListCount;
    u8 pointersPerList;
    u8 pointerListSkip; // Spacing between pointer lists, in 8 byte interval.
} NnRelocEntry;
STRUCT_SIZE_ASSERT(NnRelocEntry, 0x08);

typedef struct __attribute__((packed)) {
    u64 _dataAddress; // Address of this section's data, filled in at runtime.
    u32 dataOffset;
    u32 dataSize; // Not actually used in the relocation process.
    s32 firstEntryIndex; // First entry that belongs to this section.
    u32 entryCount;
} NnRelocSection;
STRUCT_SIZE_ASSERT(NnRelocSection, 0x18);

// Must be aligned to 8 bytes.
typedef struct __attribute__((packed)) {
    u32 signature; // Compare to NN__RLT_MAGIC.

    u32 selfOffset; // Offset to this table.
    u32 sectionCount; 

    u32 _pad32; // Padding to align the address in NnRelocSection to 8 bytes
    NnRelocSection sections[0];
    // NnRelocEntry entries[0]; // Entries follow the sections.
} NnRelocTable;
STRUCT_SIZE_ASSERT(NnRelocTable, 0x10);

static inline const NnRelocEntry* NnRelocTableGetEntries(const NnRelocTable* table) {
    return (NnRelocEntry*)(table->sections + table->sectionCount);
}

void NnRelocTableApply(NnRelocTable* table);

#endif // NN_BIN_H
