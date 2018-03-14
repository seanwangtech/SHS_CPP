/*
 * luaWrapper.cpp
 *
 *  Created on: 31 Aug. 2017
 *      Author: Xiao Wang
 */
#include "luaWrapper.h"

namespace SHS {

namespace LW{
static const char* uciWrapper = "/usr/SHS/bin/uciWrapper.lua";

static lua_State* getL(){
	//open a lua_State
	lua_State* L;
	L = luaL_newstate();
	if(L==NULL){
		return NULL;
	}
	/* load Lua base libraries */
	luaL_openlibs(L);
	/* load the script */
	if(luaL_dofile(L, uciWrapper)) {
		lua_close(L);
		return NULL;
	}
	return L;
}

std::string get_all(const char* config){
	//open a lua_State
	lua_State* L;
	L=getL();
	if(L==NULL){
		return "";
	}

	//perform function
	lua_getglobal(L, "get_all");
	lua_pushstring(L, config);
	lua_call(L, 1, 1);
	//get value
	std::string str((char *)lua_tostring(L, -1));

	//clean lua_State
	lua_pop(L, 1);
	lua_close(L);
	return str;
}

int set(const char* parameters){
	//open a lua_State
	lua_State* L;
	L=getL();
	if(L==NULL){
		return -1;
	}

	//perform function
	lua_getglobal(L, "set");
	lua_pushstring(L, parameters);
	lua_call(L, 1, 1);
	//get value
	int result = lua_toboolean(L, -1);

	//clean lua_State
	lua_pop(L, 1);
	lua_close(L);
	return result;
}

int Delete(const char* parameters){
	//open a lua_State
	lua_State* L;
	L=getL();
	if(L==NULL){
		return -1;
	}

	//perform function
	lua_getglobal(L, "delete");
	lua_pushstring(L, parameters);
	lua_call(L, 1, 1);
	//get value
	int result = lua_toboolean(L, -1);

	//clean lua_State
	lua_pop(L, 1);
	lua_close(L);
	return result;
}


}/* namespace luaWappper LW*/
}/* namespace SHS */
