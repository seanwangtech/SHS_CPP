/*
 * DB.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Xiao WANG
 */

#ifndef SHSCPP_LOOKUP_H_
#define SHSCPP_LOOKUP_H_

#include <list>
#include <string>
#include <map>
namespace SHS {
//different with Conf, this used for dynamic data which generally need to maintenance
class Lookup {
public:
	Lookup();
	virtual ~Lookup();
	int getMAC_NWK(std::string & MAC);		//not found return -1
	int getMAC_NWK(const char* MAC);		//not found return -1
	const std::string& getNWK_MAC(int NWK);	//not found return empty string
	int getDevT_EP(int DevT);		//not found return -1
	int getEP_DevT(int EP);			//not found return -1;
	bool getMACDevT_value(std::string& MAC,int DevT,int *value);//not find return false
	bool getMACDevT_value(const char* MAC,int DevT,int *value);//not find return false
	void updateMAC_NWK(std::string& MAC, int NWK);
	void updateMAC_NWK(const char* MAC, int NWK);
	void delMAC_NWK(std::string& MAC);
	void delMAC_NWK(const char* MAC);
	void updateDevT_EP(int DevT, int EP);
	void delDevT_EP(int DevT);
	void updateMACDevT_value(std::string& MAC,int DevT,int value);
	void updateMACDevT_value(const char* MAC,int DevT,int value);
	void delMACDevT_value(std::string& MAC,int DevT);
	void delMACDevT_value(const char* MAC,int DevT);
private:
	std::map<std::string,int> MAC_NWK;
	std::map<int,int> DevT_EP;
	struct MACDevT_value_t {
		std::string MAC;
		int DevT;
		int value;
	};
	std::list<struct MACDevT_value_t> MACDevT_values;
};

} /* namespace SHS */

#endif /* SHSCPP_LOOKUP_H_ */
