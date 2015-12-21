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

#include "notifydesktop.hpp"
#include <iostream>
#include "lib/timeutils.hpp"
#include <libnotify/notify.h>

NotifyDesktop::NotifyDesktop(std :: string id):Notify(id, "desktop")
{
}

NotifyDesktop::~NotifyDesktop()
{
}


void NotifyDesktop::newFail(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, int code, const std :: string &message)
{
  notify_init("Service error");
  std::string title = "Service "+serviceName+" failed!";
  std::string body = "Service "+serviceName+" got "+message+ "("+std::to_string(code)+") at "+ timeformat(outageStart);
  NotifyNotification *failNotification = notify_notification_new(title.c_str(), body.c_str(), "dialog-error");
  notify_notification_show(failNotification, NULL);
  g_object_unref(G_OBJECT(failNotification));
  notify_uninit();
}

void NotifyDesktop::newRecovery(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, std :: chrono :: system_clock :: time_point outageEnd, int code, const std :: string &message)
{
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(outageEnd - outageStart).count();
  notify_init("Service recovered");
  std::string title = "Service "+serviceName+" recovered!";
  std::string body = "Service "+serviceName+" is back to life from error "+message+ "("+std::to_string(code)+")\nHas been down for "+itemized_to_str(get_itemized(std::chrono::seconds(elapsed)));
  NotifyNotification *failNotification = notify_notification_new(title.c_str(), body.c_str(), "dialog-information");
  notify_notification_show(failNotification, NULL);
  g_object_unref(G_OBJECT(failNotification));
  notify_uninit();
}
