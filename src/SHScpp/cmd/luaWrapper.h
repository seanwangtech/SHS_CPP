/*
 * luaWrapper.h
 *
 *  Created on: 31 Aug. 2017
 *      Author: Xiao Wang
 */

#include <string>
extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}




#ifndef SHSCPP_CMD_LUAWRAPPER_H_
#define SHSCPP_CMD_LUAWRAPPER_H_

namespace SHS {
namespace LW{

std::string get_all(const char* config);
int set(const char* parameters);
int Delete(const char* parameters);



}/* namespace luaWappper LW*/
}/* namespace SHS */



#endif /* SHSCPP_CMD_LUAWRAPPER_H_ */
