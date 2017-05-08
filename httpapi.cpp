#include "httpapi.hpp"
#include "lib/timeutils.hpp"
#include "lib/gutils.hpp"
#include "sermon_exception.hpp"
#include <thread>
#include <iomanip>

HttpApi::HttpApi(Sermon& sermon, const nlohmann::json &settings):sermon(sermon)
{
	if ( (settings.find("enabled") == settings.end()) || (!settings["enabled"].get<bool>()) )
		return;

	logger("[HTTPApi] initializing...", 3);
	
	if (settings.find("port") != settings.end() && (settings["port"].is_number()))
		port = settings["port"].get<uint16_t>();
	else
		port=8761;

	if (settings.find("listen") != settings.end() && (settings["listen"].is_string()))
		listen = settings["listen"].get<std::string>();
	else
		listen="localhost";

	if (settings.find("max_accepted_clients") != settings.end() && (settings["max_accepted_clients"].is_number_float()))
		max_accepted_clients = settings["max_accepted_clients"].get<uint64_t>();
	else
		max_accepted_clients=2;

	if (settings.find("timeout") != settings.end() && (settings["timeout"].is_number()))
		timeout = settings["timeout"].get<double>();
	else
		timeout=5;
			
	server = std::make_shared<GloveHttpServer>();
	server->addRoute("/", HttpApi::index);
	server->addRest("/sermon/status", 0, GloveHttpServer::jsonApiErrorCall,
									(GloveHttpServer::url_callback)std::bind(&HttpApi::sermonStatus, this, std::placeholders::_1, std::placeholders::_2),
									(GloveHttpServer::url_callback)std::bind(&HttpApi::sermonStatusPost, this, std::placeholders::_1, std::placeholders::_2)
	);
	server->addRest("/services/basic", 0, GloveHttpServer::jsonApiErrorCall,
									(GloveHttpServer::url_callback)std::bind(&HttpApi::servicesBasicData, this, std::placeholders::_1, std::placeholders::_2));
	server->addRest("/response/stats/$id", 0, GloveHttpServer::jsonApiErrorCall,
									(GloveHttpServer::url_callback)std::bind(&HttpApi::responseStats, this, std::placeholders::_1, std::placeholders::_2));
	server->addRest("/errors/realtime", 0, GloveHttpServer::jsonApiErrorCall,
									(GloveHttpServer::url_callback)std::bind(&HttpApi::realtimeErrors, this, std::placeholders::_1, std::placeholders::_2));
	server->addRest("/errors/history/$id", 0, GloveHttpServer::jsonApiErrorCall,
									(GloveHttpServer::url_callback)std::bind(&HttpApi::historyErrorsGet, this, std::placeholders::_1, std::placeholders::_2),
									(GloveHttpServer::url_callback)std::bind(&HttpApi::historyErrorsPost, this, std::placeholders::_1, std::placeholders::_2)
	);

	server->listen(port, listen);
	server->tmcReject(true, max_accepted_clients, "Too many connections");
	server->max_accepted_clients(max_accepted_clients);
	server->timeout(timeout);

}

nlohmann::json HttpApi::outputTemplate(std::string module, std::string action, std::string error)
{
	nlohmann::json jsout({
			{ "module", module},
			{ "action", action },
			{ "success", (error.empty())?"1":"0" },
			{ "error", (error.empty())?"0":error }
		});
		return jsout;
}

void HttpApi::realtimeErrors(GloveHttpRequest& request, GloveHttpResponse& response)
{
	auto _dbdata = request.getUri().arguments["dbdata"];
	bool dbdata = ( (_dbdata=="true") || (_dbdata=="1") );
	
	nlohmann::json jsout = outputTemplate("errors/realtime", "get", "");
	jsout["data"] = sermon.currentOutages(dbdata);
	response << jsout.dump(2);
}

