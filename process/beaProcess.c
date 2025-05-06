#include "beaProcess.h"

#include "../cons/type.h"
#include "../cons/macro.h"
#include "../cons/error.h"

#include "../cons/comp.h"

#include "../cons/list.h"
#include "../cons/ptrie.h"

#include <string.h>

#include <stddef.h>

#define SCNE_ID IDENTIFIER_TO_U32('S','C','N','E')
#define ASST_ID IDENTIFIER_TO_U32('A','S','S','T')

typedef struct __attribute__((packed)) {
    NnFileHeader _00; // Identifier is SCNE_ID, signature is zero.

    u16 assetCount; // 0x20
    u16 _unk22; // 0x22

    u32 _unk24; // 0x24

    u64 assetPointersPtr; // 0x28; relocated offset to a list of relocated BeaAssetBlock offsets.
    u64 dicPtr; // 0x30; relocated offset to the dictionary (NnDic).

    u64 _unk38; // 0x38

    u64 archiveNamePtr; // 0x40; relocated offset to a NnString containing the archive name.
} BeaFileHeader;
_Static_assert(sizeof(BeaFileHeader) == 0x48, "sizeof BeaFileHeader is mismatched");

typedef struct __attribute__((packed)) {
    NnBlockHeader _00; // Identifier is ASST_ID.

    u8 compressionType; // See BeaCompressionType.
    u8 _pad8;
    u16 alignmentShift; // Alignment is 1 << alignmentShift.
    u32 dataSize;
    u32 decompressedDataSize;
    u32 _reserved; // TODO: is this a reserved field, or padding?

    u64 dataPtr; // Relocated offset to the data.
    u64 filenamePtr; // Relocated offset to a NnString containing the filename.
} BeaAssetBlock;
_Static_assert(sizeof(BeaAssetBlock) == 0x30, "sizeof BeaAssetBlock is mismatched");

void BeaPreprocess(ConsBufferView beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;

    if (fileHeader->_00.identifier != SCNE_ID)
        Panic("BeaPreprocess: header identifier is nonmatching");
    if (fileHeader->_00.signature != 0x00000000)
        Panic("BeaPreprocess: header signature is nonmatching");

    if (fileHeader->_00.byteOrderMark == NN_BOM_FOREIGN)
        Panic("BeaPreprocess: big-endian is unsupported");
    if (fileHeader->_00.byteOrderMark != NN_BOM_NATIVE)
        Panic("BeaPreprocess: invalid byte order mark");

    if (!NnFileHeaderCheckVer(&fileHeader->_00, 1,1,0))
        Panic("BeaPreprocess: unsupported version (expected v1.1.0)");
}

NnString* BeaGetArchiveName(ConsBufferView beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;

    return (NnString*)(beaData.data_u8 + fileHeader->archiveNamePtr);
}

u32 BeaGetAssetCount(ConsBufferView beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    return (u32)fileHeader->assetCount;
}

s64 BeaFindAssetIndex(ConsBufferView beaData, const char* filename) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    const NnDic* dic = (NnDic*)(beaData.data_u8 + fileHeader->dicPtr);

    const NnDicNode* node = NnDicFind(beaData.data_void, dic, filename);
    if (node == NULL)
        return -1;

    return (s64)NnDicNodeGetIndex(dic, node);
}

static BeaAssetBlock* _IndexAsset(ConsBufferView beaData, u32 assetIndex) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    if (assetIndex >= fileHeader->assetCount)
        return NULL;

    u64* assetPointers = (u64*)(beaData.data_u8 + fileHeader->assetPointersPtr);

    return (BeaAssetBlock*)(beaData.data_u8 + assetPointers[assetIndex]);
}

NnString* BeaGetAssetFilename(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return NULL;

    return (NnString*)(beaData.data_u8 + asset->filenamePtr);
}

BeaCompressionType BeaGetAssetCompressionType(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return BEA_COMPRESSION_TYPE_NONE;

    return (BeaCompressionType)asset->compressionType;
}

