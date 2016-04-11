/*
 * rabbitMQSender.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_RABBITMQSENDER_H_
#define SHSCPP_RABBITMQSENDER_H_
#include "conf.h"
#include "log.h"
#include "rabbitMQConnection.h"
#include "myMQ.h"
#include "iostream"
namespace SHS {

class RabbitMQSender {
public:
	RabbitMQSender(RabbitMQConnection & conn);
	virtual ~RabbitMQSender();
	void startSend(Conf &conf,MyMQ<Json::Value>* pMQ);
	void startSendAsThread(Conf &conf,MyMQ<Json::Value>* pMQ);
	MyMQ<Json::Value> *pMQ;
private:
	Channel chann;
	std::string getKey(Conf &conf,std::string& keyAppendix);
};

} /* namespace SHS */

#endif /* SHSCPP_RABBITMQSENDER_H_ */
