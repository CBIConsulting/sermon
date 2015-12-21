/**
*************************************************************
* @file notifymail.cpp
* @brief Breve descripción
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 15 dic 2015
* Historial de cambios:
*
*
*
*
*
*
*
*************************************************************/

#include "notifymail.hpp"
#include <iostream>
#include "lib/timeutils.hpp"
#include "lib/mailer.hpp"

NotifyMail::NotifyMail(std :: string id):Notify(id, "mail")
{
}

NotifyMail::~NotifyMail()
{
}


void NotifyMail::newFail(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, int code, const std :: string &message)
{
  auto mailBody="System Administrator,\n\n"
    "Sermon has detected an error in the following service:\n\n"
    +serviceName+"\n\n"
    "At: "+timeformat(outageStart)+"\n" +
    "Error message: "+message+" ("+std::to_string(code)+")\n\n"
    "Please, fix this as soon as possible\n";

  /* std::cout << "ITEMIZED TEST: "<<itemized_to_str(get_itemized(std::chrono::seconds(86401))); */
  /* std::cout << "ACABA DE FALLAR EL SERVICIO "<<serviceName<<"\n\nA las: "<<timeformat(outageStart)<<"\nCODIGO: "<<code<< " ERR: "<<message<<"\n\n"; */
  std::string from = (option.find("from")!=option.end())?option["from"]:"sermonmailer@root";
  std::string subject = (option.find("errorSubject")!=option.end())?option["errorSubject"]:"Error in server";

  auto debug = option.find("debug");
  if (debug == option.end())
    {
      if (!Mailer::sendmail(from, option["to"], subject, mailBody))
	std::cerr << "Could not send e-mail to administrator!!!"<<std::endl;
    }
  else if (debug->second!="hidden")
    {
      std::cerr << " ------------------------- SERVICE FAIL MAIL ------------------------"<<std::endl;
      std::cerr << mailBody;
      std::cerr << " ------------------------- SERVICE FAIL MAIL ------------------------"<<std::endl;
    }
}

void NotifyMail::newRecovery(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, std :: chrono :: system_clock :: time_point outageEnd, int code, const std :: string &message)
{
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(outageEnd - outageStart).count();
  std::string from = (option.find("from")!=option.end())?option["from"]:"sermonmailer@root";
  std::string subject = (option.find("recoverySubject")!=option.end())?option["recoverySubject"]:"Good news! Server recovery";

  auto mailBody="System Administrator,\n\n"
    "Sermon has detected recovery on the service "+serviceName+" after "+itemized_to_str(get_itemized(std::chrono::seconds(elapsed)))+".\n\n"
    "Original error: "+message+" ("+std::to_string(code)+")\n" +
    "Started at: "+timeformat(outageStart)+"\n" +
    "Ended at: "+timeformat(outageEnd)+"\n\n"
    "EVERYTHING IS FINE NOW";

  auto debug = option.find("debug");
  if (debug == option.end())
    {
      if (!Mailer::sendmail(from, option["to"], subject, mailBody))
	std::cerr << "Could not send e-mail to administrator!!!"<<std::endl;
    }
  else if (debug->second!="hidden")
    {
      std::cerr << " ------------------------- SERVICE RECOVERY MAIL ------------------------"<<std::endl;
      std::cerr << mailBody;
      std::cerr << " ------------------------- SERVICE RECOVERY MAIL ------------------------"<<std::endl;
    }
}
