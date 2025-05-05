#include "bntxProcess.h"

#include "../cons/type.h"
#include "../cons/macro.h"
#include "../cons/error.h"

#include "../tex/bcn.h"

#include "../tex/tegraSwizzle.h"

#define BNTX_ID IDENTIFIER_TO_U32('B','N','T','X')
#define NX___ID IDENTIFIER_TO_U32('N','X',' ',' ')
#define BRTI_ID IDENTIFIER_TO_U32('B','R','T','I')

typedef struct __attribute((packed)) {
    u32 platformId; // Compare to NX___ID.

    u32 textureCount;
    u64 texturePointersPtr; // Relocated offset to a list of relocated BntxTextureBlock offsets.

    u64 dataBlockPtr; // Relocated offset to the data block.
    u64 dicPtr; // Relocated offset to the dictionary (NnDic).

    u64 _unk40; // A relocated offset.
    u64 _unk48; // A relocated offset.
    u64 _unk4C;
} BntxGlobalInfo;

typedef struct __attribute((packed)) {
    NnFileHeader _00; // Identifier is BNTX_ID, signature is zero.
    BntxGlobalInfo _20;
} BntxFileHeader;

typedef enum {
    IMAGE_FORMAT_INVALID = 0,

    IMAGE_FORMAT_BC3_UNORM_SRGB = 7174,
} BntxImageFormat;

typedef enum {
    DEVICE_ACCESS_READ = (1 << 0),
    DEVICE_ACCESS_WRITE = (1 << 1),
    DEVICE_ACCESS_TEXTURE = (1 << 5),
    DEVICE_ACCESS_IMAGE = (1 << 14)
} BntxDeviceAccessFlag;

typedef enum {
    TILE_MODE_DEVICE_OPTIMIZED = 0,
    TILE_MODE_LINEAR = 1,
} BntxTileMode;

typedef enum {
    CHANNEL_MAPPING_R = 2,
    CHANNEL_MAPPING_G = 3,
    CHANNEL_MAPPING_B = 4,
    CHANNEL_MAPPING_A = 5
} BntxChannelMapping;

typedef enum {
    DIMENSION_1D = 0,
    DIMENSION_2D = 1,
    DIMENSION_3D = 2,
    DIMENSION_CUBE_MAP = 3
} BntxDimension;

typedef struct __attribute((packed)) {
    NnBlockHeader _00; // Identifier is BRTI_ID.

    u8 flags;
    u8 dimensionCount; // Minimum is 1, maximum is 3.
    u16 tileMode; // See BntxTileMode.
    u16 swizzle;
    u16 mipLevelCount;
    // 8
    u16 sampleCount; // Usually 1.
    u16 _pad16_0;
    u32 imageFormat; // See BntxImageFormat.
    // 8
    u32 deviceAccessFlags; // Bitflags; see BntxDeviceAccessFlag.
    u32 width;
    // 8
    u32 height;
    u32 depth;
    // 8
    u32 arrayLength;
    // 4
    u64 textureLayout; // Bitfield. TODO: what does this contain?
    // 8

    u32 _reserved[5];

    u32 dataSize;
    u32 alignment;
    u8 channelMap[4]; // See BntxChannelMapping.
    u8 dimension; // See BntxDimension.
    u8 _pad8;
    u16 _pad16_1;

    u64 namePtr; // Relocated offset to a NnString containing the name of this texture.
    u64 globalInfoPtr; // Relocated offset to the BntxGlobalInfo.
    u64 dataPointersPtr; // Relocated offset to relocated offsets to each mip level's image data (count is mipLevelCount).
    u64 _unk78; // Relocated offset to ?
    u64 _unk80; // Relocated offset to ?
    u64 _unk88; // Relocated offset to ?
    u64 _unk90;
    u64 _unk98; // Relocated offset to ?
} BntxTextureBlock;