void HttpApi::historyErrorsGet(GloveHttpRequest& request, GloveHttpResponse& response)
{
	auto id = request.special["id"];	
	auto args = request.getUri().arguments;
	auto _from = args["from"];
	auto _to = args["to"];
	auto output = args["output"];
	if (output.empty())
		output="json";
	auto showOutageIds = (args["showOutageIds"] == "1");
	
	std::string  _services = args["services"];
	GCommon::trim(_services);
	
	time_t from = (!_from.empty())?parseTime(_from):parseTime("daystart"),
		to = (!_to.empty())?parseTime(_to):parseTime("now");

	/* Now is now or a time in the future */
	bool untilnow = (to >= parseTime("now"));
	
	std::vector<std::string> services;
	if (!_services.empty())
		{
			auto __services = GCommon::tokenize(_services, ',');
			/* services.emplace_back(std::move(__services.front())); */
			std::copy(__services.begin(), __services.end(), std::inserter(services, services.end()));
		}
	std::map<std::string, std::string> options;
	try
		{
			nlohmann::json data;
			if (id.empty())
				data = sermon.getHistoryOutages(from, to, services, options);
			else if (!GCommon::isNumeric(id))
				data = sermon.getHistoryOutage(id);
			else
				data = sermon.getHistoryOutage(std::stoll(id));
			
			if (output == "json")
				{
					auto soutput = outputTemplate("errors/history", "get", "");
					std::map < std::string, std::string> input;
					if (!id.empty())
						input.insert({"Id: ", id});
					else
						{
							input.insert({ "From", put_time("%d/%m/%Y %H:%M:%S", from) });
							input.insert({ "To", put_time("%d/%m/%Y %H:%M:%S", to) });
							if (!services.empty())
								input.insert({"Services", _services});							
						}
					soutput["input"] = input;
					soutput["data"] = data;
					response << soutput.dump(2);
				}
			else if (output == "csv")
				{
					response << "\"Outage ID\", \"Timestamp\", \"Outage UUID\", \"Service\", \"Description\", \"Solved\", \"Bounces\", \"Outage Time\", \"User description\", \"Tags\"\n";
					for (auto d : data)
						{
							std::string line;
							line+=GCommon::quote(std::to_string(std::stoll(d["Id"].get<std::string>())))+", ";
							line+=GCommon::quote(d["created_dtm"])+", ";
							line+=GCommon::quote(d["uuid"])+", ";
							line+=GCommon::quote(d["Service_name"])+", ";
							line+=GCommon::quote(d["description"])+", ";
							line+=GCommon::quote(std::to_string(std::stoll(d["solved"].get<std::string>())))+", ";
							line+=GCommon::quote(std::to_string(std::stoll(d["bounces"].get<std::string>())))+", ";
							line+=GCommon::quote(d["totaltime"])+", ";
							line+=GCommon::quote(d["userdes"])+", ";
							line+=GCommon::quote(d["tags"]);

							line+="\n";
							response << line;
						}
				}
			else if (output == "txt")
				{
					if (!id.empty())
						response << "ID: "<<id<<"\n";
					else
						{
							response << "From: "<<put_time("%d/%m/%Y %H:%M:%S", from)<<"\n";
							response << "To: "<<put_time("%d/%m/%Y %H:%M:%S", to)<<"\n";
							response << "Services: "<<_services<<"\n";
						}
					response << "\n";

					/* It got out of hand!! I think I could make a struct for this data.
						 Initialize it at once, and don't waste so many lines.
						 Some days has passed since I began this function and I was thinking of new
						features all the time.
						*/
					/* Downtime for each service <serviceId, downtimesecs> */
					std::map<uint64_t, uint64_t> totalDowntime;
					/* Flags for services <serviceId, flags> flags can be: unsolved outages or probe fails */
					std::map<uint64_t, uint32_t> serviceFlags;
					/* Outage count */
					std::map<uint64_t, uint64_t> outageCount;
					/* Service names */
					std::map<uint64_t, std::string> serviceNames;
					
					bool serviceErrors = false;
					uint64_t totalSeconds = to - from;
					for (auto _ou=data.begin(); _ou!=data.end(); _ou++)
						{
							auto ou = *_ou;
							auto serviceId = std::stoll(ou["service_id"].get<std::string>());
							if ( (serviceId == 0) || (serviceId == (uint64_t)-1) )
								{
									response << "Service Id "<<ou["service_id"].get<std::string>()<<" not found!! Skipping...\n";
									serviceErrors= true;
									continue;
								}
							auto outageId = std::stoll(ou["Id"].get<std::string>());
							auto _totime = parseTime(ou["updated_dtm"].get<std::string>());
							auto _fromtime = parseTime(ou["created_dtm"].get<std::string>());
							auto tostr = put_time("%d/%b/%Y %H:%M:%S", _totime);
							auto fromstr = put_time("%d/%b/%Y %H:%M:%S", _fromtime);
							auto solved = std::stoll(ou["solved"].get<std::string>());
							uint64_t _totaltime;
							try
								{
									_totaltime = stoll(ou["totaltime"].get<std::string>());
								}
							catch (std::exception &e)
								{
									_totaltime = 0; /* Could be (null) or other value... */
								}
							auto _userdes = ou["userdes"].get<std::string>();
							auto userdes = (_userdes == "(null)")?"":_userdes;
							auto _downtime = totalDowntime.find(serviceId);
							if (_downtime == totalDowntime.end())
								{
									serviceNames.insert({ serviceId, ou["Service_name"].get<std::string>() }); 
									totalDowntime.insert({ serviceId, 0 });
									_downtime = totalDowntime.find(serviceId);
									if (_downtime == totalDowntime.end())
										throw SermonException("failed inserting downtime map!", 999); 
								}
							auto _flags = serviceFlags.find(serviceId);
							if (_flags == serviceFlags.end())
								{
									serviceFlags.insert({ serviceId, 0 });
									_flags = serviceFlags.find(serviceId);
									if (_flags == serviceFlags.end())
										throw SermonException("failed inserting flags map!", 999); 
								}
							auto _outagec = outageCount.find(serviceId);
							if (_outagec == outageCount.end())
								{
									outageCount.insert({ serviceId, 0 });
									_outagec = outageCount.find(serviceId);
									if (_outagec == outageCount.end())
										throw SermonException("failed inserting outage count map!", 999); 
								}
							_outagec->second++;
							
							if (solved==0)
								{
									auto newer = findNewerOutage(data, _ou);
									if (newer != data.end())
										solved=-1;
									else if (!untilnow)
										{
											auto newdata = sermon.getHistoryOutages(_fromtime, parseTime("now"), { ou["Service_name"].get<std::string>() }, { { "limit", "2" } });
											/* We try searching database, if we find any occurrence after this one, there must be a probe error */
											if (newdata.size()>1)
												solved=-1;
										}
									if (solved!=0)
										sermon.setOutageStatusError(outageId, solved); /* Error!! */
									/* Try to repair */
								}
							if (solved!=0)
								_downtime->second=_downtime->second+_totaltime;
							if (solved==0)
								_flags->second|=1;
							else if (solved==-1)
								_flags->second|=2;

							if (showOutageIds)
								response << "#" << outageId << "\t";
							response << "["<<tostr<<"] ("<<ou["Service_name"].get<std::string>()<<") ";
							if (solved>0)
								response <<ou["description"].get<std::string>()<<" from "<<fromstr<<" to "<<tostr<<" ("<<itemized_to_str_num(get_itemized(std::chrono::seconds(_totaltime)), false)<<") "<<userdes<<"\n";
							else if (solved==-1)
								response <<ou["description"].get<std::string>()<<" on "<<fromstr<<". Probe error! "<<userdes<<"\n";							
							else
								response <<ou["description"].get<std::string>()<<" from "<<fromstr<<". Not solved yet! "<<userdes<<"\n";
						}
					/* Total downtime: FALTA CALCULARLO */
					for (auto tdt : totalDowntime)
						{
							if (tdt.second >= 0)
								{
									response << serviceNames[tdt.first] << std::endl;
									response << "  Total downtime "<< put_time("%d/%b/%Y %H:%M:%S", from)<<
										" - " << put_time("%d/%m/%Y %H:%M:%S", to)<<" : " <<
										itemized_to_str_num(get_itemized(std::chrono::seconds(tdt.second)), false) <<
										" (uptime "<< ( 1 - (double)tdt.second / totalSeconds ) <<")\n";
									response << "  Outages: " << outageCount[tdt.first] << ". Mean solve time: "<<
										itemized_to_str_num(get_itemized(std::chrono::seconds(tdt.second/outageCount[tdt.first])), false) << "\n";
									if (serviceFlags[tdt.first] & 1)
										response << "   * Caution! Unsolved outage!!\n";
									if (serviceFlags[tdt.first] & 2)
										response << "   * Caution! Probe errors!!\n";
								}
						}

				}
		}
	catch (std::exception &e)
		{
			if (output=="json")
				response<< outputTemplate("errors/history", "get", e.what()).dump(2);
			else
				response << "Error: "<<e.what()<<"\n";
		}
}

