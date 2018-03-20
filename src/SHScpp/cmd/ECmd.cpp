/*
 * ECmd.cpp
 *
 *  Created on: 24 Mar 2016
 *      Author: Xiao WANG
 */

#include "ECmd.h"
#include "luaWrapper.h"
#include <openssl/md5.h>
#include "base64.h"


using std::string;
namespace SHS {

void ZB_onoff::onRabbitMQReceive(){
	Log::log.debug("ZB_onoff::onRabbitMQReceive rabbitMQ message received received\n");
	this->setTTL(TYPICAL_DELAY);//give command TYPICAL_DELAY ms time to live, if no response in 0.5 seconds, it will time out
	this->retry = 0;
	//AT+RONOFF:<NWK>,<EP>,0,<ON/OFF>
	int data = this->rabbitMQMesg["data"].asInt();
	//perform Address translation according to the bindings
	std::string MAC = this->rabbitMQMesg["ZB_MAC"].asCString();
	EP = this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt());
	Bindings::Binding_pair* pair = this->container->bindings.getBinding(MAC,EP);
	if(pair !=NULL){
		EP = pair->EP1;
		MAC = pair->MAC1;
		IsNetworkTranslated = true;
	}else{
		IsNetworkTranslated = false;
	}

	NWK_addr = this->container->lookup.getMAC_NWK(MAC);
	if (NWK_addr==-1) {
		//can not find such a device in current lookup table
		Log::log.warning("ZB_onoff::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		this->rabbitMQMesg["type"]="ZB.onoff.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.onoff."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}
	std::string onoff_cmd(this->intToHexString(data));
	if(data==2){
		//rewrite toggle command, this will help to make the toggle command more stable in unstable ZigBee network
		int value;
		this->container->lookup.getMACDevT_value(this->rabbitMQMesg["ZB_MAC"].asCString(),this->rabbitMQMesg["ZB_type"].asInt(),&value);
		if(value<0){
			//no cached value, so use AT onoff toggle
			onoff_cmd="";
		}else if(value==0){
			//use AT onoff on command replaces toggle command
			onoff_cmd = "1";
		}else if(value==1){
			//use AT onoff off command replaces toggle command
			onoff_cmd = "0";
		}else{
			//the concept is mixed, this is curtain, so keep toggle to compatible with curtain
			onoff_cmd="";
		}
	}
	string atCmd("AT+RONOFF:"
			+this->intToHexString(NWK_addr)
			+","
			+this->intToHexString(EP)
			+",0,"
			+ onoff_cmd);
	this->sendATCmd(atCmd);

}
void ZB_onoff::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		this->ResendATCmd(1);
		return;
	}
	Log::log.debug("ZB_onoff::onTimeOut command timeout\n");
	//delete exist cached values
	this->container->lookup.delMACDevT_value(this->rabbitMQMesg["ZB_MAC"].asCString(),
					this->rabbitMQMesg["ZB_type"].asInt());

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
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	if(IsNetworkTranslated){
		//BINDCMD:scrAddr,EP,CMD,Value
		boost::regex expr("BINDCMD:(\\w{4}),(\\w{2}),01,(\\w{2})");
		boost::smatch what;
		if (boost::regex_search(this->ATMsg, what, expr))
		{
			if(NWK_addr== this->parseHex(what[1].str().c_str()) && EP== this->parseHex(what[2].str().c_str())){
				int value = this->parseHex(what[3].str().c_str());
				if(value != 12 && value!= 13) return;
				else value -=12;
				this->rabbitMQMesg["type"]="ZB.onoff.resp";
				this->rabbitMQMesg["data"]=value;
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
				string defAppendixKey("ZB.onoff."+this->rabbitMQMesg["ZB_MAC"].asString());
				this->sendRMsg(defAppendixKey);
				cmdFinish();//alow next command
				return;
			}
		}
	}else{
		//DFTREP:16DC,01,0006,02,00
		//UPDATE:00124B00072880FC,E6FE,05,0006,0000,20,00
		boost::regex expr("UPDATE:(\\w{16}),(\\w{4}),(\\w{2}),0006,0000,\\w{2},(\\w{2})");
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
}


void ZB_init_discover::onRabbitMQReceive(){
	this->setTTL(1500);
	this->isResponded=false;
	this->retry =0;
	this->sendATCmd("AT+DISCOVER:");
}
void ZB_init_discover::onATReceive(){

	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	//DEV:00124B00072880FC,E6FE,05,ENABLED
	//Log::log.debug("ZB_startup_discover::oATReceive[%s]\n",this->ATMsg.c_str());
	boost::regex expr("DEV:(\\w{16}),(\\w{4}),(\\w{2}),ENABLED");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),this->parseHex(what[2].str().c_str()));
	}
	//the serial port is responded, so set the member field to true
	this->isResponded=true;
}
void ZB_init_discover::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(1500); //allow 15000 ms to retry.
		this->ResendATCmd(1);
		return;
	}
	//depress warning
	this->depressTimeoutWarning=true;
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	if(!this->isResponded) Log::log.error("ZB_startup_discover::Serial port No response \n");
	//Log::log.debug("ZB_startup_discover::%s\n",this->container->lookup.getNWK_MAC(0x4455).c_str());
	cmdFinish();//indicate command finished
}
void ZB_init_readsn::onRabbitMQReceive(){
	this->setTTL(1500);
	this->isResponded=false;
	this->retry =0;
	this->sendATCmd("AT+READSN:");
}

void ZB_init_readsn::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	boost::regex expr("SN:(\\w{16})");
	boost::smatch what;
	if(boost::regex_search(this->ATMsg,what,expr))
	{
		pthread_mutex_t sn_mutex;
		Json::Reader reader;
		Json::Value root;
		Json::StyledWriter sw;
		std::ifstream in("/etc/SHS/SHS.conf.json");
		if(in.is_open())
		{

			if(reader.parse(in,root))
			{
				root["home"]["id"]=what[1].str().c_str();
				root["rabbitmq"]["hostname"]="smart.seyas.cn";
				root["rabbitmq"]["port"]=8690;
				root["rabbitmq"]["vhost"]="baiya";
				root["rabbitmq"]["user"]="by";
				root["rabbitmq"]["password"]="by123";
				root["rabbitmq"]["exchange"]="BYGW";
				std::ofstream os;
				os.open("/etc/SHS/SHS.conf.json");
				os<<sw.write(root);
				os.close();
			}
			in.close();
		}
		/*Log::log.debug("home.id is change into SN" );*/
		pthread_mutex_init(&sn_mutex,NULL);
		pthread_mutex_lock(&sn_mutex);
		this->container->pConf->home.id=what[1].str().c_str();
		Log::log.debug("home.id is change into SN:%s\n",this->container->pConf->home.id.c_str());
		pthread_mutex_unlock(&sn_mutex);
	}
	//the serial port is responded, so set the member field to true
	this->isResponded=true;
}

