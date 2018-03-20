/*
 * conf.h
 *
 *  Created on: 20 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_CONF_H_
#define SHSCPP_CONF_H_
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <utils.h>
#include <string>
namespace SHS {
//configure for user configure
class Conf {
public:
	Conf();
	virtual ~Conf();
	struct rabbitmq_t{
		std::string hostname;
		int port;
		std::string vhost;
		std::string user;
		std::string password;
		std::string exchange;
	} rabbitmq;
	enum AP_status_t{
		pre_init,
		running
	};
	struct home_t{
		std::string id;
		std::string MAC;
		AP_status_t AP_status;
	} home;
	struct {
		std::string port;
		unsigned int baudrate;
	}serialPort;
	struct{
		std::string lookup_table;
		std::string binding_table;
	} tables_path;
	std::string logfile;
	int debugLevel;
	bool load(std::string &path);
	bool load(const char* path);
	friend class ZB_init_readsn;
};

} /* namespace SHS */

#endif /* SHSCPP_CONF_H_ */
