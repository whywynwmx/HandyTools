#ifndef LuaConfig_H
#define LuaConfig_H

#include "lua.h"
#include "lauxlib.h"

class LuaConfig
{
public:
	void parse(lua_State* L);

private:
};

void parse_lua_table(lua_State* L);

void parse_lua_table_andwrite(lua_State* L, const char* outname, bool splitsubtable = true);

#endif