void ZB_init_readsn::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(1500); //allow 15000 ms to retry.
		this->ResendATCmd(1);
		return;
	}
	//depress warning
	this->depressTimeoutWarning=true;
	Log::log.debug("ZB_init_readSN::onTimeOut\n");
	if(!this->isResponded) Log::log.error("ZB_init_readSN::Serial port No response \n");
	//Log::log.debug("ZB_startup_discover::%s\n",this->container->lookup.getNWK_MAC(0x4455).c_str());
	cmdFinish();//indicate command finished
}

unsigned int ZB_update::update_id =0 ;
unsigned int inline ZB_update::getUpdateId(){
	return ++ZB_update::update_id;
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
		    int NWK = this->parseHex(what[2].str().c_str());
		    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),NWK);
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
			    root["id"] = getUpdateId();
			    root["AP"] = this->container->pConf->home.id;
			    root["ZB_MAC"]=what[1].str();
			    root["ZB_type"] = this->container->lookup.getEP_DevT(EP);
			    root["data"]=this->parseHex(what[6].str().c_str());
			    this->rabbitMQMesg = root;
			    this->sendRMsg(("ZB.update."+what[1].str()).c_str());
		    }
		    //renew the value of the active map in ZB_update_deactive
		    ZB_update_deactive::renewDev(NWK);
	}

	//DEVRPT:00124B0007287FF2,8C18,6F
	expr = boost::regex("DEVRPT:(\\w{16}),(\\w{4})([,\\dABCDEF]+)");
	iter = boost::sregex_iterator(this->ATMsg.begin(), this->ATMsg.end(), expr);
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    //update lookup tables
	    Log::log.debug("ZB_DEVRPT::onATReceive: %s\n",what[0].str().c_str());
	    int NWK = this->parseHex(what[2].str().c_str());
	    this->container->lookup.updateMAC_NWK(what[1].str().c_str(),NWK);
	    std::string eps(what[3].str());
		for(unsigned int i=0;i< eps.length();i+=3){
			int EP = this->parseHex(eps.substr(i+1,2).c_str());
			Json::Value root;
			root["type"]="ZB.update.DevUp";
			root["id"] = getUpdateId();
			root["AP"] = this->container->pConf->home.id;
			root["ZB_MAC"]=what[1].str();
			root["ZB_type"] = this->container->lookup.getEP_DevT(EP);
			this->rabbitMQMesg = root;
			this->sendRMsg(("ZB.update.DevUp."+what[1].str()).c_str());

			//renew the value of the active map in ZB_update_deactive
			ZB_update_deactive::renewDev(NWK);
	    }
	}



	//update AT command and id field is not applicable
	Json::Value ATResp;
	ATResp["type"]="ZB.AT.resp";
	ATResp["data"]=this->ATMsg;
	ATResp["AP"]= this->container->pConf->home.id;
	ATResp["status"]=this->statusCode.succeed;
	this->rabbitMQMesg = ATResp;
	this->sendRMsg("ZB.AT");
}
void ZB_update::onRabbitMQReceive(){
	this->setTTL(Cmd::CMD_FOREVER_TTL_MARK);
}

void ZB_read::onRabbitMQReceive(){
	int value;
	int NWK;
	int EP;
	this->retry =0;
	this->rabbitMQMesg["type"]="ZB.read.resp";
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
		this->setTTL(TYPICAL_DELAY);
		this->sendATCmd(("AT+READATR:"+
				this->intToHexString(NWK)+
				","+
				this->intToHexString(EP)+
				",0,"+
				this->intToHexString(this->container->lookup.getMainClusterID(EP))+
				","+
				this->intToHexString(this->container->lookup.getMainAttrID(EP))).c_str());
	}else{
		Log::log.warning("ZB_read::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		this->rabbitMQMesg["data"]= -1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
		this->cmdFinish();
	}
}

void ZB_read::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		this->ResendATCmd(1);
		return;
	}
	this->rabbitMQMesg["data"]= -1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	this->sendRMsg(("ZB.read."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
	this->cmdFinish();
}
void ZB_read::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
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
void ZB_set::onRabbitMQReceive(){
	int NWK;
	int EP;
	this->retry=0;
	if((NWK=this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString())) >0 &&
			(EP=this->container->lookup.getDevT_EP(this->rabbitMQMesg["ZB_type"].asInt()))>0){
		//find the dev in the addr tranlation table,
		//meaning there is such a dev in the ZigBee network but not in value table
		//so read value from ZigBee network
		/*
		 AT+WRITEATR:39c5,15,0,6,0,20,1
		 WRITEATR:<Address>,<EP>,[<SendMode>],<ClusterID>,<AttrID>,<DataType>,<Data>
		 OK
		 WRITEATR:39C5,15,0006,0000,88,
		 *
		 */
		this->setTTL(TYPICAL_DELAY);
		this->value = this->rabbitMQMesg["data"].asInt();
		this->sendATCmd(("AT+WRITEATR:"+
				this->intToHexString(NWK)+
				","+
				this->intToHexString(EP)+
				",0,"+
				this->intToHexString(this->container->lookup.getMainClusterID(EP))+
				","+
				this->intToHexString(this->container->lookup.getMainAttrID(EP))+
				",20,"+
				this->intToHexString(this->value)).c_str());
	}else{
		Log::log.warning("ZB_set::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		this->rabbitMQMesg["data"]= -1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		this->sendRMsg(("ZB.set."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
		this->cmdFinish();
	}
}
void ZB_set::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	//WRITEATR:39C5,15,0006,0000,88,
	boost::regex expr("WRITEATR:(\\w{4}),(\\w{2}),(\\w{4}),(\\w{4}),\\w{2},(\\w{0,4})");
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
					value);
			    //send to rabbitMQ
		    this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(("ZB.set."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
			this->cmdFinish();
	}
}
void ZB_set::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		this->ResendATCmd(1);
		return;
	}
	this->rabbitMQMesg["data"]= -1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	this->sendRMsg(("ZB.set."+this->rabbitMQMesg["ZB_MAC"].asString()).c_str());
	this->cmdFinish();
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
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	boost::regex expr("OK");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr)){
		this->rabbitMQMesg["type"]="ZB.pjoin.resp";
		this->rabbitMQMesg["status"]=this->statusCode.succeed;
		this->sendRMsg("ZB.pjoin");
		this->cmdFinish();
	}
}

