#ifndef BEA_PROCESS_H
#define BEA_PROCESS_H

#include "../cons/buffer.h"

#include "nnBin.h"

typedef enum BeaCompressionType {
    BEA_COMPRESSION_TYPE_NONE = 0,
    BEA_COMPRESSION_TYPE_ZLIB = 1,
    BEA_COMPRESSION_TYPE_ZSTD = 2,
} BeaCompressionType;

void BeaPreprocess(ConsBufferView beaData);

// Ownership belongs to beaData.
NnString* BeaGetArchiveName(ConsBufferView beaData);

u32 BeaGetAssetCount(ConsBufferView beaData);

// Returns <0 if asset is not found.
s64 BeaFindAssetIndex(ConsBufferView beaData, const char* filename);

// Ownership belongs to beaData.
NnString* BeaGetAssetFilename(ConsBufferView beaData, u32 assetIndex);

BeaCompressionType BeaGetAssetCompressionType(ConsBufferView beaData, u32 assetIndex);
u32 BeaGetAssetAlignment(ConsBufferView beaData, u32 assetIndex);
u32 BeaGetAssetCompressedSize(ConsBufferView beaData, u32 assetIndex);
u32 BeaGetAssetDecompressedSize(ConsBufferView beaData, u32 assetIndex);

// Ownership belongs to beaData.
ConsBufferView BeaGetCompressedData(ConsBufferView beaData, u32 assetIndex);
// Ownership belongs to caller.
ConsBuffer BeaGetDecompressedData(ConsBufferView beaData, u32 assetIndex);

typedef struct BeaBuildAsset {
    const char* name; // Not owned by this structure.
    unsigned alignmentShift; // Alignment is 1 << alignmentShift.
    ConsBuffer data;
    BeaCompressionType compressionType;
} BeaBuildAsset;

ConsBuffer BeaBuild(const BeaBuildAsset* assets, u32 assetCount, const char* archiveName);

#endif
