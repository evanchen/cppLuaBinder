#include "Lunar.h"
#include "MsgEx.h"

MsgEx* g_pMsgEx = NULL;

int g_nErrFuncIdx = 0;

int LuaErrorHandler(lua_State *L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

int main(int argc, char **argv) {

	lua_State *L = lua_open();
	luaL_openlibs(L);

	lua_pushcfunction(L, LuaErrorHandler);
	g_nErrFuncIdx = lua_gettop(L);
	
	g_pMsgEx = new MsgEx();
	lua_pushlightuserdata(L, g_pMsgEx);
	lua_setglobal(L, "_Msg");
	Lunar<LuaMsgEx>::Register(L);

	//load my.lua
	if (luaL_loadfile(L, "my.lua"))	{
		fprintf(stderr, "Loadfile error:%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return -1;
	}

	if (lua_pcall(L, 0, 0, g_nErrFuncIdx) != 0) {
		fprintf(stderr, " pcall error:%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return -1;
	}

	lua_getglobal(L, "Init");
	if (lua_pcall(L, 0, 1, g_nErrFuncIdx) != 0){
		fprintf(stderr, "Init error:%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return -1;
	}

	getchar();
	return 0;
}
