#include "storage.hpp"
#include <iostream>
#include <limits>
#include "sermon_exception.hpp"
#include "lib/uuid.hpp"
#include "lib/timeutils.hpp"
#include "lib/gutils.hpp"

#ifndef NO_SQLITE_STORAGE
#  include "storageEngineSqlite.hpp"
#endif

void Storage::initialize(const nlohmann::json &settings, std::vector<std::shared_ptr< Notify>>& notifiers)
{
	if ( (settings.find("enabled") == settings.end()) || (!settings["enabled"].get<bool>()) )
		return;
	this->notifiers = notifiers;

	logger("Initializing storage", 5);

	std::string engine;
	if (settings.find("engine") != settings.end())
		engine = settings["engine"].get<std::string>();

	if (engine=="none")
		return;
#ifndef NO_SQLITE_STORAGE
	else if (engine=="sqlite")
		_engine = std::make_shared<StorageEngineSqlite>(settings, std::bind(&Storage::logger, this, std::placeholders::_1, std::placeholders::_2));
#endif
	else
		throw SermonException("No valid storage engine "+engine);

	fixOrphanOutages();

	_initialized = true;
}

void Storage::fixOrphanOutages()
{
	auto orphanOutages = _engine->fixOrphanOutages(-1);

	for (auto ou: orphanOutages)
		{
			for (auto n : notifiers)
				{
					auto serviceName = (ou["Service_name"].empty())?"FAILED TO GET SERVICE NAME":ou["Service_name"];
					auto outageTime =  (ou["created_dtm"].empty())?"FAILED TO GET OUTAGE TIME":put_time("%d/%m/%Y %H:%M:%S", parseTime(ou["created_dtm"]));
					auto subject = (ou["description"].empty())?"FAILED TO GET OUTAGE DESCRIPTION":ou["description"];

					n->newMessage("warning", serviceName,
												std::chrono::system_clock::now(),
												"Site outage from "+outageTime+" with subject "+subject+" flagged as Probe error");
				}

		}
	/* 		for (auto n : notifiers) */
	/* n->newMessage("warning", "", std::chrono::system_clock::now(), "Sample notification"); */
}

void Storage::logger(std::string what, int verbosity)
{
	if (_logger != nullptr)
		_logger(what, verbosity);
}

void Storage::close()
{
	if ((initialized()) && (_engine) )
		_engine->close();
}

void Storage::addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout)
{
	if (initialized())
		_engine->addResponseTime(service, tm, timeout);
}

uint64_t Storage::addOutage(std::string& service, std::chrono::system_clock::time_point tm, std::string description)
{
	std::string uuid = UUID::generate();
	if (initialized())
		{
			return _engine->addOutage(service, tm, description);
		}
	return 0;
}

void Storage::addBounce(uint64_t dbId, std::chrono::system_clock::time_point tm)
{
	if (!initialized())
		return;

	logger("Bounce #"+std::to_string(dbId), 2);
	_engine->addBounce(dbId, tm);

	if ( (dbId==0) || (dbId==std::numeric_limits<uint64_t>::max()) )
		return;											/* Bad bounce number or no db. */
}

void Storage::solveOutage(uint64_t dbId, std::chrono::system_clock::time_point tm)
{
	if (!initialized())
		return;

	if ( (dbId==0) || (dbId==std::numeric_limits<uint64_t>::max()) )
		return;											/* Bad bounce number or no db. */

	_engine->solveOutage(dbId, tm);
}

void Storage::setOutageError(uint64_t dbId, int val)
{
	if (!initialized())
		return;

	if (val>=0)										/* Bad error */
		return;

	if ( (dbId==0) || (dbId==std::numeric_limits<uint64_t>::max()) )
		return;											/* Bad bounce number or no db. */

	_engine->setOutageError(dbId, val);
}

std::map < std::string, std::string > Storage::getOutageById(uint64_t outageId)
{
	if (!initialized())
		return {};

	return _engine->getByOutageId(outageId);
}

std::map < std::string, std::string > Storage::getOutageByUuid(std::string uuid)
{
	if (!initialized())
		return {};

	return _engine->getByOutageUuid(uuid);
}

void Storage::setOutageDescription(uint64_t outageId, std::string description)
{
	if (!initialized())
		return;

	_engine->setOutageDescription(outageId, description);
}

void Storage::setOutageTags(uint64_t outageId, std::string tags)
{
	if (!initialized())
		return;

	_engine->setOutageTags(outageId, tags);
}

std::list<Storage::Strmap> Storage::getHistoryOutages(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const std::vector<std::string>& services, const std::map<std::string, std::string>& options)
{
	std::map<std::string, std::string> conditions = {
		{ "from", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(from)) },
		{ "to", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(to)) }
	};
	auto _limit = options.find("limit");
	if (_limit != options.end())
		conditions["limit"]= _limit->second;

	if (services.size()>0)
		conditions.insert({ "services", GCommon::implode(services, ",")});

	auto outages = _engine->getOutages(conditions);
	return outages;
}

std::list<Storage::Strmap> Storage::getServiceStats(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const std::vector<std::string>& services, const std::map<std::string, std::string>& options)
{
	std::map<std::string, std::string> conditions = {
		{ "from", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(from)) },
		{ "to", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(to)) }
	};
	if (services.size()>0)
		conditions.insert({ "services", GCommon::implode(services, ",")});

	auto responseTimes = _engine->getResponseTimeStats(conditions);
	auto outages = _engine->getOutageStats(conditions);

	auto output = responseTimes;
	for (auto &r : output)
		{
			auto id = std::stoll(r["Service_id"]);
			r["Service_id"] = std::to_string(id); /* Fix ID representation */
			if (outages.find(id) != outages.end())
				{
					Storage::Strmap outagesdata = outages[id];
					for (auto el : outages[id])
						{
							r.insert(std::pair<std::string, std::string>(el.first, el.second));
						}
				}
		}
	/* return outages; */
	/* return {}; */
	return output;
}

std::map<std::string, Storage::Strmap> Storage::getServicesDataByName(const std::vector<std::string>& services)
{
	std::map<std::string, std::string> conditions = {};
	if (services.size()>0)
		conditions.insert({ "services", GCommon::implode(services, ",")});

	auto output = _engine->getServicesDataByName(conditions);

	return output;
}

/* Outages and responses */
std::map<std::string, std::list<Storage::Strmap>> Storage::getAllServiceStats(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to, const uint64_t serviceId, const std::map<std::string, std::string>& options)
{
	std::map<std::string, std::string> conditions = {
		{ "from", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(from)) },
		{ "to", put_time("%Y-%m-%d %H:%M:%S", std::chrono::system_clock::to_time_t(to)) }
	};

	conditions.insert({ "Service_id", std::to_string(serviceId)});

	auto responseTimes = _engine->getAllResponseTimes(conditions);
	auto outages = _engine->getAllOutages(conditions);

	return { { "response", responseTimes }, { "outages", outages } };
}

std::map<std::string, std::string> Storage::getServiceData(const uint64_t serviceId)
{
	std::map<std::string, std::string> conditions = {};

	conditions.insert({ "Service_id", std::to_string(serviceId)});

	auto output = _engine->getServicesDataByName(conditions);
	if (output.empty())
		return {};

	return output.begin()->second;
}
