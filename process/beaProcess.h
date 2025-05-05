#ifndef BEA_PROCESS_H
#define BEA_PROCESS_H

#include "../cons/buffer.h"

#include "nnBin.h"

typedef enum BeaCompressionType {
    BEA_COMPRESSION_TYPE_NONE = 0,
    BEA_COMPRESSION_TYPE_ZLIB = 1,
    BEA_COMPRESSION_TYPE_ZSTD = 2,
} BeaCompressionType;

void BeaPreprocess(ConsBuffer beaData);

// Ownership belongs to beaData.
NnString* BeaGetArchiveName(ConsBuffer beaData);

u32 BeaGetAssetCount(ConsBuffer beaData);

// Returns <0 if asset is not found.
s64 BeaFindAssetIndex(ConsBuffer beaData, const char* filename);

// Ownership belongs to beaData.
NnString* BeaGetAssetFilename(ConsBuffer beaData, u32 assetIndex);

BeaCompressionType BeaGetAssetCompressionType(ConsBuffer beaData, u32 assetIndex);
u32 BeaGetAssetAlignment(ConsBuffer beaData, u32 assetIndex);
u32 BeaGetAssetCompressedSize(ConsBuffer beaData, u32 assetIndex);
u32 BeaGetAssetDecompressedSize(ConsBuffer beaData, u32 assetIndex);

// Ownership belongs to beaData.
ConsBufferView BeaGetCompressedData(ConsBuffer beaData, u32 assetIndex);
// Ownership belongs to caller.
ConsBuffer BeaGetDecompressedData(ConsBuffer beaData, u32 assetIndex);

#endif
