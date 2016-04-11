/*
 * ATAnalyser.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#ifndef SHSCPP_ATANALYSER_H_
#define SHSCPP_ATANALYSER_H_

#include "myMQ.h"
#include "container.h"
namespace SHS {

class ATAnalyser {
public:
	ATAnalyser(Container& container);
	virtual ~ATAnalyser();
	void startAnalyse(MyMQ<std::string> * pMQ);
	void startAnalyseAsThread(MyMQ<std::string> * pMQ);
private:
	Container * pContainer;
};

} /* namespace SHS */

#endif /* SHSCPP_ATANALYSER_H_ */
