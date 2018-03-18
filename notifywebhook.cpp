/**
*************************************************************
* @file notifywebhook.cpp
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

#include "notifywebhook.hpp"
#include <iostream>
#include "lib/timeutils.hpp"
#include "lib/tcolor.hpp"
#include "lib/gutils.hpp"
#include "glove/glovehttpclient.hpp"
#include "sermon_exception.hpp"

NotifyWebhook::NotifyWebhook(std :: string id):Notify(id, "webhook")
{
}

NotifyWebhook::~NotifyWebhook()
{
}


void NotifyWebhook::_newFail(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, int code, const std :: string &message)
{
	callWebHook("["+timeformat(outageStart)+"] Service "+serviceName+ " failed with error: "+message+ "("+std::to_string(code)+")");
}

void NotifyWebhook::_newBounce(const std::string& serviceName, uint64_t bounces, std::chrono::system_clock::time_point outageStart, std::chrono::system_clock::time_point outageElapsed, int code, const std::string& message)
{
}

void NotifyWebhook::_newRecovery(const std :: string & serviceName, std :: chrono :: system_clock :: time_point outageStart, std :: chrono :: system_clock :: time_point outageEnd, int code, const std :: string &message)
{
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(outageEnd - outageStart).count();

	callWebHook("["+timeformat(outageEnd)+"] Service "+serviceName+ " is back to life after "+itemized_to_str(get_itemized(std::chrono::seconds(elapsed))));
}

void NotifyWebhook::_newMessage(const std::string& type, const std::string& serviceName, std::chrono::system_clock::time_point outageStart, const std::string& message)
{
	std::string strService = (serviceName.empty())?"":"{Service "+serviceName+"}";

	callWebHook("["+timeformat(outageStart)+"] ("+type+") "+strService+" Message: "+message);
}

void NotifyWebhook::callWebHook(const std::string& message)
{
	GloveHttpClient client;
	std::string data;
	try
		{
			client.timeout(10);
			client.checkCertificates(true);
			client.maxRedirects(2);

			if (method == "POST")
				{
					data = GCommon::replace(dataTemplate, {
							{ "message", message },
							{ "escapedMessage", GCommon::escape(message, "\"", "\\") }
						}, 0, 0, true, "%", "%");
				}

			auto res = client.request (endpoint,
																 method,
																 data);

		}
	catch (GloveException& e)
		{
			std::cout << e.what()<<std::endl;
		}
}

void NotifyWebhook::_parseSpecialOptions()
{
	if (this->option["endpoint"].empty())
		throw SermonException("[NotifyWebhook] Missing endpoint");
	else
		endpoint = this->option["endpoint"];
	method = GCommon::uppercase((this->option["method"].empty())?"post":this->option["method"]);

	if (method == "POST")
		{
			if (this->option["template"].empty())
				throw SermonException("[NotifyWebhook] Missing template for post requests");
			else
				dataTemplate = this->option["template"];
		}
}
