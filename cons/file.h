#ifndef CONS_FILE_H
#define CONS_FILE_H

// CONS -- filesystem implementation

#include "buffer.h"

#include "list.h"

#include "type.h"

// Load a file from the FS into a buffer.
ConsBuffer FileLoadMem(const char* path);

// Write a file to the FS. If the view is empty, this will create an empty file.
// Returns true on success, false on failure.
bool FileWriteMem(ConsBufferView view, const char* path);

// Create a directory tree.
// Returns true on success, false on failure.
bool DirectoryCreateTree(const char* dirPath);

// Get a list of all files in a directory (file paths), including subfiles.
// Note: the strings contained in this list are dynamically allocated and must be freed.
ConsList DirectoryGetAllFiles(const char* rootPath);

// Get the name of a directory by it's path.
// Returns dynamically allocated string containing directory name on success, NULL on failure.
char* DirectoryGetName(const char* dirPath);

// Remove a file from the FS.
// Returns true on success, false on failure.
bool FileRemove(const char* path);

#endif // CONS_FILE_H