void ZB_discover::onRabbitMQReceive(){
	this->setTTL(2500);
	this->depressTimeoutWarning=true;
	this->data.clear();
	this->sendATCmd("AT+DISCOVER:");
}
void ZB_discover::onTimeOut(){
	this->rabbitMQMesg["type"]="ZB.discover.resp";
	this->rabbitMQMesg["data"]=data;
	this->rabbitMQMesg["AP"]= this->container->pConf->home.id;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg("ZB.discover");
	this->cmdFinish();
}
void ZB_discover::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	//DEV:00124B00072880FC,E6FE,05,ENABLED
	//Log::log.debug("ZB_startup_discover::oATReceive[%s]\n",this->ATMsg.c_str());
	boost::regex expr("DEV:(\\w{16}),(\\w{4}),(\\w{2}),ENABLED");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    std::string ZB_MAC =what[1].str();
	    int EP = this->parseHex(what[3].str().c_str());
	    if (EP==0) continue; //EP==0 indicate the device is dongle, so ignore it
	    int DevT = this->container->lookup.getEP_DevT(EP);
	    this->container->lookup.updateMAC_NWK(ZB_MAC.c_str(),this->parseHex(what[2].str().c_str()));
	    Json::Value arr;
	    int value;
	    arr.append(what[1].str());
	    arr.append(DevT);
	    if(this->container->lookup.getMACDevT_value(ZB_MAC,DevT,&value)){
	    	arr.append(value);
	    }else{
	    	arr.append(-1);
	    }

	    this->data.append(arr);
	}
}

void ZB_AT::onTimeOut(){
	//do not need to depress timeout warning any more, so comment this.
	//this->depressTimeoutWarning=true;

	this->cmdFinish();
}
void ZB_AT::onATReceive(){
	//allow other messages once receive any message from serial port

	//once mach the ERROR or OK, finiish the cmd and allow next command come in
	boost::regex expr("ERROR:|OK");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr)){
		this->cmdFinish();
	}
}
void ZB_AT::onRabbitMQReceive(){
	this->setTTL(2100); //avoid send too many data to serial port to avoid the serial port died
	bool isOK = false;
	if(this->rabbitMQMesg["data"].isString()){
		std::string data(this->rabbitMQMesg["data"].asCString());
		if((	data.c_str()[0]=='a' ||data.c_str()[0]=='A')
			&& (data.c_str()[1]=='t' ||data.c_str()[1]=='T')){
			isOK = true;
			this->sendATCmd(data);
		}
	}

	if(!isOK){
		this->rabbitMQMesg["type"]="ZB.AT.resp";
		this->rabbitMQMesg["data"]="error";
		this->rabbitMQMesg["AP"]= this->container->pConf->home.id;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_AT_format_error;
		this->sendRMsg("ZB.AT");
		this->cmdFinish();
	}
}

void ZB_CSLock::onRabbitMQReceive(){
	this->setTTL(800);//give command 0.8 seconds time to live, if no response in 0.8 seconds, it will time out
	//AT+CSLOCK:<NWK>,<cmd>
	int data = this->rabbitMQMesg["data"].asInt();
	std::string MAC = this->rabbitMQMesg["ZB_MAC"].asString();
	int NWK_addr = this->container->lookup.getMAC_NWK(MAC);
	if (NWK_addr==-1) {
		Log::log.warning("ZB_CSLock::onRabbitMQReceive: no such device [%s]\n",MAC.c_str());
		//can not find such a device in current lookup table
		this->rabbitMQMesg["type"]="ZB.CSLock.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.CSLock."+MAC);
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}
	string atCmd("AT+CSLOCK:"
			+this->intToHexString(NWK_addr)
			+","
			+ this->intToHexString(data));
	this->sendATCmd(atCmd);

	//it is OK to send two AT commands together
	//But it may cause problem when send 3 commands together due to the AT command has only two stage buffer.
	//I have set minimum time gap to separate AT commands in serialPort class, thus it is OK to send two AT commands one by one immediately.
	std::string * pMAC1;
	if((pMAC1 = this->container->bindings.getBindedMAC(MAC) )!= NULL &&
			(NWK_addr = this->container->lookup.getMAC_NWK(*pMAC1)) != -1){
		this->isBinded=true;
		atCmd = "AT+CSLOCK:"
				+this->intToHexString(NWK_addr)
				+","
				+ this->intToHexString(data);
		this->sendATCmd(atCmd);
	}else{
		this->isBinded=false;
	}
}
void ZB_CSLock::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command
		if(this->isBinded){
			//re-send command with index 2 (meaning second previous command)
			this->ResendATCmd(2);
			//re-send command with index 1 (meaning first previous command)
			this->ResendATCmd(1);
		}else{
			//re-send command with index 1 (meaning first previous command)
			this->ResendATCmd(1);
		}
		return;
	}
	//CSLOCK:1375,01
	boost::regex expr("CSLOCK:(\\w{4}),(\\w{2})");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr))
	{
		int NWK = this->parseHex(what[1].str().c_str());
		if(this->rabbitMQMesg["ZB_MAC"].asString().compare(this->container->lookup.getNWK_MAC(NWK))==0){

			this->rabbitMQMesg["type"]="ZB.CSLock.resp";
			this->rabbitMQMesg["data"]=this->parseHex(what[2].str().c_str());
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			string defAppendixKey("ZB.CSLock."+rabbitMQMesg["ZB_MAC"].asString());
			this->sendRMsg(defAppendixKey);
			cmdFinish();//alow next command
			return;
		}
	}
}
void ZB_CSLock::onTimeOut(){

	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		if(this->isBinded){
			//re-send command with index 2 (meaning second previous command)
			this->ResendATCmd(2);
			//re-send command with index 1 (meaning first previous command)
			this->ResendATCmd(1);
		}else{
			//re-send command with index 1 (meaning first previous command)
			this->ResendATCmd(1);
		}
		return;
	}
	Log::log.debug("ZB_CSLock::onTimeOut command timeout\n");
	this->rabbitMQMesg["type"]="ZB.CSLock.resp";
	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	string defAppendixKey("ZB.CSLock."+this->rabbitMQMesg["ZB_MAC"].asString());
	this->sendRMsg(defAppendixKey);
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}
TEM_DEFINE((std::map<int,int>),ZB_update_deactive::activeMap);
void ZB_update_deactive::onRabbitMQReceive(){
	if(this->rabbitMQMesg["type"].asString().compare("ZB.update.deactive.init")==0){
		//initialise the deactive task;
		this->setTTL(60000); //1mins
	}else if(this->rabbitMQMesg["type"].asString().compare("ZB.update.deactive.SOC")==0){
		int NWK =this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
		if(this->activeMap[NWK] <= 0){
			//stop sending ZB.update.deactive message
			this->activeMap.erase(NWK);
		}
	}
}
void ZB_update_deactive::onTimeOut(){
	std::map<int,int>::iterator it = this->activeMap.begin();
	for(;it!=this->activeMap.end();it++){
		it->second --;
		if(it->second<=0){
			//std::map<int,int>::iterator toErase=it;
			//this->activeMap.erase(toErase);
			std::string ZB_MAC= this->container->lookup.getNWK_MAC(it->first);
			Json::Value root;
			root["type"]="ZB.update.deactive";
			root["id"]=0;
			root["AP"]= this->container->pConf->home.id;
			root["ZB_MAC"] =ZB_MAC;
			this->rabbitMQMesg = root;
			this->sendRMsg(("ZB.update.deactive."+ZB_MAC).c_str());

			if(it->second == 0) {
				//warning the deactive node first time
				Log::log.warning("ZB_ device update_deactive: deactive device [%s]\n",ZB_MAC.c_str());
				//delete cached values (if find the MAC, then update, if not find anything, do nothing)
				this->container->lookup.delMACDevT_value(ZB_MAC);
			}else{
				//record the deactive node
				Log::log.debug("ZB_ device update_deactive: deactive device [%s]\n",ZB_MAC.c_str());
			}
		}
	}
	this->setTTL(60000); //1mins
}
void ZB_update_deactive::renewDev(int NWK){
    ZB_update_deactive::activeMap[NWK] = ZB_update_deactive::DEAD_PERIOD;
}


