/* @(#)sermon_exception.hpp
 */

#ifndef _SERMON_EXCEPTION_H
#define _SERMON_EXCEPTION_H 1

#include <exception>
#include <string>

class SermonException : public std::exception
{
 public:
  SermonException(const std::string &message, int code=0): _msg(message), _code(code)
  {
  }

  SermonException(const char* message, int code=0):_msg(message), _code(code)
  {
  }

  virtual ~SermonException() throw ()
    {
    }

  virtual const char* what() const throw ()
    {
      return _msg.c_str();
    }

  int code() const
  {
    return _code;
  }

 protected:
  std::string _msg;
  int _code;
};

#endif /* _SERMON_EXCEPTION_H */

