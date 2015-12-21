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
  ServiceFail(const std::string &serviceName, std::vector<std::shared_ptr<Notify>> &notifiers, int errorCode, const std::string &message);
  ServiceFail(const ServiceFail& sf);
  ServiceFail(ServiceFail&& sf);
  ~ServiceFail();

  std::string getServiceName();
  std::string getLastMessage();
  void end();
private:
  std::vector<std::shared_ptr< Notify>> notifiers;
  std::string serviceName;
  std::chrono::system_clock::time_point outageStart;
  int errorCode;
  std::string lastMessage;
};

#endif /* _SERVICE_FAIL_H */