void HttpApi::historyErrorsPost(GloveHttpRequest& request, GloveHttpResponse& response)
{
	try
		{
			auto reqdata = nlohmann::json::parse(request.getData());
			auto id = request.special["id"];	

			if (id.empty())
				throw SermonException("You must insert an #id or uuid", 999); 

			nlohmann::json _data;

			if (!GCommon::isNumeric(id))
				_data = sermon.getHistoryOutage(id);
			else
				_data = sermon.getHistoryOutage(std::stoll(id));

			if (_data.is_null())
				throw SermonException("Outage not found", 999); 

			auto data = _data[0];
			
			auto out = outputTemplate("errors/history", "post", "");
			auto userdes = reqdata["userdes"];
			auto tags = reqdata["tags"];

			if (!userdes.is_null())
				{
					if (!userdes.is_string())
						throw SermonException("User description is not a string!!", 999);

					sermon.changeOutageDescription(std::stoll(data["Id"].get<std::string>()), userdes.get<std::string>());
				}

			if (!tags.is_null())
				{
					if (!tags.is_string())
						throw SermonException("Tags field is not a string!!", 999);
					
					sermon.changeOutageDescription(std::stoll(data["Id"].get<std::string>()), tags.get<std::string>());
				}
			
			out["data"] = data;
			response << out.dump(2);
		}
	catch (std::exception& e)
		{
			auto out = outputTemplate("errors/history", "post", "Bad request: "+std::string(e.what()));
			response << out.dump(2);
		}	
	/* Modify notes */
}

