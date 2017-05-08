/* @(#)notifymail.hpp
 */

#ifndef _NOTIFYMAIL_H
#define _NOTIFYMAIL_H 1

#include "notify.hpp"

class NotifyMail : public Notify
{
public:

  NotifyMail(std :: string id);
  ~NotifyMail();

  void _newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message);
  void _newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message);
  void _newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message);
	void _newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message);
	
};

#endif /* _NOTIFYMAIL_H */

