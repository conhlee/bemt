#ifndef CONS_COMP_H
#define CONS_COMP_H

#include "buffer.h"

// Compress data into Zlib format (DEFLATE).
ConsBuffer CompressZlib(ConsBufferView data);
// Decompress Zlib data (INFLATE).
ConsBuffer DecompressZlib(ConsBufferView data, u64 decompressedSize);

// Compress data into Zstandard format.
ConsBuffer CompressZstd(ConsBufferView data);
// Decompress Zstandard data.
ConsBuffer DecompressZstd(ConsBufferView data, u64 decompressedSize);

#endif // CONS_COMP_H
