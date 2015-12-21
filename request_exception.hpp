/* @(#)request_exception.hpp
 */

#ifndef _REQUEST_EXCEPTION_H
#define _REQUEST_EXCEPTION_H 1

#include <exception>
#include <string>

class RequestException : public std::exception
{
 public:
  RequestException(const std::string &message, int code=0): _msg(message), _code(code)
  {
  }

  RequestException(const char* message, int code=0):_msg(message), _code(code)
  {
  }

  virtual ~RequestException() throw ()
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

#endif /* _REQUEST_EXCEPTION_H */

