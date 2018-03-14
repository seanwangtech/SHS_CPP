/*
 * DBMonitor.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#include "ContainerMonitor.h"
#include <unistd.h>
namespace SHS {

ContainerMonitor::ContainerMonitor(Container& container) :pContainer(&container){

}

ContainerMonitor::~ContainerMonitor() {
}


void ContainerMonitor::startMonitor(){
	while(1){
		usleep(100000);
		this->manageActCmd();
	}
}
static void * containerMonitorThread(void * ptr){
	ContainerMonitor* monitor = (ContainerMonitor*) ptr;
	monitor->startMonitor();
	return NULL;

}
void ContainerMonitor::startMonitorAsThread(){
	pthread_t thread;
	pthread_create (&thread, NULL,containerMonitorThread, (void*) this);
	pthread_detach(thread);
}

void ContainerMonitor::manageActCmd(){
	if(this->pContainer){
		this->pContainer->lockContainer();
		std::list<Cmd*> &cmds = pContainer->actCmds;
		if(cmds.size()){
			std::list<Cmd*>::iterator it = cmds.begin();
			while(it!=cmds.end()){
				int & cmdTTL = (*it)->_cmdTTL;
				if(cmdTTL>0){
					cmdTTL -= 100;
					it++;
				}else if(cmdTTL==Cmd::CMD_FINISHED_TTL_MARK){
					//meaning the command is correct tackled
			        it = cmds.erase(it);
				}else if(cmdTTL==Cmd::CMD_FOREVER_TTL_MARK){
					//just keep the command, do not remove or do not change, as it should keep forever
					it++;
				}else{
					//just meaning time up. not really time out
					//Log::log.warning("ContainerMonitor: one command timeout:%s\n",(*it)->getRabbitMQMsg()["type"].asCString());
					(*it)->_onTimeOut();
					//remove the cmd object if its cmdTTL is marked as CMD_FINISHED_TTL_MARK
					if(cmdTTL==Cmd::CMD_FINISHED_TTL_MARK) {
						//the command is really time out, warn and remove it!
						//checkout if it need to warning
						if(!(*it)->depressTimeoutWarning) {
							//print warning
							Log::log.warning("ContainerMonitor: one command timeout:%s\n",(*it)->getRabbitMQMsg()["type"].asCString());
						}else{
							//set the depress TimeoutWarning to default: false
							(*it)->depressTimeoutWarning=false;
						}
						it = cmds.erase(it);
					}
				}
			}

		}
		this->pContainer->unlockContainer();
	}else{
		Log::log.warning("ContainerMonitor:invalid container!!");
	}
}
} /* namespace SHS */
