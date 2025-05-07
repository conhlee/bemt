#include "luaProcess.h"

#include "../cons/macro.h"

#include "../cons/error.h"

#include <string.h>

#include <stddef.h>

#include "../lua/luacInterface.h"

#define BZLA_MAGIC IDENTIFIER_TO_U32('B','Z','L','A')
#define _LUA_MAGIC IDENTIFIER_TO_U32(0x1B,'L','u','a')

#define LUAC_VERSION_EXPECT (0x52)

typedef struct __attribute__((packed)) {
    u32 identifier; // Compare to _LUA_MAGIC.
    u8 version; // Compare to LUAC_VERSION_EXPECT.
    u8 format; // Should be zero.

    u8 isLittleEndian;

    u8 sizeofInt;
    u8 sizeofSizeT;
    u8 sizeofInstruction;
    u8 sizeofNumber;

    u8 integral;
} LuaBytecodeHeader;

typedef struct __attribute__((packed)) {
    u32 identifier; // Compare to BZLA_MAGIC.
    char sourceHash[16]; // MD5 (?) hash of the original source file.
    u8 bytecode[0];
} LuaFileHeader;

void LuaPreprocess(ConsBufferView luaData) {
    const LuaFileHeader* fileHeader = luaData.data_void;
    
    if (fileHeader->identifier != BZLA_MAGIC)
        Panic("LuaPreprocess: header identifier is nonmatching");

    static const char emptyHash[16] = { 0 };
    if (memcmp(fileHeader->sourceHash, emptyHash, 16) == 0)
        Warn("LuaPreprocess: source file hash is empty");

    const LuaBytecodeHeader* bytecodeHeader = (LuaBytecodeHeader*)fileHeader->bytecode;

    if (bytecodeHeader->identifier != _LUA_MAGIC)
        Panic("LuaPreprocess: bytecode header identifier is nonmatching");
    if (bytecodeHeader->version != LUAC_VERSION_EXPECT) {
        const u32 majorExpect = (LUAC_VERSION_EXPECT & 0xF0) >> 4;
        const u32 minorExpect = LUAC_VERSION_EXPECT & 0x0F;

        const u32 majorGot = (bytecodeHeader->version & 0xF0) >> 4;
        const u32 minorGot = bytecodeHeader->version & 0x0F;

        Panic(
            "LuaPreprocess: expected bytecode version %u.%u, got ver %u.%u",
            majorExpect, minorExpect, majorGot, minorGot
        );
    }

    if (bytecodeHeader->isLittleEndian == 0)
        Warn("LuaPreprocess: bytecode is big-endian");
}

const char* LuaGetSourceHash(ConsBufferView luaData) {
    const LuaFileHeader* fileHeader = luaData.data_void;
    return fileHeader->sourceHash;
}

ConsBufferView LuaGetBytecode(ConsBufferView luaData) {
    return BufferViewFromPtr(
        luaData.data_u8 + offsetof(LuaFileHeader, bytecode),
        luaData.size - offsetof(LuaFileHeader, bytecode)
    );
}

ConsBuffer LuaBuild(const char* luaSource) {
    ConsBuffer bytecodeBuffer = LuacCompile(luaSource);
    if (!BufferIsValid(&bytecodeBuffer))
        Panic("LuaBuild: failed to compile lua");
    
    ConsBuffer luaBuffer;
    BufferInit(&luaBuffer, sizeof(LuaFileHeader) + bytecodeBuffer.size);

    LuaFileHeader* fileHeader = luaBuffer.data_void;

    fileHeader->identifier = BZLA_MAGIC;
    memset(fileHeader->sourceHash, 0xFF, 16); // TODO: replace with MD5 impl

    memcpy(fileHeader->bytecode, bytecodeBuffer.data_void, bytecodeBuffer.size);

    BufferDestroy(&bytecodeBuffer);

    return luaBuffer;
}
