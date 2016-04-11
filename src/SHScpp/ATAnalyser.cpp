/*
 * ATAnalyser.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#include "ATAnalyser.h"
using std::string;
namespace SHS {

ATAnalyser::ATAnalyser(Container & container):pContainer(&container) {

}

ATAnalyser::~ATAnalyser() {
}
void ATAnalyser::startAnalyse(MyMQ<std::string> * pMQ){
	if(!pMQ) Log::log.error("ATAnalyser: invalid MyMQ obj with addr:%d\n",pMQ);
	if(!this->pContainer) Log::log.error("ATAnalyser: invalid container obj with addr:%d\n",this->pContainer);

	Log::log.debug("ATAnalyser: start addr:%d\n",this);
	while(1){
		string at = pMQ->getOneMSG();
		Log::log.debug("ATAnalyser:get A message: [%s]\n",at.c_str());
		this->pContainer->lockContainer();
		std::list<Cmd*> & cmds = this->pContainer->actCmds;
		if(cmds.size()){
			for(std::list<Cmd*>::iterator it=cmds.begin(); it != cmds.end();it++){
				Cmd& cmd = *(*it);
				cmd._onATReceive(at);
			}
		}else{
			Log::log.warning("ATAnalyser: no active command object!\n");
		}

		this->pContainer->unlockContainer();

	}
}
struct AT_analyser_temp_t {
	ATAnalyser* pAnalyser;
	MyMQ<std::string>* pMQ;
};

static void* ATAnalyserThread(void *ptr){
	AT_analyser_temp_t & para = *((AT_analyser_temp_t*)ptr);
	para.pAnalyser->startAnalyse(para.pMQ);
	return NULL;
}

void ATAnalyser::startAnalyseAsThread(MyMQ<std::string> * pMQ){
	pthread_t thread;
	static struct AT_analyser_temp_t para;//avoid the variable lost when function finished
	para. pAnalyser=this;
	para.pMQ=pMQ;
	pthread_create (&thread, NULL,ATAnalyserThread, (void*) &para);
	pthread_detach(thread);

}

} /* namespace SHS */
