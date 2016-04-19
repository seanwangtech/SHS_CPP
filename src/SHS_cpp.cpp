//============================================================================
// Name        : SHS_cpp.cpp
// Author      : Xiao Wang
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <unistd.h>

#include "SHScpp/SHS.h"
#include <sys/stat.h>
void test_rabbitReceive();
void test_serial();
void test_json();
void test_Regex();
void test_basicSystemWithNAT();
using namespace std;
// 守护进程初始化函数
void init_daemon()
{
    pid_t pid;
    int i = 0;

    if ((pid = fork()) == -1) {
        printf("Fork error !\n");
        exit(1);
    }
    if (pid != 0) {
        exit(0);        // 父进程退出
    }

    setsid();           // 子进程开启新会话，并成为会话首进程和组长进程
    if ((pid = fork()) == -1) {
        printf("Fork error !\n");
        exit(-1);
    }
    if (pid != 0) {
        exit(0);        // 结束第一子进程，第二子进程不再是会话首进程
    }
    chdir("/tmp");      // 改变工作目录
    umask(0);           // 重设文件掩码
    for (; i < getdtablesize(); ++i) {
       close(i);        // 关闭打开的文件描述符
    }

    return;
}
int main(int argc,char *argv[]){
	//init_daemon();//start daemon

	//test_rabbitReceive();

	//test_serial();
	//test_container();
	//test_json();
	//test_rabbitMQAnalyser();
	//test_ContainerMonitor();
	//test_ATAnalyser();
	//test_Regex();
	test_basicSystemWithNAT();
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
void test_json(){
	Json::Value root;
	Json::Reader reader;
	bool status = reader.parse("{\"abc\":2}",root,false);
	root["abd"]="abc";
	root["abd"]=5;
	root["dd"][0]=1;
	root["dd"][1]=2;
	root["dd"][2]=3;
	root["ee"].append(1);
	root["ee"].append(2);
	root["ee"].append(3);
	root["ee"].append(5);
	if(status){
		int a=root["abc"].asInt();
		std::cout<<root["abc"].asInt()<<"  int:"<<a<<std::endl;
		std::cout<<root["abd"]<<"  int:"<<root["abd"].asInt()<<std::endl;
		std::cout<<"root[\"dd\"] size is: "<<root["dd"].size()<<std::endl;
		std::cout<<"root[\"ee\"] size is: "<<root["ee"].size()<< root["ee"].toStyledString()<<std::endl;

	}else{
		cout<<"json parse error"<<endl;
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
void test_basicSystemWithNAT(){
	/*
	 * the bootstrap should include four sessions orderly!
	 * 1. configure the configuration file
	 * 2. start connections both to RabbitMQ and to serial port
	 * 3. create a container for the analyser as workspace
	 * 4. start analysers and monitors
	 */

	/****************************************************************************************************
	 * session 1. configure the configuration file
	 */
	//create configure object
	SHS::Conf &conf= *new SHS::Conf();
	conf.load("/etc/SHS/SHS.conf.json");

	/****************************************************************************************************
	 * session 2. start connections both to RabbitMQ and to serial port
	 */
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


	/****************************************************************************************************
	 * session 3. create a container for the analyser as workspace
	 */
	//creat a basic container for its monitor and two analysers
	SHS::Container container(conf);
	container.setRabbitMQSenderMQ(&rabbitSMQ);
	container.setSerialSenderMQ(&serialSMQ);
	container.setRabbitMQAnalyserMQ(&rabbitRMQ);

	/****************************************************************************************************
	 * session 4. start analysers and monitors
	 */
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
