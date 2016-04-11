/*
 * ECmd.cpp
 *
 *  Created on: 24 Mar 2016
 *      Author: Xiao WANG
 */

#include "ECmd.h"
using std::string;
namespace SHS {



void ZB_onoff::onRabbitMQReceive(){
	Log::log.debug("ZB_onoff::onRabbitMQReceive rabbitMQ message received received\n");
	this->setTTL(2000);//give command 2 seconds time to live, if no response in 2 seconds, it will time out
	string atCmd("AT+RONOFF:56CB,1,0,");
	this->sendATCmd(atCmd);

}
void ZB_onoff::onTimeOut(){
	Log::log.debug("ZB_onoff::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
}
void ZB_onoff::onATReceive(){
	Log::log.debug("ZB_onoff::onATReceive AT command received [%s]\n",this->ATMsg.c_str());
	//DFTREP:16DC,01,0006,02,00
	boost::regex expr("DFTREP:(\\w{4}),(\\w{2}),0006,(\\w{2}),00");
	//boost::regex expr("DFTREP:(\\w4),");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr))
	{
		std::cout<<"ZB_onoff:find NWK:"<<what[0]<<std::endl;
		string defAppendixKey("ZB.onoff");
		this->sendRMsg(defAppendixKey);
		cmdFinish();//alow next command
	}

}



} /* namespace SHS */
