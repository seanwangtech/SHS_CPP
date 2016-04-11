/*
 * DBMonitor.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_CONTAINERMONITOR_H_
#define SHSCPP_CONTAINERMONITOR_H_

#include "container.h"
namespace SHS {

class ContainerMonitor {
public:
	ContainerMonitor(Container& container);
	virtual ~ContainerMonitor();
	void startMonitor();
	void startMonitorAsThread();
private:
	Container *pContainer;
	void manageActCmd();
};

} /* namespace SHS */

#endif /* SHSCPP_CONTAINERMONITOR_H_ */
