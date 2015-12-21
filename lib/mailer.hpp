#pragma once

#include <string>

namespace Mailer
{
  int sendmail(const char *from, const char* to, const char* subject, char* body, const char* headers);
  int sendmail(const std::string &from, const std::string& to, const std::string& subject, const std::string& body);
};
