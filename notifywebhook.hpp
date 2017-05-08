/* @(#)notifywebhook.hpp
 */

#ifndef _NOTIFYWEBHOOK_H
#define _NOTIFYWEBHOOK_H 1

#include "notify.hpp"

class NotifyWebhook : public Notify
{
public:

  NotifyWebhook(std :: string id);
  ~NotifyWebhook();

  void _newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message);
  void _newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message);
  void _newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message);
	void _newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message);

protected:
	void _parseSpecialOptions();
	void callWebHook(const std::string& message);

private:
	std::string endpoint;
	std::string method;
	std::string dataTemplate;
};

#endif /* _NOTIFYWEBHOOK_H */