void HttpApi::servicesBasicData(GloveHttpRequest& request, GloveHttpResponse& response)
{
	/* Service id, Service name, probe errors, last probe, last status, configuration data */
	auto args = request.getUri().arguments;
	auto _from = args["from"];
	auto _to = args["to"];
	auto output = args["output"];
	if (output.empty())
		output="json";
	
	std::string  _services = args["services"];
	GCommon::trim(_services);
	
	std::vector<std::string> services;
	if (!_services.empty())
		{
			auto __services = GCommon::tokenize(_services, ',');
			std::copy(__services.begin(), __services.end(), std::inserter(services, services.end()));
		}

	nlohmann::json out;
	try
		{
			auto data = sermon.getServicesBasicData(services, {});
			if (output == "json")
				{
					out = outputTemplate("services/basic", "get", "");
					out["data"] = data;
					response << out.dump(2);
				}
			else
				throw SermonException("No more output methods implemented", 999); 

		}
	catch (std::exception& e)
		{
			out = outputTemplate("services/basic", "get", "Error: "+std::string(e.what()));
			response << out.dump(2);
		}
	
}

void HttpApi::sermonStatusPost(GloveHttpRequest& request, GloveHttpResponse& response)
{
	try
		{
			auto reqdata = nlohmann::json::parse(request.getData());
			std::vector < std::string > messages;
			
			if (reqdata["paused"].is_boolean())
				{
					sermon.paused(reqdata["paused"].get<bool>());
					messages.push_back("Sermon paused changed to "+std::to_string(reqdata["paused"].get<bool>()));
				}
			auto out = outputTemplate("sermon/status", "post", "");
			if (messages.size()==0)
				out["message"] = "No changes made";
			else
				out["data"] = messages;
			
			response << out.dump(2);
		}
	catch (std::exception& e)
		{
			auto out = outputTemplate("sermon/status", "post", "Bad request: "+std::string(e.what()));
			response << out.dump(2);
		}
}

void HttpApi::sermonStatus(GloveHttpRequest& request, GloveHttpResponse& response)
{
	auto out = outputTemplate("sermon/status", "get", "");
	out["data"] = {
		{ "paused", sermon.paused() },
		{ "last_action", sermon.lastAction() },
		{ "last_action_dtm", put_time("%d/%m/%Y %H:%M:%S", std::chrono::system_clock::to_time_t(sermon.lastActionDtm())) },
		{ "last_probe", sermon.lastProbe() },
		{ "last_probe_dtm", put_time("%d/%m/%Y %H:%M:%S", std::chrono::system_clock::to_time_t(sermon.lastProbeDtm())) }
	};
	response << out.dump(2);
}

