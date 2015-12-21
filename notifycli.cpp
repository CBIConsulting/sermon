/**
*************************************************************
* @file notifycli.cpp
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

#include "notifycli.hpp"
#include <iostream>
#include "lib/timeutils.hpp"
#include "lib/tcolor.hpp"

NotifyCli::NotifyCli(std :: string id):Notify(id, "cli")
{
}

NotifyCli::~NotifyCli()
{
}


void NotifyCli::newFail(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, int code, const std :: string &message)
{
  std::cout << TColor(TColor::RED) <<"["<<TColor(TColor::LIGHTRED) <<timeformat(outageStart)<<TColor(TColor::RED) <<"] Service "<<serviceName<< " failed with error: "<<message<< "("<<code<<")"<<endColor<<std::endl;
}

void NotifyCli::newRecovery(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, std :: chrono :: system_clock :: time_point outageEnd, int code, const std :: string &message)
{
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(outageEnd - outageStart).count();
  std::cout << TColor(TColor::GREEN) << "["<< TColor(TColor::LIGHTGREEN) << timeformat(outageEnd) << TColor(TColor::GREEN) << "] Service "<<serviceName<<" is back to life after "
	    << TColor(TColor::GREEN + TColor::UNDERLINE) << itemized_to_str(get_itemized(std::chrono::seconds(elapsed))) << TColor(TColor::GREEN) << endColor << std::endl;
}
