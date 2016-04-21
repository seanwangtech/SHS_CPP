/*
 * serialPort.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#include "serialPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "log.h"



namespace SHS {

#define MIN_DATA_LEN 255
#define WAIT_TIME 1 //unit is every 100 ms
using std::string;
SerialPort::SerialPort(Conf &conf):fd(0),isOpen(false),pLMQ(NULL){
		port=conf.serialPort.port;
		baudrate=conf.serialPort.baudrate;
		this->openPort();
}
SerialPort::SerialPort(std::string &port,unsigned int baudrate):fd(0),isOpen(false),pLMQ(NULL){
	this->port=port;
	this->baudrate=baudrate;
	this->openPort();
}
/***************************************************************************
 * Function get_baudRate(*) takes the baud rate as an integer, converts it 	*
 * into speed_t format, and then returns it.								*
 ***************************************************************************/
static speed_t get_baudRate(int baudRate) {
	speed_t BAUD;

	switch (baudRate) {
	case 38400:
	default:
		BAUD = B38400;
		break;
	case 115200:
		BAUD = B115200;
		break;
	case 19200:
		BAUD = B19200;
		break;
	case 9600:
		BAUD = B9600;
		break;
	case 4800:
		BAUD = B4800;
		break;
	case 2400:
		BAUD = B2400;
		break;
	case 1800:
		BAUD = B1800;
		break;
	case 1200:
		BAUD = B1200;
		break;
	case 600:
		BAUD = B600;
		break;
	case 300:
		BAUD = B300;
		break;
	case 200:
		BAUD = B200;
		break;
	case 150:
		BAUD = B150;
		break;
	case 134:
		BAUD = B134;
		break;
	case 110:
		BAUD = B110;
		break;
	case 75:
		BAUD = B75;
		break;
	case 50:
		BAUD = B50;
		break;
	} //end of switch baud_rate

	return BAUD;
}

void SerialPort::openPort(){
	fd = 0;	 /* File descriptor for the port */
	struct termios oldtio,newtio;

		/* Open modem device for reading and writing and not as controlling tty
		   because we don't want to get killed if line noise sends CTRL-C.   */
	fd = open(port.c_str(), O_RDWR | O_NOCTTY );
	if (fd < 0) {
		Log::log.error("SerialPort: error when open port:%s baudbrate:%d \n",port.c_str(),this->baudrate);
	}

	tcgetattr(fd,&oldtio); 			/* save current serial port settings */
	//memset(&newtio, 0,sizeof(newtio));/* clear struct for new port settings */
	bzero(&newtio, sizeof(newtio));

		/*
			 BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
			 CRTSCTS : output hardware flow control (only used if the cable has
					   all necessary lines. See sect. 7 of Serial-HOWTO)
			 CS8     : 8n1 (8bit,no parity,1 stopbit)
			 CLOCAL  : local connection, no modem contol
			 CREAD   : enable receiving characters
		 */
		newtio.c_cflag = CS8 | CLOCAL | CREAD;
		//set baudrate
		cfsetispeed(&newtio,get_baudRate(this->baudrate) );
		cfsetospeed(&newtio,get_baudRate(this->baudrate));

		/*
			 IGNPAR  : ignore bytes with parity errors
			 ICRNL   : map CR to NL (otherwise a CR input on the other computer
					   will not terminate input)
			 otherwise make device raw (no other input processing)
		 */
		//ninglvfiehong modified: we use different method, so remove the ICRNL translate
		newtio.c_iflag = IGNPAR;//| ICRNL; //INLCR | ICRNL | IGNCR

		/*
	         Raw output.
		 */
		newtio.c_oflag = 0;

		/*
			 ICANON  : enable canonical input
			 disable all echo functionality, and don't send signals to calling program
		 */
		newtio.c_lflag = 0;//ninglvfeihong: without ICANON to enable VTIME and VMIN
		//ICANON;

		/*
			 initialise all control characters
			 default values can be found in /usr/include/termios.h, and are given
			 in the comments, but we don't need them here
		 */
		newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
		newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
		newtio.c_cc[VERASE]   = 0;     /* del */
		newtio.c_cc[VKILL]    = 0;     /* @ */
		newtio.c_cc[VEOF]     = 0;     /* Ctrl-d */
		newtio.c_cc[VTIME]    = WAIT_TIME;     /* ninglvfeihong: set the the time to wait for data for 100ms*/
		newtio.c_cc[VMIN]     = MIN_DATA_LEN ;     /* read msg as long as possible */
		//the above two setings are very important to read successive data
		newtio.c_cc[VSWTC]    = 0;     /* '\0' */
		newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
		newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
		newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
		newtio.c_cc[VEOL]     = 0;     /* '\0' */
		newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
		newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
		newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
		newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
		newtio.c_cc[VEOL2]    = 0;     /* '\0' */

		/*
			 now clean the modem line and activate the settings for the port
		 */
		tcflush(fd, TCIFLUSH);
		 if(tcsetattr(fd,TCSANOW,&newtio) != 0){
			 Log::log.error("SerialPort: set serial port erro!\n");
		 }
		 Log::log.debug("SerialPort: set serial port OK!\n");
		 isOpen=true;
}
SerialPort::~SerialPort() {
}
void SerialPort::listen(MyMQ<string> *mq){
	this->pLMQ =mq;
	while(1){
		string serialData(this->readPort());
		if(this->pLMQ){
			this->pLMQ->sendMSG(serialData);
			Log::log.debug("SerialPort: send string to ATAnalyser:[%s]\n",serialData.c_str());
		}else{
			Log::log.debug("SerialPort: no MyMQ obj, recive serialport data:[%s]\n",serialData.c_str());
		}
	}

}
void SerialPort::directSend(string & cmd){
	int n_written = 0,
	spot = 0;

		//Initialise the output buffer
	char output[cmd.length()+2];

		//Add the control characters
	memcpy(output,cmd.c_str(),cmd.length());
	output[cmd.length()]='\r';
	output[cmd.length()+1]='\0';

		//Write all the characters
	do {
			n_written = write( fd, &output[spot], 1 );
			spot += n_written;
	} while (output[spot-1] != '\0' && n_written > 0);
}

