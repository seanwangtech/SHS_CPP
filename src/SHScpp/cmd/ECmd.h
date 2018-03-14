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
		int EP;
		int NWK_addr;
		bool IsNetworkTranslated;
		int retry;
});

SHS_CMD_CLASS_CREATE(ZB_init_discover,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int retry;
private:
		bool isResponded;
});

SHS_CMD_CLASS_CREATE(ZB_init_readsn,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int retry;
private:
		bool isResponded;
});

SHS_CMD_CLASS_CREATE(ZB_update,{
public:
		//void onRabbitMQReceive();
		void onTimeOut(){};
		void onATReceive();
		void onRabbitMQReceive();
		static unsigned int inline getUpdateId();
private:
		static unsigned int update_id;
});

SHS_CMD_CLASS_CREATE(ZB_read,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int retry;
});

SHS_CMD_CLASS_CREATE(ZB_set,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int value;
		int retry;
		ZB_set():value(-1){retry =0;};
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
		void onTimeOut();
		void onATReceive();
});

SHS_CMD_CLASS_CREATE(ZB_CSLock,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		bool isBinded;
		int retry;
		ZB_CSLock():isBinded(false){retry =0;};
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
		int retry ;
private:
		std::string toHexStr(int nubmer);
});


SHS_CMD_CLASS_CREATE(AMQPHeartBeat,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_keepAlive,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

enum Bind_state{INIT=0,READY,IN_PROCESS};
SHS_CMD_CLASS_CREATE(ZB_binding_server,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		typedef struct{
			std::string MAC;
			int EP;
			char IO;
		}Bind_unit;
		static std::list<Bind_unit> binding1;
		static std::list<Bind_unit> binding2;
		static Bind_state bind_state;
private:
		void addBindings();
});

SHS_CMD_CLASS_CREATE(ZB_bind_onoff,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
		void onATReceive();
		int retry;
});

SHS_CMD_CLASS_CREATE(AP_uci,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_get_ifconfig,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_upgrade,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_upgrade_service1,{
public:
		int stage;
		int fileLength;
		std::string url;
		std::string md5;
		void onRabbitMQReceive();
		void onTimeOut();
		AP_upgrade_service1();
});

SHS_CMD_CLASS_CREATE(AP_get_version,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_get_log,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

SHS_CMD_CLASS_CREATE(AP_discover,{
public:
		void onRabbitMQReceive();
		void onTimeOut();
});

} /* namespace SHS */

#endif /* SHSCPP_CMD_ECMD_H_ */
