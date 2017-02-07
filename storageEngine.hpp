#pragma once

#include <string>
#include <list>
#include <map>
#include <chrono>

#include "json/src/json.hpp"
#include "lib/timeutils.hpp"

class StorageEngine
{
protected:
	typedef std::function<void(std::string, int)> Logger;

public:

	StorageEngine(Logger logger): logger(logger)
	{
	}

	virtual ~StorageEngine()
	{
	}

	virtual void close() = 0;
	virtual void addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout) =0;
	virtual uint64_t addOutage(std::string& service, std::chrono::system_clock::time_point tm, std::string& description) = 0;
	virtual void addBounce(uint64_t outageId, std::chrono::system_clock::time_point tm)=0;
	virtual void solveOutage(uint64_t outageId, std::chrono::system_clock::time_point tm)=0;
	virtual void setOutageError(uint64_t outageId, int error)=0;
	virtual	std::map < std::string, std::string > getByOutageId(uint64_t outageId) =0;
	virtual	std::map < std::string, std::string > getByOutageUuid(std::string uuid) =0;
	virtual std::list <std::map < std::string, std::string > > getOutages(const std::map<std::string, std::string>& conditions) =0;

protected:
	virtual std::string timeformat(const std::chrono::system_clock::time_point& moment)
	{
		return ::timeformat(moment, "%Y-%m-%d %H:%M:%S");
	}
	Logger logger;
	
private:
	std::string name;
};
