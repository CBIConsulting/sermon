#include "mailer.hpp"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/random.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <iostream>

#define RANDOM_DEVICE "/dev/urandom"

namespace 
{
  int generate_random(unsigned char *buffer, int len)
  {
    int rfd = open(RANDOM_DEVICE, O_RDONLY);
    int generated = 0;
    int entropy;
    int total;
    if (ioctl(rfd, RNDGETENTCNT, &entropy) == -1)
      return 0;

    while ( (generated<len) && ((total=read(rfd, (buffer+generated), len-generated)) > 0) )
      {
	generated+=total;
      }

    if (total == -1)
      return 0;

    return 1;
  }

}
namespace Mailer
{
  int sendmail(const char *from, const char* to, const char* subject, char* body, const char* headers)
  {
    char messageId[200];
    char fromcopy[200];
    unsigned char randoms[10];
    time_t t;
    struct tm *tmp;
    char* stmp;

    strcpy(fromcopy, from);
    char* at = (char*)strchr(to, '@');
    if (at == NULL)				/* no @ in to */
      return 0;

    at = strchr(fromcopy, '@');
    if (at == NULL)				/* no @ in from */
      return 0;

    stmp = strchr(at, '>');
    if (stmp!=NULL)
      *stmp='\0';

    if (!generate_random(randoms, 10)) /* Generate random numbers for Message-ID */
      return 0;

    t = time(NULL);
    tmp = localtime(&t);

    sprintf(messageId, "%02x%02x%02x%02x%02x%02x.%d%d%d%d%d%d.%02x%02x%02x%02x%s", 
	    randoms[0], randoms[1],randoms[2],randoms[3],randoms[4],randoms[5],
	    1900+tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
	    randoms[6],randoms[7],randoms[8],randoms[9], at);

    FILE* sendmail = popen ("sendmail", "w");
    if (!sendmail)
      return 0;

    fprintf (sendmail,"To: %s\nFrom: %s\nSubject: %s\nMessage-Id: <%s>\n%s", to, from, subject, messageId, headers);

    if ( (strlen(headers)>0) && (headers[strlen(headers)-1]!='\n') )
      fprintf (sendmail,"\n");

    fprintf (sendmail,"\n%s\n", body);

    int res = pclose(sendmail);
    if (res == -1)
      return 0;

    return 1;
  }

  int sendmail(const std::string &from, const std::string& to, const std::string& subject, const std::string& body)
  {
    return (sendmail(from.c_str(), to.c_str(), subject.c_str(), (char*) body.c_str(), (char*)"User-agent: Sermon 0.5Beta\nX-Priority: 1 (Highest)\nImportance: High\n"));
  }

}
