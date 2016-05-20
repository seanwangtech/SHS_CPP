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

} /* namespace SHS */

#endif /* SHSCPP_CMD_ECMD_H_ */
