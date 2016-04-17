/*
 * rabbitMQConnection.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#include "rabbitMQConnection.h"
#include "log.h"
namespace SHS {
Channel::Channel(RabbitMQConnection& conn){
	this->channel_n = conn._getChannel_n();
	this->conn_ptr=&conn;
	this->queue_name.clear();
	amqp_channel_open(conn._getconn_n(), channel_n);
	die_on_amqp_error(amqp_get_rpc_reply(conn._getconn_n()), "Opening channel_n");
}
Channel::~Channel(){
	die_on_amqp_error(amqp_channel_close(this->conn_ptr->_getconn_n(),
			channel_n, AMQP_REPLY_SUCCESS), "Closing channel");
}

void Channel::publish(const std::string& exchange,const std::string& routing_key,
		const std::string& mesg ){
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 1; /* transient delivery mode */
	die_on_error(amqp_basic_publish(this->conn_ptr->_getconn_n(),
	   								this->channel_n,
	                                amqp_cstring_bytes(exchange.c_str()),
	                                amqp_cstring_bytes(routing_key.c_str()),
	                                0,
	                                0,
									&props,
		                            amqp_cstring_bytes(mesg.c_str())),
		                 "Publishing");
}

void Channel::publish(const std::string& routing_key,const std::string& mesg ){
	this->conn_ptr->getDefaultExchange().c_str();
	this->publish(this->conn_ptr->getDefaultExchange().c_str(),routing_key,mesg);
}

void Channel::binding(const std::string& routing_key){
	if(this->queue_name.empty()){
		amqp_queue_declare_ok_t *r = amqp_queue_declare(this->conn_ptr->_getconn_n(),
				this->channel_n, amqp_empty_bytes, 0, 0, 1, 1,amqp_empty_table);
		die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "Declaring queue");
		amqp_bytes_t listen_quque = amqp_bytes_malloc_dup(r->queue);
		if (listen_quque.bytes == NULL) {
			Log::log.error("Out of memory while copying queue name");
		}
		char str[listen_quque.len+1];
		for(unsigned int i=0;i<listen_quque.len;i++){
			str[i]=((char*)listen_quque.bytes)[i];
		}
		str[listen_quque.len]='\0';
		this->queue_name=std::string(str);
	}
	//binding
	amqp_queue_bind(this->conn_ptr->_getconn_n(), this->channel_n,
				amqp_cstring_bytes(this->queue_name.c_str()),
			    amqp_cstring_bytes(this->conn_ptr->getDefaultExchange().c_str()),
			    amqp_cstring_bytes(routing_key.c_str()),
			    amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "Binding");
}

void Channel::unbinding(const std::string& routing_key){
	if(this->queue_name.empty()) return;
	amqp_queue_unbind(this->conn_ptr->_getconn_n(), this->channel_n,
			amqp_cstring_bytes(this->queue_name.c_str()),
		    amqp_cstring_bytes(this->conn_ptr->getDefaultExchange().c_str()),
		    amqp_cstring_bytes(routing_key.c_str()),
		    amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "Unbinding");
}

void Channel::listen(Callable_envelope& callback){
	amqp_connection_state_t conn = this->conn_ptr->_getconn_n();
		//open channel
		//amqp_channel_open(conn, channel);
		//die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
		//set consumer
	amqp_basic_consume(conn,
			this->channel_n,
			amqp_cstring_bytes(this->queue_name.c_str()),
			amqp_empty_bytes, 0, 1, 0, amqp_empty_table);//auto ack consumer
	die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "rabbitmq_listen Consuming");


		{
			  while (1) {
			      amqp_rpc_reply_t res;
			      amqp_envelope_t envelope;

			      amqp_maybe_release_buffers(conn);

			      res = amqp_consume_message(conn, &envelope, NULL, 0);

			      if (AMQP_RESPONSE_NORMAL != res.reply_type) {
			        //break;
			    	  Log::log.error("Channel::listen, AMQP response abnormal");
			    	  continue;
			      }
			      callback.callback(envelope);
			      amqp_destroy_envelope(&envelope);
			 }
		}
		die_on_amqp_error(amqp_channel_close(conn, this->channel_n, AMQP_REPLY_SUCCESS), "Closing channel");
}
/*
static Channel::listen_callback_string_t callback=NULL;
void listen_envelope_to_string(const amqp_envelope_t& envelope);
void Channel::listen_envelope_to_string(const amqp_envelope_t& envelope){
	char str[envelope.message.body.len+1];
	for(unsigned int i=0;i<envelope.message.body.len;i++){
	 	  str[i]= ((char*)envelope.message.body.bytes)[i];
	}
	str[envelope.message.body.len] ='\0';//'\0' finish the string
	std::string str_cpp=std::string(str);
	if(callback!=NULL) callback(str_cpp);
}*/

