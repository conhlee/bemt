#ifndef BNTX_PROCESS_H
#define BNTX_PROCESS_H

#include "../cons/buffer.h"

#include "nnBin.h"

void BntxPreprocess(ConsBuffer bntxData);

const char* BntxGetTextureGroupName(ConsBuffer bntxData);

u32 BntxGetTextureCount(ConsBuffer bntxData);

// Returns <0 if texture is not found.
s64 BntxFindTextureIndex(ConsBuffer bntxData, const char* textureName);

NnString* BntxGetTextureName(ConsBuffer bntxData, u32 textureIndex);

u32 BntxGetTextureFormat(ConsBuffer bntxData, u32 textureIndex);
u32 BntxGetTextureTileMode(ConsBuffer bntxData, u32 textureIndex);

u32 BntxGetTextureWidth(ConsBuffer bntxData, u32 textureIndex);
u32 BntxGetTextureHeight(ConsBuffer bntxData, u32 textureIndex);

ConsBuffer BntxDecodeTexture(ConsBuffer bntxData, u32 textureIndex);

#endif // BNTX_PROCESS_H
