#include "storageEngineSqlite.hpp"
#include "lib/uuid.hpp"
#include "lib/gutils.hpp"

namespace
{
	enum FieldType
		{
			NONE,
			NUMBER,
			FLOAT,
			DATETIME,
			TEXT
		};
	struct FieldDescription
	{
		FieldType type;
		bool notNull;
	};

	std::map<std::string, std::map<std::string, FieldDescription>> fieldsCache;

	FieldType parseType(std::string & type)
	{
		if (type == "NUMBER")
			return NUMBER;
		else if (type == "DATETIME")
			return DATETIME;
		else if (type == "FLOAT")
			return FLOAT;
		else
			return TEXT;
	}
	
	void tragicError(std::string error)
	{
		throw SermonException("TRAGIC SQLITE ERROR: "+error);
	}
		
	FieldDescription _fieldType(sqlite3* db, std::string table, std::string field)
	{
		auto tablesp = table.find(' ');
		if (tablesp != std::string::npos)
			table = table.substr(0, tablesp);

		std::string querystr = "PRAGMA table_info("+table+")";
		sqlite3_stmt *query;

		if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
			tragicError("Could not prepare SQLite statement: "+querystr);

		int res;
		std::map<std::string, FieldDescription> fieldsdata;
		while ( (res = sqlite3_step(query)) == SQLITE_ROW)
			{
				std::string fieldName;
				FieldDescription fieldDescription;
				for (int i=0; i< sqlite3_column_count(query); ++i)
					{
						std::string colname = sqlite3_column_name(query, i);
						auto ptr = sqlite3_column_text(query,i);
						std::string colval = (ptr!=NULL)?(char*)ptr:"";
						if (colname == "name")
							fieldName = colval;
						else if (colname =="type")
							fieldDescription.type = parseType(colval);
						else if (colname =="notnull")
							fieldDescription.notNull = (colval=="1");
					}
				if (!fieldName.empty())
					fieldsdata.insert({fieldName, fieldDescription});
			}

		if (res==SQLITE_ERROR)
			tragicError("Error executing query: "+querystr);

		fieldsCache.insert({table, fieldsdata});
		
		sqlite3_finalize(query);
		
		return fieldsCache[table][field];	
	}
	
	FieldDescription fieldType(sqlite3* db, std::string table, std::string field)
	{
		auto tableData = fieldsCache.find(table);
		if (tableData == fieldsCache.end())
			return _fieldType(db, table, field);

		auto fieldData = tableData->second.find(field);
		if (fieldData == tableData->second.end())
			return {NONE};
			/* tragicError("Field "+field+" not found"); */

		return fieldData->second;
	}

	bool isnum (std::string& str)
	{
		char *endptr;
		auto num = strtoll(str.c_str(), &endptr, 10);

		return (*endptr ==0);
	}
	
	std::string parseConditions(std::list <std::pair < std::string, std::string> >& conditions)
	{
		std::string querystr;
		if (conditions.size()>0)
			{
				querystr+="WHERE ";
				bool first=true;
				for (auto &c : conditions)
					{
						std::string condType="=";
						auto lastc = c.first.back();
						switch (lastc)
							{
							case '=':
								c.first.pop_back();
								lastc = c.first.back();
								if ( (lastc=='>') || (lastc=='>') )
									{
										c.first.pop_back();
										condType.insert(0, 1, lastc);
									}
								break;
							case '>':
								c.first.pop_back();
								condType = lastc;
								break;
							case '<':
								c.first.pop_back();
								condType = lastc;

								lastc = c.first.back();
								if (lastc=='>')
									{
										c.first.pop_back();
										condType="<>";
									}
								break;
							}
						if (GCommon::endsWith(c.first, " LIKE"))
							{
								condType=" LIKE ";
								c.first.erase(c.first.find_last_of(" LIKE")-4);
							}
						bool startGroup = false;
						bool endGroup = false;
						
						if (c.first.front()=='(')
							startGroup = true, c.first.erase(c.first.begin());
						if (c.first.back()==')')
							endGroup = true, c.first.pop_back();
						
						if (c.first[0] == '|')
							querystr+="OR ",c.first.erase(c.first.begin());//+c.first.substr(1);
						else
							{
								if (!first)
									querystr+="AND ";
							}
						if (startGroup)
							querystr+="(";
						querystr+=c.first;
						querystr+=condType;
						if ( (c.second.length()>2) && (c.second.substr(0,2)=="F(") )
							querystr+=c.second.substr(2);
						else if (isnum(c.second))
							{
								/* querystr+="? "; */
								querystr+=c.second+" ";
								c.second="F(";	/* Workaround */
							}
						else
							querystr+="? ";

						if (endGroup)
							querystr+=")";

						first=false;
					}
			}
		return querystr;
	}
};

