/* @(#)sermon_app.hpp
 */

#ifndef _SERMON_APP_H
#define _SERMON_APP_H 1

#include <string>
#include <vector>
#include <map>
#include <memory>
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

  Sermon();
  virtual ~Sermon();

  /* Let's play! */
  void monitoring();

  void say(std::string what, int verbosity=5);
protected:
  void debug_notifiers();
  void debug_services();
  void serviceFail(const Service& s, int code, const std::string& message);
  void removePendingFails(const Service &s);

  void insertNotifier(const nlohmann::json &notifierJson);
  void insertService(const nlohmann::json &serviceJson);
  void siteProbe(Sermon::Service serv);
  int getUrlData(std::string url, std::string &out, double timeout=-1, bool checkCertificates=true, int max_redirects=2);

private:
  void loadConfig(std::string configFile);

  long tbp;			/* Time between probes*/
  long sap;			/* Sleep after probe (ms)*/
  int verbosity;		/* Verbosity level*/
  int maxRedirects;		/* Maximum redirects*/
  double timeout;		/* Default timeout for all services */
  bool checkCertificates;	/* Check certificates?*/
  std::vector <std::shared_ptr<Notify>> notifiers;
  std::vector <Service> services;
  std::map <std::string, ServiceFail> currentFails;
};

#endif /* _SERMON_APP_H */

