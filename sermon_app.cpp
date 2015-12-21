/**
*************************************************************
* @file sermon_app.cpp
* @brief Breve descripción
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 09 dic 2015
* Historial de cambios:
*
*
*
*
*
*
*
*************************************************************/

#include "sermon_app.hpp"
#include <iostream>
#include <fstream>
#include "lib/cfileutils.h"
#include "sermon_exception.hpp"
#include "request_exception.hpp"
#include "lib/tcolor.hpp"
#include "lib/timeutils.hpp"
#include <exception>
#include <chrono>
#include <thread>
#include "glove/glove.hpp"
#include "notifymail.hpp"
#include "notifycli.hpp"
#ifdef DESKTOP_NOTIFICATION
#include "notifydesktop.hpp"
#endif
namespace
{
  const std::string white_spaces( " \f\n\r\t\v" );

  /* Default config values as numbers */
  std::map<std::string, long> defaultNumbers = { {"tbp", 1}, {"sap", 100 }, { "verbosity", 1}, {"max_redirects", 10} };
  bool defaultCheckCertificate = true;
  double defaultTimeout = 20;

  bool findConfigFile(std::string &configFile)
  {
    std::vector<std::string> paths = {"/etc/sermon/config.json", "/etc/sermon.json", "./sermon.json", "./config.json", "./config/sermon.json"};

    for (auto p : paths)
      {
    	if (file_exists(p.c_str())==1)
    	  {
    	    configFile = p;
    	    return true;
    	  }
      }

    return false;
  }

  int parseFirstLine(std::string input, std::string& protocol, std::string &statusmsg)
  {
    std::string::size_type space_pos, space_pos2;
    std::string temp_status;
    int status=0;

    space_pos = input.find(' ');

    if (space_pos == std::string::npos)
      throw RequestException("Couldn't parse protocol from received data");

    protocol = input.substr(0, space_pos);
    space_pos2 = input.find(' ', space_pos+1);

    if (space_pos == std::string::npos)
      throw RequestException("Couldn't parse status from received data");

    temp_status = input.substr(space_pos+1, space_pos2-space_pos-1);
    statusmsg = input.substr(space_pos2+1);

    try
      {
	status = std::stoi(temp_status);
      }
    catch (std::invalid_argument ia)
      {
	throw RequestException("Wrong status code");
      }
    catch (std::out_of_range org)
      {
	throw RequestException("Wrong status code");
      }

    return status;
  }

  std::string trim( std::string str, const std::string& trimChars = white_spaces )
  {
    std::string::size_type pos_end = str.find_last_not_of( trimChars );
    std::string::size_type pos_start = str.find_first_not_of( trimChars );

    return str.substr( pos_start, pos_end - pos_start + 1 );
  }

  void extract_headers( std::string input, std::map<std::string, std::string> &headers, int start)
  {
    std::string::size_type colon;
    std::string::size_type crlf;

    if ( ( colon = input.find(':', start) ) != std::string::npos && 
	 ( ( crlf = input.find(Glove::CRLF, start) ) != std::string::npos ) )
      {
	if (crlf<colon)
	  {
	    /* Not a header!! */
	  }
	else
	  headers.insert( std::pair<std::string, std::string>( trim( input.substr(start, colon - start) ),
							       trim( input.substr(colon+1, crlf - colon -1 ) ) )
			  );

	extract_headers(input, headers, crlf+2);
      }
  }
};

Sermon::Sermon()
{
  std::string configFile;
  if (!findConfigFile(configFile))
    throw SermonException("Config file not found!!");

  std::cout << TColor(TColor::MAGENTA) << "Loading config file... ";
  loadConfig(configFile);
  std::cout << TColor(TColor::GREEN) << "OK" << endColor << "\n";

}

Sermon::~Sermon()
{
}

