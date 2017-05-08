#pragma once

#include <string>
#include <chrono>
#include <memory>
#include "storageEngine.hpp"
#include "notify.hpp"

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
	void initialize(const nlohmann::json &settings, std::vector<std::shared_ptr< Notify>>& notifiers);
	void close();
	void addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout);
	/* output uuid or empty if error */
	uint64_t addOutage(std::string& service, std::chrono::system_clock::time_point, std::string description);
	void addBounce(uint64_t dbId, std::chrono::system_clock::time_point tm);
	void solveOutage(uint64_t dbId, std::chrono::system_clock::time_point tm);
	void setOutageError(uint64_t dbId, int val);

	Strmap getOutageById(uint64_t outageId);
	Strmap getOutageByUuid(std::string uuid);

	void setOutageDescription(uint64_t outageId, std::string description);
	void setOutageTags(uint64_t outageId, std::string tags);

	std::list<Strmap> getHistoryOutages(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const std::vector<std::string>& services, const std::map<std::string, std::string>& options);
	std::list<Strmap> getServiceStats(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const std::vector<std::string>& services, const std::map<std::string, std::string>& options);
	std::map<std::string, std::list<Strmap>> getAllServiceStats(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const uint64_t serviceId, const std::map<std::string, std::string>& options);

	std::map<std::string, Strmap> getServicesDataByName(const std::vector<std::string>& services);
	std::map<std::string, std::string> getServiceData(const uint64_t serviceId);
	bool initialized()
	{
		return _initialized;
	}

protected:
	/* When and Outage is not flagged as solved or error. (Solved=0, or it's currently hapenning), but there are
	   more outages for the same service in database.
		 It happens when sermon hangs (or is killed) and the program starts again.
	   Old outages with this condition must be flagged as Probe_fail (-1) */
	void fixOrphanOutages();
	
private:
	bool _initialized;
	std::shared_ptr<StorageEngine> _engine;
	std::vector<std::shared_ptr< Notify>> notifiers;

	Logger _logger;
};
