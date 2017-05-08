/* @(#)notifycli.hpp
 */

#ifndef _NOTIFYCLI_H
#define _NOTIFYCLI_H 1

#include "notify.hpp"

class NotifyCli : public Notify
{
public:

  NotifyCli(std :: string id);
  ~NotifyCli();

  void _newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message);
  void _newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message);
  void _newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message);
	void _newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message);
};

#endif /* _NOTIFYCLI_H */

