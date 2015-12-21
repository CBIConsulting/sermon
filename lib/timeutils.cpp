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
#include <string>

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

std::string timeformat(const std::chrono::system_clock::time_point& moment, const std::string &format)
{
  std::tm tm;
  const time_t tim = std::chrono::system_clock::to_time_t(moment);
  localtime_r(&tim, &tm);

  return std::put_time(&tm, format.c_str());
}
