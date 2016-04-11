/*
 * myMQ.h
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#ifndef SHSCPP_MYMQ_H_
#define SHSCPP_MYMQ_H_


#include <pthread.h>
#include <list>
#include <string>
#include <json/json.h>
namespace SHS{

template<class T> class MyMQ {
public:
	MyMQ();
	//MyMQ(const MyMQ&)=delete;
	virtual ~MyMQ();
	void sendMSG(T &js);
	T getOneMSG();
protected:
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	std::list<T> mq;
};

}
#endif /* SHSCPP_MYMQ_H_ */
