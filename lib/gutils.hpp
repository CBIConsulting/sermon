#pragma once

#include <string>
#include <list>
#include <vector>
#include <ctime>
#include <map>
#include <deque>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace GCommon
{
  std::string replace(std::string source, std::string from, std::string to, int offset=0, int times=0);
  std::string replace(std::string source, std::map<std::string,std::string>strMap, int offset=0, int times=0, bool delimiters=true);
  std::string replace(std::string source, const std::map<std::string,std::string>strMap, int offset, int times, bool delimiters, std::string before, std::string after="");

  std::string escape(std::string source, const std::string escapable="\"\\", const std::string escapeChar="\\");
  std::string quote(std::string source, const std::string _quote="\"", const std::string escapeChar="\\");
  std::string quote(const char*source, const std::string _quote="\"", const std::string escapeChar="\\");

	std::string implode(const std::vector<std::string>& container, std::string separator, bool skipEmpty=true);
	
  std::deque< std::string > tokenize( const std::string& source, const std::string &delimiters );
	std::deque<std::string> tokenize(const std::string& str, char delimiter);

  extern std::string whiteSpaces;

  void trimRight( std::string& str,
		  const std::string& trimChars = whiteSpaces );
  void trimLeft( std::string& str,
		 const std::string& trimChars = whiteSpaces );
  void trim( std::string& str, const std::string& trimChars = whiteSpaces );

  void setDefaultDelimiters(std::string start, std::string end);
  std::string defaultStartDelimiter();
  std::string defaultEndDelimiter();
  std::string defaultStartDelimiter(std::string start);
  std::string defaultEndDelimiter(std::string end);

	std::string strpad(std::string, std::string::size_type n, char c, int type=0);
	
  bool isNumeric(const std::string& input);
	bool endsWith(std::string const & value, std::string const & ending);

  template <typename T>
  std::string to_string ( T num, int precision=-1, int width=-1, bool fixed=false, char fill='0')
  {
    std::ostringstream ss;
    if (precision>-1)
      ss << std::setprecision(precision);
    if (width>-1)
      ss << std::setw(width);
    if (fixed)
      ss << std::fixed;
    ss << std::setfill(fill);

    ss << num;
    return ss.str();
  }

  std::string humanSize(long double size, int precision=-1);
};

namespace GCommon
{
  bool findFile(std::string &configFile, const std::list<std::string>& paths, const std::map<std::string, std::string>& replacements={});
  bool findDir(std::string &directory, const std::list<std::string>& paths, const std::map<std::string, std::string>& replacements={});
  std::string put_time(std::string format, time_t tim=0);

  class GlobalSettings
  {
  public:
    static void basics(std::string programName, std::string programShortName, std::string programFile, std::string programVersion);
    static void set(std::string key, std::string value);
    static std::string get(std::string key);
    static const std::map<std::string, std::string> &all();

  private:
    static std::map<std::string, std::string> storage;
  };
};

namespace GCommon
{
  template <typename K, typename V>
  typename std::vector< std::map <K, V> >::iterator findMV(std::vector<std::map<K, V> > &collection, K mykey, V myvalue)
  {
    auto found = std::find_if(collection.begin(), collection.end(), [mykey, myvalue] (std::map<std::string, std::string> current) {
	auto it = current.find(mykey);
	if (it == current.end())
	  return false;
      
	return (it->second == myvalue);
      });

    return found;
  }
}
