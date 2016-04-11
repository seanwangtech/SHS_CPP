/*
 * cmd.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#include "cmd.h"
#include "../log.h"
namespace SHS {
Cmd::Cmd():_cmdTTL(0),pMutex(NULL),container(NULL) {

}

Cmd::~Cmd() {
}

void Cmd::waitCmdFinish(pthread_mutex_t* pMutex){
	if(pMutex){
		this->pMutex = pMutex;
		pthread_mutex_unlock(this->pMutex);//insure the mutex is unlocked
		pthread_mutex_lock(this->pMutex);
		pthread_mutex_lock(this->pMutex);
	}else{
		Log::log.error("Cmd: waitCmdFinish: invalid mutex\n");
	}
}
void Cmd::cmdFinish(){
	if(this->pMutex){
		pthread_mutex_unlock(this->pMutex);
	}else{
		Log::log.debug("Cmd: waitCmdFinish: no mutex unlocked\n");
	}
	this->_cmdTTL =this->CMD_FINISHED_TTL_MARK;//indicate the command is finished, not time out! if container monitor detected this, it should just remove it without calling on timeout!
}
void Cmd::setContainer(Container * container){
	this->container=container;
}
void Cmd::setTTL(int ms){
	this->_cmdTTL = ms;
}
void Cmd::_onRabbitMQReceive(Json::Value & Msg){
	this->rabbitMQMesg=Msg;
	this->onRabbitMQReceive();
}

void Cmd::_onTimeOut(){
	this->onTimeOut();
}

void Cmd::_onATReceive(std::string& Msg){
	this->ATMsg=Msg;
	this->onATReceive();
}
Json::Value & Cmd::getRabbitMQMsg(){
	return this->rabbitMQMesg;
}

std::string& Cmd::getATMsg(){
	return this->ATMsg;
}

void Cmd::sendRMsg(std::string& defaultKeyAppendix){
	this->rabbitMQMesg["defaultKeyAppendix"] =  defaultKeyAppendix;
	if(this->container->pRabbitMQSenderMQ){
		this->container->pRabbitMQSenderMQ->sendMSG(this->rabbitMQMesg);
	}else{
		Log::log.error("Cmd: sendRMsg: have no valid rabbitMQsenderMQ\n");
	}
}
void Cmd::sendATCmd(std::string& ATCmd){

	if(this->container->pSerialSenderMQ){
		this->container->pSerialSenderMQ->sendMSG(ATCmd);
	}else{
		Log::log.error("Cmd: sendATCmd: have no valid serialMQsenderMQ\n");
	}
}

} /* namespace SHS */