void ZB_IR::onRabbitMQReceive(){
	this->retry=0;
	this->setTTL(TYPICAL_DELAY);//give command TYPICAL_DELAY ms time to live, if no response in 0.5 seconds, it will time out
	int NWK_addr =this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
	if (NWK_addr==-1) {
		Log::log.warning("ZB_IR::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		//can not find such a device in current lookup table
		this->rabbitMQMesg["type"]="ZB.IR.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.IR."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}

	int IR_cmd= this->rabbitMQMesg["IR_cmd"].asInt();
	//AT+IR:<NWK>,<EndPoint>,<MsgType>,<code>
	std::string data= (this->rabbitMQMesg["data"].isNull() ? "": this->rabbitMQMesg["data"].asString());
	string atCmd("AT+IR:"
				+this->intToHexString(NWK_addr)
				+",8D,"
				+this->intToHexString(IR_cmd)
				+","
				+this->toHexStr(this->rabbitMQMesg["remoteId"].asInt())
				+ data);
	this->sendATCmd(atCmd);
}
std::string ZB_IR::toHexStr(int number){
	int i=0;
	char str[9];
	unsigned int num = (unsigned int)number;
	for(;i<8;i++){
		int temp = num%16;
		num >>= 4;
		if(i%2==0){
			if(temp<10 ){
				str[i+1]=temp+'0';
			}else{
				str[i+1]=temp-10+'A';
			}
		}else{
			if(temp<10 ){
				str[i-1]=temp+'0';
			}else{
				str[i-1]=temp-10+'A';
			}

		}
	}
	str[8]='\0';
	return std::string(str);

}
void ZB_IR::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		this->ResendATCmd(1);
		return;
	}
	Log::log.debug("ZB_IR::onTimeOut command timeout\n");
	this->rabbitMQMesg["type"]="ZB.IR.resp";
	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["status"]=this->statusCode.ZB_time_out;
	string defAppendixKey("ZB.IR."+this->rabbitMQMesg["ZB_MAC"].asString());
	this->sendRMsg(defAppendixKey);
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}
void ZB_IR::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	//IR：<NWK>,<EndPoint>,<MsgType>,<status>
	boost::regex expr("IR:(\\w{4}),(\\w{2}),(\\w{2}),(\\w{2})");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr))
	{
		int NWK = this->parseHex(what[1].str().c_str());
		int EP = this->parseHex(what[2].str().c_str());
		int MsgType = this->parseHex(what[3].str().c_str());
		int status = this->parseHex(what[4].str().c_str());
		if(this->rabbitMQMesg["ZB_MAC"].asString().compare(this->container->lookup.getNWK_MAC(NWK))==0
				&& EP == 0x8D && MsgType ==this->rabbitMQMesg["IR_cmd"].asInt()){
			this->rabbitMQMesg["type"]="ZB.IR.resp";
			if(status ==0){
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
				string defAppendixKey("ZB.IR."+rabbitMQMesg["ZB_MAC"].asString());
				this->sendRMsg(defAppendixKey);
			}else{
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.ZB_IR_cmd_failure;
				string defAppendixKey("ZB.IR."+rabbitMQMesg["ZB_MAC"].asString());
				this->sendRMsg(defAppendixKey);
			}
			cmdFinish();//alow next command
			return;
		}
	}
}

void AMQPHeartBeat::onRabbitMQReceive(){
	if(this->rabbitMQMesg["type"].asString().compare("AMQPHeartBeat.init")==0){
		//initialise Heatbeat task
		this->setTTL(10000); //10s
		this->rabbitMQMesg["type"]="AMQPHeartBeat.req";
	}
}
void AMQPHeartBeat::onTimeOut(){
	this->sendRMsg("AMQPHeartBeat");
	this->setTTL(10000);// time out after 10s then we can send the heartBeat again
}

void AP_keepAlive::onRabbitMQReceive(){
	if(this->rabbitMQMesg["type"].asString().compare("AP.keepAlive.init")==0){
		//initialise Heatbeat task
		this->setTTL(300000); //300s/5min
		Json::Value root;
		root["type"]="AP.keepAlive";
		root["id"] = ZB_update::getUpdateId();
		root["AP"] = this->container->pConf->home.id;
		this->rabbitMQMesg = root;
		this->sendRMsg("AP.keepAlive");
	}
}
void AP_keepAlive::onTimeOut(){
	this->rabbitMQMesg["id"] = ZB_update::getUpdateId();
	this->sendRMsg("AP.keepAlive");
	this->setTTL(300000);// time out after 300s/5min then we can send the keepAlive again
}