void Sermon::loadConfig(std :: string configFile)
{
  std::ifstream ifs(configFile);
  if (ifs.fail())
    throw SermonException("Could not open config file "+configFile);

  nlohmann::json json;
  try
    {
      json << ifs;
    }
  catch (std::invalid_argument ia)
    {
      throw SermonException ("Invalid argument in JSON ("+std::string(ia.what())+")");
    }
  catch (std::exception e)
    {
      throw SermonException ("Error reading JSON ("+std::string(e.what())+")");
    }

  /* Put everything in its variables */
  if (json.find("tbp") == json.end())
    this->tbp = defaultNumbers["tbp"];
  else
    this->tbp = json["tbp"].get<long>();

  if (json.find("sap") == json.end())
    this->sap = defaultNumbers["sap"];
  else
    this->sap = json["sap"].get<long>();

  if (json.find("verbosity") == json.end())
    this->verbosity = defaultNumbers["verbosity"];
  else
    this->verbosity = json["verbosity"].get<int>();

  if (json.find("max_redirects") == json.end())
    this->maxRedirects = defaultNumbers["max_redirects"];
  else
    this->maxRedirects = json["max_redirects"].get<int>();

  if (json.find("check_certificates") == json.end())
    this->checkCertificates = defaultCheckCertificate;
  else
    this->checkCertificates = json["check_certificates"].get<bool>();

  if (json.find("timeout") == json.end())
    this->timeout = defaultTimeout;
  else
    {
      std::cout << "TIMOEUT: "<<json["timeout"].get<double>()<<std::endl;
      this->timeout = json["timeout"].get<double>();
    }
  /* Timeout not done */

  auto notify = json.find("notify");
  if ((notify != json.end()) && (notify->is_array()) )
    {
      for (auto i : *notify)
	{
	  insertNotifier(i);
	}
    }
  /* debug_notifiers(); */

  auto services = json.find("services");
  if (services == json.end())
    throw SermonException("No services configured!!");

  for (auto i : *services)
    {
      insertService(i);
    }

  if (this->services.size()==0)
    throw SermonException("No services configured");
}

void Sermon::debug_notifiers()
{
  unsigned c=0;
  for (auto i: this->notifiers)
    {
      std::cout << std::endl << "-- NOTIFIER "<<c++<<"----------"<<std::endl;
      std::cout << "  TYPE: "<<i->getType()<<std::endl;
      for (auto j: i->getOptions())
	{
	  std::cout << "OPT("<<j.first<<") = "<<j.second<<std::endl;
	}
    }
}

void Sermon::debug_services()
{
  unsigned c=0;
  for (auto i: this->services)
    {
      std::cout << std::endl << "-- SERVICE "<<c++<<"----------"<<std::endl;
      std::cout << "  URL: "<<i.url<<std::endl;
    }
}

void Sermon::monitoring()
{
  /* We can use a stop variable. If we use multi-threading we
     could stop it from another thread. But with caution! */
  while (1)
    {
      for (auto i: this->services)
	{
	  try
	    {
	      /* Sample error */
	      /* throw GloveException(123, "Mensaje de error"); */
	      siteProbe(i);
	      removePendingFails(i);

	      /* Sleep after probe */
	      if (this->sap)
		std::this_thread::sleep_for(std::chrono::milliseconds(this->sap));
	    }
	  catch (GloveException &e)
	    {
	      this->serviceFail(i, e.code(), e.what());
	    }
	  catch (RequestException &e)
	    {
	      this->serviceFail(i, e.code(), e.what());
	    }
	}

      this->say("Sleeping for " + std::to_string(this->tbp) + "milliseconds...", 2);

      std::this_thread::sleep_for(std::chrono::milliseconds(this->tbp));
      this->say("Current fails: " + std::to_string(this->currentFails.size()), 3);
    }
}

void Sermon::serviceFail(const Service & s, int code, const std :: string & message)
{
  std::string serviceName = (s.name.empty())?s.url:s.name;
  if (this->currentFails.find(serviceName)!=this->currentFails.end())
    return;			/* If error found, end up here*/

  ServiceFail fail(s.name, this->notifiers, code, message);

  currentFails.insert({s.url, std::move(fail)});
}

void Sermon::removePendingFails(const Service & s)
{
  std::string serviceName = (s.name.empty())?s.url:s.name;
  auto fail = this->currentFails.find(serviceName);
  if (fail==this->currentFails.end())
    return;			/* If error found, end up here*/

  fail->second.end();
  this->currentFails.erase(fail);
}

void Sermon::insertNotifier(const nlohmann::json &notifierJson)
{
  std::map<std::string, std::string> options;
  std::string type;

  for (auto i=notifierJson.begin(); i != notifierJson.end(); ++i)
    {
      if (i.key() == "type")
	type = i.value().get<std::string>();
      else
	options[i.key()] = i.value().get<std::string>();
    }
  std::shared_ptr<Notify> notifier;
  if (type == "mail")
    {
      notifier = std::make_shared<NotifyMail>(NotifyMail("auto"));
    }
  else if (type == "cli")
    {
      notifier = std::make_shared<NotifyCli>(NotifyCli("auto"));
    }
#ifdef DESKTOP_NOTIFICATION
  else if (type == "desktop")
    {
      notifier = std::make_shared<NotifyDesktop>(NotifyDesktop("auto"));
    }
#endif
  else
    throw SermonException("Notifier type \""+type+"\" not found. Maybe you didn't compile with that support.\n");

  this->say("New notifier type: "+type, 4);
  notifier->setOptions(options);
  this->notifiers.push_back(notifier);
}

