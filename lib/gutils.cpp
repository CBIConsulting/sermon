#include "gutils.hpp"
#include "cfileutils.h"
#include <iostream>

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

namespace
{
	const char dirSlash = '/';
};

namespace GCommon
{
  std::string _defaultStartDelimiter = ":";
  std::string _defaultEndDelimiter = "";
  std::string whiteSpaces( " \f\n\r\t\v" );

  std::string replace(std::string source, std::string from, std::string to, int offset, int times)
  {
    int total = 0;
    std::string::size_type pos=offset;

    do
      {
	pos = source.find(from, pos);

	if (pos == std::string::npos)
	  break;

	source.replace(pos, from.length(), to);
	pos+=to.size();

      } while ( (times==0) || (++total<times) );

    return source;
  }

  std::string replace(std::string source, std::map<std::string,std::string>strMap, int offset, int times, bool delimiters, std::string before, std::string after)
  {
    int total = 0;
    std::string::size_type pos=offset;
    std::string::size_type newPos;
    std::string::size_type lowerPos;
    std::string::size_type delsize;

    if (strMap.size() == 0)
      return source;

    if (delimiters)
      delsize = before.length() + after.length();

    do
      {
	std::string rep;
	for (auto i=strMap.begin(); i!=strMap.end(); ++i)
	  {
	    auto fromStr = i->first;
	    newPos = (delimiters)?
	      source.find(before + fromStr + after, pos):
	      source.find(fromStr, pos);

	    if ( (i==strMap.begin()) || (newPos<lowerPos) )
	      {
		rep = fromStr;
		lowerPos = newPos;
	      }
	  }

	pos = lowerPos;
	if (pos == std::string::npos)
	  break;

	std::string toStr = strMap[rep];
	source.replace(pos, rep.length()+((delimiters)?delsize:0), toStr);
	pos+=toStr.size();

      } while ( (times==0) || (++total<times) );

    return source;
  }

  std::string replace(std::string source, std::map<std::string,std::string>strMap, int offset, int times, bool delimiters)
  {
    return (delimiters)?replace(source, strMap, offset, times, delimiters, _defaultStartDelimiter, _defaultEndDelimiter):
      replace(source, strMap, offset, times, delimiters, "");
  }

  std::string escape(std::string source, const std::string escapable, std::string escapeChar)
  {
    std::map<std::string, std::string> substmap;
    for (auto _ch : escapable)
      {
	std::string ch(5, '\0');
	ch[0] = _ch;
	substmap.insert({ ch, escapeChar+ch });
      }

    return replace(source, substmap, false);
  }

  std::deque< std::string > tokenize( const std::string& source, const std::string &delimiters )
  {
    std::string map( 256, '\0' );
    for( auto &ch : delimiters )
      map[ ch ] = '\1';
    std::deque< std::string > tokens(1);
    for( const unsigned char &ch : source ) {
      /**/ if( !map.at(ch)          ) tokens.back().push_back( ch );
      else if( tokens.back().size() ) tokens.push_back( std::string() );
    }
    while( tokens.size() && !tokens.back().size() ) tokens.pop_back();
    return tokens;
  }

	std::deque<std::string> tokenize(const std::string& str, char delimiter)
	{
		std::deque<std::string> ret; 
		std::string::const_iterator i = str.begin();

		while (i != str.end())
			{
				i = find_if(i, str.end(), [delimiter](char c)   {
						return (c!=delimiter);
					}); 
				std::string::const_iterator j = find_if(i, str.end(), [delimiter](char c)   {
						return (c==delimiter);
					}); 
				if (i != str.end())         
					ret.push_back(std::string(i, j)); i = j;
			}
		return ret;
	}

	std::string implode(const std::vector<std::string>& container, std::string separator, bool skipEmpty)
	{
		std::string out;
		bool first = true;
		
		for (auto str : container)
			{
				if (str.empty())
					continue;

				if (!first)
					out+=separator;
				else
					first=false;
				
				out+=str;
			}
		return out;
	}

  std::string quote(std::string source, const std::string _quote, const std::string escapeChar)
  {
    if (escapeChar.empty())
      source=_quote+source+_quote;
    else
      source=_quote+escape(source, _quote+escapeChar, escapeChar)+_quote;

    return source;
  }

  std::string quote(const char* source, const std::string _quote, const std::string escapeChar)
  {
    std::string _source(source);
    return quote(_source, _quote, escapeChar);
  }

