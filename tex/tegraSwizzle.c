#include "tegraSwizzle.h"

#include "../cons/macro.h"

#include <string.h>

#define GOB_WIDTH_IN_BYTES (64)
#define GOB_HEIGHT_IN_BYTES (8)
#define GOB_SIZE_IN_BYTES (GOB_WIDTH_IN_BYTES * GOB_HEIGHT_IN_BYTES)

static inline u64 div_round_up(u64 x, u64 d) {
    return (x + d - 1) / d;
}

static inline u64 next_multiple_of(u64 a, u64 b) {
    if (b == 0)
        return a;
    return ((a + b - 1) / b) * b;
}

static inline u64 width_in_gobs(u64 width, u64 bytesPerPixel) {
    return div_round_up(width * bytesPerPixel, GOB_WIDTH_IN_BYTES);
}

static inline u64 height_in_blocks(u64 height, u64 blockHeight) {
    return div_round_up(height, blockHeight * GOB_HEIGHT_IN_BYTES);
}

static inline u64 height_in_gobs(u64 height, u64 blockHeight) {
    return height_in_blocks(height, blockHeight) * blockHeight;
}

static u64 block_depth(u64 depth) {
    u64 depthAndHalf = depth + (depth / 2);
    if (depthAndHalf >= 16) {
        return 16;
    } else if (depthAndHalf >= 8) {
        return 8;
    } else if (depthAndHalf >= 4) {
        return 4;
    } else if (depthAndHalf >= 2) {
        return 2;
    }

    return 1;
}

static inline u64 deswizzled_mip_size(
    u64 width, u64 height, u64 depth,
    u64 bytesPerPixel
) {
    return width * height * depth * bytesPerPixel;
}

static u64 swizzled_mip_size(
    u64 width, u64 height, u64 depth,
    u64 blockHeight, u64 bytesPerPixel
) {
    u64 widthInGobs = width_in_gobs(width, bytesPerPixel);
    u64 heightInGobs = height_in_gobs(height, blockHeight);
    u64 depthInGobs = next_multiple_of(depth, block_depth(depth));

    u64 gobCount = widthInGobs * heightInGobs * depthInGobs;
    return gobCount * GOB_SIZE_IN_BYTES;
}

static inline u64 slice_size(u64 blockHeight, u64 blockDepth, u64 widthInGobs, u64 height) {
    u64 robSize = GOB_SIZE_IN_BYTES * blockHeight * blockDepth * widthInGobs;
    return div_round_up(height, blockHeight * GOB_HEIGHT_IN_BYTES) * robSize;
}

static inline u64 gob_address_z(u64 z, u64 blockHeight, u64 blockDepth, u64 sliceSize) {
    return (z / blockDepth * sliceSize) + ((z & (blockDepth - 1)) * GOB_SIZE_IN_BYTES * blockHeight);
}

static inline u64 gob_address_y(u64 y, u64 blockHeightInBytes, u64 blockSizeInBytes, u64 imageWidthInGobs) {
    u64 blockY = y / blockHeightInBytes;
    u64 blockInnerRow = y % blockHeightInBytes / GOB_HEIGHT_IN_BYTES;
    return blockY * blockSizeInBytes * imageWidthInGobs + blockInnerRow * GOB_SIZE_IN_BYTES;
}

static inline u64 gob_address_x(u64 x, u64 blockSizeInBytes) {
    u64 blockX = x / GOB_WIDTH_IN_BYTES;
    return blockX * blockSizeInBytes;
}

static inline void deswizzle_gob_row(
    ConsBufferView dst, u64 dstOffset, ConsBufferView src, u64 srcOffset
) {
    memcpy(dst.data_u8 + dstOffset + 48, src.data_u8 + srcOffset + 288, 16);
    memcpy(dst.data_u8 + dstOffset + 32, src.data_u8 + srcOffset + 256, 16);
    memcpy(dst.data_u8 + dstOffset + 16, src.data_u8 + srcOffset + 32,  16);
    memcpy(dst.data_u8 + dstOffset + 0,  src.data_u8 + srcOffset + 0,   16);
}

static const u64 GOB_ROW_OFFSETS[] = { 0, 16, 64, 80, 128, 144, 192, 208 };

static inline void deswizzle_complete_gob(ConsBufferView dst, ConsBufferView src, u64 rowSizeInBytes) {
    for (u64 i = 0; i < ARR_LIT_LEN(GOB_ROW_OFFSETS); i++) {
        u64 offset = GOB_ROW_OFFSETS[i];
        deswizzle_gob_row(dst, rowSizeInBytes * i, src, offset);
    }
}

