/*
 * rabbitMQAnalyser.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#ifndef SHSCPP_RABBITMQANALYSER_H_
#define SHSCPP_RABBITMQANALYSER_H_
#include "rabbitMQReceiver.h"
#include <json/json.h>
#include "myMQ.h"
#include "container.h"
#include "Lookup.h"
namespace SHS {

class RabbitMQAnalyser {
public:
	RabbitMQAnalyser(Container *pContainer);
	//RabbitMQAnalyser(RabbitMQReceiver & receiver, DB & DB );
	virtual ~RabbitMQAnalyser();
	void startAnalyse(MyMQ<Json::Value> * pMQ);
	void startAnalyseAsThread(MyMQ<Json::Value> * pMQ);
private:
	Container *pContainer;
};

} /* namespace SHS */

#endif /* SHSCPP_RABBITMQANALYSER_H_ */