/* Min. Max. Avg response time */
void HttpApi::responseStats(GloveHttpRequest& request, GloveHttpResponse& response)
{
	auto args = request.getUri().arguments;
	auto _from = args["from"];
	auto _to = args["to"];
	auto output = args["output"];
	if (output.empty())
		output="json";
	auto id = request.special["id"];	

	time_t from = (!_from.empty())?parseTime(_from):10000, /* by default a date in the past*/
		to = (!_to.empty())?parseTime(_to):parseTime("now");

	nlohmann::json out;
	try
		{
			std::string  _services = args["services"];
			if ( (!_services.empty()) && (!id.empty()) )
		
				GCommon::trim(_services);
	
			std::vector<std::string> services;
			if (!_services.empty())
				{
					auto __services = GCommon::tokenize(_services, ',');
					/* services.emplace_back(std::move(__services.front())); */
					std::copy(__services.begin(), __services.end(), std::inserter(services, services.end()));
				}


			if ( (!id.empty()) && (!services.empty()) )
				throw SermonException("Use services attribute or service Id, not both", 999); 

			nlohmann::json data;
			if (!id.empty())
				data = sermon.getServiceStats(from, to, std::stoll(id));
			else
				data = sermon.getServiceStats(from, to, services, {});
			if (output == "json")
				{
					out = outputTemplate("response/stats", "get", "");
					out["data"] = data;
					response << out.dump(2);
				}
			else if (output=="csv")
				{
					if (!id.empty())
						{
							response << "# Time; Response time\n";
							auto& resdata = data["stats"]["response"];
							/* En el futuro podemos hacerlo para los outages, los recovery times y demás, pero por ahora sólo está para esto */
							for (auto d: resdata)
								{
									response << d["Probe_dtm"]<<";"<<d["time"]<<"\n";
								}
							/* Only one service */
						}
					else
						{
							/* Multiple services */
							response << "Service ID;Service name;Time start;Time End;Response avg;"
								"Response min;Response max;Probe errors;Probe_err_first;Probe_err_last;"
								"Outtime_avg;Outtime_max;Outtime_min;Outtimes_count;Outages_start;Outages_end\n";
							for (auto d: data)
								{
									response << std::stoll(d["Service_id"].get<std::string>())<<";";
									response << d["Service_name"]<<";";
									response << ((d["Time_start"].is_null())?"":d["Time_start"])<< ";";
									response << ((d["Time_end"].is_null())?d["Time_end"]:"")<< ";";
									response << ((d["Response_avg"].is_null())?"":d["Response_avg"])<< ";";
									response << ((d["Response_min"].is_null())?"":d["Response_min"])<< ";";
									response << ((d["Response_max"].is_null())?"":d["Response_max"])<< ";";
									response << ((d["Probe_errors"].is_null())?0:std::stoll(d["Probe_errors"].get<std::string>()))<< ";";
									response << ((d["Probe_err_first"].is_null())?"":d["Probe_err_first"])<< ";";
									response << ((d["Probe_err_last"].is_null())?"":d["Probe_err_last"])<< ";";
									response << ((d["Outtime_avg"].is_null())?"":d["Outtime_avg"])<< ";";
									response << ((d["Outtime_max"].is_null())?"":d["Outtime_max"])<< ";";
									response << ((d["Outtime_min"].is_null())?"":d["Outtime_min"])<< ";";
									response << ((d["Outtimes_count"].is_null())?0:std::stoll(d["Outtimes_count"].get<std::string>()))<< ";";
									response << ((d["Outages_start"].is_null())?"":d["Outages_start"])<< ";";
									response << ((d["Outages_end"].is_null())?"":d["Outages_end"])<< ";";							
									response << "\n";
								}
						}
				}

		}
	catch (std::exception& e)
		{
			out = outputTemplate("response/stats", "get", "Error: "+std::string(e.what()));
			response << out.dump(2);
		}
}

void HttpApi::index(GloveHttpRequest& request, GloveHttpResponse& response)
{
	response<< "Main page. Tiny documentation here\n";
}

void HttpApi::finish()
{
	/* End listening socket */
}

void HttpApi::logger(std::string what, int verbosity)
{
	sermon.say(what, verbosity);
}

/* Find outage newer than current but with same service */
nlohmann::json::iterator HttpApi::findNewerOutage(nlohmann::json& data, nlohmann::json::iterator from)
{
	auto currservice = std::stoll((*from)["service_id"].get<std::string>());
	while (++from != data.end())
		{
			auto itservice = std::stoll((*from)["service_id"].get<std::string>());
			if (itservice == currservice)
				return from;
		}
	return data.end();
}
