/* @(#)sermon_app.hpp
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include "notify.hpp"
#include "service_fail.hpp"
#include "json/src/json.hpp"


class Sermon
{
public:
  struct Service
  {
    std::string url;
    std::string name;
    double timeout;		/* in seconds */
    bool checkCertificates;
  };

  Sermon(std::string configFile="");
  virtual ~Sermon();

  /* Let's play! */
  void monitoring();
	void exit();

  void say(std::string what, int verbosity=5);
	std::vector<nlohmann::json> currentOutages(bool allData = false);
	/* Options, we can include min. outage times, unrecovered, etc */
	nlohmann::json getHistoryOutages(time_t from, time_t to, const std::vector<std::string>& services = {}, const std::map<std::string, std::string>& options = {}); 
	nlohmann::json getHistoryOutage(std::string uuid);
	nlohmann::json getHistoryOutage(uint64_t outageId);
	nlohmann::json getServiceStats(time_t from, time_t to, const std::vector<std::string>& services = {}, const std::map<std::string, std::string>& options = {});
	nlohmann::json getServiceStats(time_t from, time_t to, const uint64_t serviceId, const std::map<std::string, std::string>& options = {});
	nlohmann::json getServicesBasicData(const std::vector<std::string>& services = {}, const std::map<std::string, std::string>& options = {});

	void changeOutageDescription(uint64_t outageId, std::string description);
	void changeOutageTags(uint64_t outageId, std::string tags);
	void setOutageStatusError(uint64_t outageId, int error);
	bool paused();
	bool paused(bool status);
	std::string lastAction();
	std::string lastProbe();
	std::chrono::system_clock::time_point lastActionDtm();	
	std::chrono::system_clock::time_point lastProbeDtm();	
protected:
  void debug_notifiers();
  void debug_services();
  void serviceFail(const Service& s, int code, const std::string& message);
  void removePendingFails(const Service &s);
  void insertNotifier(const nlohmann::json &notifierJson);
  void insertService(const nlohmann::json &serviceJson);
  void siteProbe(Sermon::Service serv);
  int getUrlData(std::string url, std::string &out, double timeout=-1, bool checkCertificates=true, int max_redirects=2);

	std::string lastAction(std::string action);
	std::string lastProbe(std::string action);
	
private:
  void loadConfig(std::string configFile);

  long tbp;			/* Time between probes*/
  long sap;			/* Sleep after probe (ms)*/
  int verbosity;		/* Verbosity level*/
  int maxRedirects;		/* Maximum redirects*/
  double timeout;		/* Default timeout for all services */
  bool checkCertificates;	/* Check certificates?*/
	bool __exit;
	bool __paused;									/* Paused? */
	std::string __lastAction;
	std::string __lastProbe;
	std::chrono::system_clock::time_point __lastActionDtm;
	std::chrono::system_clock::time_point __lastProbeDtm;
	
  std::vector <std::shared_ptr<Notify>> notifiers;
  std::vector <Service> services;
	std::mutex currentFails_mutex;
	std::map <std::string, ServiceFail> currentFails;
};

