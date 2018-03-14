/*
 * Bindings.cpp
 *
 *  Created on: Apr 8, 2017
 *      Author: pupu
 */

#include "Bindings.h"
using std::string;

#include <iostream>
#include <fstream>

namespace SHS {

Bindings::Bindings() {
}

Bindings::~Bindings() {
}

void Bindings::updateBinding(std::string MAC0,int EP0, std::string MAC1, int EP1){
	//update the bindings
	this->updateBindingInternal(MAC0,EP0, MAC1, EP1);
	//save to hard driver
	save();
}
void Bindings::updateBindingInternal(std::string MAC0,int EP0, std::string MAC1, int EP1){
	//update the bindings
	if(this->bindingList.empty()){
		//the list is empty, so insert the data
		Binding_pair data={MAC0,EP0,MAC1,EP1};
		this->bindingList.insert(this->bindingList.begin(),data);
		return;
	}
	std::list<Binding_pair>::iterator it = this->bindingList.begin();
	while(it != this->bindingList.end()){
		if(it->EP0 == EP0 && it->MAC0.compare(MAC0)==0){
			//find the data, so change the value
			it->EP1 = EP1;
			it->MAC1 = MAC1;
			return;
		}else if(it->EP1 == EP1 && it->MAC1.compare(MAC1)==0){
			//find the data, so change the value
			it->EP0 = EP0;
			it->MAC0 = MAC0;
			return;
		}else{
			it++;
		}
	}
	if(it == this->bindingList.end()){
		//meaning find nothing, so insert the data
		Binding_pair data={MAC0,EP0,MAC1,EP1};
		this->bindingList.insert(this->bindingList.begin(),data);
	}
}
Bindings::Binding_pair * Bindings::getBinding(std::string MAC0,int EP0){
	std::list<Binding_pair>::iterator it = this->bindingList.begin();
	while(it != this->bindingList.end()){
		if(it->EP0 == EP0 && it->MAC0.compare(MAC0)==0){
			//find the bindings
			return &(*it);
		}else{
			it++;
		}
	}
	return NULL;
}

Bindings::Binding_pair * Bindings::getBindingR(std::string MAC1,int EP1){
	std::list<Binding_pair>::iterator it = this->bindingList.begin();
	while(it != this->bindingList.end()){
		if(it->EP1 == EP1 && it->MAC1.compare(MAC1)==0){
			//find the bindings
			return &(*it);
		}else{
			it++;
		}
	}
	return NULL;
}

std::string * Bindings::getBindedMAC(std::string MAC){
	std::list<Binding_pair>::iterator it = this->bindingList.begin();
	while(it != this->bindingList.end()){
		if(it->MAC0.compare(MAC)==0){
			//find the bindings
			return &((*it).MAC1);
		}else if(it->MAC1.compare(MAC)==0){
			//find the bindings
			return &((*it).MAC0);
		}else{
			it++;
		}
	}
	return NULL;
}

void Bindings::load(Conf &conf){
	this->binding_table_path = conf.tables_path.binding_table;

	Json::Value root;   // will contain the root value after parsing.
	Json::CharReaderBuilder builder;
	std::ifstream test(this->binding_table_path.c_str(), std::ifstream::binary);
	if(!test.is_open()){
		Log::log.debug("Bindings::load: Cannot find binding file, no bindings loaded!\n");
		test.close();
		return;
	}
	std::string errs;
	bool ok = Json::parseFromStream(builder, test, &root, &errs);
	if ( !ok )
	{
		Log::log.error("Bindings::load: Error to parse file: %s\n",this->binding_table_path.c_str());
	}
	//std::cout<<root.toStyledString()<<std::endl;
	int size = root["bindings"].size();
	for(int i=0;i<size;i++){
		this->updateBindingInternal(root["bindings"][i][0].asString(),root["bindings"][i][1].asInt(),
				root["bindings"][i][2].asString(),root["bindings"][i][3].asInt());
	}
	test.close();
}
void Bindings::save(){
	Json::Value root;
	std::list<Binding_pair>::iterator it = this->bindingList.begin();
	while(it != this->bindingList.end()){
		Json::Value tmp;
		tmp[0]=it->MAC0;
		tmp[1]=it->EP0;
		tmp[2]=it->MAC1;
		tmp[3]=it->EP1;
		root["bindings"].append(tmp);
		it++;
	}
	Json::StyledStreamWriter writer;
	std::ofstream file(this->binding_table_path.c_str());
	writer.write(file,root);
	file.close();
}
} /* namespace SHS */
