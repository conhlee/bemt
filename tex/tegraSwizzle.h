#ifndef TEGRA_SWIZZLE_H
#define TEGRA_SWIZZLE_H

#include "../cons/type.h"

#include "../cons/buffer.h"

// Ported from https://github.com/ScanMountGoat/tegra_swizzle

u64 deswizzled_mip_size(
    u64 width, u64 height, u64 depth,
    u64 bytesPerPixel
);

u64 swizzled_mip_size(
    u64 width, u64 height, u64 depth,
    u64 blockHeight, u64 bytesPerPixel
);

ConsBuffer deswizzle_block_linear(
    u32 width, u32 height, u32 depth,
    ConsBufferView source, u32 blockHeight, u32 bytesPerPixel
);

#endif // TEGRA_SWIZZLE_H