Bind_state ZB_binding_server::bind_state = INIT;
std::list<ZB_binding_server::Bind_unit> ZB_binding_server::binding1;
std::list<ZB_binding_server::Bind_unit> ZB_binding_server::binding2;
void ZB_binding_server::onRabbitMQReceive(){
	if(this->rabbitMQMesg["type"].asString().compare("ZB.binding.server.init")==0){
		//initialise binding background service
		this->setTTL(Cmd::CMD_FOREVER_TTL_MARK); //never timeout
		this->bind_state = READY;
	}
}
void ZB_binding_server::onATReceive(){
	//match serial port BINDREQ message
	//BINDREQ:AB1F,EP,IO,EP,IO… or UNBINDREQ:AB1F,EP,IO,EP,IO…
	//BINDCMD:scrAddr,EP,CMD,Value
	boost::regex expr("(BINDREQ|UNBINDREQ|BINDCMD):(\\w{4})([,\\dABCDEF]+)");
	boost::sregex_iterator iter(this->ATMsg.begin(), this->ATMsg.end(), expr);
	boost::sregex_iterator end;
	for( ; iter != end; ++iter ) {
	    boost::smatch const &what = *iter;
	    //update lookup tables
	    Log::log.debug("ZB_binding_server::onATReceive: %s\n",what[0].str().c_str());

	    int NWK = this->parseHex(what[2].str().c_str());
	    std::string MAC = this->container->lookup.getNWK_MAC(NWK);
	    if(MAC.empty()) {
	    	//no such device found in local cache network translation table.
		    Log::log.warning("ZB_binding_server::onATReceive: unable to find such a device in local network tanslation table!\n");
	    	return;
	    }
	    std::string eps(what[3].str());
	    if(what[1].str().compare("BINDCMD") == 0){
	    	//forward the command according to the binding information
	    	int EP = this->parseHex(eps.substr(1,2).c_str());
	    	int cmd = this->parseHex(eps.substr(4,2).c_str());
	    	int value = this->parseHex(eps.substr(7,2).c_str());
	    	Bindings::Binding_pair* pair = this->container->bindings.getBinding(MAC,EP);
	    	if(pair != NULL){
	    		if(cmd==1){
					//switch onoff control
					Json::Value root;
					root["type"]="ZB.bind_onoff.req";
					root["data"]=value;
					root["ZB_MAC"]=pair->MAC1;
					root["ZB_EP"]=pair->EP1;
					this->sendToRabbitMQAnalyser(root);

	    		}
	    	}else if((pair = this->container->bindings.getBindingR(MAC,EP))!=NULL){
	    		//cannot find in forward direction, try to find in reverse direction
	    		if(cmd==1){
	    			if(value <10){
						//reverse switch onoff control
						Json::Value root;
						root["type"]="ZB.bind_onoff.req";
						root["data"]=value;
						root["ZB_MAC"]=pair->MAC0;
						root["ZB_EP"]=pair->EP0;
						this->sendToRabbitMQAnalyser(root);
	    			}
					else if(value <=11){
						//translate value
						value =value-10;
						//address translation and update to cloud, this message is only sent by secondary switch
						//update local value cache
				    	this->container->lookup.updateMACDevT_value(pair->MAC0,
													this->container->lookup.getEP_DevT(pair->EP0),
													value);
						//send update to rabbitMQ
						Json::Value root;
						root["type"]="ZB.update";
						root["id"] = ZB_update::getUpdateId();
						root["AP"] = this->container->pConf->home.id;
						root["ZB_MAC"]=pair->MAC0;
						root["ZB_type"] = this->container->lookup.getEP_DevT(pair->EP0);
						root["data"]=value;
						this->rabbitMQMesg = root;
						this->sendRMsg(("ZB.update."+pair->MAC0).c_str());
					}else if(value <=13){
						//translate value
						value =value-12;
						//update local value cache
						this->container->lookup.updateMACDevT_value(pair->MAC0,
									this->container->lookup.getEP_DevT(pair->EP0),
									value);

						//reverse switch onoff control
						Json::Value root;
						root["type"]="ZB.bind_onoff.req";
						root["data"]=value+3; //stop re-forwarding
						root["ZB_MAC"]=pair->MAC0;
						root["ZB_EP"]=pair->EP0;
						this->sendToRabbitMQAnalyser(root);

						//reverse switch onoff control
						root.clear();
						root["type"]="ZB.update";
						root["id"] = ZB_update::getUpdateId();
						root["AP"] = this->container->pConf->home.id;
						root["ZB_MAC"]=pair->MAC0;
						root["ZB_type"] = this->container->lookup.getEP_DevT(pair->EP0);
						root["data"]=value;
						this->rabbitMQMesg = root;
						this->sendRMsg(("ZB.update."+pair->MAC0).c_str());
					}
	    		}
	    	}
	    }else if(what[1].str().compare("BINDREQ") == 0){
	    	if(this->bind_state == READY) {
	    		this->bind_state = IN_PROCESS;
	    		this->binding1.clear();
				for(unsigned int i=0;i+4 < eps.length();i+=5){
					Bind_unit bind_unit;
					bind_unit.MAC=MAC;
					bind_unit.EP = this->parseHex(eps.substr(i+1,2).c_str());
					bind_unit.IO = eps.at(i+4);
					this->binding1.push_back(bind_unit);
				}
				//set timeout 10s
				this->setTTL(10000);
	    	}else if(this->bind_state == IN_PROCESS){
	    		this->bind_state = READY;
	    		this->binding2.clear();
	    		this->setTTL(Cmd::CMD_FOREVER_TTL_MARK); //cancel timer
				for(unsigned int i=0;i+4 < eps.length();i+=5){
					Bind_unit bind_unit;
					bind_unit.MAC=MAC;
					bind_unit.EP = this->parseHex(eps.substr(i+1,2).c_str());
					bind_unit.IO = eps.at(i+4);
					this->binding2.push_back(bind_unit);
				}
				//Add the request into bindings
				this->addBindings();
	    	}else{
	    		//error
	    	}
	    }else if(what[1].str().compare("UNBINDREQ") == 0){
	    	//implement in future
	    }
	}
}
void ZB_binding_server::onTimeOut(){
	this->setTTL(Cmd::CMD_FOREVER_TTL_MARK); //avoid perform onTimeOut again; cancel timer
	if(this->bind_state == IN_PROCESS){
		//indicate fail to perform binding process
		this->bind_state = READY; //reset bind state to ready to receive bind request.
	}
}

