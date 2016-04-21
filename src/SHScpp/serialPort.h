/*
 * serialPort.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_SERIALPORT_H_
#define SHSCPP_SERIALPORT_H_
#include "conf.h"
#include <termios.h>
#include <fcntl.h>
#include "myMQ.h"
#include <string>
namespace SHS {

class SerialPort {
public:
	explicit SerialPort(Conf &conf);
	SerialPort(std::string &port,unsigned int baudrate);
	virtual ~SerialPort();
	void listen(MyMQ<std::string>* pMQ);
	void listenAsThread(MyMQ<std::string>* pMQ);
	void startSender(MyMQ<std::string>* pMQ);
	void startSenderAsThread(MyMQ<std::string>* pMQ);
	void directSend(std::string & cmd);

private:
	void openPort();
	void closePort();
	char* readPort();
	unsigned int baudrate;
	std::string port;
	int fd;
	bool isOpen;
	MyMQ<std::string>* pLMQ;
	struct termios oldtio;
};

} /* namespace SHS */

#endif /* SHSCPP_SERIALPORT_H_ */