static inline u64 gob_offset(u64 x, u64 y) {
    // lmao
    return ((x % 64) / 32) * 256 + ((y % 8) / 2) * 64 + ((x % 32) / 16) * 32 + (y % 2) * 16 + (x % 16);
}

static inline void swizzle_deswizzle_gob(bool doDeswizzle,
    ConsBufferView dst, ConsBufferView src,
    u64 x0, u64 y0, u64 z0, u64 width, u64 height,
    u64 bytesPerPixel, u64 gobAddress
) {
    for (u64 y = 0; y < GOB_HEIGHT_IN_BYTES; y++) {
        for (u64 x = 0; x < GOB_WIDTH_IN_BYTES; x++) {
            if (
                ((y0 + y) < height) &&
                ((x0 + x) < (width * bytesPerPixel))
            ) {
                u64 swizzledOffset = gobAddress + gob_offset(x, y);
                u64 linearOffset = (z0 * width * height * bytesPerPixel) +
                    ((y0 + y) * width * bytesPerPixel) +
                    (x0 + x);
                
                if (doDeswizzle)
                    dst.data_u8[linearOffset] = src.data_u8[swizzledOffset];
                else
                    dst.data_u8[swizzledOffset] = src.data_u8[linearOffset];
            }
        }
    }
}

static void swizzle_inner(bool doDeswizzle,
    u64 width, u64 height, u64 depth,
    ConsBufferView source, ConsBufferView dest,
    u64 blockHeight, u64 blockDepth, u64 bytesPerPixel
) {
    u64 widthInGobs = width_in_gobs(width, bytesPerPixel);

    u64 sliceSize = slice_size(blockHeight, blockDepth, widthInGobs, height);

    u64 blockWidth = 1;
    u64 blockSizeInBytes = GOB_SIZE_IN_BYTES * blockWidth * blockHeight * blockDepth;
    u64 blockHeightInBytes = GOB_HEIGHT_IN_BYTES * blockHeight;

    for (u64 z0 = 0; z0 < depth; z0++) {
        u64 offsetZ = gob_address_z(z0, blockHeight, blockDepth, sliceSize);

        for (u64 y0 = 0; y0 < height; y0 += GOB_HEIGHT_IN_BYTES) {
            u64 offsetY = gob_address_y(
                y0, blockHeightInBytes, blockSizeInBytes, widthInGobs
            );

            for (u64 x0 = 0; x0 < (width * bytesPerPixel); x0 += GOB_WIDTH_IN_BYTES) {
                u64 offsetX = gob_address_x(x0, blockSizeInBytes);

                u64 gobAddress = offsetZ + offsetY + offsetX;

                if (
                    (x0 + GOB_WIDTH_IN_BYTES) < (width * bytesPerPixel) &&
                    (y0 + GOB_HEIGHT_IN_BYTES) < height
                ) {
                    u64 linearOffset = (z0 * width * height * bytesPerPixel) +
                        (y0 * width * bytesPerPixel) +
                        x0;

                    u64 rowSizeInBytes = width * bytesPerPixel;

                    if (doDeswizzle) {
                        ConsBufferView destView = BufferViewFromPtr(
                            dest.data_u8 + linearOffset, dest.size - linearOffset
                        );
                        ConsBufferView sourceView = BufferViewFromPtr(
                            source.data_u8 + gobAddress, source.size - gobAddress
                        );

                        deswizzle_complete_gob(destView, sourceView, rowSizeInBytes);
                    }
                    else {
                        // TODO: implement
                        // swizzle_complete_gob;
                    }
                }
                else {
                    swizzle_deswizzle_gob(doDeswizzle,
                        dest, source,
                        x0, y0, z0, width, height,
                        bytesPerPixel, gobAddress
                    );
                }
            }
        }
    }
}

ConsBuffer deswizzle_block_linear(
    u32 width, u32 height, u32 depth,
    ConsBufferView source, u32 blockHeight, u32 bytesPerPixel
) {
    u64 expectedSize = swizzled_mip_size(width, height, depth, blockHeight, bytesPerPixel);
    if (source.size < expectedSize)
        return (ConsBuffer){ 0 };

    ConsBuffer buffer;
    BufferInit(&buffer, deswizzled_mip_size(width, height, depth, bytesPerPixel));

    u64 blockDepth = block_depth(depth);

    swizzle_inner(true,
        width, height, depth,
        source, BUFFER_TO_VIEW(buffer),
        blockHeight, blockDepth, bytesPerPixel
    );

    return buffer;
}
