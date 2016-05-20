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
Cmd::StatusCode Cmd::statusCode;
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

//this function was limited use in AT command parse
int Cmd::parseHex(std::string & hex_str){
	int result =0;
	for(unsigned int i =0;i<hex_str.length();i++){
		result<<=4;
		char ch = hex_str.data()[i];
		if(ch <='F' && ch>='A'){
			result|=(ch-'A' +10);
		}else if(ch<='9' && ch>='0'){
			result|=(ch-'0');
		}else return -1;
	}
	return result;
}int Cmd::parseHex(const char* hex_str){
	std::string str(hex_str);
	return this->parseHex(str);
}

std::string Cmd::intToHexString(int number){
	std::stringstream stream;
	stream << std::hex << number;
	return stream.str();
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
	this->sendRMsg(str);
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

void Cmd::sendToRabbitMQAnalyser(Json::Value & Msg){
	if(this->container->pRabbitMQAnalyserMQ){
		this->container->pRabbitMQAnalyserMQ->sendMSG(Msg);
	}else{
		Log::log.error("sendToRabbitMQAnalyser: the necessary MyMQ invalid, did you forget container.setRabbitMQAnalyserMQ()");
	}
}
void Cmd::sendToRabbitMQAnalyser(std::string & cmdType){
	Json::Value Msg;
	Msg["type"] = cmdType+".req";
	this->sendToRabbitMQAnalyser(Msg);
}
void Cmd::sendToRabbitMQAnalyser(const char* cmdType){
	std::string str(cmdType);
	this->sendToRabbitMQAnalyser(str);
}
}

namespace SHS{

void Delay_init_cmdobj::onTimeOut(){
	/*Cmd * cmdObj;

	cmdObj=this->container->getCmdObj("ZB_init_discover");
	cmdObj->setTTL(2500); //time out in 2.5 seconds
	cmdObj->sendATCmd("AT+DISCOVER:");
	this->container->regActCmd(cmdObj);

	*/
	/*
	 * before sending next ATcommand, we may need sleep some time (1ms or 10ms or more according to the command type)
	 * to make sure that previous command has enough time to be fetched or copied by ZigBee dongle (because the UART buffer is really limited at only 128 Bytes).
	 * by optimising the AT command on ZigBee dongle, we may not need to do this.
	 */
	//usleep(20000);

	//start-up discover help to built NAT talbe
	this->sendToRabbitMQAnalyser("ZB.init.discover");

	//deal with update message!!
	//Cmd* cmdObj;
	//cmdObj=this->container->getCmdObj("ZB_update");
	//cmdObj->setTTL(Cmd::CMD_FOREVER_TTL_MARK);
	//this->container->regActCmd(cmdObj);
	Json::Value msg;
	msg["type"]="ZB.update.req";
	msg["__attribute_uart_exclusive"]=false;
	this->sendToRabbitMQAnalyser(msg);

	this->cmdFinish();

}


}/* namespace SHS */


