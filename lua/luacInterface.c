#include "luacInterface.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../cons/error.h"

_Static_assert(LUA_VERSION_NUM == 502, "Lua v5.2 is required");

ConsBuffer LuacCompile(const char* luaSource) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_loadstring(L, luaSource) != LUA_OK)
        Panic("LuacCompile: failed to load script: %s", lua_tostring(L, -1));

    // get string.dump function
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "dump");
    lua_remove(L, -2); // Remove the "string" table

    // swap the compiled chunk and the dump function
    lua_insert(L, -2); // put compiled chunk below dump

    // call string.dump(compiled_chunk)
    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
        Panic("LuacCompile: failed to dump bytecode: %s", lua_tostring(L, -1));

    size_t bytecodeSize;
    const void* bytecode = lua_tolstring(L, -1, &bytecodeSize);

    ConsBuffer buffer;
    BufferInitCopy(&buffer, bytecode, bytecodeSize);

    lua_close(L);

    return buffer;
}
