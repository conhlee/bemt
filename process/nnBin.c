#include "nnBin.h"

#include <string.h>

bool NnFileHeaderCheckVer(
    const NnFileHeader* fileHeader, u16 versionMajor, u8 versionMinor, u8 versionBugfix
) {
    if (fileHeader == NULL)
        return false;

    u16 myVersionMajor;
    if (fileHeader->byteOrderMark == NN_BOM_FOREIGN)
        myVersionMajor = __builtin_bswap16(fileHeader->versionMajor);
    else
        myVersionMajor = fileHeader->versionMajor;

    return
        myVersionMajor == versionMajor &&
        fileHeader->versionMinor == versionMinor &&
        fileHeader->versionBugfix == versionBugfix;
}

static u32 _ExtractRefBit(const char* key, u32 keyLen, u32 refBit) {
    if (key == NULL)
        return 0;

    u32 invByteIdx = refBit >> 3;
    if (invByteIdx < keyLen) {
        u32 byteIdx = keyLen - 1 - invByteIdx;
        u32 shift = refBit & 7;

        return ((u8)(key[byteIdx]) >> shift) & 1;
    }

    return 0;
}

const NnDicNode* NnDicFind(void* baseData, const NnDic* dic, const char* key) {
    if (dic == NULL || key == NULL)
        return NULL;

    const u32 keyLength = strlen(key);
    const u32 totalNodes = dic->nodeCount + 1;

    const NnDicNode* prevNode = dic->nodes;
    const NnDicNode* node = dic->nodes + prevNode->leftIndex;

    while (prevNode->refBitPos < node->refBitPos) {
        prevNode = node;

        u32 bit = _ExtractRefBit(key, keyLength, node->refBitPos);
        u32 nextIndex = bit ? node->rightIndex : node->leftIndex;
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

void NnApplyRelocationTable(NnRelocTable* table) {
    void* binStart = (u8*)table - table->selfOffset;
    NnFileHeader* fileHeader = (NnFileHeader*)binStart;

    NnRelocEntry* entries = (NnRelocEntry*)(table->sections + table->sectionCount);

    for (u32 sectionIndex = 0; sectionIndex < table->sectionCount; sectionIndex++) {
        NnRelocSection* section = table->sections + sectionIndex;

        for (u32 entryIndex = 0; entryIndex < section->entryCount; entryIndex++) {
            NnRelocEntry* entry = entries + section->firstEntryIndex + entryIndex;

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

    fileHeader->flags |= 1;
}
