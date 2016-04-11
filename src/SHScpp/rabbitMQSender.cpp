/*
 * rabbitMQSender.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#include "rabbitMQSender.h"
#include "sstream"
namespace SHS {

RabbitMQSender::RabbitMQSender(RabbitMQConnection & conn):pMQ(NULL),chann(conn.getChannel()) {

}

RabbitMQSender::~RabbitMQSender() {
}
std::string RabbitMQSender::getKey(Conf &conf,std::string& keyAppendix){
	if(conf.home.AP_status ==Conf::running){
		std::ostringstream ss;
		ss<<"APs."<<conf.home.id<<".AP_P."<<keyAppendix;
		return ss.str();
	}else{
		return ("pre_init."+conf.home.MAC+".AP_P."+keyAppendix);
	}
}
void RabbitMQSender::startSend(Conf &conf,MyMQ<Json::Value>* pMQ){
	if(!pMQ) Log::log.error("RabbitMQSender: invalid MyMQ obj with addr:%d\n",pMQ);
	this->pMQ =pMQ;

	while(1){
		Json::Value msg= pMQ->getOneMSG();
		msg["type"]=msg["token0"].asString()+"."+msg["token1"].asString()+".resp";
		msg.removeMember("token0");
		msg.removeMember("token1");
		Json::Value reply_to = msg["reply_to"];
		if(reply_to.isNull()){
			//default key
			msg.removeMember("reply_to");
			Json::Value keyAppendix=msg["defaultKeyAppendix"];
			if(keyAppendix.isNull()){
				//no default key appendix
				if(!pMQ) Log::log.warning("RabbitMQSender: %s command has no default key Appendix\n",msg["type"].asCString());
			}else{
				//has default key appendix
				msg.removeMember("defaultKeyAppendix");
				std::string keyAppendixStr = keyAppendix.asString();
				chann.publish(conf.rabbitmq.exchange,getKey(conf,keyAppendixStr),msg.toStyledString());
			}
		}else{
			std::string reply_str =reply_to.asString();
			if(reply_str.empty()){
				Log::log.warning("RabbitMQAnalyser: invalid reply key:%s\n",reply_str.c_str());
				//reply to the default key
				Json::Value keyAppendix=msg["defaultKeyAppendix"];
				if(keyAppendix.isNull()){
					//no default key appendix
					if(!pMQ) Log::log.warning("RabbitMQSender: %s command has no default key Appendix\n",msg["type"].asCString());
				}else{
					//has default key appendix
					msg.removeMember("defaultKeyAppendix");
					std::string keyAppendixStr = keyAppendix.asString();
					chann.publish(conf.rabbitmq.exchange,getKey(conf, keyAppendixStr),msg.toStyledString());
				}

			}else{
				//reply to the key
				msg.removeMember("defaultKeyAppendix");
				chann.publish(conf.rabbitmq.exchange,reply_str,msg.toStyledString());
			}
		}
	}
}


struct rabbitmq_sender_temp_t {
	Conf* pConf;
	RabbitMQSender* pSender;
	MyMQ<Json::Value>* pMQ;
};
static void* rabbitMQSenderThread(void *ptr){
	rabbitmq_sender_temp_t & para = *((rabbitmq_sender_temp_t*)ptr);
	para.pSender->startSend(*para.pConf,para.pMQ);
	return NULL;
}
void RabbitMQSender::startSendAsThread(Conf & conf,MyMQ<Json::Value>* pMQ){
	pthread_t thread;
	static struct rabbitmq_sender_temp_t para;//avoid the variable lost when function finished
	para.pConf=&conf;
	para.pSender=this;
	para.pMQ=pMQ;
	pthread_create (&thread, NULL,rabbitMQSenderThread, (void*) &para);
	pthread_detach(thread);
}

} /* namespace SHS */
