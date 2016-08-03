/*
 * ECmd.h
 *
 *  Created on: 24 Mar 2016
 *      Author: Xiao WANG
 */

#ifndef SHSCPP_CMD_ECMD_H_
#define SHSCPP_CMD_ECMD_H_
#include "cmd.h"
namespace SHS {

SHS_CMD_CLASS_CREATE(ZB_onoff,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
});

SHS_CMD_CLASS_CREATE(ZB_init_discover,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
private:
		bool isResponded;
});


SHS_CMD_CLASS_CREATE(ZB_update,{
public:
		ZB_update():update_id(0){};
		//void onRabbitMQReceive();
		void onTimeOut(){};
		void onATReceive();
		void onRabbitMQReceive();
private:
		unsigned int update_id;
});

SHS_CMD_CLASS_CREATE(ZB_read,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
});

SHS_CMD_CLASS_CREATE(ZB_set,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int value;
		ZB_set():value(-1){};
});

SHS_CMD_CLASS_CREATE(ZB_pjoin,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
});

SHS_CMD_CLASS_CREATE(ZB_discover,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
private:
		Json::Value data;
});

SHS_CMD_CLASS_CREATE(ZB_AT,{
public:
		void onRabbitMQReceive();
		void onTimeOut(){};
});

SHS_CMD_CLASS_CREATE(ZB_CSLock,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
});
template<typename T> struct template_argument_type;
template<typename T, typename U> struct template_argument_type<T(U)> { typedef U type; };
#define TEM_DEFINE(t,name) template_argument_type<void(t)>::type name
SHS_CMD_CLASS_CREATE(ZB_update_deactive,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		static TEM_DEFINE((std::map<int,int>),activeMap);
		static const int DEAD_PERIOD=12;//mins , if no update in this period will be taken as a deactive device
		static void renewDev(int NWK);
});

SHS_CMD_CLASS_CREATE(ZB_IR,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int MsgType;
		ZB_IR():MsgType(-1){};
});


} /* namespace SHS */

#endif /* SHSCPP_CMD_ECMD_H_ */
