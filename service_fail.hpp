/* @(#)service_fail.hpp
 */

#ifndef _SERVICE_FAIL_H
#define _SERVICE_FAIL_H 1

#include <string>
#include <chrono>
#include <vector>
#include "notify.hpp"
#include <iostream>
#include <memory>

class ServiceFail
{
public:
  ServiceFail(const std::string &serviceName, uint64_t dbId, std::vector<std::shared_ptr<Notify>> &notifiers, int errorCode, const std::string &message);
  ServiceFail(const ServiceFail& sf);
  ServiceFail(ServiceFail&& sf);
  virtual ~ServiceFail();

  std::string getServiceName();
  std::string getLastMessage();
  void end();
	
	void bounce();
	uint64_t getBounces();
	std::chrono::system_clock::time_point getStartTime();
	int getErrorCode()
	{
		return errorCode;
	}
	uint64_t dbndx()
	{
		return dbId;
	}
private:
  std::vector<std::shared_ptr< Notify>> notifiers;
  std::string serviceName;
  std::chrono::system_clock::time_point outageStart;
  int errorCode;
	uint64_t bounces;
  std::string lastMessage;
	uint64_t dbId;
};

#endif /* _SERVICE_FAIL_H */

