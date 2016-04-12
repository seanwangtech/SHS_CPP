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


void ZB_initdiscover::onRabbitMQReceive(){
	this->sendATCmd("AT+DISCOVER:");
}
void ZB_initdiscover::onATReceive(){

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
void ZB_initdiscover::onTimeOut(){
	this->setTTL(2500);
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	cmdFinish();//indicate command finished
}


} /* namespace SHS */
