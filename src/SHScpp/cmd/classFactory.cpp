/*
 * classFactory.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Author: Xiao WangModified from online resource
 */

#include "classFactory.h"
#include <iostream>

using std::string;
using std::map;
using std::make_pair;
namespace SHS {

ClassFactory::ClassFactory() {
}

ClassFactory::~ClassFactory() {
}

void ClassFactory::Dump() const {
  std::cout<<"\n=====ClassFactory Dump START ========== \n";
  int count = 0;
  for (std::map<string, ObjectCreate_t>::const_iterator it = factoryMap_.begin(); it!=factoryMap_.end(); ++it) {
    std::cout<<"count=="<<count
    <<" className="<<it->first.c_str()
    <<" CreateObject_t="<<it->second<<std::endl;
    ++count;
  }
  std::cout<<"\n===ClassFactory DUMP END ============\n";
}

bool ClassFactory::AddObject(const string& className, ObjectCreate_t pCreate) {
  factoryMap_.insert(make_pair(className, pCreate));
  return true;
}

bool ClassFactory::AddObject(const char* className, ObjectCreate_t pCreate) {
  string class_name(className);
  factoryMap_.insert(make_pair(class_name, pCreate));
  return true;
}

void* ClassFactory::GetObject(const string& className) {
  if (factoryMap_.count(className) == 1) {
    //这里就调用CreateObject_t()
    return factoryMap_[className]();
  } else {
	return NULL;
  }
}

void* ClassFactory::GetObject(const char* className) {
  string class_name(className);
  return GetObject(class_name);
}

/*
 * 我在考虑这里要不要使用std::move(factoryMap_)
 */
map<string, ObjectCreate_t> ClassFactory::GetMap() const {
  return factoryMap_;
}
} /* namespace SHS */
