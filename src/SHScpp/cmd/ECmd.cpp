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
		Log::log.warning("ZB_onoff::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
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


void ZB_init_discover::onRabbitMQReceive(){
	this->setTTL(2500);
	this->isResponded=false;
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
	//the serial port is responded, so set the member field to true
	this->isResponded=true;
}
void ZB_init_discover::onTimeOut(){
	Log::log.debug("ZB_startup_discover::onTimeOut\n");
	if(!this->isResponded) Log::log.error("ZB_startup_discover::Serial port No response \n");
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
			    root["id"] = ++this->update_id;
			    root["AP"] = (unsigned int) this->container->pConf->home.id;
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
			root["id"] = ++this->update_id;
			root["AP"] = (unsigned int) this->container->pConf->home.id;
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
	ATResp["AP"]= (unsigned int )this->container->pConf->home.id;
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
		Log::log.warning("ZB_read::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
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
void ZB_set::onRabbitMQReceive(){
	int NWK;
	int EP;
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
		this->setTTL(1500);
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
	this->data.clear();
	this->sendATCmd("AT+DISCOVER:");
}
void ZB_discover::onTimeOut(){
	this->rabbitMQMesg["type"]="ZB.discover.resp";
	this->rabbitMQMesg["data"]=data;
	this->rabbitMQMesg["AP"]= (unsigned int )this->container->pConf->home.id;
	this->rabbitMQMesg["status"]=this->statusCode.succeed;
	this->sendRMsg("ZB.discover");
	this->cmdFinish();
}
void ZB_discover::onATReceive(){
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

void ZB_AT::onRabbitMQReceive(){
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
		this->rabbitMQMesg["AP"]= (unsigned int )this->container->pConf->home.id;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_AT_format_error;
		this->sendRMsg("ZB.AT");
	}
	this->cmdFinish();
}

void ZB_CSLock::onRabbitMQReceive(){
	this->setTTL(2000);//give command 2 seconds time to live, if no response in 2 seconds, it will time out
	//AT+CSLOCK:<NWK>,<cmd>
	int data = this->rabbitMQMesg["data"].asInt();
	int NWK_addr = this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
	if (NWK_addr==-1) {
		Log::log.warning("ZB_CSLock::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		//can not find such a device in current lookup table
		this->rabbitMQMesg["type"]="ZB.CSLock.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.CSLock."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}
	string atCmd("AT+CSLOCK:"
			+this->intToHexString(NWK_addr)
			+","
			+ this->intToHexString(data));
	this->sendATCmd(atCmd);
}
void ZB_CSLock::onATReceive(){
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
			root["AP"]= (unsigned int )this->container->pConf->home.id;
			root["ZB_MAC"] =ZB_MAC;
			this->rabbitMQMesg = root;
			this->sendRMsg(("ZB.update.deactive."+ZB_MAC).c_str());

			if(it->second == 0) {
				//record the deactive node first time
				Log::log.warning("ZB_ device update_deactive: deactive device [%s]\n",ZB_MAC.c_str());
			}
		}
	}
	this->setTTL(60000); //1mins
}
void ZB_update_deactive::renewDev(int NWK){
    ZB_update_deactive::activeMap[NWK] = ZB_update_deactive::DEAD_PERIOD;
}


void ZB_IR::onRabbitMQReceive(){
	this->setTTL(500);//give command 0.5 seconds time to live, if no response in 0.5 seconds, it will time out
	int NWK_addr =this->container->lookup.getMAC_NWK(this->rabbitMQMesg["ZB_MAC"].asCString());
	if (NWK_addr==-1) {
		Log::log.warning("ZB_IR::onRabbitMQReceive: no such device [%s]\n",this->rabbitMQMesg["ZB_MAC"].asCString());
		//can not find such a device in current lookup table
		this->rabbitMQMesg["type"]="ZB.CSLock.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_no_dev;
		string defAppendixKey("ZB.CSLock."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		this->cmdFinish();
		return;
	}

	std::string IR_cmd(this->rabbitMQMesg["IR_cmd"].asCString());
	if(IR_cmd.compare("send")==0){
		this->MsgType=0x00;
		//AT+IR:<NWK>,<EndPoint>,<MsgType>,<code>string atCmd("AT+CSLOCK:"
		string atCmd("AT+IR:"
					+this->intToHexString(NWK_addr)
					+",8D,00,"
					+ this->rabbitMQMesg["data"].asString());
		this->sendATCmd(atCmd);

	}else if(IR_cmd.compare("rev_onoff")==0){
		this->MsgType=0x02;
		//AT+IR:<NWK>,<EndPoint>,<MsgType>,<code>string atCmd("AT+CSLOCK:"
		string atCmd("AT+IR:"
					+this->intToHexString(NWK_addr)
					+",8D,02,"
					+ this->intToHexString(this->rabbitMQMesg["data"].asInt()));
		this->sendATCmd(atCmd);
	}else{
		this->rabbitMQMesg["type"]="ZB.IR.resp";
		this->rabbitMQMesg["data"]=-1;
		this->rabbitMQMesg["status"]=this->statusCode.ZB_IR_cmd_unsupport;
		string defAppendixKey("ZB.IR."+this->rabbitMQMesg["ZB_MAC"].asString());
		this->sendRMsg(defAppendixKey);
		cmdFinish();
	}
}
void ZB_IR::onTimeOut(){
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
				&& EP == 0x8D && MsgType ==this->MsgType){
			this->rabbitMQMesg["type"]="ZB.IR.resp";
			if(MsgType==0x00){
				//this->rabbitMQMesg["data"];
				//is the reply of "send" message
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
			}else if(MsgType==0x02){
				//is the reply of "rev_onoff" message
				if(status==0 || status ==1){
					this->rabbitMQMesg["data"]=status;
					string defAppendixKey("ZB.IR."+rabbitMQMesg["ZB_MAC"].asString());
					this->sendRMsg(defAppendixKey);
				}else{
					this->rabbitMQMesg["data"]=-1;
					this->rabbitMQMesg["status"]=this->statusCode.ZB_IR_cmd_failure;
					string defAppendixKey("ZB.IR."+rabbitMQMesg["ZB_MAC"].asString());
					this->sendRMsg(defAppendixKey);
				}
			}
			cmdFinish();//alow next command
			return;
		}
	}
}


} /* namespace SHS */
