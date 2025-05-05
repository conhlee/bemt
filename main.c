#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "cons/cons.h"

#include "process/beaProcess.h"
#include "process/bntxProcess.h"

#include "stb/stb_image_write.h"

#include "tex/tegraSwizzle.h"

void usage(char* arg0) {
    printf(
        "bemt (Bezel Engine MultiTool) v0.0\n"
        "a hacky piece of software by Conhlee LLC Ltd Inc Enterprises Group\n"
        "Solutions Incorporated Co. Limited BV CV VOF Gmbh Holdings\n"
        "Worldwide International Corporation\n"
        "\n"
        "usage: %s [mode] [input_files_or_directories] <output_file>\n"
        "\n"
        "modes:\n"
        "     bea_extract      Extract all files from a BEA archive.\n"
        "     bea_pack         Pack the input directory into a BEA archive.\n"
        "\n"
        "     bntx_extract     Extract all textures from a BNTX texture group.\n",
        arg0
    );
}

int main(int argc, char** argv) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    const char* mode = argv[1];

    if (strcasecmp(mode, "bea_extract") == 0) {
        printf("-- Extracting BEA --\n");

        ConsBuffer buffer = FileLoadMem(argv[2]);
        if (!BufferIsValid(&buffer))
            Panic("Failed to load BEA file");

        BeaPreprocess(buffer);

        const NnString* archiveName = BeaGetArchiveName(buffer);

        u32 assetCount = BeaGetAssetCount(buffer);
        for (u32 i = 0; i < assetCount; i++) {
            const NnString* filename = BeaGetAssetFilename(buffer, i);

            printf("Extracting: %.*s ..", (int)filename->len, filename->str);
            fflush(stdout);

            char filePath[1024];
            snprintf(
                filePath, sizeof(filePath), "%.*s/%.*s",
                (int)archiveName->len, archiveName->str, (int)filename->len, filename->str
            );

            char* lastSlash = strrchr(filePath, '/');
            if (lastSlash) {
                *lastSlash = '\0';
                if (DirectoryCreateTree(filePath)) {
                    printf(" FAIL\n");
                    continue;
                }
                *lastSlash = '/';
            }

            ConsBuffer decompressedData = BeaGetDecompressedData(buffer, i);
            if (!BufferIsValid(&decompressedData)) {
                printf(" FAIL\n");
                continue;
            }

            int writeRes = FileWriteMem(BUFFER_TO_VIEW(decompressedData), filePath);
            BufferDestroy(&decompressedData);

            if (writeRes)
                printf(" FAIL\n");
            else
                printf(" OK\n");
        }

        BufferDestroy(&buffer);
    }
    else if (strcasecmp(mode, "bntx_test") == 0) {
        ConsBuffer bufferTiled = FileLoadMem("/Users/angelo/Downloads/128_bc3_tiled.bin");
        ConsBuffer bufferLinear = FileLoadMem("/Users/angelo/Downloads/128_bc3.bin");

        ConsBuffer buffer = deswizzle_block_linear(128 / 4, 128 / 4, 1, BUFFER_TO_VIEW(bufferTiled), 4, 16);

        printf("result: %u\n", BufferViewCompare(BUFFER_TO_VIEW(buffer), BUFFER_TO_VIEW(bufferLinear)));
    }
    else if (strcasecmp(mode, "bntx_extract") == 0) {
        printf("-- Extracting BNTX --\n");

        ConsBuffer buffer = FileLoadMem(argv[2]);
        if (!BufferIsValid(&buffer))
            Panic("Failed to load BNTX file");

        BntxPreprocess(buffer);

        const char* groupName = BntxGetTextureGroupName(buffer);
        u32 textureCount = BntxGetTextureCount(buffer);

        printf("%s (%u textures)\n", groupName, textureCount);

        for (u32 i = 0; i < textureCount; i++) {
            printf("    - %s (%u %u)\n", BntxGetTextureName(buffer, i)->str, BntxGetTextureFormat(buffer, i), BntxGetTextureTileMode(buffer,i));
            ConsBuffer decoded = BntxDecodeTexture(buffer, i);
            if (BufferIsValid(&decoded)) {
                char nameBuf[512];
                snprintf(nameBuf, sizeof(nameBuf), "%s.png", BntxGetTextureName(buffer, i)->str);
                int res = stbi_write_png(
                    nameBuf,
                    BntxGetTextureWidth(buffer, i), BntxGetTextureHeight(buffer, i),
                    4, decoded.data_void, BntxGetTextureWidth(buffer, i) * 4
                );
                BufferDestroy(&decoded);
            }
        }

        BufferDestroy(&buffer);
    }
    else {
        usage(argv[0]);
        return 1;
    }

    printf("\nAll done.\n");
    return 0;
}