void StorageEngineSqlite::createTable(std::string name, std::list<std::pair<std::string, std::string> > fields, bool ifNotExists)
{
	if (_readonly)
		return;

  char *error = NULL;
  std::string query = "CREATE TABLE ";
  if (ifNotExists)
    query+="IF NOT EXISTS ";

  query+=name+" (";
  bool first=true;
  for (auto v: fields)
    {
      if (!first)
				query+=",\n";
      query+=v.first+" "+v.second;
      first=false;
    }
  query+=")";

  if (sqlite3_exec(db, query.c_str(), NULL, 0, &error) != SQLITE_OK)
    throw SQLiteException(1, "Error creating table: "+std::string(error));
}

long unsigned StorageEngineSqlite::update(std::string table, long unsigned rowid, std::map<std::string, std::string> fields, bool clean)
{
	if (_readonly)
		return 0;

  std::string querystr, values;
  querystr = "UPDATE "+table+" SET ";
	bool first = true;
  for (auto f : fields)
    {
      if (!first)
				{
					querystr+=", ";
				}

			first = false;

      querystr=querystr+f.first+"=";
			/* Functions and so */
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				querystr+=f.second.substr(2);
			else
				querystr+="?";
    }
	if ( (fieldType(db, table, "mtime").type == DATETIME) && (fields.find("mtime")==fields.end()) )
		{
			querystr+=", mtime=DATETIME('now')";
		}
	
  querystr+=" WHERE ROWID="+std::to_string(rowid);
  sqlite3_stmt *query;

  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
    throw SQLiteException(2, "Could not prepare SQLite statement: "+querystr);

  unsigned order=1;
  for (auto f : fields)
    {
      int res;
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				continue;
			try
				{
					if (fieldType(db, table, f.first).type == NUMBER)
						{
							res = sqlite3_bind_int(query, order++, std::stoi(f.second));
						}
					else
						if (fieldType(db, table, f.first).type == FLOAT)
						{
							res = sqlite3_bind_double(query, order++, std::stod(f.second));
						}
					else
						{
							res = sqlite3_bind_text(query, order++, f.second.c_str(), f.second.length(), SQLITE_TRANSIENT);
						}
				}
			catch (std::exception& e)
				{
					/* Invalid conversion */
					return 0;
				}
      if (res != SQLITE_OK)
				throw SQLiteException(3, "Failed binding value into prepared statement. Query: "+querystr);
    }

  int res = res = sqlite3_step(query);

  if (res!=SQLITE_DONE)
    throw SQLiteException(4, "Error executing query: "+querystr);

  sqlite3_finalize(query);
  return rowid;
}

long unsigned StorageEngineSqlite::update(std::string table, std::map<std::string, std::string> fields, std::list <std::pair < std::string, std::string> > conditions, bool clean)
{
	if (_readonly)
		return 0;

  std::string querystr, values;
  querystr = "UPDATE "+table+" SET ";
	bool first = true;
  for (auto f : fields)
    {
      if (!first)
				{
					querystr+=", ";
				}

			first = false;

      querystr=querystr+f.first+"=";
			/* Functions and so */
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				querystr+=f.second.substr(2);
			else
				querystr+="?";
    }
	if ( (fieldType(db, table, "mtime").type == DATETIME) && (fields.find("mtime")==fields.end()) )
		{
			querystr+=", mtime=DATETIME('now')";
		}
	
	querystr+=" "+::parseConditions(conditions);
  sqlite3_stmt *query;

  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
    throw SQLiteException(2, "Could not prepare SQLite statement: "+querystr);

  unsigned order=1;

  for (auto f : fields)
    {
      int res;
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				continue;
			try
				{
					if (fieldType(db, table, f.first).type == NUMBER)
						{
							res = sqlite3_bind_int(query, order++, std::stoi(f.second));
						}
					else
						if (fieldType(db, table, f.first).type == FLOAT)
						{
							res = sqlite3_bind_double(query, order++, std::stod(f.second));
						}
					else
						{
							res = sqlite3_bind_text(query, order++, f.second.c_str(), f.second.length(), SQLITE_TRANSIENT);
						}
				}
			catch (std::exception& e)
				{
					/* Invalid conversion */
					return 0;
				}
      if (res != SQLITE_OK)
				throw SQLiteException(3, "Failed binding value into prepared statement. Query: "+querystr);
    }
  for (auto f : conditions)
    {
      int res;

			if ( (f.second.length()>=2) && (f.second.substr(0,2)=="F(") )
				continue;

			res = sqlite3_bind_text(query, order++, f.second.c_str(), -1, SQLITE_TRANSIENT);
      if (res != SQLITE_OK)
				throw SQLiteException(14, "Failed binding value into prepared statement. Query: "+querystr);
    }
	
  int res = res = sqlite3_step(query);

  if (res!=SQLITE_DONE)
    throw SQLiteException(4, "Error executing query: "+querystr);

  return sqlite3_changes(db);
}

