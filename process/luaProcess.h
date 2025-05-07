#ifndef LUA_PROCESS_H
#define LUA_PROCESS_H

#include "../cons/buffer.h"

void LuaPreprocess(ConsBufferView luaData);

// 128-bit hash (probably MD5) of the source file.
const char* LuaGetSourceHash(ConsBufferView luaData);

ConsBufferView LuaGetBytecode(ConsBufferView luaData);

ConsBuffer LuaBuild(const char* luaSource);

#endif // LUA_PROCESS_H
