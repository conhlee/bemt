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
        "     bea_unpack       Extract all assets from a BEA archive.\n"
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

    if (strcasecmp(mode, "bea_unpack") == 0) {
        printf("-- Unpacking BEA --\n");

        ConsBuffer buffer = FileLoadMem(argv[2]);
        if (!BufferIsValid(&buffer))
            Panic("Failed to load BEA file");
        ConsBufferView beaView = BUFFER_TO_VIEW(buffer);

        BeaPreprocess(beaView);

        const NnString* archiveName = BeaGetArchiveName(beaView);

        u32 assetCount = BeaGetAssetCount(beaView);
        for (u32 i = 0; i < assetCount; i++) {
            const NnString* filename = BeaGetAssetFilename(beaView, i);

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
                    printf(" DIR CREATE FAIL\n");
                    continue;
                }
                *lastSlash = '/';
            }

            ConsBuffer decompressedData = BeaGetDecompressedData(beaView, i);
            if (!BufferIsValid(&decompressedData)) {
                printf(" DECOMPRESS FAIL\n");
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
    else if (strcasecmp(mode, "bea_pack") == 0) {          
        char* rootDirPath = strdup(argv[2]);

        // Remove trailing slashes.
        char* rootDirPathEnd = rootDirPath + strlen(rootDirPath) - 1;
        while (rootDirPathEnd > rootDirPath && *rootDirPathEnd == '/') {
            *rootDirPathEnd = '\0';
            rootDirPathEnd--;
        }

        char* archiveName = DirectoryGetName(rootDirPath);

        printf("-- Creating archive '%s' --\n\n", archiveName);

        printf("Getting files:\n");
        fflush(stdout);

        ConsList fileList = DirectoryGetAllFiles(rootDirPath);

        if (ListIsEmpty(&fileList)) {
            Panic("Failed to open directory at path '%s'!", argv[2]);
        }

        const u64 rootDirPathLen = strlen(rootDirPath);

        BeaBuildAsset* buildAssets = malloc(sizeof(BeaBuildAsset) * fileList.elementCount);
        for (u32 i = 0; i < fileList.elementCount; i++) {
            char* filePath = *(char**)ListGet(&fileList, i);

            buildAssets[i].name = filePath + rootDirPathLen + 1;
            buildAssets[i].compressionType = BEA_COMPRESSION_TYPE_ZSTD;
            buildAssets[i].alignmentShift = 8; // 256byte alignment by default.
            
            printf("    - %s\n", buildAssets[i].name);

            // This is a little iffy ..
            buildAssets[i].dataView = BUFFER_TO_VIEW(FileLoadMem(filePath));
            if (!BufferViewIsValid(&buildAssets[i].dataView)) {
                Panic("Failed to open file at path '%s'!", filePath);
            }
        }

        printf("Serializing archive..");
        fflush(stdout);

        ConsBuffer beaBuffer = BeaBuild(buildAssets, fileList.elementCount, archiveName);

        free(archiveName);

        printf(" OK\nWriting file to path '%s'..", argv[3]);
        fflush(stdout);

        if (FileWriteMem(BUFFER_TO_VIEW(beaBuffer), argv[3])) {
            Panic("Failed to write archive to disk!");
        }

        printf(" OK\n");

        free(rootDirPath);

        for (unsigned i = 0; i < fileList.elementCount; i++) {
            BufferDestroy(&buildAssets[i].dataView.as_buffer);
            free(*(char**)ListGet(&fileList, i));
        }
        ListDestroy(&fileList);
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