long unsigned StorageEngineSqlite::insert(std::string table, std::map<std::string, std::string> fields, bool clean)
{
	if (_readonly)
		return 0;

  std::string querystr, values;
  querystr = "INSERT INTO "+table+" (";
	
  for (auto f : fields)
    {
      if (!values.empty())
				{
					querystr+=", ";
					values+=", ";
				}
      querystr+=f.first;
			/* Functions and so */
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				values+=f.second.substr(2);
			else
				values+="?";
    }
	/* Auto control fields */
	if ( (fieldType(db, table, "ctime").type == DATETIME) && (fields.find("ctime")==fields.end()) )
		{
			querystr+=", ctime";
			values+=", DATETIME('now')";
		}
	if ( (fieldType(db, table, "mtime").type == DATETIME) && (fields.find("mtime")==fields.end()) )
		{
			querystr+=", mtime";
			values+=", DATETIME('now')";
		}
	/* Auto control fields */
  querystr+=") VALUES ("+values+")";
  sqlite3_stmt *query;
	
  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
    throw SQLiteException(5, "Could not prepare SQLite statement: "+querystr);

  unsigned order=1;
  for (auto f : fields)
    {
      int res;
			if ( (f.second.length()>2) && (f.second.substr(0,2)=="F(") )
				continue;
			try
				{
					if (fieldType(db, table, f.first).type == NUMBER)
						{
							res = sqlite3_bind_int(query, order++, std::stoi(f.second));
						}
					else
						if (fieldType(db, table, f.first).type == FLOAT)
						{
							res = sqlite3_bind_double(query, order++, std::stod(f.second));
						}
					else
						{
							res = sqlite3_bind_text(query, order++, f.second.c_str(), f.second.length(), SQLITE_TRANSIENT);
						}
				}
			catch (std::exception& e)
				{
					/* Invalid conversion */
					return 0;
				}
      if (res != SQLITE_OK)
				throw SQLiteException(6, "Failed binding value into prepared statement. Query: "+querystr);
    }

  int res = res = sqlite3_step(query);
  if (res!=SQLITE_DONE)
    throw SQLiteException(7, "Error executing query: "+querystr);

  long unsigned insertId = sqlite3_last_insert_rowid(db);
  sqlite3_finalize(query);

  return insertId;
}

long unsigned StorageEngineSqlite::getRowid(std::string table, std::list <std::pair < std::string, std::string> > conditions)
{
	auto res = getData(table, "rowid", conditions, "LIMIT 1");
	if (res.empty())
		return 0;

	auto total = res.front().find("rowid");
	if (total == res.front().end())
		return 0;

	return std::stoi(total->second);
}

unsigned StorageEngineSqlite::tableCount(std::string table, std::list <std::pair < std::string, std::string> > conditions, std::string extra)
{
	auto res = getData(table, "COUNT(ROWID) AS total", conditions, extra);
	if (res.empty())
		return 0;

	auto total = res.front().find("total");
	if (total == res.front().end())
		return 0;

	return std::stoi(total->second);
}

long unsigned StorageEngineSqlite::destroy(std::string table, long unsigned rowid)
{
	if (_readonly)
		return 0;
	
  std::string querystr = "DELETE FROM "+table+" WHERE rowid="+std::to_string(rowid);
  sqlite3_stmt *query;

  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
    throw SQLiteException(8, "Could not prepare SQLite statement: "+querystr);

  int res = res = sqlite3_step(query);
  if (res!=SQLITE_DONE)
    throw SQLiteException(9, "Error executing query: "+querystr);

  sqlite3_finalize(query);
	return 1;	
}

long unsigned StorageEngineSqlite::destroy(std::string table, std::list <std::pair < std::string, std::string> > conditions, std::string extra)
{
  std::string querystr = "DELETE FROM "+table+" ";

	querystr+=::parseConditions(conditions);

  querystr+=extra;

  sqlite3_stmt *query;

  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
    throw SQLiteException(10, "Could not prepare SQLite statement: "+querystr);

	unsigned order=1;
  for (auto f : conditions)
    {
      int res;

			if ( (f.second.length()>=2) && (f.second.substr(0,2)=="F(") )
				continue;
			
			res = sqlite3_bind_text(query, order++, f.second.c_str(), -1, SQLITE_TRANSIENT);
      if (res != SQLITE_OK)
				throw SQLiteException(14, "Failed binding value into prepared statement. Query: "+querystr);
    }

  int res = res = sqlite3_step(query);
  if (res!=SQLITE_DONE)
    throw SQLiteException(12, "Error executing query: "+querystr);

  sqlite3_finalize(query);
	return 1;
}

