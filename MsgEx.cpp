#include "MsgEx.h"
#include <string.h>


const char LuaMsgEx::className[] = "LuaMsgEx";
Lunar<LuaMsgEx>::RegType LuaMsgEx::methods[] = {    
	LUNAR_DECLARE_METHOD(LuaMsgEx, ReadMsg),
	LUNAR_DECLARE_METHOD(LuaMsgEx, WriteMsg),
	LUNAR_DECLARE_METHOD(LuaMsgEx,Print),
	{0, 0} 
};


int LuaMsgEx::ReadMsg(lua_State *pL) {
	Msg* pMsg = m_pMsgEx->GetMsg();
	assert(pMsg != NULL);

	int nTopOld = lua_gettop(pL);

	//stack top is lua msg table
	lua_pushnumber(pL, pMsg->m_msgId);
	lua_setfield(pL, -2, "id");

	int len = strnlen_s(pMsg->m_buf, MAX_BUF_LEN);
	
	lua_pushlstring(pL, pMsg->m_buf, len);
	lua_setfield(pL, -2, "buf");

	assert(lua_gettop(pL) == nTopOld);

	lua_pushboolean(pL, true);
	return 1;
}


int LuaMsgEx::WriteMsg(lua_State* pL) {
	Msg* pMsg = m_pMsgEx->GetMsg();
	assert(pMsg != NULL);

	int nTopOld = lua_gettop(pL);
	//stack top is lua msg table
	lua_getfield(pL, -1, "id");
	int id = luaL_checkinteger(pL, -1);
	lua_pop(pL, 1);
	pMsg->m_msgId = id;


	lua_getfield(pL, -1, "buf");
	const char *str = luaL_checkstring(pL, -1);
	strcpy_s(pMsg->m_buf, MAX_BUF_LEN, str);
	lua_pop(pL, 1);

	assert(lua_gettop(pL) == nTopOld);

	lua_pushboolean(pL, true);
	return 1;
}

int LuaMsgEx::Print(lua_State* pL) {
	Msg* pMsg = m_pMsgEx->GetMsg();
	assert(pMsg != NULL);

	pMsg->ToString();

	lua_pushboolean(pL, true);
	return 1;
}

void NotRegisted(){
	//do nothing
}