u32 BeaGetAssetAlignment(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return (u32)(1 << asset->alignmentShift);
}

u32 BeaGetAssetCompressedSize(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return asset->dataSize;
}

u32 BeaGetAssetDecompressedSize(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return asset->decompressedDataSize;
}

ConsBufferView BeaGetCompressedData(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return (ConsBufferView){ 0 };

    ConsBufferView view;
    view.data_u8 = beaData.data_u8 + asset->dataPtr;
    view.size = asset->dataSize;

    return view;
}

ConsBuffer BeaGetDecompressedData(ConsBufferView beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;

    ConsBufferView dataView;
    dataView.data_u8 = beaData.data_u8 + asset->dataPtr;
    dataView.size = asset->dataSize;

    switch ((BeaCompressionType)asset->compressionType) {
    case BEA_COMPRESSION_TYPE_NONE:
        BufferInitCopyView(&buffer, dataView);
        break;
    case BEA_COMPRESSION_TYPE_ZLIB:
        buffer = DecompressZlib(dataView, asset->decompressedDataSize);
        break;
    case BEA_COMPRESSION_TYPE_ZSTD:
        buffer = DecompressZstd(dataView, asset->decompressedDataSize);
        break;
    default:
        Warn("BeaGetCompressedData: invalid compression type (%u)", (u32)asset->compressionType);
        buffer = (ConsBuffer){ 0 };
        break;
    }

    return buffer;
}

// #define BEA_BUILD_ENABLE_DIC_TEST