std::list <std::map < std::string, std::string > > StorageEngineSqlite::getData(std::string table, std::string fields, std::list <std::pair < std::string, std::string> > conditions, std::string extra)
{
  std::string querystr = "SELECT "+fields+" FROM "+table+" ";
	querystr+=::parseConditions(conditions);
	
  querystr+=extra;

  sqlite3_stmt *query;
  if (sqlite3_prepare_v2(db, querystr.c_str(), -1, &query, NULL) != SQLITE_OK)
		throw SQLiteException(13, "Could not prepare SQLite statement: "+querystr);

  unsigned order=1;
  for (auto f : conditions)
    {
      int res;

			if ( (f.second.length()>=2) && (f.second.substr(0,2)=="F(") )
				continue;
			
			res = sqlite3_bind_text(query, order++, f.second.c_str(), -1, SQLITE_TRANSIENT);
      if (res != SQLITE_OK)
				throw SQLiteException(14, "Failed binding value into prepared statement. Query: "+querystr);
    }

  int res;
  std::list <std::map < std::string, std::string > > resdata;

  while ( (res = sqlite3_step(query)) == SQLITE_ROW)
    {
      std::map < std::string, std::string > row;

      for (int i=0; i< sqlite3_column_count(query); ++i)
				{
					switch (sqlite3_column_type(query, i))
						{
						case SQLITE_INTEGER:
						case SQLITE_FLOAT:
							row.insert({sqlite3_column_name(query, i), std::to_string(sqlite3_column_double(query,i))});
							break;
						case SQLITE_TEXT:
							row.insert({sqlite3_column_name(query, i), std::string((const char*)sqlite3_column_text(query,i))});
							break;
						case SQLITE_NULL:
							row.insert({sqlite3_column_name(query, i), "(null)"});
							break;
						}
				}
      resdata.push_back(row);
    }

  if (res==SQLITE_ERROR)
    throw SQLiteException(15, "Error executing query: "+querystr);

	sqlite3_finalize(query);

  return resdata;
}

StorageEngineSqlite::StorageEngineSqlite(const nlohmann::json &settings, Logger logger):StorageEngine(logger),_readonly(false)
{
	logger("Initializing SQLite...", 3);
	if (settings.find("file") == settings.end())
		throw SermonException("No SQLite file given");
	
	auto file = settings["file"].get<std::string>();

	if ( (settings.find("readonly") != settings.end()) && (settings.find("readonly")->is_boolean()) )
		_readonly = settings.find("readonly")->get<bool>();
	
	initialize(file);
	
}

void StorageEngineSqlite::close()
{
	logger("Shuting down SQLite...", 3);
	shutdown();
}

