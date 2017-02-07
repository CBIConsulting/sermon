#pragma once

#include <string>
#include <chrono>
#include <memory>
#include "storageEngine.hpp"

class Storage
{
public:
	typedef std::function<void(std::string, int)> Logger;
	typedef std::map < std::string, std::string > Strmap;
	
	Storage():_initialized(false),_engine(nullptr), _logger(nullptr)
	{
	}
	virtual ~Storage()
	{
	}

	void setLogger(Logger func)
	{
		_logger = func;
	}

	void logger(std::string what, int verbosity);	
	void initialize(const nlohmann::json &settings);
	void close();
	void addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout);
	/* output uuid or empty if error */
	uint64_t addOutage(std::string& service, std::chrono::system_clock::time_point, std::string description);
	void addBounce(uint64_t dbId, std::chrono::system_clock::time_point tm);
	void solveOutage(uint64_t dbId, std::chrono::system_clock::time_point tm);
	void setOutageError(uint64_t dbId, int val);

	Strmap getOutageById(uint64_t outageId);
	Strmap getOutageByUuid(std::string uuid);
	std::list<Strmap> getHistoryOutages(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const std::vector<std::string>& services, const std::map<std::string, std::string>& options);		
	bool initialized()
	{
		return _initialized;
	}

private:
	bool _initialized;
	std::shared_ptr<StorageEngine> _engine;
	Logger _logger;
};
