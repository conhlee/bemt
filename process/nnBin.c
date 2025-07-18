#include "nnBin.h"

#include <string.h>

bool NnFileHeaderCheckVer(
    const NnFileHeader* fileHeader, u16 versionMajor, u8 versionMinor, u8 versionBugfix
) {
    if (fileHeader == NULL)
        return false;

    const u16 fileVersionMajor = (fileHeader->byteOrderMark == NN_BOM_FOREIGN) ?
        __builtin_bswap16(fileHeader->versionMajor) :
        fileHeader->versionMajor;

    const u8 fileVersionMinor = fileHeader->versionMinor;

    if (fileVersionMajor == versionMajor)
        return fileVersionMinor <= versionMinor;

    return false;
}

static u32 _NnDicExtractRefBit(const char* key, u32 keyLen, u32 refBit) {
    if (key == NULL)
        return 0;

    const u32 invByteIdx = refBit >> 3;
    if (invByteIdx < keyLen) {
        const u32 byteIdx = keyLen - 1 - invByteIdx;
        const u32 shift = refBit & 7;

        return ((u8)(key[byteIdx]) >> shift) & 1;
    }

    return 0;
}

const NnDicNode* NnDicFind(void* baseData, const NnDic* dic, const char* key) {
    if (baseData == NULL || dic == NULL || key == NULL)
        return NULL;

    const u32 keyLength = strlen(key);
    const u32 totalNodes = dic->nodeCount + 1;

    const NnDicNode* prevNode = dic->nodes;
    const NnDicNode* node = dic->nodes + prevNode->leftIndex;

    while (prevNode->refBitPos < node->refBitPos) {
        prevNode = node;

        const u32 bit = _NnDicExtractRefBit(key, keyLength, node->refBitPos);
        const u32 nextIndex = (bit == 0) ? node->leftIndex : node->rightIndex;
        if (nextIndex >= totalNodes)
            return NULL;

        node = dic->nodes + nextIndex;
    }

    const NnString* nodeKey = (NnString*)((u8*)baseData + node->namePtr);
    if (nodeKey->len != keyLength)
        return NULL;
    if (strncmp(nodeKey->str, key, nodeKey->len) != 0)
        return NULL;

    return node;
}

void NnRelocTableApply(NnRelocTable* table) {
    if (table == NULL)
        return;

    void* binStart = (void*)((u8*)table - table->selfOffset);
    // NnFileHeader* fileHeader = (NnFileHeader*)binStart;

    const NnRelocEntry* entries = (NnRelocEntry*)(table->sections + table->sectionCount);

    for (u32 sectionIndex = 0; sectionIndex < table->sectionCount; sectionIndex++) {
        const NnRelocSection* section = table->sections + sectionIndex;

        for (u32 entryIndex = 0; entryIndex < section->entryCount; entryIndex++) {
            const NnRelocEntry* entry = entries + section->firstEntryIndex + entryIndex;

            u64* currentPointerList = (u64*)((u8*)binStart + entry->offsetToPointerList);
            for (u32 pointerListIdx = 0; pointerListIdx < entry->pointerListCount; pointerListIdx++) {
                for (u32 pointerIndex = 0; pointerIndex < entry->pointersPerList; pointerIndex++) {
                    if (*currentPointerList != 0)
                        *currentPointerList += (u64)binStart;
                    currentPointerList++;
                }
                currentPointerList += entry->pointerListSkip;
            }
        }
    }

    // Hmmm .. Maybe we should leave the file header alone ..
    // fileHeader->flags |= 1;
}
