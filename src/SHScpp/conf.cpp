/*
 * conf.cpp
 *
 *  Created on: 20 Mar 2016
 *      Author: xiao Wang
 */

#include "conf.h"

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
	this->home.id=0;
	this->home.MAC="000000000000";
	this->serialPort.baudrate=115200;
	this->serialPort.port="/dev/ttyUSB0";

}

Conf::~Conf() {

}

} /* namespace SHS */
