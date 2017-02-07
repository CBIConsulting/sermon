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

void Storage::initialize(const nlohmann::json &settings)
{
	if ( (settings.find("enabled") == settings.end()) || (!settings["enabled"].get<bool>()) )
		return;
	
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

	_initialized = true;
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

	logger("BOUNCE OF "+std::to_string(dbId), 2);	
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

