#include "beaProcess.h"

#include "../cons/type.h"
#include "../cons/macro.h"
#include "../cons/error.h"

#include "../cons/comp.h"

#define SCNE_ID IDENTIFIER_TO_U32('S','C','N','E')
#define ASST_ID IDENTIFIER_TO_U32('A','S','S','T')

typedef struct __attribute((packed)) {
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

typedef struct __attribute((packed)) {
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
_Static_assert(sizeof(BeaAssetBlock) == 0x30, "sizeof BeaFileHeader is mismatched");

void BeaPreprocess(ConsBuffer beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;

    if (fileHeader->_00.identifier != SCNE_ID)
        Panic("BeaPreprocess: header identifier is nonmatching");
    if (fileHeader->_00.signature != 0x00000000)
        Panic("BeaPreprocess: header signature is nonmatching");

    if (fileHeader->_00.byteOrderMark == NN_BOM_FOREIGN)
        Panic("BeaPreprocess: big-endian is unsupported");
    if (fileHeader->_00.byteOrderMark != NN_BOM_NATIVE)
        Panic("BeaPreprocess: invalid byte order mark");

    if (!NnCheckFileHeaderVer(&fileHeader->_00, 1,1,0))
        Panic("BeaPreprocess: unsupported version (expected v1.1.0)");
}

NnString* BeaGetArchiveName(ConsBuffer beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;

    return (NnString*)(beaData.data_u8 + fileHeader->archiveNamePtr);
}

u32 BeaGetAssetCount(ConsBuffer beaData) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    return (u32)fileHeader->assetCount;
}

s64 BeaFindAssetIndex(ConsBuffer beaData, const char* filename) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    const NnDic* dic = (NnDic*)(beaData.data_u8 + fileHeader->dicPtr);

    const NnDicNode* node = NnDicFind(beaData.data_void, dic, filename);
    if (node == NULL)
        return -1;

    return (s64)NnDicNodeGetIndex(dic, node);
}

static BeaAssetBlock* _IndexAsset(ConsBuffer beaData, u32 assetIndex) {
    const BeaFileHeader* fileHeader = beaData.data_void;
    if (assetIndex >= fileHeader->assetCount)
        return NULL;

    u64* assetPointers = (u64*)(beaData.data_u8 + fileHeader->assetPointersPtr);

    return (BeaAssetBlock*)(beaData.data_u8 + assetPointers[assetIndex]);
}

NnString* BeaGetAssetFilename(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return NULL;

    return (NnString*)(beaData.data_u8 + asset->filenamePtr);
}

BeaCompressionType BeaGetAssetCompressionType(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return BEA_COMPRESSION_TYPE_NONE;

    return (BeaCompressionType)asset->compressionType;
}

u32 BeaGetAssetAlignment(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return (u32)(1 << asset->alignmentShift);
}

u32 BeaGetAssetCompressedSize(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return asset->dataSize;
}

u32 BeaGetAssetDecompressedSize(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return 0;

    return asset->decompressedDataSize;
}

ConsBufferView BeaGetCompressedData(ConsBuffer beaData, u32 assetIndex) {
    BeaAssetBlock* asset = _IndexAsset(beaData, assetIndex);
    if (asset == NULL)
        return (ConsBufferView){ 0 };

    ConsBufferView view;
    view.data_u8 = beaData.data_u8 + asset->dataPtr;
    view.size = asset->dataSize;

    return view;
}

ConsBuffer BeaGetDecompressedData(ConsBuffer beaData, u32 assetIndex) {
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
        buffer = (ConsBuffer){ 0 };
        break;
    }

    return buffer;
}
