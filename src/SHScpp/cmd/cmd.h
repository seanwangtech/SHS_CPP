/*
 * cmd.h
 *
 *  Created on: 21 Mar 2016
 *      Author: pupu
 */

#ifndef SHSCPP_CMD_CMD_H_
#define SHSCPP_CMD_CMD_H_
#include "classFactory.h"
#include <pthread.h>
#include <list>
#include "json/json.h"
#include <boost/regex.hpp>
//ninglvfeihong:below declare is important for include eachother
namespace SHS{
class Cmd;
class ActCmd;
}
#include "../container.h"
namespace SHS{
class Cmd {
public:
	Cmd();
	virtual ~Cmd();
	virtual void onRabbitMQReceive(){};//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	virtual void onTimeOut()=0;//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	virtual void onATReceive(){};//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	void waitCmdFinish(pthread_mutex_t * pMutex);
	void cmdFinish();
	void setContainer(Container * container);
	void _onRabbitMQReceive(Json::Value & Msg);
	void _onATReceive(std::string& Msg);
	void _onTimeOut();
	void sendRMsg(std::string& defaultKeyAppendix);
	void sendRMsg(const char* defaultKeyAppendix);
	void sendATCmd(std::string& ATCmd);
	void sendATCmd(const char* ATCmd);
	void sendToRabbitMQAnalyser(Json::Value & Msg);
	void sendToRabbitMQAnalyser(std::string & cmdType);
	void sendToRabbitMQAnalyser(const char* cmdType);
	std::string& getATMsg();
	Json::Value & getRabbitMQMsg();
	void setTTL(int ms);
	int _cmdTTL;
	const static int CMD_FINISHED_TTL_MARK=-9999;
	const static int CMD_FOREVER_TTL_MARK=-9998;
protected:
	pthread_mutex_t * pMutex;
	Container* container;
	Json::Value rabbitMQMesg;
	std::string ATMsg;
};
class ActCmd{
public:
	void reg();
private:
	std::list<Cmd*> actCmds;
};
};

namespace SHS{
#define SHS_CMD_CLASS_CREATE(class_name,class_defination) 	\
		class class_name: public Cmd class_defination;		\
		static void* CreateClass##class_name ();			\
		static void* CreateClass##class_name (){	   	 	\
			static class_name obj;							\
			return (void*)(&obj);							\
		};											    	\
		static bool _##class_name##_Unused __attribute__((unused))= ClassFactoryInstance().AddObject(#class_name, CreateClass##class_name);

}

namespace SHS{

/*
 * this class is dedicated for initial the special command object
 * this class is not invoked by Analyser, thus some functionality of this command object will be restricted.
 * this small inconvenient feature may be fixed in the future, but currently:
 * Please do not use below function:
 * 	this->sendATCmd(): In current AT command version, we cannot send next AT command before the previous Cmd obj is fully tackled.
 * 		thus UART sender is exclusively managed by an Analyser which initialise a Cmd Obj and active it and other thread will do below tasks:
 * 				monitors:monitor the time execution (active) time of this command (onTimeOut)
 * 				ANAnalyser: receive UART reply and invoked onATReceive function in this Cmd Obj
 *
 *
 * In a word: just take this class as a normal initial process rather than a Cmd Obj!!!!!!!!!
 *
 * In this class:
 * this->sendToRabbitMQAnalyser() is highly recommended when want to using serial port. You may create a Cmd object and deal with UART relate thing in it.
 * this->sendToRabbitMQAnalyser() also allows re-analyse a message come from rabbitMQ server
 */

SHS_CMD_CLASS_CREATE(Delay_init_cmdobj,{
public:
		void onTimeOut();
});


} /* namespace SHS */

//below include cannot be place into SHS name space. should put outside SHS name space
#include "ECmd.h"
#endif /* SHSCPP_CMD_CMD_H_ */
