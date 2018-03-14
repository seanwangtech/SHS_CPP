/*
 * rabbitMQConnection.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */
#include "rabbitMQConnection.h"
#include "log.h"
#include <sys/time.h>
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
	int x = amqp_basic_publish(this->conn_ptr->_getconn_n(),
		   								this->channel_n,
		                                amqp_cstring_bytes(exchange.c_str()),
		                                amqp_cstring_bytes(routing_key.c_str()),
		                                0,
		                                0,
										&props,
			                            amqp_cstring_bytes(mesg.c_str()));
	if(x<0){
		label_reconnet:
		//meaning error occur
		Log::log.warning("%s: %s\n", "publishing", amqp_error_string2(x));
		Log::log.warning("Channel::publish: Reconnecting to RabbitMQ\n");

		//first close all the things
		amqp_channel_close(this->conn_ptr->_getconn_n(), this->channel_n, AMQP_REPLY_SUCCESS);
		amqp_connection_close(this->conn_ptr->_getconn_n(), AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(this->conn_ptr->_getconn_n());

		//reconnect to rabbitMQ server!
		this->conn_ptr->reconnnect();
		amqp_channel_open(this->conn_ptr->_getconn_n(), channel_n);
		if(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()).reply_type !=AMQP_RESPONSE_NORMAL){
			Log::log.warning("Channel::publish:Opening channel_n error\n");
		}else{
			x = amqp_basic_publish(this->conn_ptr->_getconn_n(),
													this->channel_n,
													amqp_cstring_bytes(exchange.c_str()),
													amqp_cstring_bytes(routing_key.c_str()),
													0,
													0,
													&props,
													amqp_cstring_bytes(mesg.c_str()));
		}
		if(x<0){
			sleep(2);
			goto label_reconnet;
		}

	}

	//die_on_error(x, "Publishing");
}

void Channel::publish(const std::string& routing_key,const std::string& mesg ){
	this->conn_ptr->getDefaultExchange().c_str();
	this->publish(this->conn_ptr->getDefaultExchange().c_str(),routing_key,mesg);
}

void Channel::addBinding(const std::string& routing_key){
	this->bindings.push_front(routing_key);
}

void Channel::delBinding(const std::string& routing_key){
	//string overrided the "==" operator, so it will automaticly delete the string has that value, rather than by comparing object point
	this->bindings.remove(routing_key);
}

