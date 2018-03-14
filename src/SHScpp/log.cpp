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
#include <sys/stat.h>
namespace SHS {
//static member initialize important!!!!!!!!!!!!!! without this, it tend to have linker error
Log Log::log;

Log::Log():
	firstLog(true),
	logFile("/etc/SHS/shs.log"),
	debugLevel(3){

	pthread_mutex_init(&this->mutex,NULL);
}

Log::~Log() {
}
void Log::error(const char* format, ...){
	if(this->debugLevel< 1) return;
	this->checkLogfile();
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile.c_str(), "a");
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
	FILE* file = fopen(this->logFile.c_str(), "a");
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
	if(this->debugLevel< 3) return;
	this->checkLogfile();
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile.c_str(), "a");
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
	if(this->debugLevel< 2) return;
	this->checkLogfile();
	va_list args;
	va_start(args, format);

	struct sysinfo info;
	long uptime;
	pthread_mutex_lock(&mutex);
	FILE* file = fopen(this->logFile.c_str(), "a");
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


void Log::setConf(Conf &conf){
	this->debugLevel = conf.debugLevel;
	this->logFile = conf.logfile;
}
void Log::setLogFile(const char* logfile){
	this->logFile = logfile;
}
void Log::setDebugLevel(int debugLevel){
	this->debugLevel = debugLevel;
}

unsigned long Log::get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}
void Log::checkLogfile(){
	//limit logfile size within 1MB = 1048576 = 1024*1024
	if(Log::get_file_size(this->logFile.c_str()) >= 1048576){
		//if the logfile too big, then delete it
		remove(this->logFile.c_str());
	}
}

} /* namespace SHS */
