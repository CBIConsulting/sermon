/* @(#)notify.hpp
 */

#ifndef _NOTIFY_H
#define _NOTIFY_H 1

#include <string>
#include <map>
#include <chrono>

class Notify
{
public:
  Notify(std::string id, std::string type);
  Notify(Notify&& n) noexcept;
  Notify(const Notify& n);
  virtual ~Notify();

  void setOptions(std::map<std::string, std::string> options);
  std::string getType()
  {
    return type;
  }

  std::map<std::string, std::string> getOptions()
  {
    return option;
  }

  std::string getOption(std::string optionKey)
  {
    return option[optionKey];
  }

  virtual void newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message) =0;
  virtual void newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message) =0;

protected:
  std::string id;
  std::string type;
  std::map<std::string, std::string> option;
};

#endif /* _NOTIFY_H */

