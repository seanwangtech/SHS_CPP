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
	//AT+RONOFF:<NWK>,<EP>,0,<ON/OFF>
	int data = this->rabbitMQMesg["data"].asInt();
	string atCmd("AT+RONOFF:"
			+this->intToHexString(this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString()))
			+","
			+this->intToHexString(this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt()))
			+",0,"
			+ (data>1 ? "" :this->intToHexString(data)));
	this->sendATCmd(atCmd);

}
void ZB_onoff::onTimeOut(){
	Log::log.debug("ZB_onoff::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
}
void ZB_onoff::onATReceive(){
	Log::log.debug("ZB_onoff::onATReceive AT command received [%s]\n",this->ATMsg.c_str());
	//DFTREP:16DC,01,0006,02,00
	//UPDATE:00124B00072880FC,E6FE,05,0006,0000,20,00
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


void ZB_init_discover::onRabbitMQReceive(){
	this->setTTL(2500);
	this->sendATCmd("AT+DISCOVER:");
}
void ZB_init_discover::onATReceive(){

	//DEV:00124B00072880FC,E6FE,05,ENABLED
	//Log::log.debug("ZB_startup_discover::oATReceive[%s]\n",this->ATMsg.c_str());
	boost::regex expr("DEV:(\\w{16}),(\\w{4}),(\\w{2}),ENABLED");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    std::cout<<(*iter)[1]<<'\n';
		std::cout<<"ZB_startup_discover:discover:"<<what[0]<<std::endl;
	    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),this->parseHex(what[2].str().c_str()));
	}
}
void ZB_init_discover::onTimeOut(){
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	//Log::log.debug("ZB_startup_discover::%s\n",this->container->lookup.getNWK_MAC(0x4455).c_str());
	cmdFinish();//indicate command finished
}


} /* namespace SHS */
