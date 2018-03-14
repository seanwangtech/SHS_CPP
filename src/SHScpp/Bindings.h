/*
 * Bindings.h
 *
 *  Created on: Apr 8, 2017
 *      Author: Xiao WANG
 */

#ifndef SHSCPP_BINDINGS_H_
#define SHSCPP_BINDINGS_H_

#include <list>
#include <string>
#include <map>
#include "conf.h"
#include "json/json.h"
#include "log.h"


namespace SHS {

class Bindings {
public:
	Bindings();
	virtual ~Bindings();
	typedef struct{
		std::string MAC0;
		int EP0;
		std::string MAC1;
		int EP1;
	} Binding_pair;
	void updateBinding(std::string MAC0,int EP0, std::string MAC1, int EP1);
	Binding_pair * getBinding(std::string MAC0,int EP0);
	Binding_pair * getBindingR(std::string MAC1,int EP1);
	std::string * getBindedMAC(std::string);
	void load(Conf &conf);
private:
	std::string binding_table_path;
	std::list<Binding_pair> bindingList;
	void updateBindingInternal(std::string MAC0,int EP0, std::string MAC1, int EP1);
	void save();
};


} /* namespace SHS */

#endif /* SHSCPP_BINDINGS_H_ */
