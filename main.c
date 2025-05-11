#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "cons/cons.h"

#include "process/beaProcess.h"
#include "process/luaProcess.h"
#include "process/bntxProcess.h"

#include "lua/unluacInterface.h"

#include "stb/stb_image_write.h"

#include "tex/tegraSwizzle.h"

void usage(char* arg0) {
    printf(
        "bemt (Bezel Engine MultiTool) v0.5\n"
        "a hacky piece of software by Conhlee LLC Ltd Inc Enterprises Group\n"
        "Solutions Incorporated Co. Limited BV CV VOF Gmbh Holdings\n"
        "Worldwide International Corporation\n"
        "\n"
        "usage: %s [mode] <input_file_or_directory> <output_file_or_directory>\n"
        "\n"
        "modes:\n"
        "     bea_unpack       Extract all assets from a BEA archive.\n"
        "     bea_pack         Pack the input directory into a BEA archive.\n"
        "\n"
        "     lua_decomp       Decompile a binary lua file.\n"
        "     lua_comp         Compile a lua file.\n"
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
        printf("-- Unpacking BEA at path '%s' --\n\n", argv[2]);

        ConsBuffer beaData = FileLoadMem(argv[2]);
        if (!BufferIsValid(&beaData))
            Panic("Failed to load BEA file");
        ConsBufferView beaView = BUFFER_TO_VIEW(beaData);

        BeaPreprocess(beaView);

        const NnString* archiveName = BeaGetArchiveName(beaView);

        const char* outputDir = argv[3];

        printf("Extracting assets:\n");

        u32 assetCount = BeaGetAssetCount(beaView);
        for (u32 i = 0; i < assetCount; i++) {
            const NnString* filename = BeaGetAssetFilename(beaView, i);

            printf("    - Extracting: %.*s ..", (int)filename->len, filename->str);
            fflush(stdout);

            char filePath[1024];
            snprintf(
                filePath, sizeof(filePath), "%s%s%.*s/%.*s",
                outputDir, (outputDir != NULL) ? "/" : "",
                (int)archiveName->len, archiveName->str, (int)filename->len, filename->str
            );

            char* lastSlash = strrchr(filePath, '/');
            if (lastSlash) {
                *lastSlash = '\0';
                if (!DirectoryCreateTree(filePath))
                    Panic("Failed to create directory tree at '%s' ..", filePath);

                *lastSlash = '/';
            }

            ConsBuffer decompressedData = BeaGetDecompressedData(beaView, i);
            if (!BufferIsValid(&decompressedData))
                Panic("Failed to decompress asset '%s' ..", filename->str);

            if (!FileWriteMem(BUFFER_TO_VIEW(decompressedData), filePath))
                Panic("Failed to write asset '%s' to path '%s' ..", filename->str, filePath);

            BufferDestroy(&decompressedData);

            printf(" OK\n");
        }

        BufferDestroy(&beaData);
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

        printf("-- Creating archive '%s' from path '%s' --\n\n", archiveName, rootDirPath);

        printf("Loading assets..");
        fflush(stdout);

        ConsList filePathList = DirectoryGetAllFiles(rootDirPath);
        const u64 assetCount = filePathList.elementCount;

        if (ListIsEmpty(&filePathList)) {
            Panic("Failed to open directory at path '%s'!", argv[2]);
        }

        const u64 rootDirPathLen = strlen(rootDirPath);

        BeaBuildAsset* buildAssets = malloc(sizeof(BeaBuildAsset) * assetCount);
        for (u64 i = 0; i < assetCount; i++) {
            char* filePath = *(char**)ListGet(&filePathList, i);

            buildAssets[i].name = filePath + rootDirPathLen + 1;
            buildAssets[i].compressionType = BEA_COMPRESSION_TYPE_ZSTD;
            buildAssets[i].alignmentShift = 12; // 4096 byte alignment by default.

            buildAssets[i].data = FileLoadMem(filePath);
            if (!BufferIsValid(&buildAssets[i].data)) {
                Panic("Failed to open file at path '%s'!", filePath);
            }
        }

        printf(" OK\n");

        ConsBuffer beaBuffer = BeaBuild(buildAssets, assetCount, archiveName);

        free(archiveName);

        printf("Writing final archive to path '%s'..", argv[3]);
        fflush(stdout);

        if (!FileWriteMem(BUFFER_TO_VIEW(beaBuffer), argv[3])) {
            Panic("Failed to write archive to disk!");
        }

        printf(" OK\n");

        free(rootDirPath);

        for (u64 i = 0; i < assetCount; i++) {
            BufferDestroy(&buildAssets[i].data);
            free(*(char**)ListGet(&filePathList, i));
        }
        ListDestroy(&filePathList);
    }
    else if (strcasecmp(mode, "lua_decomp") == 0) {
        printf("-- Decompiling Lua at path '%s' --\n\n", argv[2]);

        printf("Extracing bytecode..");
        fflush(stdout);

        ConsBuffer buffer = FileLoadMem(argv[2]);
        if (!BufferIsValid(&buffer))
            Panic("Failed to load Lua file ..");
        ConsBufferView luaView = BUFFER_TO_VIEW(buffer);

        LuaPreprocess(luaView);

        ConsBufferView luacView = LuaGetBytecode(luaView);
    
        char tempFilePath[] = "bemt_luadec_temp_XXXXXX";
        if (mkstemp(tempFilePath) == -1)
            Panic("mkstemp failed ..");

        if (!FileWriteMem(luacView, tempFilePath))
            Panic("Failed to write temporary luac file ..");

        BufferDestroy(&buffer);

        printf(" OK\nRunning unluac..");
        fflush(stdout);

        char* decompiled = UnluacRun(tempFilePath);
        if (decompiled == NULL)
            Panic("Failed to decompile Lua ..");

        printf(" OK\n");

        if (!FileRemove(tempFilePath))
            Warn("Failed to remove temporary luac file ..");

        if (!FileWriteMem(BufferViewFromCstr(decompiled), argv[3])) {
            Panic("Failed to write decompiled lua to disk!");
        }

        free(decompiled);
    }
    else if (strcasecmp(mode, "lua_comp") == 0) {
        printf("-- Compiling Lua --\n\n");

        printf("Compiling..");
        fflush(stdout);

        ConsBuffer sourceData = FileLoadMem(argv[2]);
        if (!BufferIsValid(&sourceData))
            Panic("Failed to load source Lua file ..");

        // Null-terminate.
        BufferGrow(&sourceData, 1);
        sourceData.data_s8[sourceData.size - 1] = '\0';

        ConsBuffer compiledBuf = LuaBuild((const char*)sourceData.data_s8);

        BufferDestroy(&sourceData);

        printf(" OK\n");

        if (!FileWriteMem(BUFFER_TO_VIEW(compiledBuf), argv[3])) {
            Panic("Failed to write lua binary to disk!");
        }

        BufferDestroy(&compiledBuf);
    }
    else if (strcasecmp(mode, "bntx_test") == 0) {
        ConsBuffer bufferTiled = FileLoadMem("/Users/angelo/Downloads/128_bc3_tiled.bin");
        ConsBuffer bufferLinear = FileLoadMem("/Users/angelo/Downloads/128_bc3.bin");

        ConsBuffer buffer = deswizzle_block_linear(128 / 4, 128 / 4, 1, BUFFER_TO_VIEW(bufferTiled), 4, 16);

        printf("result: %u\n", BufferViewCompare(BUFFER_TO_VIEW(buffer), BUFFER_TO_VIEW(bufferLinear)));
    }
    else if (strcasecmp(mode, "bntx_extract") == 0) {
        printf("-- Extracting BNTX --\n");

        ConsBuffer bntxData = FileLoadMem(argv[2]);
        if (!BufferIsValid(&bntxData))
            Panic("Failed to load BNTX file");
        ConsBufferView bntxView = BUFFER_TO_VIEW(bntxData);

        BntxPreprocess(bntxView);

        const char* groupName = BntxGetTextureGroupName(bntxView);
        u32 textureCount = BntxGetTextureCount(bntxView);

        printf("%s (%u textures)\n", groupName, textureCount);

        for (u32 i = 0; i < textureCount; i++) {
            printf("    - %s (%u %u)\n", BntxGetTextureName(bntxView, i)->str, BntxGetTextureFormat(bntxView, i), BntxGetTextureTileMode(bntxView,i));
            ConsBuffer decoded = BntxDecodeTexture(bntxView, i);
            if (BufferIsValid(&decoded)) {
                char nameBuf[512];
                snprintf(nameBuf, sizeof(nameBuf), "%s.png", BntxGetTextureName(bntxView, i)->str);
                int res = stbi_write_png(
                    nameBuf,
                    BntxGetTextureWidth(bntxView, i), BntxGetTextureHeight(bntxView, i),
                    4, decoded.data_void, BntxGetTextureWidth(bntxView, i) * 4
                );
                BufferDestroy(&decoded);
            }
        }

        BufferDestroy(&bntxData);
    }
    else {
        printf("Invalid mode '%s' ..\n\n", mode);

        usage(argv[0]);
        return 1;
    }

    printf("\nAll done.\n");
    return 0;
}
