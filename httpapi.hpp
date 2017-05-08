#pragma once

#include "sermon_app.hpp"
#include "json/src/json.hpp"
#include "glove/glovehttpserver.hpp"

							
class HttpApi
{
public:
	HttpApi(Sermon& sermon, const nlohmann::json &settings);
	virtual ~HttpApi()
	{
	}

	void finish();

protected:
  static void index(GloveHttpRequest& request, GloveHttpResponse& response);
	void sermonStatus(GloveHttpRequest& request, GloveHttpResponse& response);
	void sermonStatusPost(GloveHttpRequest& request, GloveHttpResponse& response);
  void realtimeErrors(GloveHttpRequest& request, GloveHttpResponse& response);
  void servicesBasicData(GloveHttpRequest& request, GloveHttpResponse& response);
  void responseStats(GloveHttpRequest& request, GloveHttpResponse& response);
  void historyErrorsGet(GloveHttpRequest& request, GloveHttpResponse& response);
  void historyErrorsPost(GloveHttpRequest& request, GloveHttpResponse& response);
	
private:
	void logger(std::string what, int verbosity);	
	static nlohmann::json outputTemplate(std::string module, std::string action, std::string error);
	nlohmann::json::iterator findNewerOutage(nlohmann::json& data, nlohmann::json::iterator from);

	Sermon& sermon;
	uint16_t port;
  std::shared_ptr<GloveHttpServer> server;	
  std::string listen;
	double timeout;
	uint64_t max_accepted_clients;
};
