#pragma once

#include "storageEngine.hpp"
#include "sermon_exception.hpp"
#include <sqlite3.h>
#include <map>

class SQLiteException : public SermonException
{
public:
  /**
   * SQLiteException
   *
   * @param code      Error code
   * @param message   Error message
   *
   */
  SQLiteException(const int& code, const std::string &message): SermonException(message, code)
  {
  }

  virtual ~SQLiteException() throw ()
  {
  }
};

class StorageEngineSqlite : public StorageEngine
{
public:
	StorageEngineSqlite(const nlohmann::json &settings, Logger logger);
	void close();

	void addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout);
	uint64_t addOutage(std::string& service, std::chrono::system_clock::time_point tm, std::string& description);
	void addBounce(uint64_t outageId, std::chrono::system_clock::time_point tm);
	void solveOutage(uint64_t outageId, std::chrono::system_clock::time_point tm);
	void setOutageError(uint64_t outageId, int error);
	std::map < std::string, std::string > getByOutageId(uint64_t outageId);
	std::map < std::string, std::string > getByOutageUuid(std::string uuid);
	std::list <std::map < std::string, std::string > > getOutages(const std::map<std::string, std::string>& conditions);
		
private:
	sqlite3* db;
	std::map<std::string, std::string> statusTable;
	std::map<std::string, uint64_t> servicesTable;
	
	void initialize(std::string filename);
	void dbInitialize();
	void createTables();
	void shutdown();
  void createTable(std::string name, std::list<std::pair<std::string, std::string> > fields, bool ifNotExists=false);
	unsigned tableCount(std::string table, std::list <std::pair < std::string, std::string> > conditions = {}, std::string extra="");
	long unsigned getRowid(std::string table, std::list <std::pair < std::string, std::string> > conditions);
  long unsigned insert(std::string table, std::map<std::string, std::string> fields, bool clean=true);
  long unsigned update(std::string table, long unsigned rowid, std::map<std::string, std::string> fields, bool clean=true);
	long unsigned destroy(std::string table, long unsigned rowid);
  long unsigned destroy(std::string table, std::list <std::pair < std::string, std::string> > conditions, std::string extra="");
  std::list <std::map < std::string, std::string > > getData(std::string table, std::string fields, std::list <std::pair < std::string, std::string> > conditions = {}, std::string extra="");

	void tragic(std::string error);

	uint64_t getServiceId(const std::string& service, bool create=true);
	void insertResponseTime(uint64_t serviceId, std::chrono::system_clock::time_point tm, uint64_t timeout, int error=0);
	uint64_t insertOutage(uint64_t serviceId, std::chrono::system_clock::time_point tm, std::string& description);
	uint64_t bounceOutage(uint64_t outageId, std::chrono::system_clock::time_point tm);
	void flagOutageAsSolved(uint64_t outageId, std::chrono::system_clock::time_point tm);
	void flagOutageAsError(uint64_t outageId, std::chrono::system_clock::time_point tm, int error);
};
