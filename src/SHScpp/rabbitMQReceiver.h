/*
 * rabbitMQReceiver.h
 *
 *  Created on: 18 Mar 2016
 *      Author: xiao Wang
 */

#ifndef SHSCPP_RABBITMQRECEIVER_H_
#define SHSCPP_RABBITMQRECEIVER_H_
#include "conf.h"
#include "log.h"
#include "rabbitMQConnection.h"
#include "myMQ.h"
#include "iostream"
namespace SHS {

class RabbitMQReceiver {
public:
	RabbitMQReceiver(RabbitMQConnection & conn);
	virtual ~RabbitMQReceiver();
	void startReceive(Conf &conf,MyMQ<Json::Value>* pMQ);//listen according the Conf object and sent the data to pMQ
	void startReceiveAsThread(Conf &conf,MyMQ<Json::Value>* pMQ);
	MyMQ<Json::Value> *pMQ;
private:
	Channel chann;
};

} /* namespace SHS */

#endif /* SHSCPP_RABBITMQRECEIVER_H_ */
