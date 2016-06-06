/*
 * log.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
namespace SHS {
//static member initialize important!!!!!!!!!!!!!! without this, it tend to have linker error
Log Log::log;

Log::Log():
	firstLog(true),
	logFile("/etc/SHS/shs.log"){

	pthread_mutex_init(&this->mutex,NULL);
}

Log::~Log() {
}
void Log::error(const char* format, ...){
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile, "a");
	if (file == NULL) {
		printf("Can not open the logFile\n");
		return;
	}
	int error = sysinfo(&info);
	if(error){
		uptime = -1;
	}else {
		uptime= info.uptime;
	}
	fprintf(file,"[%ld] ERROR:",uptime);
	vfprintf(file,format,args);
	fclose(file);
	pthread_mutex_unlock(&mutex);

	va_end(args);
	exit(-1);
}
void Log::vrecord(const char* format,__gnuc_va_list args){
	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile, "a");
	if (file == NULL) {
		printf("Can not open the logFile\n");
		return;
	}
	int error = sysinfo(&info);
	if(error){
		uptime = -1;
	}else {
		uptime= info.uptime;
	}
	fprintf(file,"[%ld]:",uptime);
	vfprintf(file,format,args);
	fprintf(file,"\n");
	fclose(file);
	pthread_mutex_unlock(&mutex);
}

void Log::record(const char* format,...){

	va_list args;
	va_start(args, format);

	this->vrecord(format,args);

	va_end(args);
}
void Log::debug(const char* format, ...){
	return;
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile, "a");
	if (file == NULL) {
		printf("Can not open the logFile\n");
		return;
	}
	int error = sysinfo(&info);
	if(error){
		uptime = -1;
	}else {
		uptime= info.uptime;
	}
	fprintf(file,"[%ld]:",uptime);
	vfprintf(file,format,args);
	fclose(file);
	pthread_mutex_unlock(&mutex);


	va_end(args);
}
void Log::warning(const char* format, ...){
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile, "a");
	if (file == NULL) {
		printf("Can not open the logFile\n");
		return;
	}
	int error = sysinfo(&info);
	if(error){
		uptime = -1;
	}else {
		uptime= info.uptime;
	}
	fprintf(file,"[%ld] WARNING:",uptime);
	vfprintf(file,format,args);
	fclose(file);
	pthread_mutex_unlock(&mutex);


	va_end(args);
}



} /* namespace SHS */
