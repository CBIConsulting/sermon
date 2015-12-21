/**
*************************************************************
* @file service_fail.cpp
* @brief Breve descripción
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 14 dic 2015
* Historial de cambios:
*
*
*************************************************************/

#include "service_fail.hpp"

ServiceFail::ServiceFail(const std::string &serviceName, std :: vector < std::shared_ptr<Notify>> & notifiers, int errorCode, const std::string &message):
  serviceName(serviceName), errorCode(errorCode), lastMessage(message), notifiers(notifiers)
{
  outageStart = std::chrono::system_clock::now();
  for (auto n : notifiers)
    n->newFail(serviceName, outageStart, errorCode, message);
}

ServiceFail::ServiceFail(const ServiceFail & sf): notifiers(sf.notifiers), 
						  serviceName(sf.serviceName),
						  outageStart(sf.outageStart),
						  errorCode(sf.errorCode),
						  lastMessage(sf.lastMessage)
{
}

ServiceFail::ServiceFail(ServiceFail && sf) : notifiers(std::move(sf.notifiers)), 
					      serviceName(std::move(sf.serviceName)),
					      outageStart(std::move(sf.outageStart)),
					      errorCode(std::move(sf.errorCode)),
					      lastMessage(std::move(sf.lastMessage))
{
}

ServiceFail::~ServiceFail()
{
}

std::string ServiceFail::getServiceName()
{
  return serviceName;
}

std::string ServiceFail::getLastMessage()
{
  return lastMessage;
}

void ServiceFail::end()
{
  for (auto n : notifiers)
    n->newRecovery(serviceName, outageStart, std::chrono::system_clock::now(), errorCode, lastMessage);
}
