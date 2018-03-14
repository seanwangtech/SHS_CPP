/*
 * myMQ.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: Xiao Wang
 */

#include "myMQ.h"
template<class T>
SHS::MyMQ<T>::MyMQ() {
	pthread_mutex_init(&this->mutex,NULL);
	pthread_cond_init(&this->cond,NULL);
}
template<class T>
SHS::MyMQ<T>::~MyMQ() {
	//please check again, the mutex and the cond have not destroied.
	//when destroied these, we have to ensure no thread is pending on mutex or cond.
	/*pthread_mutex_lock(&mutex);
	for (std::list<T>::iterator it = mq.begin(); it != mq.end(); it++){
		it->empty();
	}
	mq.clear();
	pthread_mutex_unlock(&mutex);
	*/
}
template<class T>
void SHS::MyMQ<T>::sendMSG(T & js){

	pthread_mutex_lock(&mutex);
	if(mq.size()>20){
		//limit the mq size in a reasonable size by remove old message
		mq.pop_front();
	}
	mq.push_back(js);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

}
template<class T>
T SHS::MyMQ<T>::getOneMSG(){
	pthread_mutex_lock(&mutex);
	while(mq.size()==0) pthread_cond_wait(&cond,&mutex);
	T result = *mq.begin();
	mq.pop_front();
	pthread_mutex_unlock(&mutex);
	return result;
}
/*********************************************
 * declare classes that needed for MyMQ
 */
template class SHS::MyMQ<std::string>;
template class SHS::MyMQ<Json::Value>;

