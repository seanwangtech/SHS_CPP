/*
 * DB.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#include "Lookup.h"

namespace SHS {
using std::string;
Lookup::Lookup() {

}

Lookup::~Lookup() {
}
int Lookup::getMAC_NWK(string& MAC){
	if(MAC_NWK.empty()) return -1;
	std::map<string,int>::iterator it = MAC_NWK.find(MAC);
	if(it==MAC_NWK.end()){
		return -1;
	}else{
		return it->second;
	}
}
int Lookup::getMAC_NWK(const char* MAC){
	std::string str(MAC);
	return this->getMAC_NWK(str);
}
const string& Lookup::getNWK_MAC(int NWK){
	static const string noResult;
	if(MAC_NWK.empty()) return noResult;
	std::map<string,int>::iterator it = MAC_NWK.begin();
	for(;it!=this->MAC_NWK.end();it++){
		if(it->second == NWK){
			return it->first;
		}
	}
	return noResult;
}
int Lookup::getDevT_EP(int DevT){
	if(DevT_EP.empty()) return -1;
	std::map<int,int>::iterator it = this->DevT_EP.find(DevT);
	if(it==this->DevT_EP.end()){
		return -1;
	}else{
		return it->second;
	}
}

int Lookup::getEP_DevT(int EP){
	if(DevT_EP.empty()) return -1;
	std::map<int,int>::iterator it=this->DevT_EP.begin();
	for(;it!=this->DevT_EP.end();it++){
		if(it->second == EP){
			return it->first;
		}
	}
	return -1;
}

void Lookup::updateMAC_NWK(std::string& MAC, int NWK){
	MAC_NWK.erase(MAC);
	MAC_NWK.insert(MAC_NWK.begin(),std::pair<string,int>(MAC,NWK));
}

void Lookup::updateMAC_NWK(const char* MAC, int NWK){
	std::string str(MAC);
	this->updateMAC_NWK(str,NWK);
}
void Lookup::delMAC_NWK(std::string& MAC){
	MAC_NWK.erase(MAC);
}
void Lookup::delMAC_NWK(const char* MAC){
	MAC_NWK.erase(MAC);
}
void Lookup::updateDevT_EP(int DevT,int EP){
	DevT_EP.erase(DevT);
	DevT_EP.insert(DevT_EP.begin(),std::pair<int,int>(DevT,EP));
}
bool Lookup::getMACDevT_value(std::string& MAC,int DevT,int* value){
	if(this->MACDevT_values.empty()) return false;
	std::list<struct MACDevT_value_t>::iterator it = this->MACDevT_values.begin();
	while(it != this->MACDevT_values.end()){
		if(it->DevT ==DevT && it->MAC.compare(MAC)==0){
			*value=it->value;
			return true;
		}else{
			it++;
		}
	}
	return false;
}

bool Lookup::getMACDevT_value(const char* MAC,int DevT,int *value){
	std::string str(MAC);
	return this->getMACDevT_value(str,DevT,value);
}
void Lookup::updateMACDevT_value(std::string& MAC,int DevT,int value){

	if(this->MACDevT_values.empty()){
		//the list is empty, so insert the data
		struct MACDevT_value_t data={MAC,DevT,value};
		this->MACDevT_values.insert(this->MACDevT_values.begin(),data);
		return;
	}
	std::list<struct MACDevT_value_t>::iterator it = this->MACDevT_values.begin();
	while(it != this->MACDevT_values.end()){
		if(it->DevT ==DevT && it->MAC.compare(MAC)==0){
			//the the data, so change the value
			it->value = value;
			return;
		}else{
			it++;
		}
	}
	if(it == this->MACDevT_values.end()){
		//meaning find nothing, so insert the data
		struct MACDevT_value_t data={MAC,DevT,value};
		this->MACDevT_values.insert(this->MACDevT_values.begin(),data);
	}
}

void Lookup::updateMACDevT_value(const char* MAC,int DevT,int value){
	std::string str(MAC);
	this->updateMACDevT_value(str,DevT,value);
}
void Lookup::delMACDevT_value(std::string& MAC,int DevT){
	if(this->MACDevT_values.empty()) return;
	std::list<struct MACDevT_value_t>::iterator it = this->MACDevT_values.begin();
	while(it != this->MACDevT_values.end()){
		if(it->DevT ==DevT && it->MAC.compare(MAC)==0){
			it = this->MACDevT_values.erase(it);
		}else{
			it++;
		}
	}
}

void Lookup::delMACDevT_value(const char* MAC,int DevT){
	std::string str(MAC);
	this->delMACDevT_value(str,DevT);
}


} /* namespace SHS */
