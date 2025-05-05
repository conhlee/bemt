#include "file.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

ConsBuffer LoadWholeFile(const char* path) {
    ConsBuffer buffer = {0};

    if (path == NULL)
        return buffer;

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return buffer;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return buffer;
    }

    u64 fileSize = ftell(fp);
    if (fileSize == (u64)-1L) {
        fclose(fp);
        return buffer;
    }

    BufferInit(&buffer, fileSize);

    rewind(fp);
    u64 bytesCopied = fread(buffer.data_void, 1, buffer.size, fp);
    if (bytesCopied < fileSize && ferror(fp)) {
        fclose(fp);
        BufferDestroy(&buffer);
        return buffer;
    }

    fclose(fp);
    return buffer;
}

int WriteWholeFile(ConsBufferView view, const char* path) {
    if (path == NULL)
        return 1;

    FILE* fp = fopen(path, "wb");
    if (fp == NULL)
        return 1;

    if (BufferViewIsValid(&view)) {
        u64 bytesCopied = fwrite(view.data_void, 1, view.size, fp);
        if (bytesCopied < view.size && ferror(fp)) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int CreateDirectories(const char* _dirPath) {
    if (_dirPath == NULL)
        return 1;

    u64 dirPathLen = strlen(_dirPath);
    char* dirPath = (char*)malloc(dirPathLen + 1);
    memcpy(dirPath, _dirPath, dirPathLen + 1);

    if (dirPathLen > 0 && dirPath[dirPathLen - 1] == '/')
        dirPath[--dirPathLen] = '\0';

    char* p = dirPath + 1;
    for (; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dirPath, S_IRWXU) != 0 && errno != EEXIST) {
                free(dirPath);
                return 1;
            }
            *p = '/';
        }
    }

    if (mkdir(dirPath, S_IRWXU) != 0 && errno != EEXIST) {
        free(dirPath);
        return 1;
    }

    free(dirPath);
    return 0;
}