ConsBuffer BeaBuild(const BeaBuildAsset* assets, u32 assetCount, const char* archiveName) {
    if (assets == NULL)
        Panic("BeaBuild: assets is NULL");
    if (archiveName == NULL)
        Panic("BeaBuild: archiveName is NULL");

    if (assetCount > 0xFFFF)
        Panic("BeaBuild: too many assets: exceeds max of 65535!");

    ConsList compressedDataList;
    ListInit(&compressedDataList, sizeof(ConsBuffer), assetCount);

    for (u32 i = 0; i < assetCount; i++) {
        const BeaBuildAsset* asset = assets + i;

        if (!BufferViewIsValid(&asset->dataView))
            Panic("BeaBuild: asset %u ('%s') has an invalid data view", i, asset->name);

        ConsBuffer compressedData;
        switch (asset->compressionType) {
        case BEA_COMPRESSION_TYPE_NONE:
            BufferInitCopyView(&compressedData, asset->dataView);
            break;
        case BEA_COMPRESSION_TYPE_ZLIB:
            compressedData = CompressZlib(asset->dataView);
            break;
        case BEA_COMPRESSION_TYPE_ZSTD:
            compressedData = CompressZstd(asset->dataView);
            break;
        default:
            Panic("BeaBuild: asset %u ('%s') has an invalid compression type (%u)", i, asset->name, (u32)asset->compressionType);
            break;
        }

        if (!BufferIsValid(&compressedData))
            Panic("BeaBuild: failed to compress asset %u ('%s')", i, asset->name);
        
        ListAdd(&compressedDataList, &compressedData);
    }
    
    u64 stringPoolSize = sizeof(NnStringPool);

    u64 emptyStringOffset = stringPoolSize; // Offset of string pool is added later.
    stringPoolSize += ALIGN_UP_2(sizeof(NnString) + 1); // Empty string (just the null terminator).

    // Asset names.
    u64 assetNamesOffset = stringPoolSize; // Offset of string pool is added later.
    for (u32 i = 0; i < assetCount; i++)
        stringPoolSize += ALIGN_UP_2(sizeof(NnString) + strlen(assets[i].name) + 1);

    u64 archiveNameOffset = stringPoolSize; // Offset of string pool is added later.
    stringPoolSize += ALIGN_UP_2(sizeof(NnString) + strlen(archiveName) + 1); // Archive name.

    // There's two in the file header right out of the gate (2 consecutive fields + 1 field).
    u64 relocationCount = 2;
    relocationCount += 1 * assetCount; // 1 for every asset block (2 consecutive fields).

    ConsPtrie dicTrie;
    PtrieInit(&dicTrie);

    for (u32 i = 0; i < assetCount; i++)
        PtrieInsert(&dicTrie, assets[i].name);

    ConsFlatPtrie dicTrieFlat;
    PtrieFlatten(&dicTrie, &dicTrieFlat);

    PtrieDestroy(&dicTrie);

    // Sort trie.
    for (u32 i = 0; i < assetCount; i++) {
        ConsFlatPtrieNode* found = PtrieSearchFlat(&dicTrieFlat, assets[i].name);
        if (found == NULL) {
            Panic("BeaBuild: PtrieSearchFlat failed .. something is very wrong");
        }

        PtrieSwapFlatNode(&dicTrieFlat, i + 1, (found - dicTrieFlat.nodes));
    }

    relocationCount += 1 * (1 + dicTrieFlat.nodeCount); // 1 for every dictionary node.

    u64 binSize = sizeof(BeaFileHeader);

    binSize += 8 * assetCount; // Asset block pointers.
    binSize += 40 * assetCount; // Unknown zeroed 40 bytes per asset?

    const u64 dictionaryOffset = binSize;
    binSize += sizeof(NnDic) + (sizeof(NnDicNode) * (dicTrieFlat.nodeCount + 1)); // Dictionary.

    const u64 firstBlockOffset = binSize;
    binSize += sizeof(BeaAssetBlock) * assetCount; // Asset blocks.

    if (firstBlockOffset > 0xFFFF)
        Panic("BeaBuild: first block offset exceeds max of 65535");
    
    const u64 stringPoolOffset = binSize;
    binSize += stringPoolSize; // String pool.

    const u64 stringPoolEndOffset = binSize;

    emptyStringOffset += stringPoolOffset;
    archiveNameOffset += stringPoolOffset;
    assetNamesOffset += stringPoolOffset;

    // Relocation table (for some reason, this is aligned to 8 bytes).
    binSize = ALIGN_UP_8(binSize);

    const u64 relocationTableOffset = binSize;
    binSize += sizeof(NnRelocTable) + sizeof(NnRelocSection) + (sizeof(NnRelocEntry) * relocationCount);

    const u64 reflectedFileSize = binSize - 8;

    binSize = ALIGN_UP_8(binSize);
    const u64 dataStartOffset = binSize;

    // Asset data.
    for (u32 i = 0; i < assetCount; i++) {
        ConsBuffer* compressedData = ListGet(&compressedDataList, i);
        binSize += ALIGN_UP_8(compressedData->size);
    }

    ConsBuffer beaBuffer;
    BufferInit(&beaBuffer, binSize);

    BeaFileHeader* fileHeader = beaBuffer.data_void;

    fileHeader->_00.identifier = SCNE_ID;
    fileHeader->_00.signature = 0x00000000;

    // v1.1.0
    fileHeader->_00.versionBugfix = 0;
    fileHeader->_00.versionMinor = 1;
    fileHeader->_00.versionMajor = 1;

    fileHeader->_00.byteOrderMark = NN_BOM_NATIVE;
    fileHeader->_00.alignmentShift = 3; // 1 << 3 = 16 bytes.
    fileHeader->_00.targetAddrSize = 0; // Default to 64.

    fileHeader->_00.filenameOffset = 0x00000000; // BEA doesn't use this field.

    fileHeader->_00.flags = 0x0000;

    fileHeader->_00.firstBlockOffset = (u16)firstBlockOffset;
    fileHeader->_00.relocationTableOffset = (u32)relocationTableOffset;

    fileHeader->_00.fileSize = reflectedFileSize; // lol

    fileHeader->assetCount = (u16)assetCount;

    fileHeader->_unk22 = 0x0000;
    fileHeader->_unk24 = 0x00000000;

    // Asset block pointers directly follow the file header.
    fileHeader->assetPointersPtr = sizeof(BeaFileHeader);

    fileHeader->dicPtr = dictionaryOffset;

    fileHeader->_unk38 = 0x0000000000000000;

    fileHeader->archiveNamePtr = archiveNameOffset;

    u64* assetPointers = (u64*)(fileHeader + 1);
    for (u32 i = 0; i < assetCount; i++)
        assetPointers[i] = firstBlockOffset + (sizeof(BeaAssetBlock) * i);

    u64 nextDataOffset = dataStartOffset;
    u64 nextAssetNameOffset = assetNamesOffset;

    // Asset blocks, asset data & asset names.
    for (u32 i = 0; i < assetCount; i++) {
        const BeaBuildAsset* asset = assets + i;
        BeaAssetBlock* assetBlock = (BeaAssetBlock*)(beaBuffer.data_u8 + assetPointers[i]);

        assetBlock->_00.signature = ASST_ID;
        assetBlock->_00.offsetToNextBlock = sizeof(BeaAssetBlock);
        assetBlock->_00.blockSize = sizeof(BeaAssetBlock);
        assetBlock->_00._reserved = 0x00000000;

        ConsBuffer* compressedData = ListGet(&compressedDataList, i);

        assetBlock->compressionType = (u8)asset->compressionType;
        assetBlock->_pad8 = 0x00;
        assetBlock->alignmentShift = (u16)asset->alignmentShift;
        assetBlock->dataSize = (u32)compressedData->size;
        assetBlock->decompressedDataSize = (u32)asset->dataView.size;
        assetBlock->_reserved = 0x00000000;

        // Move asset data.
        memcpy(beaBuffer.data_u8 + nextDataOffset, compressedData->data_void, compressedData->size);

        assetBlock->dataPtr = nextDataOffset;
        nextDataOffset += ALIGN_UP_8(compressedData->size);

        BufferDestroy(compressedData);

        // Copy asset name.
        NnString* assetName = (NnString*)(beaBuffer.data_u8 + nextAssetNameOffset);
        assetName->len = (u16)strlen(asset->name);
        strcpy(assetName->str, asset->name);

        assetBlock->filenamePtr = nextAssetNameOffset;
        nextAssetNameOffset += ALIGN_UP_2(sizeof(NnString) + assetName->len + 1);
    }

    // We can clean up compressedDataList; all data has been moved out.
    ListDestroy(&compressedDataList);

    // The dictionary. We handle this after the asset blocks so we can
    // steal the name offsets.
    NnDic* dic = (NnDic*)(beaBuffer.data_u8 + dictionaryOffset);

    dic->signature = NN__DIC_MAGIC;
    dic->nodeCount = dicTrieFlat.nodeCount;
    for (u64 i = 0; i < dicTrieFlat.nodeCount + 1; i++) {
        dic->nodes[i].refBitPos = (s32)dicTrieFlat.nodes[i].refBit;
        dic->nodes[i].leftIndex = (u16)dicTrieFlat.nodes[i].leftIndex;
        dic->nodes[i].rightIndex = (u16)dicTrieFlat.nodes[i].rightIndex;

        if (i == 0 || i > assetCount) {
            dic->nodes[i].namePtr = emptyStringOffset;
        }
        else {
            BeaAssetBlock* assetBlock = (BeaAssetBlock*)(beaBuffer.data_u8 + assetPointers[i - 1]);
            dic->nodes[i].namePtr = assetBlock->filenamePtr;
        }
    }

    // We don't need the flat trie anymore, we can get rid of it
    PtrieDestroyFlat(&dicTrieFlat);

    #ifdef BEA_BUILD_ENABLE_DIC_TEST
    for (u32 i = 0; i < assetCount; i++) {
        const char* targetKey = assets[i].name;

        const NnDicNode* node = NnDicFind(beaBuffer.data_void, dic, targetKey);
        if (node == NULL) {
            Panic("BeaBuild: dic test failed: node with key '%s' not found", targetKey);
        }

        const u32 nodeIndex = (node - dic->nodes) - 1;
        if (nodeIndex != i) {
            Panic("BeaBuild: dic test failed: node with key '%s' has wrong index %u (expected %u)", targetKey, nodeIndex, i);
        }
    }
    #endif

    // String pool (really just the block header & misc. strings, we just wrote the asset names).
    NnStringPool* stringPool = (NnStringPool*)(beaBuffer.data_u8 + stringPoolOffset);

    stringPool->_00.signature = NN__STR_MAGIC;
    stringPool->_00.offsetToNextBlock = 0;
    stringPool->_00.blockSize = stringPoolSize;
    stringPool->_00._reserved = 0x00000000;

    // Archive name & asset names. Empty string doesn't count.
    stringPool->stringCount = 1 + assetCount;

    NnString* currentString = (NnString*)(beaBuffer.data_u8 + emptyStringOffset);
    currentString->len = 0;
    currentString->str[0] = '\0';

    currentString = (NnString*)(beaBuffer.data_u8 + archiveNameOffset);
    currentString->len = strlen(archiveName);
    strcpy(currentString->str, archiveName);

    // Relocation table.
    NnRelocTable* relocTable = (NnRelocTable*)(beaBuffer.data_u8 + relocationTableOffset);
    
    relocTable->signature = NN__RLT_MAGIC;
    relocTable->selfOffset = (u32)relocationTableOffset;
    relocTable->sectionCount = 1;
    relocTable->_reserved = 0x00000000;

    NnRelocSection* relocSection = relocTable->sections + 0;

    relocSection->_dataAddress = 0x0000000000000000;
    relocSection->dataOffset = 0;
    relocSection->dataSize = (u32)stringPoolEndOffset;
    relocSection->firstEntryIndex = 0;
    relocSection->entryCount = relocationCount;

    NnRelocEntry* curRelocEntry = (NnRelocEntry*)NnRelocTableGetEntries(relocTable);

    // First up is the file header.

    // assetPointersPtr, dicPtr
    curRelocEntry->offsetToPointerList = offsetof(BeaFileHeader, assetPointersPtr);
    curRelocEntry->pointerListCount = 1;
    curRelocEntry->pointersPerList = 2;
    curRelocEntry->pointerListSpacing = 0;

    curRelocEntry++;

    // archiveNamePtr
    curRelocEntry->offsetToPointerList = offsetof(BeaFileHeader, archiveNamePtr);
    curRelocEntry->pointerListCount = 1;
    curRelocEntry->pointersPerList = 1;
    curRelocEntry->pointerListSpacing = 0;

    curRelocEntry++;

    // The dictionary nodes..
    u64 nodesOffset = dictionaryOffset + offsetof(NnDic, nodes);
    for (u32 i = 0; i < dic->nodeCount + 1; i++) {
        // namePtr
        curRelocEntry->offsetToPointerList = nodesOffset + (sizeof(NnDicNode) * i) + offsetof(NnDicNode, namePtr);
        curRelocEntry->pointerListCount = 1;
        curRelocEntry->pointersPerList = 1;
        curRelocEntry->pointerListSpacing = 0;

        curRelocEntry++;
    }

    // Finally, the asset blocks.
    for (u32 i = 0; i < assetCount; i++) {
        // dataPtr, filenamePtr
        curRelocEntry->offsetToPointerList = assetPointers[i] + offsetof(BeaAssetBlock, dataPtr);
        curRelocEntry->pointerListCount = 1;
        curRelocEntry->pointersPerList = 2;
        curRelocEntry->pointerListSpacing = 0;

        curRelocEntry++;
    }

    return beaBuffer;
}