  void setDefaultDelimiters(std::string start, std::string end)
  {
    _defaultStartDelimiter = start;
    _defaultEndDelimiter = end;
  }

  std::string DefaultStartDelimiter()
  {
    return _defaultStartDelimiter;
  }

  std::string defaultEndDelimiter()
  {
    return _defaultEndDelimiter;
  }

  std::string DefaultStartDelimiter(std::string start)
  {
    _defaultStartDelimiter = start;
    return _defaultStartDelimiter;
  }

  std::string defaultEndDelimiter(std::string end)
  {
    _defaultEndDelimiter = end;
    return _defaultEndDelimiter;
  }

  void trimRight( std::string& str,
		  const std::string& trimChars)
  {
    std::string::size_type pos = str.find_last_not_of( trimChars );
    str.erase( pos + 1 );
  }

  void trimLeft( std::string& str,
		 const std::string& trimChars)
  {
    std::string::size_type pos = str.find_first_not_of( trimChars );
    str.erase( 0, pos );
  }

  void trim( std::string& str, const std::string& trimChars)
  {
    trimRight( str, trimChars );
    trimLeft( str, trimChars );
  }

};

namespace
{
  bool _findFile(std::string &file, const std::list<std::string>& paths, const std::map<std::string, std::string>& replacements)
  {
    for (auto p : paths)
      {
	p = GCommon::replace(p, replacements);
	if (file_exists(p.c_str())==1)
	  {
	    file = p;
	    return true;
	  }
      }
  }
};

namespace GCommon
{
  std::map<std::string, std::string> GlobalSettings::storage;

  bool findDir(std::string &directory, const std::list<std::string>& paths, const std::map<std::string, std::string>& replacements)
  {
      for (auto p : paths)
	{
	  p = replace(p, replacements);
	  if (directory_exists(p.c_str())==1)
	    {
	      directory = p;
	      return true;
	    }
	}
  }

  bool findFile(std::string &file, const std::list<std::string>& paths, const std::map<std::string, std::string>& replacements)
  {
    return _findFile(file, paths, replacements);
    /* std::vector<std::string> paths = {"/etc/sermon/config.json", "/etc/sermon.json", "./sermon.json", "./config.json", "./config/sermon.json"}; */
    return false;
  }

  std::string put_time(std::string format, time_t tim)
  {
    struct tm tm;
    if (tim==0)
      tim = time (NULL);

    localtime_r(&tim, &tm);
		std::stringstream ss;
		ss<<std::put_time(&tm, format.c_str());
    return ss.str();
  }

	std::string pathSlash(std::string& path)
	{
		trim(path);
		
		if (path.empty())
			return path;

		if (path.back()!=dirSlash)
			path+=dirSlash;

		return path;
	}

  void GlobalSettings::basics(std::string programName, std::string programShortName, std::string programFile, std::string programVersion)
  {
    storage.insert({"name", programName});
    storage.insert({"sname", programShortName});
    storage.insert({"exec", programFile});
    storage.insert({"version", programVersion});
  }

  void GlobalSettings::set(std::string key, std::string value)
  {
    storage.insert({key, value});
  }

  std::string GlobalSettings::get(std::string key)
  {
    return storage[key];
  }

  const std::map<std::string, std::string> &GlobalSettings::all()
  {
    return storage;
  }

  std::string humanSize(long double size, int precision)
  {
    static char units[10][6]={"bytes","Kb","Mb","Gb","Tb","Pb","Eb","Zb","Yb","Bb"};  
    int i= 0;

    while (size>1024) {
      size = size /1024;
      i++;
    }

    return GCommon::to_string(size, precision, -1, true)+units[i];
  }

  bool isNumeric(const std::string& input)
  {
    return std::all_of(input.begin(), input.end(), ::isdigit);
  }

	bool endsWith(std::string const & value, std::string const & ending)
	{
		if (ending.size() > value.size())
			return false;
		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
	}

	std::string strpad(std::string source, std::string::size_type n, char c, int type)
	{
		if (source.length()>=n)
			return source;						/* nothing to do. */
		auto count = n-source.length();
	
		/* pad in the beginning */
		if (type==0)
			source.insert(source.begin(), count, c);
		else if (type==1)
			source.append(count, c);

		return source;
	}

	std::string uppercase(std::string s)
	{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		return s;
	}

};