struct serialPortListenThreadPara_t{
	SerialPort* serial;
	MyMQ<string>* mq;
};
static void *serialPortListenThread(void * ptr){
	serialPortListenThreadPara_t* para= (serialPortListenThreadPara_t*) ptr;
	para->serial->listen(para->mq);
	return NULL;
}
void SerialPort::listenAsThread(MyMQ<string> *mq){
	pthread_t thread;
	static struct serialPortListenThreadPara_t para;//avoid the variable lost when function finished
	para.serial=this;
	para.mq=mq;
	pthread_create (&thread, NULL,serialPortListenThread, (void*) &para);
	pthread_detach(thread);
}

void SerialPort::startSender(MyMQ<std::string>* pMQ){
	if(!pMQ) Log::log.error("RabbitMQAnalyser: invalid MyMQ obj with addr:%d\n",pMQ);
	while(1){
		std::string msg= pMQ->getOneMSG();
		this->directSend(msg);
	}
}

static void *serialPortSenderThread(void * ptr){
	serialPortListenThreadPara_t* para= (serialPortListenThreadPara_t*) ptr;
	para->serial->startSender(para->mq);
	return NULL;
}
void SerialPort::startSenderAsThread(MyMQ<std::string>* pMQ){
	pthread_t thread;
	static struct serialPortListenThreadPara_t para;//avoid the variable lost when function finished
	para.serial=this;
	para.mq=pMQ;
	pthread_create (&thread, NULL,serialPortSenderThread, (void*) &para);
	pthread_detach(thread);
}

char* SerialPort::readPort(){
	/*static keep data the data in a safe place so that when function return,
 	 the data will not lost,however, this lead this funciton cannot be re-entry:
	 meaning this function can not be called in two thread at same time*/
	static char buffer[4096];
	int pre_pos=0;//ninglvfeihong:to recode the position in buffer which full of data
	//ninglvfeihong: optimized
	//memset(buffer, 0, sizeof(buffer));
	int read_len=0;
	//ninglvfeihong modified
	while((read_len =read( fd, (buffer+pre_pos), sizeof(buffer)-pre_pos-1 )) != -1)
	{
			//ninglvfeihog: remove '\0', optimize the read data
		int zero_count=0;
		for(int i=0;i<read_len;i++){
			if(zero_count)buffer[pre_pos+i-zero_count] = buffer[pre_pos+i];
			if(buffer[i+pre_pos]=='\0'){
				zero_count++;
			}
		}
		read_len-=zero_count;

			//the code is print out the received data in hex format
			/*
			for(int i=0;i<read_len;i++){
				printf("%.2X ",buffer[pre_pos+i]);
			}
		 	 */
		pre_pos+=read_len;
		if(read_len  <  MIN_DATA_LEN-zero_count ){//minus zero_count is very important
			//ninglvfeihong: the following data is not successive
			buffer[pre_pos] = '\0';//finish the string
			return buffer;
		}else{
			//the following data is successive
			if(sizeof(buffer)-pre_pos< MIN_DATA_LEN){//ninglvfeihong: buffer[] space is insufficient
				buffer[pre_pos] = '\0';//finish the string
				return buffer;//there is a risk for the buffer data, because the data is in stack are temporary
			}else{
				if(buffer[pre_pos-2]=='\r'&&buffer[pre_pos-1]=='\n'){
					//indicating the AT conmand response just right finished
					buffer[pre_pos] = '\0';//finish the string/
					return buffer;//there is a risk for the buffer data, because the data is in stack are temporary
				}else{
					//ninglvfeihong: there are enough space for next reading in buffer[]
					//do nothing just continue reading
					//continue reading
				}
			}
		}
		/*
		//Check for end of line and remove it
		if(buffer[strlen(buffer)-1] == '\r' || buffer[strlen(buffer)-1] == '\n')
			buffer[strlen(buffer)-1] = '\0';
			//Copy the useful buffer and return
		return (char*)buffer;
		 */
	}
	//Else return null
	return NULL;
}




} /* namespace SHS */