void Channel::binding(){
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
	//binding
	std::list<std::string>::iterator it = this->bindings.begin();
	for(;it!=this->bindings.end();it++){
		amqp_queue_bind(this->conn_ptr->_getconn_n(), this->channel_n,
				amqp_cstring_bytes(this->queue_name.c_str()),
			    amqp_cstring_bytes(this->conn_ptr->getDefaultExchange().c_str()),
			    amqp_cstring_bytes((*it).c_str()),
			    amqp_empty_table);
		die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "Binding");
	}
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
	struct timeval timeout;
	timeout.tv_usec=0;
	timeout.tv_sec=0;
	const int heartBitSendInterval = 10;
	time_t t, previous_send = time(NULL);
	Log::log.debug("Channel::listen, out binding...\n");
	this->binding();
	Log::log.debug("Channel::listen, out binding, OK...\n");
	amqp_basic_consume(conn,
			this->channel_n,
			amqp_cstring_bytes(this->queue_name.c_str()),
			amqp_empty_bytes, 0, 1, 0, amqp_empty_table);//auto ack consumer
	die_on_amqp_error(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()), "rabbitmq_listen Consuming");

		{
			  while (1) {
			      amqp_rpc_reply_t res;
			      res.reply_type=AMQP_RESPONSE_NONE;
			      amqp_envelope_t envelope;

			      amqp_maybe_release_buffers(conn);
			      t=time(NULL);
			      if(t==(time_t)-1){
			    	  //get time error
			    	  timeout.tv_sec = heartBitSendInterval;
			      }else{
			    	  int duration = t-previous_send;
			    	  if(duration<0){
			    		  //indicate previous time error
			    		  timeout.tv_sec = heartBitSendInterval;
			    		  previous_send =t;
			    	  }else if(duration<heartBitSendInterval){
			    		  timeout.tv_sec = heartBitSendInterval - duration;
			    	  }else{
			    		  //need to send heartbeat and reset previous send time;
			    		  timeout.tv_sec = heartBitSendInterval;
			    		  previous_send =t;
			    		  //is normal time out, so try send heartbeat make sure the connection is OK, and maintain connection with server.
			    		  if(this->heartbeat_send("ReceiverP1")!=AMQP_STATUS_OK){
			    			  goto label_listen_reconnet;
			    		  }
			    	  }
			      }
			      res = amqp_consume_message(conn, &envelope, &timeout, 0);

			      if (AMQP_RESPONSE_NORMAL != res.reply_type) {
			    	  //may be normal time out, so try send heartbeat make sure the connection is OK, and maintain connection with server.
			    	  if(this->heartbeat_send("ReceiverP2")!=AMQP_STATUS_OK){
			    		  goto label_listen_reconnet;
			    	  }else{
			    		  previous_send = time(NULL);
			    		  continue;
			    	  }

			    	  label_listen_reconnet:
			    	  Log::log.warning("Channel::listen, AMQP response abnormal:%d\n",res.reply_type);
			    	  Log::log.warning("Channel::listen, reconnecting...\n");
			    	  //first close previous
			    	  amqp_channel_close(this->conn_ptr->_getconn_n(), this->channel_n, AMQP_REPLY_SUCCESS);
			    	  Log::log.debug("Channel::listen, conn closing...\n");
			    	  amqp_connection_close(this->conn_ptr->_getconn_n(), AMQP_REPLY_SUCCESS);
			    	  Log::log.debug("Channel::listen, destroy conn ...\n");
			    	  amqp_destroy_connection(this->conn_ptr->_getconn_n());

			    	  //reconnect to rabbitMQ server!
			    	  Log::log.debug("Channel::listen, start reconnecting ...\n");
			    	  this->conn_ptr->reconnnect();

			    	  //reassign the conn to the new created connection
			    	  conn =this->conn_ptr->_getconn_n();
			    	  Log::log.debug("Channel::listen, channel opening...\n");
			    	  amqp_channel_open(this->conn_ptr->_getconn_n(), channel_n);
			    	  //rebind
			    	  Log::log.debug("Channel::listen, start binding ...\n");
			    	  this->binding();
			    	  //reset the consumer
			    	  Log::log.debug("Channel::listen, reset consumer ...\n");
			    	  amqp_basic_consume(conn,
			    				this->channel_n,
			    				amqp_cstring_bytes(this->queue_name.c_str()),
			    				amqp_empty_bytes, 0, 1, 0, amqp_empty_table);//auto ack consumer

			    	  Log::log.debug("Channel::listen, get_rpc_reply...\n");
			    	  if(amqp_get_rpc_reply(this->conn_ptr->_getconn_n()).reply_type !=AMQP_RESPONSE_NORMAL){
			    	  	Log::log.warning("Channel::listen:Opening channel_n error\n");
			    	  	sleep(2);
			    	  	goto label_listen_reconnet;
			    	  }else{
				    	  Log::log.debug("Channel::listen, connection recovered...\n");
			    		  continue;
			    	  }
			      }
			      Log::log.debug("Channel::listen:get a rabbitMQ message\n");
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

int Channel::heartbeat_send(const char* info){
    amqp_frame_t heartbeat;
    heartbeat.channel = 0; //channel nubmer must be 0 according to the AMQP specification
    heartbeat.frame_type = AMQP_FRAME_HEARTBEAT;

    int res = amqp_send_frame(this->conn_ptr->_getconn_n(), &heartbeat);
    if (AMQP_STATUS_OK == res) {
    	Log::log.debug("[%s] Channel::heartbeat_send OK:%d\n",info,res);
    }else{
    	Log::log.debug("[%s] Channel::heartbeat_send not OK:%d\n",info,res);
    }
    return res;
}

RabbitMQConnection::RabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
		const std::string& user, const std::string& password) :
		conn(NULL),
		channel_n(0){
	this->hostname =hostname;
	this->port = port;
	this->vhost = vhost;
	this->user = user;
	this->password=password;
	this->default_exchange="SHS";
	this->_rabbitMQConnection(hostname,port,vhost,user,password);
}
RabbitMQConnection::RabbitMQConnection(const SHS::Conf& conf):
	conn(NULL),
	channel_n(0){
	//ninglvfeihong wait check
	this->hostname = conf.rabbitmq.hostname;
	this->port = conf.rabbitmq.port;
	this->vhost = conf.rabbitmq.vhost;
	this->user = conf.rabbitmq.user;
	this->password=conf.rabbitmq.password;
	this->default_exchange=conf.rabbitmq.exchange;
	this->_rabbitMQConnection(hostname,port,vhost,user,password);
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
void RabbitMQConnection::reconnnect(){
	this->_rabbitMQConnection(hostname,port,vhost,user,password);
}

amqp_channel_t RabbitMQConnection::_getChannel_n() {
	return this->getOneChannel();
}
amqp_connection_state_t RabbitMQConnection::_getconn_n()const{
	return this->conn;
}

void RabbitMQConnection::_rabbitMQConnection(const std::string& hostname, int port, const std::string& vhost,
		const std::string& user, const std::string& password) {
	while (true){
		this->channel_n=0;
		this->conn = amqp_new_connection();
		amqp_socket_t * socket = amqp_tcp_socket_new(conn);
		if (!socket) {
			die("creating TCP socket");
		}
		Log::log.debug("Channel::listen, amqp_socket_open...\n");
		int status = amqp_socket_open(socket, hostname.c_str(), port);
		if (status) {
			amqp_connection_close(this->conn,AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(this->conn);
			Log::log.warning("RabbitMQConnection::_rabbitMQConnection fail to open TCP socket, retrying...\n");
			sleep (10);
			continue;
		}

		amqp_rpc_reply_t login_status;
		Log::log.debug("Channel::listen, amqp_login...\n");
		login_status = amqp_login(this->conn, vhost.c_str(), 0, 131072, 25, AMQP_SASL_METHOD_PLAIN, user.c_str(), password.c_str());
		if(login_status.reply_type == AMQP_RESPONSE_NORMAL) {
			Log::log.debug("Channel::listen, amqp_login OK...\n");

			break;
		}
		else {
			Log::log.warning("RabbitMQConnection::_rabbitMQConnection: error %d: logging in fail, retrying ...\n",login_status.reply_type);
			amqp_connection_close(this->conn,AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(this->conn);
			sleep (1);
			continue;
		}

	}
}
} /* namespace SHS */