void BntxPreprocess(ConsBuffer bntxData) {
    const BntxFileHeader* fileHeader = bntxData.data_void;

    if (fileHeader->_00.identifier != BNTX_ID)
        Panic("BntxPreprocess: header identifier is nonmatching");
    if (fileHeader->_00.signature != 0x00000000)
        Panic("BntxPreprocess: header signature is nonmatching");

    if (fileHeader->_00.byteOrderMark == NN_BOM_FOREIGN)
        Panic("BntxPreprocess: big-endian is unsupported");
    if (fileHeader->_00.byteOrderMark != NN_BOM_NATIVE)
        Panic("BntxPreprocess: invalid byte order mark");

    if (!NnCheckFileHeaderVer(&fileHeader->_00, 4,1,0))
        Panic("BntxPreprocess: unsupported version (expected v4.1.0)");

    if (fileHeader->_20.platformId != NX___ID)
        Panic("BntxPreprocess: platform ID is nonmatching (expected NX)");

    if (fileHeader->_20.textureCount == 0)
        Panic("BntxPreprocess: texture count is zero");
}

const char* BntxGetTextureGroupName(ConsBuffer bntxData) {
    const BntxFileHeader* fileHeader = bntxData.data_void;

    return (const char*)(bntxData.data_u8 + fileHeader->_00.filenameOffset);
}

u32 BntxGetTextureCount(ConsBuffer bntxData) {
    const BntxFileHeader* fileHeader = bntxData.data_void;

    return fileHeader->_20.textureCount;
}

s64 BntxFindTextureIndex(ConsBuffer bntxData, const char* textureName) {
    const BntxFileHeader* fileHeader = bntxData.data_void;
    const NnDic* dic = (NnDic*)(bntxData.data_u8 + fileHeader->_20.dicPtr);

    const NnDicNode* node = NnDicFind(bntxData.data_void, dic, textureName);
    if (node == NULL)
        return -1;

    return (s64)NnDicNodeGetIndex(dic, node);
}

static BntxTextureBlock* _IndexTexture(ConsBuffer bntxData, u32 textureIndex) {
    const BntxFileHeader* fileHeader = bntxData.data_void;
    if (textureIndex >= fileHeader->_20.textureCount)
        return NULL;

    u64* texturePointers = (u64*)(bntxData.data_u8 + fileHeader->_20.texturePointersPtr);

    return (BntxTextureBlock*)(bntxData.data_u8 + texturePointers[textureIndex]);
}

NnString* BntxGetTextureName(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return NULL;

    return (NnString*)(bntxData.data_u8 + texture->namePtr);
}

u32 BntxGetTextureFormat(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return 0;

    return texture->imageFormat;
}

u32 BntxGetTextureTileMode(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return 0;

    return texture->tileMode;
}

u32 BntxGetTextureWidth(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return 0;
    return texture->width;
}
u32 BntxGetTextureHeight(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return 0;
    return texture->height;
}

ConsBuffer BntxDecodeTexture(ConsBuffer bntxData, u32 textureIndex) {
    BntxTextureBlock* texture = _IndexTexture(bntxData, textureIndex);
    if (texture == NULL)
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, texture->width * texture->height * 4);

    u64* dataPointers = (u64*)(bntxData.data_u8 + texture->dataPointersPtr);

    ConsBufferView swizzledView = BufferViewFromPtr(
        bntxData.data_u8 + dataPointers[0], texture->dataSize
    );

    ConsBuffer deswizzled = deswizzle_block_linear(
        texture->width / 4, texture->height / 4, texture->depth, swizzledView, 4, 16
    );

    unsigned char* currentBlock = deswizzled.data_u8;
    unsigned char* currentOutput = buffer.data_u8;
    while (currentBlock < (u8*)BufferGetEnd(&deswizzled) && currentOutput + (16 * 4) <= (u8*)BufferGetEnd(&buffer)) {
        BCNDecode_BC3(currentBlock, (u8(*)[4][4])currentOutput);
        currentBlock += 16;
        currentOutput += 16 * 4;
    }

    BufferDestroy(&deswizzled);

    return buffer;
}
