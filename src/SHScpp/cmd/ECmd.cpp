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
	int NWK_addr = this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
	if (NWK_addr==-1) {
		//can not find such a device in current lookup table
		this->rabbitMQMesg["type"]="ZB.onoff.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.onoff."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}
	string atCmd("AT+RONOFF:"
			+this->intToHexString(NWK_addr)
			+","
			+this->intToHexString(this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt()))
			+",0,"
			+ (data>1 ? "" :this->intToHexString(data)));
	this->sendATCmd(atCmd);

}
void ZB_onoff::onTimeOut(){
	Log::log.debug("ZB_onoff::onTimeOut command timeout\n");
	this->rabbitMQMesg["type"]="ZB.onoff.resp";
	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	string defAppendixKey("ZB.onoff."+this->rabbitMQMesg["ZB_MAC"].asString());
	this->sendRMsg(defAppendixKey);
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}
void ZB_onoff::onATReceive(){
	Log::log.debug("ZB_onoff::onATReceive AT command received [%s]\n",this->ATMsg.c_str());
	//DFTREP:16DC,01,0006,02,00
	//UPDATE:00124B00072880FC,E6FE,05,0006,0000,20,00
	boost::regex expr("UPDATE:(\\w{16}),(\\w{4}),(\\w{2}),0006,0000,20,(\\w{2})");
	//boost::regex expr("DFTREP:(\\w4),");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr))
	{
		if(this->rabbitMQMesg["ZB_MAC"].asString().compare(what[1].str())==0 &&
		this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt())== this->parseHex(what[3].str().c_str())){

			//std::cout<<"ZB_onoff:find NWK:"<<what[0]<<std::endl;
			this->rabbitMQMesg["type"]="ZB.onoff.resp";
			this->rabbitMQMesg["data"]=this->parseHex(what[4].str().c_str());
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			string defAppendixKey("ZB.onoff."+what[4].str());
			this->sendRMsg(defAppendixKey);
			cmdFinish();//alow next command
			return;
		}
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
	    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),this->parseHex(what[2].str().c_str()));
	}
}
void ZB_init_discover::onTimeOut(){
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	//Log::log.debug("ZB_startup_discover::%s\n",this->container->lookup.getNWK_MAC(0x4455).c_str());
	cmdFinish();//indicate command finished
}

void ZB_update::onATReceive(){
	//UPDATE:00124B0007288312,4455,04,0006,0000,20,01
	boost::regex expr("UPDATE:(\\w{16}),(\\w{4}),(\\w{2}),(\\w{4}),(\\w{4}),\\w{2},(\\w{2,4})");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
		    boost::smatch const &what = *iter;
		    //update lookup tables
		    Log::log.debug("ZB_update::onATReceive: %s\n",what[0].str().c_str());
		    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),this->parseHex(what[2].str().c_str()));
		    int EP = this->parseHex(what[3].str().c_str());
		    if(this->container->lookup.checkIsMainValue(
		    		EP,	this->parseHex(what[4].str().c_str()),
					this->parseHex(what[5].str().c_str()))){
		    	this->container->lookup.updateMACDevT_value(what[1].str().c_str(),
											this->container->lookup.getEP_DevT(EP),
											this->parseHex(what[6].str().c_str()));

		    	//send update to rabbitMQ
			    Json::Value root;
			    root["type"]="ZB.update";
			    root["id"] = ++this->update_id;
			    root["AP"] = (unsigned int) this->container->pConf->home.id;
			    root["ZB_MAC"]=what[1].str();
			    root["ZB_type"] = this->container->lookup.getEP_DevT(EP);
			    root["data"]=this->parseHex(what[6].str().c_str());
			    this->rabbitMQMesg = root;
			    this->sendRMsg(("ZB.update."+what[1].str()).c_str());
		    }
	}
}

void ZB_read::onRabbitMQReceive(){
	int value;
	int NWK;
	int EP;
	if(this->container->lookup.getMACDevT_value(this->rabbitMQMesg["ZB_MAC"].asCString(),
			this->rabbitMQMesg["ZB_type"].asInt(),&value)){
		//find the value in the the value table
		this->rabbitMQMesg["data"]=value;
		this->rabbitMQMesg["status"]=this->statusCode.succeed;
		this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
		this->cmdFinish();
	}else if((NWK=this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString())) >0 &&
			(EP=this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt()))>0){
		//find the dev in the addr tranlation table,
		//meaning there is such a dev in the ZigBee network but not in value table
		//so read value from ZigBee network
		/*
		 * 		at+readatr:4455,4,0,402,0
		 *		READATR:<Address>,<EP>,[<SendMode>],<ClusterID>,<AttrID>,...,< AttrID >
		 *		OK
		 *
		 *		RESPATR:4455,04,0402,0000,00,29,0727
		 */
		this->setTTL(1500);
		this->sendATCmd(("AT+READATR:"+
				this->intToHexString(NWK)+
				","+
				this->intToHexString(EP)+
				",0,"+
				this->intToHexString(this->container->lookup.getMainClusterID(EP))+
				","+
				this->intToHexString(this->container->lookup.getMainAttrID(EP))).c_str());
	}else{
		this->rabbitMQMesg["data"]= -1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
		this->cmdFinish();
	}
}

void ZB_read::onTimeOut(){
	this->rabbitMQMesg["data"]= -1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
	this->cmdFinish();
}
void ZB_read::onATReceive(){
	//RESPATR:4455,04,0402,0000,00,29,0727
	boost::regex expr("RESPATR:(\\w{4}),(\\w{2}),(\\w{4}),(\\w{4}),00,\\w{2},(\\w{2,4})");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
		    boost::smatch const &what = *iter;
		    //check whether the value is main value of a EndPoint
		    if(!this->container->lookup.checkIsMainValue(
		    		this->parseHex(what[2].str().c_str()),
					this->parseHex(what[3].str().c_str()),
					this->parseHex(what[4].str().c_str()))) continue;
		    //update the value table
		    std::string ZB_MAC = this->container->lookup.getNWK_MAC(this->parseHex(what[1].str().c_str()));
		    this->container->lookup.updateMACDevT_value(ZB_MAC,
		    		this->container->lookup.getEP_DevT(this->parseHex(what[2].str().c_str())),
					this->parseHex(what[5].str().c_str()));

		    //send to rabbitMQ
		    this->rabbitMQMesg["data"]=this->parseHex(what[5].str().c_str());
		    this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
			this->cmdFinish();
	}
}
void ZB_pjoin::onRabbitMQReceive(){
	this->setTTL(800);
	this->sendATCmd(("AT+PJOIN:"+this->rabbitMQMesg["data"].asString()).c_str());
}
void ZB_pjoin::onTimeOut(){
	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["type"]="ZB.pjoin.resp";
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	this->sendRMsg("ZB.pjoin");
	this->cmdFinish();
}
void ZB_pjoin::onATReceive(){
	boost::regex expr("OK");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr)){
		this->rabbitMQMesg["type"]="ZB.pjoin.resp";
		this->rabbitMQMesg["status"]=this->statusCode.succeed;
		this->sendRMsg("ZB.pjoin");
		this->cmdFinish();
	}
}


} /* namespace SHS */
