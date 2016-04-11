/*
 * classFactory.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WangModified from online resource
 */
#ifndef MQUEUE_INCLUDE_CLASS_FACTORY_H_
#define MQUEUE_INCLUDE_CLASS_FACTORY_H_

#include <string>
#include <map>

namespace SHS{

typedef void* (*ObjectCreate_t)();

/*
 * 使用factory模式来模拟JAVA的反射机制
 */
class ClassFactory {
 public:
  ClassFactory();
  ClassFactory(const ClassFactory&) = delete;
  ClassFactory& operator=(const ClassFactory&) = delete;
  virtual ~ClassFactory();
  void Dump() const;

 public:


  bool AddObject(const std::string& className, ObjectCreate_t pCreate);
  bool AddObject(const char* className, ObjectCreate_t pCreate);
  void* GetObject(const std::string& className);
  void* GetObject(const char* className);

  std::map<std::string, ObjectCreate_t> GetMap()const;

 private:
  std::map<std::string, ObjectCreate_t> factoryMap_;
};

/*
 * 把这个singleton设置为inline模式也是遵守《Effective C++》的条款4
 */
inline ClassFactory& ClassFactoryInstance() {
  static ClassFactory sInstance;
  return sInstance;
}

#define DECLARE_CLASS_CREATE(class_name)	    	\
	static void* CreateClass##class_name ();

#define IMPL_CLASS_CREATE(class_name)	            \
	static void* CreateClass##class_name (){	    \
		return (void*)(new class_name());			\
	};											    \
	static bool _##class_name##_Unused __attribute__((unused))= ClassFactoryInstance().AddObject(#class_name, CreateClass##class_name);

//#的作用是在class_name的左右两边都加上双引号，##的作用是连接两个字符串
}
#endif /* MQUEUE_INCLUDE_CLASSS_FACTORY_H_ */
