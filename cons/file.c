#include "file.h"

#include "linklist.h"

#include "error.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#ifdef _WIN32

#include <windows.h>
#include <direct.h>  // _mkdir
#include <io.h>      // _access

#define PATH_SEPARATOR '\\'

#else // _WIN32

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <libgen.h>

#define PATH_SEPARATOR '/'
#define MAX_PATH (4096)

#endif // _WIN32

#include <errno.h>

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

    u64 fileSize = (u64)ftell(fp);
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

bool FileWriteMem(ConsBufferView view, const char* path) {
    if (path == NULL)
        return false;

    FILE* fp = fopen(path, "wb");
    if (fp == NULL)
        return false;

    if (BufferViewIsValid(&view)) {
        u64 bytesCopied = fwrite(view.data_void, 1, view.size, fp);
        if (bytesCopied < view.size && ferror(fp)) {
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

bool DirectoryCreateTree(const char* _dirPath) {
    if (_dirPath == NULL)
        return false;

    u64 dirPathLen = strlen(_dirPath);
    char* dirPath = malloc(dirPathLen + 1);
    if (!dirPath)
        return false;
    memcpy(dirPath, _dirPath, dirPathLen + 1);

    // Normalize path separators on Windows
    #ifdef _WIN32
    for (u64 i = 0; i < dirPathLen; ++i) {
        if (dirPath[i] == '/')
            dirPath[i] = '\\';
    }
    #endif

    if (dirPathLen > 0 && (dirPath[dirPathLen - 1] == '/' || dirPath[dirPathLen - 1] == '\\'))
        dirPath[--dirPathLen] = '\0';

    char* p = dirPath;
    // For Windows, handle drive letter e.g. "C:\"
    if (
        #ifdef _WIN32
        dirPathLen > 2 && dirPath[1] == ':' && (dirPath[2] == '\\' || dirPath[2] == '/')
        #else
        dirPathLen > 0 && dirPath[0] == '/'
        #endif
    ) {
        p = dirPath + 1;
    }

    for (; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            #ifdef _WIN32
                if (_mkdir(dirPath) != 0 && errno != EEXIST) {
                    free(dirPath);
                    return false;
                }
            #else
                if (mkdir(dirPath, 0700) != 0 && errno != EEXIST) {
                    free(dirPath);
                    return false;
                }
            #endif
            *p = PATH_SEPARATOR;
        }
    }

    #ifdef _WIN32
    if (_mkdir(dirPath) != 0 && errno != EEXIST) {
        free(dirPath);
        return false;
    }
    #else
    if (mkdir(dirPath, 0700) != 0 && errno != EEXIST) {
        free(dirPath);
        return false;
    }
    #endif

    free(dirPath);
    return true;
}

static bool _IsDirectory(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
#endif
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

#ifdef _WIN32
        WIN32_FIND_DATAA findFileData;
        char searchPath[MAX_PATH];
        snprintf(searchPath, MAX_PATH, "%s\\*", currentPath);
        HANDLE hFind = FindFirstFileA(searchPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            Warn("DirectoryGetAllFiles: FindFirstFile failed on path '%s'; skipping", currentPath);
            free(currentPath);
            continue;
        }

        do {
            const char* name = findFileData.cFileName;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            char fullPath[MAX_PATH];
            snprintf(fullPath, MAX_PATH, "%s\\%s", currentPath, name);

            if (_IsDirectory(fullPath)) {
                LinkListInsertTail(&queue, (u64)strdup(fullPath));
            } else {
                char* fileCopy = strdup(fullPath);
                ListAdd(&fileList, &fileCopy);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);

        FindClose(hFind);
#else
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
            }
            else {
                char* fileCopy = strdup(fullPath);
                ListAdd(&fileList, &fileCopy);
            }
        }

        closedir(dir);
#endif
        free(currentPath);
    }

    return fileList;
}

char* DirectoryGetName(const char* dirPath) {
    char resolvedPath[MAX_PATH];

#ifdef _WIN32
    if (_fullpath(resolvedPath, dirPath, MAX_PATH) == NULL) {
        Warn("DirectoryGetName: _fullpath failed ..");
        return NULL;
    }

    // Find last path separator
    char* lastSep = strrchr(resolvedPath, '\\');
#else
    if (realpath(dirPath, resolvedPath) == NULL) {
        Warn("DirectoryGetName: realpath failed ..");
        return NULL;
    }

    char* lastSep = strrchr(resolvedPath, '/');
#endif

    if (lastSep == NULL)
        return strdup(resolvedPath);

    return strdup(lastSep + 1);
}

bool FileRemove(const char* path) {
    return remove(path) == 0;
}
