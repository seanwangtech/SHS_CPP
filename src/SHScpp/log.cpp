/*
 * log.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#include "log.h"

#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>
namespace SHS {
//static member initialize important!!!!!!!!!!!!!! without this, it tend to have linker error
Log Log::log;

Log::Log() {

}

Log::~Log() {
}
void Log::error(const char* format, ...){
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	va_end(args);
	exit(-1);
}
void Log::debug(const char* format, ...){
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	va_end(args);
}
void Log::warning(const char* format, ...){
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	va_end(args);
}



} /* namespace SHS */
