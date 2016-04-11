/*
 * log.h
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_LOG_H_
#define SHSCPP_LOG_H_

namespace SHS {

class Log {
public:
	Log();
	virtual ~Log();
	void error(const char* format,...);
	void warning(const char* format,...);
	void debug(const char* format,...);
	static Log log;
};

} /* namespace SHS */

#endif /* SHSCPP_LOG_H_ */