void Sermon::insertService(const nlohmann::json &serviceJson)
{
  std::string url;
  std::string name;
  double timeout = this->timeout;
  bool checkCertificates = this->checkCertificates;

  for (auto i=serviceJson.begin(); i != serviceJson.end(); ++i)
    {
      if (i.key() == "url")
	url = i.value().get<std::string>();
      else if (i.key() == "name")
	name = i.value().get<std::string>();
      else if (i.key() == "check_certificate")
	checkCertificates = i.value().get<bool>();
      else if (i.key() == "timeout")
	{
	  timeout = i.value().get<double>();
	}
      else
	throw SermonException("Invalid option \""+i.key()+"\" in configuration file.");
    }

  if (name.empty())
    name = url;

  this->say("New Service "+name+ "Timeout "+ std::to_string(timeout) +" and check cert="+std::to_string(checkCertificates), 4);

  this->services.push_back({url, name, timeout, checkCertificates});
}

void Sermon::siteProbe(Sermon::Service serv)
{
  std::string contents;
  /* try */
  /*   { */
   int status = getUrlData(serv.url, contents, serv.timeout, serv.checkCertificates, this->maxRedirects);
  /* std::cout << "Final status: "<<status<<"\n"; */

  if (status != 200)
    throw RequestException("Site returned invalid status code: "+std::to_string(status));

  /*   } */
  /* catch (GloveException &e) */
  /*   { */
  /* std::cout << "Exception: "<<e.what() << std::endl; */
  /* } */

}

int Sermon::getUrlData(std::string url, std::string &out, double timeout, bool checkCertificates, int max_redirects)
{
  if (max_redirects < 0)
    throw RequestException ("Max redirects reached!");

  Glove g;
  GloveBase::uri u = Glove::get_from_uri(url);
  std::map<std::string, std::string> httpheaders;
  std::string protocol, statusmsg;
  int status;
  out="";
  /* We can debug uri here */
  /* u.host="localhost"; */
  /* u.port=1234; */
  if (checkCertificates)
    g.SSLFlags(Glove::SSL_FLAG_ALL);
  g.shutdown_on_destroy(true);
  g.timeout_when_data(false);
  g.remove_exceptions (Glove::EXCEPTION_DISCONNECTED);
  this->say("Connecting "+url+" with timeout "+std::to_string(timeout), 2);
  if (timeout != -1)
    g.timeout(timeout);
  g.connect(u);
  if (u.rawpath.empty())
    u.rawpath = "/";
  g.send("GET "+u.rawpath+" HTTP/1.1\r\n"
	 "Host: "+u.host+"\r\n"
	 "Accept: text/html,application/xhtml+xml,application/xml\r\n"
	 "Connection: close\r\n"
	 "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/45.0.2454.101 Chrome/45.0.2454.101 Safari/537.36\r\n"
	 "\r\n");
  out = g.receive();
  auto firstline = out.find(GloveBase::CRLF); /* End of first line. Protocol Code Status...*/
  if (firstline == std::string::npos)
    throw RequestException("Didn't receive protocol or status");

  status = parseFirstLine(out.substr(0, firstline), protocol, statusmsg);

  auto headerend = out.find(GloveBase::CRLF2);
  if (headerend == std::string::npos)
    throw RequestException("Unexpected data received");

  extract_headers(out.substr(firstline+1, headerend-firstline-1), httpheaders, 0);
  this->say("Return status: "+std::to_string(status), 3);

  if ( (status>=300) && (status<400) )
    {
      auto location = httpheaders.find("Location");
      if ( (location != httpheaders.end()) || ((location = httpheaders.find("location")) != httpheaders.end()) )
	{
	  std::string newUrl = location->second;
	  newUrl = newUrl.substr(newUrl.find_first_not_of("/")); /* remove starting / */
	  if (newUrl.find(u.service)!=0)
	    newUrl=u.service+"://"+u.host+"/"+newUrl;
	  return getUrlData(newUrl, out, timeout, checkCertificates, max_redirects-1);
	}
      else
	{
	  throw RequestException("Redirection requested but Location not found");
	}
    }
  else
    return status;
}

void Sermon::say(std :: string what, int verbosity)
{
  int color;
  static unsigned short colors[] = { TColor::BLUE, TColor::GREEN, TColor::CYAN, TColor::MAGENTA, TColor::BROWN };

  if (verbosity>this->verbosity)
    return;

  if ( (verbosity<1) || (verbosity>5) )
    this->say("Wrong verbosity level "+std::to_string(verbosity));

  color = colors[verbosity-1];

  std::cout << TColor(color) << "[" << TColor(color+8) << timeformat(std::chrono::system_clock::now()) << TColor(color) << "] "<<what<<endColor<<std::endl;
}
