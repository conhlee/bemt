#ifndef LUAC_INTERFACE_H
#define LUAC_INTERFACE_H

#include "../cons/buffer.h"

ConsBuffer LuacCompile(const char* luaSource);

#endif // LUAC_INTERFACE_H