void ZB_binding_server::addBindings(){
	if(this->binding1.size() == this->binding2.size()){
		while(!this->binding1.empty()){

			Bind_unit unit0 = *(this->binding1.begin());
			this->binding1.pop_front();
			Bind_unit unit1 = *(this->binding2.begin());
			this->binding2.pop_front();
			if(unit0.IO == '0' && unit1.IO == '1'){
				this->container->bindings.updateBinding(unit0.MAC,unit0.EP,unit1.MAC,unit1.EP);
			}else if(unit0.IO == '1' && unit1.IO == '0'){
				this->container->bindings.updateBinding(unit1.MAC,unit1.EP,unit0.MAC,unit0.EP);
			}else{
				//not match, error
			}
		}
	}else {
		//binding size check failure;
	}
}

void ZB_bind_onoff::onRabbitMQReceive(){
	this->retry=0;
	Log::log.debug("ZB_bind_onoff::onRabbitMQReceive rabbitMQ message received received\n");
	this->setTTL(TYPICAL_DELAY);//give command TYPICAL_DELAY time to live, if no response in 500 seconds, it will time out
	//AT+RONOFF:<NWK>,<EP>,0,<ON/OFF>
	int data = this->rabbitMQMesg["data"].asInt();
	int NWK_addr = this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
	if (NWK_addr==-1) {
		//can not find such a device in current lookup table
		Log::log.warning("ZB_onoff::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		this->cmdFinish();
		return;
	}
	std::string onoff_cmd(this->intToHexString(data));
	string atCmd("AT+RONOFF:"
			+this->intToHexString(NWK_addr)
			+","
			+this->intToHexString(this->rabbitMQMesg["ZB_EP"].asInt())
			+",0,"
			+ onoff_cmd);
	this->sendATCmd(atCmd);

}
void ZB_bind_onoff::onTimeOut(){
	if(this->retry < this->RETRY_MAX){
		// retry
		this->retry++;
		this->setTTL(TYPICAL_DELAY); //allow TYPICAL_DELAY ms to retry.
		this->ResendATCmd(1);
		return;
	}
	Log::log.warning("ZB_bind_onoff::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}
void ZB_bind_onoff::onATReceive(){
	if(this->detectATErr() ==1){
		//AT command ERROR:01, indicate serial FCS error, so re-send command with index 1 (meaning first previous command)
		this->ResendATCmd(1);
		return;
	}
	Log::log.debug("ZB_bind_onoff::onATReceive AT command received [%s]\n",this->ATMsg.c_str());
	boost::regex expr("ERROR:|OK");
	boost::smatch what;
	if (boost::regex_search(this->ATMsg, what, expr)){
		this->cmdFinish();
	}

}

void AP_uci::onRabbitMQReceive(){
	this->setTTL(10000);//some command may need long time to be performed, for example network command
	std::string OP = this->rabbitMQMesg["OP"].asString();
	this->rabbitMQMesg["type"]="AP.uci.resp";
	if(this->rabbitMQMesg["data"].isArray()){
		if(	OP.compare("get")==0){
			std::string data = LW::get_all(this->rabbitMQMesg["data"][0].asCString());
			if(data.length()==0){
				//indicate error
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.LUA_RUN_ERROR;
			}else{
				Json::Value jdata;
				Json::Reader reader;
				bool parsedSuccess = reader.parse(data,jdata,false);
				if(parsedSuccess){
					this->rabbitMQMesg["data"]=jdata;
					this->rabbitMQMesg["status"]=this->statusCode.succeed;
				}else{
					this->rabbitMQMesg["data"]=-1;
					this->rabbitMQMesg["status"]=this->statusCode.LUA_RUN_ERROR;
				}
			}
			this->sendRMsg("AP.uci");
			this->cmdFinish();
			return;
		}else if(OP.compare("set")==0){
			int result = LW::set(this->rabbitMQMesg["data"].toStyledString().c_str());
			if(result==1 || result==0){
				//indicate error
				this->rabbitMQMesg["data"]=result;
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
			}else{
				//indicate error
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.LUA_RUN_ERROR;
			}
			this->sendRMsg("AP.uci");
			this->cmdFinish();
			return;
		}else if(OP.compare("delete")==0){
			int result = LW::Delete(this->rabbitMQMesg["data"].toStyledString().c_str());
			if(result==1 || result==0){
				//indicate error
				this->rabbitMQMesg["data"]=result;
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
			}else{
				//indicate error
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.LUA_RUN_ERROR;
			}
			this->sendRMsg("AP.uci");
			this->cmdFinish();
			return;
		}else if(OP.compare("reload")==0 ){
			char buffer[200];
			std::string result = "";
			std::string path = "/etc/init.d/"+this->rabbitMQMesg["data"][0].asString();
			FILE* pipe =NULL;
			if (0 == access(path.c_str(), 0)) {
				pipe = popen((path+" restart").c_str(), "r");
			}

			if (!pipe) {
				//fail operation
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
				this->sendRMsg("AP.uci");
				this->cmdFinish();
				return;
			};
			try {
				while (!feof(pipe)) {
					if (fgets(buffer, 200, pipe) != NULL)
						result += buffer;
				}
			} catch (...) {
				pclose(pipe);
				//fail operation
				this->rabbitMQMesg["data"]=-1;
				this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
				this->sendRMsg("AP.uci");
				this->cmdFinish();
				return;
			}
			pclose(pipe);

			this->rabbitMQMesg["data"]=result;
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg("AP.uci");
			this->cmdFinish();
			return;

		}
	}

	//indicate error
	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["status"]=this->statusCode.UCI_FORMAT_ERROR;
	this->sendRMsg("AP.uci");
	this->cmdFinish();
}
void AP_uci::onTimeOut(){
	Log::log.warning("AP_uci::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}

void AP_get_ifconfig::onRabbitMQReceive(){
	this->setTTL(1000);
	char buffer[200];
	std::string result = "";
	std::string routingKey="AP.get.ifconfig";
	this->rabbitMQMesg["type"]="AP.get.ifconfig.resp";
	std::string path = "ifconfig";
	FILE* pipe =NULL;
//	if (0 == access(path.c_str(), 0)) {
		pipe = popen((path).c_str(), "r");
//	}
	if (!pipe) {
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	};
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 200, pipe) != NULL)
				result += buffer;
		}
	} catch (...) {
		pclose(pipe);
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	}
	pclose(pipe);

	this->rabbitMQMesg["data"]=result;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg(routingKey);
	this->cmdFinish();
	return;
}

void AP_get_ifconfig::onTimeOut(){
	Log::log.warning("AP_get_ifconfig::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}

static int MD5Check(const char * filename, const char* md5sum){
	char md5[MD5_DIGEST_LENGTH*2+1];
	char md5sum2[MD5_DIGEST_LENGTH*2+1];
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    FILE *inFile = fopen (filename, "rb");

    if (!inFile) {
    	return -1;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    fclose (inFile);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) sprintf(md5+2*i,"%02x", c[i]);
    md5[2*i]='\0';
    std::string md5_str((const char*)md5);
    for(i=0;i<MD5_DIGEST_LENGTH*2 && md5sum[i]!='\0';i++){
    	if(md5sum[i]<='Z' && md5sum[i]>='A')
    	    md5sum2[i] = md5sum[i]-('Z'-'z');
    	else
    		md5sum2[i] = md5sum[i];
    }
    md5sum2[2*MD5_DIGEST_LENGTH]='\0';
    std::string md5sum_str(md5sum2);
    if(md5sum_str.compare(md5_str) == 0){
    	return 1;
    }else{
    	return 0;
    }
    return 0;
}
void AP_upgrade::onRabbitMQReceive(){
	int status;
	this->setTTL(1000);
	std::string routingKey="AP.upgrade";
	this->rabbitMQMesg["type"]="AP.upgrade.resp";

	int UT = this->rabbitMQMesg["UT"].isInt()? this->rabbitMQMesg["UT"].asInt() : -1;
	if(UT == 1){
		if((!this->rabbitMQMesg["cancel"].isNull())
				&& this->rabbitMQMesg["cancel"].isInt()
				&&this->rabbitMQMesg["cancel"].asInt()==1){
			//cannel upgrade
			this->rabbitMQMesg["type"] = "AP.upgrade.service1.cancel";
			this->rabbitMQMesg["__attribute_uart_exclusive"]=false;  //attribute is non exclusive
			this->sendToRabbitMQAnalyser(this->rabbitMQMesg); //attribute is non exclusive
		}else{
			this->rabbitMQMesg["type"] = "AP.upgrade.service1.init";
			this->rabbitMQMesg["__attribute_uart_exclusive"]=false;  //attribute is non exclusive
			this->sendToRabbitMQAnalyser(this->rabbitMQMesg); //attribute is non exclusive

		}
		this->cmdFinish();
		return;

	}else if(UT == 2){
		status = system("opkg update && opkg install shs&");
		if(status ==0){
			this->rabbitMQMesg["stage"] = 1; // indicate download OK
			this->rabbitMQMesg["data"] = "shs software is updating! may take long time.";
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(routingKey);
			this->cmdFinish();
			return;
		}else{
			this->rabbitMQMesg["stage"] = 1; // indicate download OK
			this->rabbitMQMesg["data"] = "update fail";
			this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
			this->sendRMsg(routingKey);
			this->cmdFinish();
			return;
		}

	}


	this->rabbitMQMesg["data"]=-1;
	this->rabbitMQMesg["status"]=this->statusCode.MSG_FORMAT_ERROR;
	this->sendRMsg(routingKey);
	this->cmdFinish();
}
void AP_upgrade::onTimeOut(){
	Log::log.warning("AP_upgrade::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}
int static getHttpFileLen(const char * url, string &lenstr){
	FILE* pipe =NULL;
	char buffer[200];
	std::string result = "";
	pipe = popen(("wget --spider "+std::string(url) + " 2>&1").c_str(), "r");
	if (!pipe) {
		//fail operation
		return -1;
	};
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 200, pipe) != NULL)
				result += buffer;
		}
	} catch (...) {
		pclose(pipe);
		return -1;
	}
	pclose(pipe);

	boost::regex expr("^Length:\\s*(\\d+)");
	boost::smatch what;
	int len = -1;
	if (boost::regex_search(result, what, expr))
	{
		lenstr = what[1].str();
		len = std::atoi(lenstr.c_str());

	}

	Log::log.warning("the length is: %d",len);
	return len;

}


AP_upgrade_service1::AP_upgrade_service1():
	stage(0){
			fileLength = 0;
			url="";
			md5="";
}

void AP_upgrade_service1::onRabbitMQReceive(){
	int status;
	std::string routingKey="AP.upgrade";
	if(this->rabbitMQMesg["type"].asString().compare("AP.upgrade.service1.init")==0){
		this->rabbitMQMesg["type"]="AP.upgrade.resp";
		this->setTTL(1000); //ttl 1s
		if(this->stage ==0){
			//start download
			this->stage =1;
			url = this->rabbitMQMesg["URL"].isString() ? this->rabbitMQMesg["URL"].asString() : "";
			md5 = this->rabbitMQMesg["md5"].isString() ? this->rabbitMQMesg["md5"].asString() : "";
			if(url.length()==0){
				//default URL
				url = "http://opkg.baiyatech.com/repos/shs-project/openwrt-15.05-ar71xx-generic-by-shs-squashfs-sysupgrade.bin";
			}
			string lenstr ="";
			this->fileLength = getHttpFileLen(url.c_str(),lenstr);
			status = system(("wget "+url +" -O /tmp/by-shs-firmware.bin 1>/dev/null 2>/dev/null&").c_str());
			if(this->fileLength>0 && status==0){
				this->rabbitMQMesg["stage"] = this->stage; //indicate downloading
				this->rabbitMQMesg["data"] = std::string("File Size: ") +lenstr +" Bytes, is downloading..."; //indicate downloading
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
				this->stage=2;
				this->sendRMsg(routingKey);
				return;
			}else{
				this->rabbitMQMesg["stage"] = this->stage; // indicate download status
				this->rabbitMQMesg["data"] = -1; //indicate downloading
				this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
				this->sendRMsg(routingKey);
				this->cmdFinish();
				this->stage =0;
				return;
			}


		}else{
			//is doing upgrade return error
			this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
			this->rabbitMQMesg["data"] = -1;
			this->rabbitMQMesg["status"]=this->statusCode.UPGRADE_STAGE_ERROR1;
			this->sendRMsg(routingKey);
			return;
		}
	}if(this->rabbitMQMesg["type"].asString().compare("AP.upgrade.service1.cancel")==0){
		this->rabbitMQMesg["type"]="AP.upgrade.resp";
		if(this->stage >0){
			//cancel
			system("killall wget");
			system("rm -rf /tmp/by-shs-firmware.bin");
			this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
			this->rabbitMQMesg["data"] = "Upgrade is cancelled";
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(routingKey);
			//reset the state
			this->stage=0;
			this->cmdFinish();
		}else{
			//return error, no need cancel
			//is doing upgrade return error
			this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
			this->rabbitMQMesg["data"] = "the system is not upgrading, do not need to cancel";
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(routingKey);
			this->cmdFinish();
			this->stage=0;
			return;
		}
	}
}
void AP_upgrade_service1::onTimeOut(){
	int status;
	std::string routingKey="AP.upgrade";
	this->setTTL(1000);
	this->depressTimeoutWarning=true;

	if(this->stage==2){
		int downloadedSize = Log::get_file_size("/tmp/by-shs-firmware.bin");
		//the file may not exist. thie downloadedSize will equal -1;
		if(downloadedSize==this->fileLength){
			//download successful;
			this->rabbitMQMesg["stage"] = this->stage;
			this->rabbitMQMesg["data"] = "download finished"; //indicate downloading
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(routingKey);

			this->stage=3;

			if(md5.length()>0){
				//status = system(("echo \""+md5 +" /tmp/by-shs-firmware.bin\" | md5sum -sc").c_str());
				status = MD5Check("/tmp/by-shs-firmware.bin",md5.c_str());
				if(status ==-1 ){
					this->rabbitMQMesg["stage"] = this->stage;
					this->rabbitMQMesg["data"] = -1; //check running exception
					this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
					this->sendRMsg(routingKey);
					this->cmdFinish();
					this->stage = 0;
					system("rm -rf /tmp/by-shs-firmware.bin");
					return;
				}else if(status == 0){
					this->rabbitMQMesg["stage"] = this->stage;
					this->rabbitMQMesg["data"] = -1;
					this->rabbitMQMesg["status"]=this->statusCode.VERIFY_ERROR;
					this->sendRMsg(routingKey);
					this->cmdFinish();
					this->stage = 0;
					system("rm -rf /tmp/by-shs-firmware.bin");
					return;
				}else if(status == 1){
					this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
					this->rabbitMQMesg["data"] = "md5sum Verified OK";
					this->rabbitMQMesg["status"]=this->statusCode.succeed;
					this->sendRMsg(routingKey);
				}
			}

			this->stage = 4;
			status = system("sysupgrade -d 2 /tmp/by-shs-firmware.bin&");
			if(status ==0){
				this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
				this->rabbitMQMesg["data"] = "the system is updating! may take long time.";
				this->rabbitMQMesg["status"]=this->statusCode.succeed;
				this->sendRMsg(routingKey);
				this->cmdFinish();
				this->stage = 0;
				return;
			}else{
				this->rabbitMQMesg["stage"] = this->stage; // indicate download OK
				this->rabbitMQMesg["data"] = "updating fail";
				this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
				this->sendRMsg(routingKey);
				this->cmdFinish();
				this->stage = 0;
				return;
			}

		}else{
			//is downloading
			char ratio[20];
			sprintf(ratio,"%6.2f",double(downloadedSize)/this->fileLength*100);
			this->rabbitMQMesg["stage"] = this->stage;
			this->rabbitMQMesg["data"] = std::string(ratio)+"\% is downloaded"; //indicate downloading
			this->rabbitMQMesg["status"]=this->statusCode.succeed;
			this->sendRMsg(routingKey);
			return;
		}
	}


	this->cmdFinish();
}

void AP_get_version::onRabbitMQReceive(){
	this->setTTL(1000);
	char buffer[200];
	std::string result = "";
	std::string routingKey="AP.get.version";
	this->rabbitMQMesg["type"]="AP.get.version.resp";
	FILE* pipe =NULL;
	pipe = popen( "opkg list_installed | grep shs", "r");
	if (!pipe) {
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	};
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 200, pipe) != NULL)
				result += buffer;
		}
	} catch (...) {
		pclose(pipe);
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	}
	pclose(pipe);

	this->rabbitMQMesg["data"]=result;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg(routingKey);
	this->cmdFinish();
	return;
}
void AP_get_version::onTimeOut(){
	Log::log.warning("AP_get_version::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}

void AP_get_log::onRabbitMQReceive(){
	this->setTTL(3000);
	//buf_s is the buffer size, should be an integral multiple of 3
	size_t buf_s= 1000*3;
	char buffer[buf_s];
	char* result;
	std::string routingKey="AP.get.log";
	this->rabbitMQMesg["type"]="AP.get.log.resp";
	FILE* pipe =NULL;
	std::string cmd = "cd $(dirname " + this->container->pConf->logfile +") && " "tar -czf - $(basename "+ this->container->pConf->logfile +")";
	pipe = popen( cmd.c_str(), "r");
	if (!pipe) {
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	};
	try {
		//first allocate 2k memory
		size_t additonal_sz = buf_s/3*4+1;
		result = (char*) malloc(additonal_sz);

		if(result == NULL){
			throw new std::exception;
		}

		size_t result_size=0;

		while (!feof(pipe)) {
			size_t read_sz = fread((void *)buffer,1,buf_s,pipe);
			if (read_sz>0){

				//Log::log.debug("read_sz:%d\n",read_sz);
				//keep additional_sz enough
				char* result2 = (char*) realloc((void*)result, additonal_sz+result_size);
				if(result2){
					result = result2;
				}else{
					free(result);
					throw new std::exception;
				}

				size_t encoded_sz = base64_encode2((unsigned char*)buffer,read_sz,(unsigned char*)result+result_size);
				result_size += encoded_sz;
			}
		}
		//write \0
		result[result_size] = '\0';
	} catch (...) {
		pclose(pipe);
		//fail operation
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.RUN_TIME_ERROR;
		this->sendRMsg(routingKey);
		this->cmdFinish();
		return;
	}
	pclose(pipe);

	this->rabbitMQMesg["data"]=result;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg(routingKey);
	this->cmdFinish();
	free(result);
	return;

}
void AP_get_log::onTimeOut(){
	Log::log.warning("AP_get_log::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}




void AP_discover::onRabbitMQReceive(){
	std::string routingKey="AP.discover";
	this->rabbitMQMesg["type"] = "AP.discover.resp";
	this->rabbitMQMesg["AP"]= this->container->pConf->home.id;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg(routingKey);
	this->cmdFinish();
	return;
}
void AP_discover::onTimeOut(){
	Log::log.warning("AP_discover::onTimeOut command timeout\n");
	cmdFinish();//alow next command and remove it from active cmd object list
	return;
}


} /* namespace SHS */
