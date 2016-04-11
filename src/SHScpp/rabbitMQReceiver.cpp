/*
 * rabbitMQReceiver.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#include "rabbitMQReceiver.h"
#include <sstream>
namespace SHS {

RabbitMQReceiver::RabbitMQReceiver(RabbitMQConnection & conn):pMQ(NULL),chann(conn.getChannel()) {

}

RabbitMQReceiver::~RabbitMQReceiver() {
}
void RabbitMQReceiver::startReceive(Conf & conf,MyMQ<Json::Value>* pMQ){
	//chann.publish("test.a","I am test");
	this->pMQ =pMQ;
	class Callback :public SHS::Channel::Callable_string{
	public:
		Callback(MyMQ<Json::Value> & myMQ):mq(myMQ){}
		void callback(const std::string & str){
			Json::Value root;
			Json::Reader reader;
			bool parsedSuccess = reader.parse(str,root,false);
			if(not parsedSuccess)
			{
			   // Report failures and their locations
			   // in the document.
				std::ostringstream ss;
				ss<<"Failed to parse JSON in rabbitmq receiver thread"<<std::endl
			       //<<reader.getFormatedErrorMessages()
			       <<std::endl;
				Log::log.warning(ss.str().c_str());
			}else{
				if(&mq){
					mq.sendMSG(root);
				}else{
					Log::log.error("rabbitMQReceiver do not have a valid MyMQ: pMQ addr:%d\n",&mq);
				}
			}
		}
		MyMQ<Json::Value> &mq;
	};
	if(conf.home.AP_status==Conf::pre_init){
		chann.binding("pre_init.AP_C.#");
		chann.binding("pre_init."+conf.home.MAC+".AP_C.#");
	}else{
		chann.binding("APs.AP_C.#");
		std::ostringstream ss;
		ss<<"APs."<<conf.home.id<<".AP_C.#";
		chann.binding(ss.str());
	}
	Callback f(*this->pMQ);
	chann.listen(f);
}

struct rabbitmq_receiver_temp_t {
	Conf* pConf;
	RabbitMQReceiver* pReceiver;
	MyMQ<Json::Value>* pMQ;
};
static void* rabbitMQReceiverThread(void *ptr){
	rabbitmq_receiver_temp_t & para = *((rabbitmq_receiver_temp_t*)ptr);
	para.pReceiver->startReceive(*para.pConf,para.pMQ);
	return NULL;
}
void RabbitMQReceiver::startReceiveAsThread(Conf & conf,MyMQ<Json::Value>* pMQ){
	pthread_t thread;
	static struct rabbitmq_receiver_temp_t para;//avoid the variable lost when function finished
	para.pConf=&conf;
	para.pReceiver=this;
	para.pMQ=pMQ;
	pthread_create (&thread, NULL,rabbitMQReceiverThread, (void*) &para);
	pthread_detach(thread);
}

} /* namespace SHS */