void Channel::listen(Callable_string& callback){
	class EnvelopeCallback:public Callable_envelope{
	public:
			void callback(const amqp_envelope_t& envelope){
				char str[envelope.message.body.len+1];
					for(unsigned int i=0;i<envelope.message.body.len;i++){
					 	  str[i]= ((char*)envelope.message.body.bytes)[i];
					}
					str[envelope.message.body.len] ='\0';//'\0' finish the string
					std::string str_cpp=std::string(str);
					if(this->callable_str!=NULL) this->callable_str->callback(str_cpp);
			}
			Channel::Callable_string *callable_str;
	};
	EnvelopeCallback callback_envelopeToString;

	callback_envelopeToString.callable_str=&callback;
	listen(callback_envelopeToString);
}
void Channel::listen(Callback_envelope_t callback){
	class Callable: public Callable_envelope{
	public:
		void callback(const amqp_envelope_t& envelope){
			f(envelope);
		}
		Callback_envelope_t f;
	};
	Callable foo;
	foo.f = callback;
	listen(foo);
}
void Channel::listen(Callback_string_t callback){
	class Callable: public Callable_string{
	public:
		void callback(const std::string& string){
			f(string);
		}
		Callback_string_t f;
	};
	Callable foo;
	foo.f = callback;
	listen(foo);
}
RabbitMQConnection::RabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
		const std::string& user, const std::string& password) :
		conn(NULL),
		channel_n(0){
	this->default_exchange="SHS";
	this->_rabbitMQConnection(hostname,port,vhost,user,password);
}
RabbitMQConnection::RabbitMQConnection(const SHS::Conf& conf):
	conn(NULL),
	channel_n(0){
	//ninglvfeihong wait check
	this->_rabbitMQConnection(conf.rabbitmq.hostname,conf.rabbitmq.port,conf.rabbitmq.vhost
			,conf.rabbitmq.user,conf.rabbitmq.password);
	this->default_exchange=conf.rabbitmq.exchange;
}
RabbitMQConnection::~RabbitMQConnection() {
}
Channel RabbitMQConnection::getChannel(){
	Channel channel(*this);
	return channel;
}
amqp_channel_t RabbitMQConnection::getOneChannel(){
	channel_n++;
	return channel_n;
}
amqp_channel_t RabbitMQConnection::_getChannel_n() {
	return this->getOneChannel();
}
amqp_connection_state_t RabbitMQConnection::_getconn_n()const{
	return this->conn;
}

void RabbitMQConnection::_rabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
		const std::string& user, const std::string& password) {
	this->channel_n=0;
	this->conn = amqp_new_connection();
	amqp_socket_t * socket = amqp_tcp_socket_new(conn);
	if (!socket) {
		die("creating TCP socket");
	}
	int status = amqp_socket_open(socket, hostname.c_str(), port);
	if (status) {
		die("opening TCP socket");
	}
	die_on_amqp_error(amqp_login(this->conn, vhost.c_str(), 0, 131072, 60, AMQP_SASL_METHOD_PLAIN, user.c_str(), password.c_str()),
			                    "Logging in");
}
} /* namespace SHS */
