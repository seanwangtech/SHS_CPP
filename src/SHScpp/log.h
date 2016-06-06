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
	static Log log;
private:
	bool firstLog;
	const char * const logFile;
	pthread_mutex_t mutex;
};

} /* namespace SHS */

#endif /* SHSCPP_LOG_H_ */
