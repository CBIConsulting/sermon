/**
*************************************************************
* @file timeutils.cpp
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

#include "timeutils.hpp"
#include "gutils.hpp"
#include <string>
#include <sstream>
#include <iostream>

itemized_time get_itemized(std::chrono::seconds seconds)
{
  static const itemized_time ref = {
    std::chrono::hours(24),
    std::chrono::hours(1),
    std::chrono::minutes(1),
    std::chrono::seconds(1)
  };
  itemized_time res;
  auto c=0;
  for (auto &i : ref)
    {
      res[c++] = seconds / i.count();
      seconds=seconds%i;
    }
  return res;
}

std::string itemized_to_str(const itemized_time &it, itemized_time_parts parts, std::string separator, std::string lastSeparator)
{
  std::string out;
  auto last = it.begin();

  for (auto c = it.begin()+1; c!= it.end(); ++c)
    {
      if (c->count() != 0)
				last = c;
    }
  auto n=0;
  for (auto c = it.begin(); c!= it.end(); ++c, ++n)
    {
      if (c->count() == 0)
				continue;
      if (!out.empty())
				/* add separator */
				out+=(c == last)?lastSeparator:separator;
      out+=std::to_string(c->count())+" ";
      out+=(c->count()==1)?std::get<1>(parts[n]):std::get<0>(parts[n]);
    }
  return out;
}

std::string itemized_to_str_num(const itemized_time &it, bool showZeroDays)
{
	std::string out;
	auto days = std::get<0>(it);
	auto hours = std::get<1>(it);
	auto minutes = std::get<2>(it);
	auto seconds = std::get<3>(it);
	if ( (showZeroDays) || (days.count()>0) )
		out+=GCommon::strpad(std::to_string(days.count()), 2, '0')+":";

	out+=GCommon::strpad(std::to_string(hours.count()), 2, '0')+":"+
		GCommon::strpad(std::to_string(minutes.count()), 2, '0')+":"+
		GCommon::strpad(std::to_string(seconds.count()), 2, '0');

	return out;
}

std::string timeformat(const std::chrono::system_clock::time_point& moment, const std::string &format)
{
	std::tm tm;
	const time_t tim = std::chrono::system_clock::to_time_t(moment);
	localtime_r(&tim, &tm);
	std::stringstream ss;
	ss << std::put_time(&tm, format.c_str());
	return ss.str();
}

/* std::string timeformat(const std::chrono::system_clock::time_point& moment, const std::string &format) */
/* { */
/*   std::tm tm; */
/*   const time_t tim = std::chrono::system_clock::to_time_t(moment); */
/*   localtime_r(&tim, &tm); */

/*   return std::put_time(&tm, format.c_str()); */
/* } */

time_t parseTime(std::string strTime)
{
  /* We must improve it a little bit */
  std::string format;
  struct tm tm;
  time_t now = time(NULL);
  localtime_r(&now, &tm);

  if (strTime == "now")
    return now;
  else if (strTime == "yesterday")
    return now-86400;
	else if (strTime == "daystart")
		{
			tm.tm_hour=0;
			tm.tm_min=0;
			tm.tm_sec=0;
			return mktime(&tm);
		}
	else if (strTime == "thismonth")
		{
			tm.tm_mday=1;
			tm.tm_hour=0;
			tm.tm_min=0;
			tm.tm_sec=0;
			return mktime(&tm);
		}

  if (strTime.find('/') != std::string::npos)
    format = "%d/%m/%Y %H:%M:%S";
	else if (strTime.find('-') != std::string::npos)
		format = "%Y-%m-%d %H:%M:%S";
	else
		format = "%H:%M:%S";

  char *conv = strptime(strTime.c_str(), format.c_str(), &tm);

  time_t newtime = mktime(&tm);
  if ( (conv == NULL) && (newtime == now) )
    return 0;

  return newtime;
}

std::string put_time(std::string format, time_t tim)
{
	struct tm tm;
	if (tim==0)
		tim = time (NULL);

	localtime_r(&tim, &tm);
	std::stringstream ss;
	ss << std::put_time(&tm, format.c_str());
	return ss.str();
}
