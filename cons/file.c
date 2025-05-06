#include "file.h"

#include "linklist.h"

#include "error.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

#include <libgen.h>

#include <errno.h>

#define MAX_PATH (4096)

ConsBuffer FileLoadMem(const char* path) {
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

int FileWriteMem(ConsBufferView view, const char* path) {
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

int DirectoryCreateTree(const char* _dirPath) {
    if (_dirPath == NULL)
        return 1;

    u64 dirPathLen = strlen(_dirPath);
    char* dirPath = malloc(dirPathLen + 1);
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

static int _IsDirectory(const char* path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

ConsList DirectoryGetAllFiles(const char* rootPath) {
    ConsList fileList;
    ListInit(&fileList, sizeof(char*), 128);

    ConsLLNode* queue = NULL;
    LinkListInsertTail(&queue, (u64)strdup(rootPath));

    while (queue) {
        ConsLLNode* currentNode = queue;
        queue = queue->next;
        if (queue != NULL)
            queue->prev = NULL;

        char* currentPath = (char*)currentNode->data;
        LinkListDeleteNode(&currentNode, currentNode);

        DIR* dir = opendir(currentPath);
        if (dir == NULL) {
            Warn("DirectoryGetAllFiles: opendir failed on path '%s'; skipping", currentPath);
            free(currentPath);
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullPath[MAX_PATH];
            snprintf(fullPath, MAX_PATH, "%s/%s", currentPath, entry->d_name);

            if (_IsDirectory(fullPath)) {
                LinkListInsertTail(&queue, (u64)strdup(fullPath));
            } else {
                char* fileCopy = strdup(fullPath);
                ListAdd(&fileList, &fileCopy);
            }
        }

        closedir(dir);
        free(currentPath);
    }

    return fileList;
}

char* DirectoryGetName(const char* dirPath) {
    char resolvedPath[MAX_PATH];
    if (realpath(dirPath, resolvedPath) == NULL) {
        Warn("DirectoryGetName: realpath failed ..");
        return NULL;
    }

    char pathCopy[MAX_PATH];
    strncpy(pathCopy, resolvedPath, MAX_PATH - 1);
    pathCopy[MAX_PATH - 1] = '\0';

    return strdup(basename(pathCopy));
}
