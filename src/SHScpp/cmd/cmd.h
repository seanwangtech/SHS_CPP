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
	virtual void onRabbitMQReceive()=0;//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	virtual void onTimeOut()=0;//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	virtual void onATReceive()=0;//this function need be overrided by extended command, and the container is locked for this function, so the function can full use the container
	void waitCmdFinish(pthread_mutex_t * pMutex);
	void cmdFinish();
	void setContainer(Container * container);
	void _onRabbitMQReceive(Json::Value & Msg);
	void _onATReceive(std::string& Msg);
	void _onTimeOut();
	void sendRMsg(std::string& defaultKeyAppendix);
	void sendATCmd(std::string& ATCmd);
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



} /* namespace SHS */

//below include cannot be place into SHS name space. should put outside SHS name space
#include "ECmd.h"
#endif /* SHSCPP_CMD_CMD_H_ */
