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
	this->rabbitmq.hostname="amqp.baiyatech.com";
	this->rabbitmq.port=5672;
	this->rabbitmq.vhost="/";
	this->rabbitmq.user="shs-ap2";
	this->rabbitmq.password="T7ibPV0R7zPg";
	this->rabbitmq.exchange="SHS";
	this->home.AP_status=running;
	this->home.id=1;
	this->home.MAC="000000000000";
	this->serialPort.baudrate=115200;
	this->serialPort.port="/dev/ttyUSB0";
	this->tables_path.lookup_table = "/etc/SHS/lookup.json";
	this->tables_path.binding_table = "/etc/SHS/bindings.json";
	this->logfile = "/etc/SHS/shs.log";
	this->debugLevel = 3; //default print all the debug information 0:none 1:just error 2: error and warning 3: all debug information
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
		this->home.id = root["home"]["id"].asString();

	}
	if(! root["serialport"]["port"].isNull())
		this->serialPort.port =root["serialport"]["port"].asString();
	if(! root["serialport"]["baudrate"].isNull())
		this->serialPort.baudrate =root["serialport"]["baudrate"].asInt();
	if(! root["tables_path"]["lookup_table"].isNull())
		this->tables_path.lookup_table =root["tables_path"]["lookup_table"].asString();
	if(! root["tables_path"]["binding_table"].isNull())
		this->tables_path.binding_table =root["tables_path"]["binding_table"].asString();
	if(! root["logfile"].isNull())
		this->logfile =root["logfile"].asString();
	if(! root["debugLevel"].isNull())
		this->debugLevel =root["debugLevel"].asInt();
	//Log::log.debug("Lookup::load: EP %d\n",this->getDevT_EP(23));
	return true;
}
bool Conf::load(const char* path){
	std::string str(path);
	return this->load(str);
}
} /* namespace SHS */
