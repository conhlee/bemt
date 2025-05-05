#ifndef CONS_FILE_H
#define CONS_FILE_H

#include "buffer.h"

#include "list.h"

#include "type.h"

// Load a file from the FS into a buffer.
ConsBuffer FileLoadMem(const char* path);

// Write a file to the FS. If the view is empty, this will create an empty file.
// Returns 0 on success, 1 on failure.
int FileWriteMem(ConsBufferView view, const char* path);

// Create a directory tree.
// Returns 0 on success, 1 on failure.
int DirectoryCreateTree(const char* dirPath);

#endif // CONS_FILE_H
