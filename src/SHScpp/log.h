/*
 * log.h
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_LOG_H_
#define SHSCPP_LOG_H_

#include <stdio.h>
#include <pthread.h>
#include "conf.h"
namespace SHS {

class Log {
public:
	Log();
	virtual ~Log();
	void error(const char* format,...);
	void warning(const char* format,...);
	void debug(const char* format,...);
	void vrecord(const char* format,__gnuc_va_list args);
	void record(const char* format,...);
	void setConf(Conf &conf);
	void setLogFile(const char* logfile);
	void setDebugLevel(int debugLevel);
	static unsigned long get_file_size(const char *path);
	static Log log;
private:
	void checkLogfile();
	bool firstLog;
	std::string logFile;
	int debugLevel;
	pthread_mutex_t mutex;
};

} /* namespace SHS */

#endif /* SHSCPP_LOG_H_ */
