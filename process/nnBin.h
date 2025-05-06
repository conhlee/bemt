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

    u32 fileSize; // Might be inaccurate.
} NnFileHeader;
_Static_assert(sizeof(NnFileHeader) == 0x20, "sizeof NnFileHeader is mismatched");

// Returns true if the version is matching, false if not. Accounts for foreign endianness.
bool NnFileHeaderCheckVer(
    const NnFileHeader* fileHeader, u16 versionMajor, u8 versionMinor, u8 versionBugfix
);

typedef struct __attribute__((packed)) {
    u32 signature;

    u32 offsetToNextBlock; // Relative to the start of this block. If zero, then no block follows.
    u32 blockSize;
    u32 _reserved;
} NnBlockHeader;
_Static_assert(sizeof(NnBlockHeader) == 0x10, "sizeof NnBlockHeader is mismatched");

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
_Static_assert(sizeof(NnString) == 0x02, "sizeof NnString is mismatched");

typedef struct __attribute__((packed)) {
    // Why is the string pool is considered a block ..??
    NnBlockHeader _00; // Compare signature to NN__STR_MAGIC.

    u32 stringCount;

    // The first string. All strings are aligned by 2 to ensure an aligned
    // 16-bit read on the length field.
    NnString firstString[0];
} NnStringPool;
_Static_assert(sizeof(NnStringPool) == 0x14, "sizeof NnStringPool is mismatched");

typedef struct __attribute__((packed)) {
    s32 refBitPos; // Root node always has the value -1 (npos).
    u16 leftIndex; // Followed when bit is unset. Always followed on root node.
    u16 rightIndex; // Followed when bit is set.

    u64 namePtr; // Relocated offset to a NnString containing the name.
} NnDicNode;
_Static_assert(sizeof(NnDicNode) == 0x10, "sizeof NnDicNode is mismatched");

typedef struct __attribute__((packed)) {
    u32 signature; // Compare to NN__DIC_MAGIC.

    s32 nodeCount; // Exclusive of root node.
    NnDicNode nodes[1];
} NnDic;
_Static_assert(sizeof(NnDic) == 0x18, "sizeof NnDic is mismatched");

const NnDicNode* NnDicFind(void* baseData, const NnDic* dic, const char* key);

static inline u32 NnDicNodeGetIndex(const NnDic* dic, const NnDicNode* node) {
    if (dic == NULL || node == NULL)
        return 0;
    return node - (dic->nodes + 1);
}

// Pointers with a value of zero are not relocated.
typedef struct __attribute__((packed)) {
    u32 offsetToPointerList; // Relative to the start of the file.
    u16 pointerListCount;
    u8 pointersPerList;
    u8 pointerListSpacing; // Spacing between pointer lists, in bytes.
} NnRelocEntry;
_Static_assert(sizeof(NnRelocEntry) == 0x08, "sizeof NnRelocEntry is mismatched");

typedef struct __attribute__((packed)) {
    u64 _dataAddress; // Address of this section's data, filled in at runtime.
    u32 dataOffset;
    u32 dataSize; // Not actually used in the relocation process.
    u32 firstEntryIndex; // First entry that belongs to this section.
    u32 entryCount;
} NnRelocSection;
_Static_assert(sizeof(NnRelocSection) == 0x18, "sizeof NnRelocSection is mismatched");

// Must be aligned to 8 bytes.
typedef struct __attribute__((packed)) {
    u32 signature; // Compare to NN__RLT_MAGIC.

    u32 selfOffset; // Offset to this table.
    u32 sectionCount; 
    u32 _reserved; // TODO: this might actually be padding

    NnRelocSection sections[0];
    // NnRelocEntry entries[0]; // Entries follow the sections.
} NnRelocTable;
_Static_assert(sizeof(NnRelocTable) == 0x10, "sizeof NnRelocTable is mismatched");

static inline const NnRelocEntry* NnRelocTableGetEntries(const NnRelocTable* table) {
    return (NnRelocEntry*)(table->sections + table->sectionCount);
}

#endif // NN_BIN_H
