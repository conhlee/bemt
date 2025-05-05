#ifndef TEGRA_SWIZZLE_H
#define TEGRA_SWIZZLE_H

#include "../cons/type.h"

#include "../cons/buffer.h"

ConsBuffer deswizzle_block_linear(
    u32 width, u32 height, u32 depth,
    ConsBufferView source, u32 blockHeight, u32 bytesPerPixel
);

#endif // TEGRA_SWIZZLE_H
