//============================================================================
// Name        : SHS_cpp.cpp
// Author      : Xiao Wang
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <unistd.h>

#include "SHScpp/SHS.h"

void test_rabbitReceive();
void test_serial();
void test_container();
void test_json();
void test_rabbitMQAnalyser();
void test_ContainerMonitor();
void test_ATAnalyser();
void test_Regex();
void test_basicSystemWithoutNAT();
using namespace std;
int main(int argc,char *argv[]){

	//test_rabbitReceive();

	//test_serial();
	//test_container();
	//test_json();
	//test_rabbitMQAnalyser();
	//test_ContainerMonitor();
	//test_ATAnalyser();
	//test_Regex();
	test_basicSystemWithoutNAT();
/*
	pthread_t listen_thread,listen_thread1;
	rabbit_init_connect();
	init_rabbit_listen_thread(&listen_thread);
	init_serial_listen_thread(&listen_thread1);
	pthread_join(listen_thread, NULL);
	pthread_join(listen_thread1, NULL);
	*/

/*
    pthread_join(thread1, NULL);
    init_rabbit_listen_thread();

	//printf("the argc is :%d \n", argc);
	//printf("the argv is :%s \n", argv[0]);
	//printf("the argv is :%s \n", RABBITMQ_HOST);
	if(argc<ARG_LEN){

		printf("too less argument!!\n");
		//exit(0);
	}
	*/


}
void test_rabbitReceive(){
	SHS::Conf &conf= *new SHS::Conf();
	SHS::RabbitMQConnection conn(conf);
	SHS::RabbitMQReceiver receiver(conn);
	SHS::MyMQ<Json::Value> mq;
	receiver.startReceiveAsThread(conf,&mq);

	while(1){
		Json::Value root=mq.getOneMSG();
		std::cout<<root.toStyledString()<<std::endl;
	}
}

void test_serial(){
	string str;
		SHS::Conf* conf =new SHS::Conf();
		//SHS::SerialPort* serial=new SHS::SerialPort(conf->serialPort.port,conf->serialPort.baudrate);

		//cout<<conf->serialPort.baudrate <<"   "<<conf->serialPort.port<<endl;
		SHS::SerialPort &serial =*new SHS::SerialPort(*conf);
		serial.listenAsThread(NULL);
		while(1){
			cin>>str;
			serial.directSend(str);
		}
}
void test_container(){
	SHS::ClassFactoryInstance().Dump();
	SHS::Container container;
	string msg ="at command";
	container.getCmdObj(*new string("ZB_onoff"))->_onATReceive(msg);
}
void test_json(){
	Json::Value root;
	Json::Reader reader;
	bool status = reader.parse("{\"abc\":2}",root,false);
	root["abd"]="abc";
	root["abd"]=5;
	if(status){
		int a=root["abc"].asInt();
		std::cout<<root["abc"].asInt()<<"  int:"<<a<<std::endl;
		std::cout<<root["abd"]<<"  int:"<<root["abd"].asInt()<<std::endl;
	}else{
		cout<<"json parse error"<<endl;
	}

}
void test_rabbitMQAnalyser(){

	SHS::Conf &conf= *new SHS::Conf();
	SHS::RabbitMQConnection conn(conf);
	SHS::RabbitMQReceiver receiver(conn);
	SHS::MyMQ<Json::Value> mq;
	receiver.startReceiveAsThread(conf,&mq);
	SHS::Container container;
	SHS::RabbitMQAnalyser analyser(&container);
	analyser.startAnalyseAsThread(&mq);
	while(1){
		sleep(3);
		std::cout<<"the contiainer has "<<container.actCmds.size()<<" active commands"<<endl;
		if(container.actCmds.size()) {
			(*container.actCmds.begin())->_onTimeOut();
			container.delActCmd(*container.actCmds.begin());
		}
	}
}
void test_ContainerMonitor(){
	//create configure object
	SHS::Conf &conf= *new SHS::Conf();

	//create a rabbitmq connection
	SHS::RabbitMQConnection conn(conf);

	//start rabbiMQ receiver based on the connection and send the received message to a self-defined Message Queue
	SHS::RabbitMQReceiver receiver(conn);
	SHS::MyMQ<Json::Value> mq;
	receiver.startReceiveAsThread(conf,&mq);

	//creat a basic container for its monitor and the rabbitMQ analyser
	SHS::Container container;

	//start the rabbitMQ analyser
	SHS::RabbitMQAnalyser analyser(&container);
	analyser.startAnalyseAsThread(&mq);

	//start the container monitor
	SHS::ContainerMonitor monitor(container);
	monitor.startMonitorAsThread();

	//main thread sleep
	while(1){
		sleep(3);

	}
}

