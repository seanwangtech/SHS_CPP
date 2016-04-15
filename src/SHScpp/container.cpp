/*
 * container.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: Xiao WANG
 */

#include "container.h"

namespace SHS {

Container::Container():
		actCmds(),
		pSerialSenderMQ(NULL),
		pRabbitMQSenderMQ(NULL),
		pRabbitMQAnalyserMQ(NULL),
		pConf(NULL){
	pthread_mutex_init(&this->containersMutex,NULL);
	this->delayInitCmdObj();
}

Container::~Container() {
}
Cmd* Container::getCmdObj(std::string & objName){
	Cmd* pCmd = (Cmd *) ClassFactoryInstance().GetObject(objName);
	if(pCmd){
		pCmd->setContainer(this);
		return pCmd;
	}else{
		return pCmd;
	}
}

Cmd* Container::getCmdObj(const char* objName){
	std::string objName_str(objName);
	return this->getCmdObj(objName_str);
}
void Container::regActCmd(Cmd* cmdObj){
	//remove all the cmdObj which equal cmdObj,if not exist, remove nothing.
	if(cmdObj){
		this->actCmds.remove(cmdObj);
		cmdObj->setTTL(1500); //default new registered cmmand time out time is 1.5 second
		this->actCmds.insert(this->actCmds.begin(),cmdObj);
	}else{
		Log::log.warning("Container:invalid command object when regActCmd()");
	}
}
void Container::delActCmd(Cmd* cmdObj){
	this->actCmds.remove(cmdObj);
	/*
	if(this->actCmds.empty()) return;
	std::list<Cmd*> it = this->actCmds.begin();
	for(;it!=this->actCmds.end();it++){
		if(*it==cmdObj) break;
	}
	if(it!=this->actCmds.end()) this->actCmds;
	*/
}

void Container::setSerialSenderMQ(MyMQ<std::string>* pSerialSenderMQ){
	this->pSerialSenderMQ = pSerialSenderMQ;
}
void Container::setRabbitMQSenderMQ(MyMQ<Json::Value> * pRabbitMQSenderMQ){
	this->pRabbitMQSenderMQ =pRabbitMQSenderMQ;
}
void Container::setRabbitMQAnalyserMQ(MyMQ<Json::Value> * pRabbitMQAnalyserMQ){
	this->pRabbitMQAnalyserMQ = pRabbitMQAnalyserMQ;
}
void Container::setConf(Conf* pConf){
	this->pConf = pConf;
}
void Container::lockContainer(){
	pthread_mutex_lock(&this->containersMutex);
}
void Container::unlockContainer(){
	pthread_mutex_unlock(&this->containersMutex);
}
void Container::delayInitCmdObj(){

	Cmd* cmdObj;
	this->lockContainer();

	cmdObj=this->getCmdObj("Delay_init_cmdobj");
	cmdObj->setTTL(200);//set time out 0.2 seconds
	this->regActCmd(cmdObj);

	this->unlockContainer();
}
} /* namespace SHS */
