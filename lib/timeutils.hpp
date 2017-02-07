/* @(#)timeutils.hpp
 */

#ifndef _TIMEUTILS_H
#define _TIMEUTILS_H 1

#include <chrono>
#include <tuple>
#include <array>
#include <string>
#include <iomanip>

#if defined(__GNUC__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ <= 50200 )
  namespace std
  {
    static std::string put_time( const std::tm* tmb, const char* fmt )
    {
      std::string s( 128, '\0' );
      size_t written;
      while( !(written=strftime( &s[0], s.size(), fmt, tmb ) ) )
        s.resize( s.size() + 128 );
      s[written] = '\0';

      return s.c_str();
    }
  }
#endif

using itemized_time = std::array<std::chrono::seconds, 4>;
using itemized_time_parts = std::array<std::tuple<const char*, const char*>, 4>;

itemized_time get_itemized(std::chrono::seconds seconds);
std::string itemized_to_str(const itemized_time &it, itemized_time_parts parts={ 
    std::make_tuple("days", "day"), std::make_tuple("hours", "hour"), std::make_tuple("minutes", "minute"), std::make_tuple("seconds", "second") },
  std::string separator=", ",
  std::string lastSeparator=" and "
  );
std::string itemized_to_str_num(const itemized_time &it, bool showZeroDays=true);
std::string timeformat(const std::chrono::system_clock::time_point& moment, const std::string& format="%d/%m/%Y %H:%M:%S");

/* Must improve these two */
time_t parseTime(std::string strTime);
std::string put_time(std::string format, time_t tim);

#endif /* _TIMEUTILS_H */

