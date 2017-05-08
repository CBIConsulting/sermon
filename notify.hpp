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

	void newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message);
	void newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message);
  void newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message);
	/* Type: message type could be (outage, fail, notice, recovery... or whatever */
	void newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message);

protected:
  std::string id;
  std::string type;
  std::map<std::string, std::string> option;
	
  virtual void _newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message) =0;
	virtual void _newBounce(const std::string& serviceName, uint64_t bounces,  std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message) =0;
  virtual void _newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message) =0;
	/* Type: message type could be (outage, fail, notice, recovery... or whatever */
	virtual void _newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message) = 0;
	virtual void _parseSpecialOptions();

private:
	std::map<std::string, uint32_t> enabledTypes;
	uint32_t defaultEnabledType;
	bool enabled;
	
	void parseSpecialOptions();
	/* uint32_t because in the future we may use flags for sth */
	uint32_t enabledNotificationType(const std::string& type);
};

#endif /* _NOTIFY_H */

