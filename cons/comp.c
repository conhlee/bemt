#include "comp.h"

#include "error.h"

#include <stdlib.h>

#include <zlib.h>
#include <zstd.h>

ConsBuffer CompressZlib(ConsBufferView data) {
    if (!BufferViewIsValid(&data))
        return (ConsBuffer){ 0 };

    if (data.size > 0xFFFFFFFF)
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, compressBound(data.size));

    z_stream strm = { 0 };
    strm.avail_in = (u32)data.size;
    strm.next_in = data.data_u8;

    strm.avail_out = (u32)buffer.size;
    strm.next_out = buffer.data_u8;

    const int init = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (init != Z_OK) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    const int ret = deflate(&strm, Z_FINISH);
    deflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    BufferResize(&buffer, strm.total_out);
    return buffer;
}

ConsBuffer DecompressZlib(ConsBufferView data, u64 decompressedSize) {
    if (!BufferViewIsValid(&data))
        return (ConsBuffer){ 0 };

    if (data.size > 0xFFFFFFFF || decompressedSize > 0xFFFFFFFF)
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, decompressedSize);

    z_stream strm = { 0 };
    strm.avail_in = (u32)data.size;
    strm.next_in = data.data_u8;

    strm.avail_out = (u32)buffer.size;
    strm.next_out = buffer.data_u8;

    const int init = inflateInit(&strm);
    if (init != Z_OK) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    const int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    BufferResize(&buffer, strm.total_out);
    return buffer;
}

ConsBuffer CompressZstd(ConsBufferView data) {
    if (!BufferViewIsValid(&data))
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, ZSTD_compressBound(data.size));

    u64 compressedSize = ZSTD_compress(
        buffer.data_void, buffer.size,
        data.data_void, data.size, 17
    );
    if (ZSTD_isError(compressedSize)) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    BufferResize(&buffer, compressedSize);
    return buffer;
}

ConsBuffer DecompressZstd(ConsBufferView data, u64 _decompressedSize) {
    if (!BufferViewIsValid(&data))
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, _decompressedSize);

    u64 decompressedSize = ZSTD_decompress(
        buffer.data_void, buffer.size,
        data.data_void, data.size
    );
    if (ZSTD_isError(decompressedSize)) {
        BufferDestroy(&buffer);
        return (ConsBuffer){ 0 };
    }

    BufferResize(&buffer, decompressedSize);
    return buffer;
}