void test_ATAnalyser(){
	//create configure object
	SHS::Conf &conf= *new SHS::Conf();

	//create a rabbitmq connection
	SHS::RabbitMQConnection conn(conf);

	//start rabbiMQ receiver based on the connection and send the received message to a self-defined Message Queue
	SHS::RabbitMQReceiver receiver(conn);
	SHS::MyMQ<Json::Value> mq;
	receiver.startReceiveAsThread(conf,&mq);

	//creat a basic container for its monitor and two analysers
	SHS::Container container;

	//start the rabbitMQ analyser
	SHS::RabbitMQAnalyser analyser(&container);
	analyser.startAnalyseAsThread(&mq);

	//start the container monitor
	SHS::ContainerMonitor monitor(container);
	monitor.startMonitorAsThread();

	//Create a MyMQ for serial port and ATAnalyser
	SHS::MyMQ<string> serial_mq;
	//start serial port according to the configure
	SHS::SerialPort serial(conf);
	serial.listenAsThread(&serial_mq);
	//start AT analyser
	SHS::ATAnalyser atAnalyser(container);
	atAnalyser.startAnalyseAsThread(&serial_mq);

	//main thread sleep
	while(1){
		string str;
		cin>>str;
		serial.directSend(str);
	}
}
void test_Regex(){

    std::string text("abc abd");
    boost::regex regex("ab(.)");

    boost::sregex_iterator iter(text.begin(), text.end(), regex);
    boost::sregex_iterator end;

    for( ; iter != end; ++iter ) {
    	//boost::smatch const &what = *iter;
        std::cout<<(*iter)[1]<<'\n';
    }

}
void test_basicSystemWithoutNAT(){
	//create configure object
	SHS::Conf &conf= *new SHS::Conf();

	/*
	 * start rabbiMQ receiver and sender based on the connection and send the received message to a self-defined Message Queue
	 * according to the rabbitMQ, we can use different channels from one connection in different thread.
	 * however, due to the bug of the rabbitmq-C bug, we cannot use multi-channels in different thread from one channel,
	 * so we have to create two connections for receiver and sender respectively.
	 * I try to create only one channel, however, it tend to die when sender gets channel from the connection occasionally, so I change it to use two connections.
	 *
	 */

	//create a rabbitmq connection for receiver
	SHS::RabbitMQConnection connR(conf);
	SHS::RabbitMQReceiver receiver(connR);
	SHS::MyMQ<Json::Value> rabbitRMQ;
	receiver.startReceiveAsThread(conf,&rabbitRMQ);

	//create a rabbitmq connection for sender
	SHS::RabbitMQConnection connS(conf);
	SHS::RabbitMQSender sender(connS);
	SHS::MyMQ<Json::Value> rabbitSMQ;
	sender.startSendAsThread(conf,&rabbitSMQ);


	//Create a MyMQ for serial port and ATAnalyser
	//start serial port according to the configure
	SHS::MyMQ<string> serialLMQ;
	SHS::SerialPort serial(conf);
	serial.listenAsThread(&serialLMQ);
	SHS::MyMQ<string> serialSMQ;
	serial.startSenderAsThread(&serialSMQ);


	//creat a basic container for its monitor and two analysers
	SHS::Container container;
	container.setConf(&conf);
	container.setLookup(NULL);
	container.setRabbitMQSenderMQ(&rabbitSMQ);
	container.setSerialSenderMQ(&serialSMQ);
	container.setRabbitMQAnalyserMQ(&rabbitRMQ);

	//start the rabbitMQ analyser
	SHS::RabbitMQAnalyser analyser(&container);
	analyser.startAnalyseAsThread(&rabbitRMQ);

	//start the container monitor
	SHS::ContainerMonitor monitor(container);
	monitor.startMonitorAsThread();

	//start AT analyser
	SHS::ATAnalyser atAnalyser(container);
	atAnalyser.startAnalyseAsThread(&serialLMQ);

	//main thread sleep
	while(1){
		sleep(100);
	}
}