void StorageEngineSqlite::initialize(std::string filename)
{
  if (sqlite3_initialize() != SQLITE_OK)
    tragic("Sqlite3 can't be initialized");

  if (sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
    tragic("Sqlite3 file "+std::string(filename.c_str())+" can't be opened");

	dbInitialize();
}

void StorageEngineSqlite::createTables()
{
	logger("Creating basic tables...", 3);

	/* Services monitored */
	createTable("Services", {
			{ "id", "TEXT UNIQUE" },
			{ "created_dtm", "DATETIME" }
		}, true);

	createTable("Outages", {
			{ "service_id", "NUMBER" },
			{ "uuid", "TEXT UNIQUE" },
			{ "description", "TEXT" },
			{ "created_dtm", "DATETIME" },
			{ "updated_dtm", "DATETIME" },
			{ "solved", "NUMERIC" },
			{ "notify_dtm", "DATETIME" }, /* Is notified? */
			{ "bounces", "NUMERIC" }, /* Times the error is thrown */
			{ "tags", "TEXT" },			/* User tags */
			{ "totaltime", "TEXT" },
			{ "userdes", "TEXT" } /* User description */
		}, true);

	createTable("ResponseTime", {
			{ "service_id", "NUMBER" },
			{ "probe_dtm", "DATETIME" },
			{ "time", "NUMBER" },
			{ "error", "NUMBER" }
		}, true);

	insert("Status", {
			{ "id", "Version" },
			{ "val", "1" }
		});
}

void StorageEngineSqlite::dbInitialize()
{
	createTable("Status", {
			{ "id", "TEXT UNIQUE" },
			{ "val", "TEXT" }
		}, true);

	auto data = getData("Status", "*");
	for (auto _d : data)
		statusTable.insert({ _d["id"], _d["val"]});

	if (statusTable["Version"].empty())
		createTables();
	else
		logger("Current database version: "+statusTable["Version"],3);
	/* Create tables */
}

void StorageEngineSqlite::shutdown()
{
  sqlite3_close_v2(db);	
}

void StorageEngineSqlite::tragic(std::string error)
{
	tragicError(error);
}

uint64_t StorageEngineSqlite::getServiceId(const std::string& service, bool create)
{
	auto serv = servicesTable.find(service);
	if (serv != servicesTable.end())
		return serv->second;

	auto sData = getData("Services", "ROWID, id", {
			{"id", service }
		}, "LIMIT 1");
	if (sData.size()>0)
		{
			try
				{
					auto serviceId = stoll(sData.front()["rowid"]);
					servicesTable.insert({ service, serviceId });
					return serviceId;
				}
			catch (std::exception &e)
				{
					logger("There was a problem looking for service "+service+" in database", 1);
					return 0;
				}
		}
	else if (!create)
		{
			return 0;
		}
	else
		{
			auto serviceId = insert("Services", {
					{ "id", service },
					{ "created_dtm", "F(DATETIME('now')" }
				});
			servicesTable.insert({ service, serviceId });
			return serviceId;
		}
}

void StorageEngineSqlite::addResponseTime(std::string& service, std::chrono::system_clock::time_point tm, uint64_t timeout)
{
	auto serviceId = getServiceId(service);
	if (serviceId==0)
		return;

	try
		{
			insertResponseTime(serviceId, tm, timeout);

			logger("New time: "+timeformat(tm)+" resp: "+std::to_string(timeout), 3);
		}
	catch (std::exception& e)
		{
			logger("There was a problem storing response time: "+std::string(e.what()), 1);
		}
}

void StorageEngineSqlite::insertResponseTime(uint64_t serviceId, std::chrono::system_clock::time_point tm, uint64_t timeout, int error)
{
		insert("ResponseTime", {
			{"service_id", std::to_string(serviceId) },
			{"probe_dtm", timeformat(tm) },
			{"time", std::to_string(timeout)},
			{"error", std::to_string(error)}
	 });
}

uint64_t StorageEngineSqlite::insertOutage(uint64_t serviceId, std::chrono::system_clock::time_point tm, std::string& description)
{
	auto uuid = UUID::generate();
	uint64_t outageId = insert("Outages", {
			{ "service_id", std::to_string(serviceId) },
			{ "uuid", uuid },
			{ "description", description },
			{ "created_dtm", timeformat(tm) },
			{ "updated_dtm", timeformat(tm) },
			{ "solved", "0" },
			{ "bounces", "0" }					 
		});

	return outageId;
}

uint64_t StorageEngineSqlite::bounceOutage(uint64_t outageId, std::chrono::system_clock::time_point tm)
{
	update("Outages", outageId, {
			{ "updated_dtm", timeformat(tm) },
			{ "bounces", "F(bounces+1" }
		});
}

void StorageEngineSqlite::flagOutageAsSolved(uint64_t outageId, std::chrono::system_clock::time_point tm)
{
	update("Outages", outageId, {
			{ "updated_dtm", timeformat(tm) },
			{ "totaltime", "F(STRFTIME(\"%s\",'"+timeformat(tm)+"')-STRFTIME(\"%s\", created_dtm)" }, 
			{ "solved", "1" }
		});	
}

void StorageEngineSqlite::flagOutageAsError(uint64_t outageId, std::chrono::system_clock::time_point tm, int error)
{
	update("Outages", outageId, {
			{ "updated_dtm", timeformat(tm) },
			{ "solved", std::to_string(error) }
		});	
}

uint64_t StorageEngineSqlite::addOutage(std::string& service, std::chrono::system_clock::time_point tm, std::string& description)
{
	auto serviceId = getServiceId(service);
	if (serviceId==0)
		return 0;
	
	try
		{
			insertResponseTime(serviceId, tm, 0, 1);
			auto outageId = insertOutage(serviceId, tm, description);
			return outageId;
		}
	catch (std::exception& e)
		{
			logger("There was a problem storing outage: "+std::string(e.what()), 1);
		}
	return (uint64_t)-1;					/* Very very big number*/
}

void StorageEngineSqlite::addBounce(uint64_t outageId, std::chrono::system_clock::time_point tm)
{
	if (outageId==0)
		return;

	try
		{
			bounceOutage(outageId, tm);
		}
	catch (std::exception& e)
		{
			logger("There was a problem storing bounce: "+std::string(e.what()), 1);
		}
			
}

void StorageEngineSqlite::solveOutage(uint64_t outageId, std::chrono::system_clock::time_point tm)
{
	if (outageId==0)
		return;

	try
		{
			flagOutageAsSolved(outageId, tm);
		}
	catch (std::exception& e)
		{
			logger("There was a problem storing solved outage: "+std::string(e.what()), 1);
		}
}

void StorageEngineSqlite::setOutageError(uint64_t outageId, int error)
{
	if (outageId==0)
		return;
	if (error>=0)
		return;
	
	try
		{
			flagOutageAsError(outageId, std::chrono::system_clock::now(), error);
		}
	catch (std::exception& e)
		{
			logger("There was a problem setting outage error: "+std::string(e.what()), 1);
		}	
}

std::map < std::string, std::string > StorageEngineSqlite::getByOutageId(uint64_t outageId)
{
	try
		{
			auto sData = getData("Outages O LEFT JOIN Services S ON O.Service_id=S.rowid", "O.rowid AS Id, O.*, S.id AS Service_name", {
					{"O.rowid", std::to_string(outageId) }
				}, "LIMIT 1");
			if (sData.size()==0)
				return {};
			else
				return sData.front();
		}
	catch (std::exception &e)
		{
			return {};
		}
}

std::map < std::string, std::string > StorageEngineSqlite::getByOutageUuid(std::string uuid)
{
	try
		{
			auto sData = getData("Outages O LEFT JOIN Services S ON O.Service_id=S.rowid", "O.rowid AS Id, O.*, S.id AS Service_name", {
					{"uuid LIKE", uuid }
				}, "LIMIT 1");
			if (sData.size()==0)
				return {};
			else
				return sData.front();
		}
	catch (std::exception &e)
		{
			return {};
		}
}

void StorageEngineSqlite::setOutageDescription(uint64_t outageId, std::string description)
{
	update("Outages", outageId, {
			{ "updated_dtm", timeformat(std::chrono::system_clock::now()) },
			{ "userdes", description }
		});
}

void StorageEngineSqlite::setOutageTags(uint64_t outageId, std::string tags)
{
	update("Outages", outageId, {
			{ "updated_dtm", timeformat(std::chrono::system_clock::now()) },
			{ "tags", tags }
		});
}

std::list <std::map < std::string, std::string > > StorageEngineSqlite::getOutages(const std::map<std::string, std::string>& conditions)
{
	std::list <std::pair < std::string, std::string> > queryConditions;
	auto _from = conditions.find("from");
	auto _to = conditions.find("to");
	auto _limit = conditions.find("limit");
	
	std::string extra;
	if ( (_from == conditions.end()) || (_to == conditions.end()) )
		{
			logger("StorageEngine::getOutages() bad request. Missing from or to", 1);
			return {};
		}
	if (_limit != conditions.end())
		extra+=" LIMIT "+_limit->second;
	
	queryConditions.push_back({ "O.created_dtm>=", _from->second});
	queryConditions.push_back({ "O.created_dtm<=", _to->second});
	auto servs = conditions.find("services");
	if (servs != conditions.end())
		{
			auto ss = GCommon::tokenize(servs->second, (char)',');
			auto lastOne = --ss.end();
			
			for (auto srv=ss.begin(); srv!=ss.end(); ++srv)
				{
					auto serviceId = getServiceId(*srv, false);
					if (serviceId==0)
						{
							throw SermonException("Service: "+*srv+" not found");
						}
					if (srv==ss.begin())	/* First one */
						queryConditions.push_back({std::string((srv==lastOne)?"":"(")+"Service_id", std::to_string(serviceId)});
					else if (srv==lastOne) /* Last one */
						queryConditions.push_back({"|Service_id)", std::to_string(serviceId)});
					else
						queryConditions.push_back({"|Service_id", std::to_string(serviceId)});
				}
		}

	auto data = getData("Outages O LEFT JOIN Services S ON O.Service_id=S.rowid", "O.rowid AS Id, O.*, S.id AS Service_name", queryConditions, extra);
	
	return data;
}

std::list <std::map < std::string, std::string > > StorageEngineSqlite::getOrphanOutages(const std::map<std::string, std::string>& conditions)
{
	// SELECT O.rowid, S.id, O.* FROM Outages O LEFT JOIN Services S ON S.rowid=O.Service_id  WHERE O.Solved=0 AND O.Created_dtm IS NOT (SELECT MAX(Created_dtm) FROM Outages OO WHERE OO.Service_id=O.Service_id);

	std::list <std::pair < std::string, std::string> > queryConditions = {
		{ "O.Solved", "0" },
		{ "O.Created_dtm!=", "F((SELECT MAX(Created_dtm) FROM Outages OO WHERE OO.Service_id=O.Service_id)" }
	};
	
	auto data = getData("Outages O LEFT JOIN Services S ON O.Service_id=S.rowid", "O.rowid AS Id, S.id AS Service_name, O.*", queryConditions);
	
	return data;
	//	return {};
}

uint64_t StorageEngineSqlite::doFixOrphanOutages(int32_t newCode)
{
	std::list <std::pair < std::string, std::string> > queryConditions = {
		{ "O.Solved", "0" },
		{ "O.Created_dtm!=", "F((SELECT MAX(Created_dtm) FROM Outages OO WHERE OO.Service_id=O.Service_id)" }
	};

	return update("Outages", {
			{ "solved", std::to_string(newCode) },
			{ "updated_dtm", "F(DATETIME('now')" }
		},
		{
			{ "Solved", "0" },
			{ "Created_dtm!=", "F((SELECT MAX(Created_dtm) FROM Outages OO WHERE OO.Service_id=Outages.Service_id)" }
		});		
}

std::list <std::map < std::string, std::string > > StorageEngineSqlite::fixOrphanOutages(uint32_t newCode, const std::map<std::string, std::string>& conditions)
{
	auto outages = getOrphanOutages(conditions);
	auto n = doFixOrphanOutages(newCode);
	return outages;
}

std::list <std::map < std::string, std::string > > StorageEngineSqlite::getResponseTimeStats(const std::map<std::string, std::string>& conditions)
{
	std::list <std::pair < std::string, std::string> > queryConditions;
	auto _from = conditions.find("from");
	auto _to = conditions.find("to");

	if (_from != conditions.end())
		queryConditions.push_back({ "RT.probe_dtm>=", _from->second});
	if (_to != conditions.end())
		queryConditions.push_back({ "RT.probe_dtm<=", _to->second});
	
	auto servs = conditions.find("services");
	if (servs != conditions.end())
		{
			auto ss = GCommon::tokenize(servs->second, (char)',');
			auto lastOne = --ss.end();
			
			for (auto srv=ss.begin(); srv!=ss.end(); ++srv)
				{
					auto serviceId = getServiceId(*srv, false);
					if (serviceId==0)
						{
							throw SermonException("Service: "+*srv+" not found");
						}
					if (srv==ss.begin())	/* First one */
						queryConditions.push_back({std::string((srv==lastOne)?"":"(")+"Service_id", std::to_string(serviceId)});
					else if (srv==lastOne) /* Last one */
						queryConditions.push_back({"|Service_id)", std::to_string(serviceId)});
					else
						queryConditions.push_back({"|Service_id", std::to_string(serviceId)});
				}
		}
	else if ((servs = conditions.find("Service_id")) != conditions.end())
		{
			queryConditions.push_back({ "Service_id", servs->second});
		}

	auto data = getData("ResponseTime RT LEFT JOIN Services S ON RT.Service_id=S.rowid", "S.id AS Service_name, S.rowid AS Service_id, MIN(probe_dtm) AS Time_start, MAX(probe_dtm) AS Time_end, MIN(time) AS Response_min, MAX(time) AS Response_max, AVG(time) AS Response_avg", queryConditions, "GROUP BY service_id");
	
	return data;
}

std::map <uint64_t , std::map < std::string, std::string > > StorageEngineSqlite::getOutageStats(const std::map<std::string, std::string>& conditions)
{
	/* SELECT service_id, COUNT(outtime) AS Outtimes_count, AVG(outtime) AS Outtime_avg, MIN(outtime) AS Outtime_min, MAX(outtime) AS Outtime_max, MIN(created_dtm) AS Outages_start, MAX(created_dtm) AS Outages_end, * FROM (SELECT STRFTIME("%s",updated_dtm)-STRFTIME("%s",created_dtm) AS outtime, * FROM Outages WHERE solved=1) GROUP BY service_id; */

	std::list <std::pair < std::string, std::string> > queryConditions;
	auto _from = conditions.find("from");
	auto _to = conditions.find("to");

	if (_from != conditions.end())
		queryConditions.push_back({ "O.created_dtm>=", _from->second});
	
	if (_to != conditions.end())
		queryConditions.push_back({ "O.created_dtm<=", _to->second});
	
	auto servs = conditions.find("services");
	if (servs != conditions.end())
		{
			auto ss = GCommon::tokenize(servs->second, (char)',');
			auto lastOne = --ss.end();
			
			for (auto srv=ss.begin(); srv!=ss.end(); ++srv)
				{
					auto serviceId = getServiceId(*srv, false);
					if (serviceId==0)
						{
							throw SermonException("Service: "+*srv+" not found");
						}
					if (srv==ss.begin())	/* First one */
						queryConditions.push_back({std::string((srv==lastOne)?"":"(")+"Service_id", std::to_string(serviceId)});
					else if (srv==lastOne) /* Last one */
						queryConditions.push_back({"|Service_id)", std::to_string(serviceId)});
					else
						queryConditions.push_back({"|Service_id", std::to_string(serviceId)});
				}
		}
	else if ((servs = conditions.find("Service_id")) != conditions.end())
		{
			queryConditions.push_back({ "Service_id", servs->second});
		}
	
	auto solved = getData("(SELECT STRFTIME(\"%s\",updated_dtm)-STRFTIME(\"%s\",created_dtm) AS outtime, * FROM Outages WHERE solved=1) O LEFT JOIN Services S ON O.service_id=S.rowid",
												"O.service_id AS Service_id, S.id AS Service_name, COUNT(O.outtime) AS Outtimes_count, AVG(O.outtime) AS Outtime_avg, MIN(O.outtime) AS Outtime_min, MAX(O.outtime) AS Outtime_max, MIN(O.created_dtm) AS Outages_start, MAX(O.created_dtm) AS Outages_end", queryConditions, "GROUP BY service_id");

	queryConditions.push_back({"O.solved", "-1"});
	auto probeErrors = getData("Outages O LEFT JOIN Services S ON O.service_id=S.rowid",
														 "O.service_id AS Service_id, S.id AS Service_name, COUNT(O.rowid) AS Probe_errors, MIN(O.Created_dtm) AS First_dtm, MAX(O.Created_dtm) AS Last_dtm",
														 queryConditions,
														 "GROUP BY service_id");

	std::map <uint64_t , std::map < std::string, std::string > > output;
	for (auto i : solved)
		{
			auto serviceId = stoll(i["Service_id"]);
			auto oel = output.find(serviceId);
			if (oel == output.end())
				{
					auto inst = output.insert({serviceId, {
								{ "Service_id",  std::to_string (serviceId) },
								{"Service_name", i["Service_name"] },									
							}});
					if (!inst.second)
						throw SermonException("Cannot collect stats information");
					
					oel = inst.first;
				}

			oel->second["Outimes_count"] = std::to_string(std::stoll(i["Outtimes_count"]));
			oel->second["Outtime_avg"] = i["Outtime_avg"];
			oel->second["Outtime_min"] = i["Outtime_min"];
			oel->second["Outtime_max"] = i["Outtime_max"];
			oel->second["Outages_start"] = i["Outages_start"];
			oel->second["Outages_end"] = i["Outages_end"];		
		}

		for (auto i : probeErrors)
		{
			auto serviceId = stoll(i["Service_id"]);
			auto oel = output.find(serviceId);
			if (oel == output.end())
				{
					auto inst = output.insert({serviceId, {
								{ "Service_id",  std::to_string (serviceId) },
								{"Service_name", i["Service_name"] },									
							}});
					if (!inst.second)
						throw SermonException("Cannot collect stats information");
					
					oel = inst.first;
				}
			oel->second["Probe_errors"] = std::to_string(std::stoll(i["Probe_errors"]));
			oel->second["Probe_err_first"] = i["First_dtm"];
			oel->second["Probe_err_last"] = i["Last_dtm"];
		}

		/* return solved; */
	return output;
}

std::map<std::string, std::map < std::string, std::string > > StorageEngineSqlite::getServicesDataByName(const std::map<std::string, std::string>& conditions)
{
	std::map<std::string, std::map < std::string, std::string > > output;
	auto outageStats = getOutageStats(conditions);
	auto responseStats = getResponseTimeStats(conditions);

	for (auto& s : responseStats)
		{
			auto serviceId = std::stoll(s["Service_id"]);
			auto outagess = outageStats.find(serviceId);
			if (outagess != outageStats.end())
				{
					for (auto oss : outagess->second)
						{
							s[oss.first] = oss.second;
						}
				}
			
			output.insert({ s["Service_name"], s });
		}
	
	return output;
}

std::list <std::map<std::string, std::string>> StorageEngineSqlite::getAllResponseTimes(const std::map<std::string, std::string>& conditions)
{
	std::list <std::pair < std::string, std::string> > queryConditions;
	auto _from = conditions.find("from");
	auto _to = conditions.find("to");
	auto _sid = conditions.find("Service_id");
	
	if ( (_from == conditions.end()) || (_to == conditions.end()) || (_sid == conditions.end()) )
		throw SermonException("Not enough data for getAllResponseTimes");
	
	queryConditions.push_back({ "RT.probe_dtm>=", _from->second});
	queryConditions.push_back({ "RT.probe_dtm<=", _to->second});
	queryConditions.push_back({ "RT.service_id", _sid->second});

	return getData("ResponseTime RT", "rowid AS Response_id, probe_dtm AS Probe_dtm, time, error", queryConditions, "ORDER BY probe_dtm ASC");
}

std::list <std::map<std::string, std::string>> StorageEngineSqlite::getAllOutages(const std::map<std::string, std::string>& conditions)
{
	std::list <std::pair < std::string, std::string> > queryConditions;
	auto _from = conditions.find("from");
	auto _to = conditions.find("to");
	auto _sid = conditions.find("Service_id");
	
	if ( (_from == conditions.end()) || (_to == conditions.end()) || (_sid == conditions.end()) )
		throw SermonException("Not enough data for getAllResponseTimes");
	
	queryConditions.push_back({ "O.created_dtm>=", _from->second});
	queryConditions.push_back({ "O.created_dtm<=", _to->second});
	queryConditions.push_back({ "O.service_id", _sid->second});

	return getData("Outages O", "rowid AS Outage_num, uuid AS Outage_id, created_dtm AS Created_dtm, updated_dtm AS `Update`, totaltime, solved", queryConditions, "ORDER BY created_dtm ASC");
}

