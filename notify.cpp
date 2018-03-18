/**
*************************************************************
* @file notify.cpp
* @brief Breve descripción
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 14 dic 2015
* Historial de cambios:
*
*
*
*
*
*
*
*************************************************************/

#include "notify.hpp"
#include <iostream>
#include "lib/timeutils.hpp"
#include "lib/gutils.hpp"

Notify::Notify(std :: string id, std :: string type):type(type)
{
  /* Mirar si el ID es auto y generar uno si es así */
  this->id = id;
}

Notify::~Notify()
{
}

void Notify::setOptions(std :: map < std :: string, std :: string > options)
{
  for (auto i : options)
    {
      this->option[i.first] = i.second;
    }
	parseSpecialOptions();
}

void Notify::parseSpecialOptions()
{
	defaultEnabledType=1;
	enabled = true;
	auto types = this->option.find("notifications");
	if (types != this->option.end())
		{
			auto typelist = GCommon::tokenize(types->second, ',');
			if (typelist.size()>0)
				{
					for (auto t : typelist)
						{
							GCommon::trim(t);
							if (t=="all")
								defaultEnabledType=1;
							else if (t=="none")
								defaultEnabledType=0;
							else
								enabledTypes[t] = 1;
						}
				}
		}
	auto _enabled = this->option.find("enabled");
	if (_enabled != this->option.end())
		{
			auto __enabled = GCommon::uppercase(_enabled->second);

			if ( (__enabled == "0") || (__enabled=="NO") || (__enabled=="FALSE") )
				enabled = false;
		}
	_parseSpecialOptions();
}

uint32_t Notify::enabledNotificationType(const std::string& type)
{
	auto typeEnabled = enabledTypes.find(type);
	return (typeEnabled!=enabledTypes.end())?typeEnabled->second:defaultEnabledType;
}

Notify::Notify(Notify && n) noexcept : id(std::move(n.id)), type(std::move(n.type)), option(std::move(n.option))
{
}

Notify::Notify(const Notify & n)
{
  this->id = n.id;
  this->type = n.type;
  this->option = n.option;
}

void Notify::newFail(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, int code, const std::string& message)
{
	if (!enabled)
		return;

	if (enabledNotificationType("outage"))
		_newFail(serviceName, outageStart, code, message);
}

void Notify::newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message)
{
	if (!enabled)
		return;

	/* Custom notification for bounces ? */
	if (!enabledNotificationType("ignorebounce"))
		_newBounce(serviceName, bounces, outageStart, outageElapsed, code, message);
}

void Notify::newRecovery(const std::string& serviceName, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageEnd, int code, const std::string& message)
{
	if (!enabled)
		return;

	if (enabledNotificationType("recovery"))
		_newRecovery(serviceName, outageStart, outageEnd, code, message);
}

void Notify::newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message)
{
	if (!enabled)
		return;

	if (enabledNotificationType(type))
		_newMessage(type, serviceName, outageStart, message);
}

/* May be overriden in notifiers */
void Notify::_parseSpecialOptions()
{
}
