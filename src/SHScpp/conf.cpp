/*
 * conf.cpp
 *
 *  Created on: 20 Mar 2016
 *      Author: xiao Wang
 */

#include "conf.h"
#include "json/json.h"
#include "log.h"
#include <fstream>
namespace SHS {

Conf::Conf(){
	//this->rabbitmq.hostname="shs.ninglvfeihong.com";
	this->rabbitmq.hostname="101.200.171.184";
	this->rabbitmq.port=5672;
	this->rabbitmq.vhost="/";
	this->rabbitmq.user="shs";
	this->rabbitmq.password="accessnet10";
	this->rabbitmq.exchange="SHS";
	this->home.AP_status=running;
	this->home.id=1;
	this->home.MAC="000000000000";
	this->serialPort.baudrate=115200;
	this->serialPort.port="/dev/ttyUSB0";
	this->tables_path.lookup_table = "/etc/SHS/lookup.json";

}

Conf::~Conf() {

}

bool Conf::load(std::string & path){
	Json::Value root;   // will contain the root value after parsing.
	Json::CharReaderBuilder builder;
	std::ifstream test(path.c_str(), std::ifstream::binary);
	std::string errs;
	bool ok = Json::parseFromStream(builder, test, &root, &errs);
	if ( !ok )
	{
		Log::log.error("Lookup::load: Error to parse file: %s\n",path.c_str());
	}
	if(! root["rabbitmq"]["hostname"].isNull())
		this->rabbitmq.hostname =root["rabbitmq"]["hostname"].asString();
	if(! root["rabbitmq"]["port"].isNull())
		this->rabbitmq.port =root["rabbitmq"]["port"].asInt();
	if(! root["rabbitmq"]["vhost"].isNull())
		this->rabbitmq.vhost =root["rabbitmq"]["vhost"].asString();
	if(! root["rabbitmq"]["user"].isNull())
		this->rabbitmq.user =root["rabbitmq"]["user"].asString();
	if(! root["rabbitmq"]["password"].isNull())
		this->rabbitmq.password =root["rabbitmq"]["password"].asString();
	if(! root["rabbitmq"]["exchange"].isNull())
		this->rabbitmq.exchange =root["rabbitmq"]["exchange"].asString();
	if(! root["home"]["id"].isNull()){
		int id = root["home"]["id"].asInt();
		if(id==-1){
			Log::log.error("Please configure hoem.id in %s\n",path.c_str());
		}else{
			this->home.id =root["home"]["id"].asInt();
		}
	}
	if(! root["serialport"]["port"].isNull())
		this->serialPort.port =root["serialport"]["port"].asString();
	if(! root["serialport"]["baudrate"].isNull())
		this->serialPort.baudrate =root["serialport"]["baudrate"].asInt();
	if(! root["tables_path"]["lookup_table"].isNull())
		this->tables_path.lookup_table =root["tables_path"]["lookup_table"].asString();
	//Log::log.debug("Lookup::load: EP %d\n",this->getDevT_EP(23));
	return true;
}
bool Conf::load(const char* path){
	std::string str(path);
	return this->load(str);
}
} /* namespace SHS */
