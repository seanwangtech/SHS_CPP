/*
 * cmd.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#include "cmd.h"
#include "../log.h"
namespace SHS {
using std::string;
Cmd::Cmd():_cmdTTL(0),pMutex(NULL),container(NULL) {
	this->rabbitMQMesg["type"]="default";

}

Cmd::~Cmd() {
}

void Cmd::waitCmdFinish(pthread_mutex_t* pMutex){
	if(this->_cmdTTL > 0){
		//the condition is meet the condition to wait
		if(pMutex){
			this->pMutex = pMutex;
			pthread_mutex_unlock(this->pMutex);//insure the mutex is unlocked
			pthread_mutex_lock(this->pMutex);
			pthread_mutex_lock(this->pMutex);
		}else{
			Log::log.error("Cmd: waitCmdFinish: invalid mutex\n");
		}
	}else{
		//do not need to wait, cmdTTL<=0 indicates the command has finished or some special situation. Just show debug information the continue
		Log::log.debug("Cmd::waitCmdFinish: cmdTTL <=0, do not need to wait, the command may finished in onRabbitMQReceive() function\n");
	}
}
void Cmd::cmdFinish(){
	if(this->pMutex){
		pthread_mutex_unlock(this->pMutex);
	}else{
		Log::log.debug("Cmd: waitCmdFinish: no mutex unlocked\n");
	}
	this->_cmdTTL =this->CMD_FINISHED_TTL_MARK;//indicate the command is finished, the container monitor will remove the cmd object from the active command object list
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
void Cmd::sendRMsg(const char * defaultKeyAppendix){
	string str(defaultKeyAppendix);
	this->sendATCmd(str);
}
void Cmd::sendATCmd(std::string& ATCmd){

	if(this->container->pSerialSenderMQ){
		this->container->pSerialSenderMQ->sendMSG(ATCmd);
	}else{
		Log::log.error("Cmd: sendATCmd: have no valid serialMQsenderMQ\n");
	}
}
void Cmd::sendATCmd(const char* ATCmd){
	std::string atCmd(ATCmd);
	this->sendATCmd(atCmd);
}

}

namespace SHS{

void Delay_init_cmdobj::onTimeOut(){
	Cmd * cmdObj;

	cmdObj=this->container->getCmdObj("ZB_startup_discover");
	cmdObj->setTTL(2500); //time out in 2.5 seconds
	cmdObj->sendATCmd("AT+DISCOVER:");
	this->container->regActCmd(cmdObj);

	/*
	 * before sending next ATcommand, we may need sleep some time (1ms or 10ms or more according to the command type)
	 * to make sure that previous command has enough time to be fetched or copied by ZigBee dongle (because the UART buffer is really limited at only 128 Bytes).
	 * by optimising the AT command on ZigBee dongle, we may not need to do this.
	 */
	//usleep(20000);

	this->cmdFinish();

}

void ZB_startup_discover::onATReceive(){

	//DEV:56CB,01,ENABLED
	//Log::log.debug("ZB_startup_discover::oATReceive[%s]\n",this->ATMsg.c_str());
	boost::regex expr("DEV:(\\w{4}),(\\w{2}),ENABLED");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    std::cout<<(*iter)[1]<<'\n';
		std::cout<<"ZB_startup_discover:discover:"<<what[0]<<std::endl;
	}
}
void ZB_startup_discover::onTimeOut(){
	this->setTTL(2500);
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	cmdFinish();//indicate command finished
}


}/* namespace SHS */


