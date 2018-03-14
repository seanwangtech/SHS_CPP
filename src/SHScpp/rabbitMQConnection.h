/*
 * rabbitMQConnection.h
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_RABBITMQCONNECTION_H_
#define SHSCPP_RABBITMQCONNECTION_H_
#include "conf.h"
#include <pthread.h>
#include <unistd.h>
#include <list>
namespace SHS {
class Channel;
class RabbitMQConnection;

class Channel{
public:
	Channel(RabbitMQConnection& conn);
	virtual ~Channel();
	void publish(const std::string& exchange,const std::string& routing_key,const std::string& mesg );
	void publish(const std::string& routing_key,const std::string& mesg );
	void addBinding(const std::string& routing_key);
	void delBinding(const std::string& routing_key);

	class Callable_string{
	public:
		virtual ~Callable_string(){};
		virtual void callback(const std::string& string)=0;
	};

	class Callable_envelope{
	public:
		virtual ~Callable_envelope(){};
		virtual void callback(const amqp_envelope_t& envelope)=0;
	};
	typedef void (*Callback_string_t)(const std::string& string);
	typedef void (*Callback_envelope_t)(const amqp_envelope_t& envelope);
	void listen(Callable_string& callback);
	void listen(Callable_envelope& callback);
	void listen(Callback_string_t callback);
	void listen(Callback_envelope_t callback);
	int heartbeat_send(const char * info);

private:
	amqp_channel_t channel_n;
	RabbitMQConnection * conn_ptr;
	std::string queue_name;
	std::list<std::string> bindings;
	void binding();
	void unbinding(const std::string& routing_key);
};

class RabbitMQConnection {
public:
	RabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
			const std::string& user, const std::string& password);

	RabbitMQConnection(const SHS::Conf& conf);
	virtual ~RabbitMQConnection();
	Channel getChannel();
	amqp_channel_t _getChannel_n();
	amqp_connection_state_t _getconn_n ()const;
	std::string getDefaultExchange() const{return this->default_exchange;};
	void reconnnect();
protected:
	amqp_channel_t getOneChannel();
private:
	amqp_connection_state_t conn;
	amqp_channel_t channel_n;
	std::string default_exchange;
	std::string hostname;
	int port;
	std::string vhost;
	std::string user;
	std::string password;
	void _rabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
				const std::string& user, const std::string& password);
};

} /* namespace SHS */

#endif /* SHSCPP_RABBITMQCONNECTION_H_ */
