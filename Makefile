### Makefile --- 

## Author: Gaspar Fernández <blakeyed@totaki.com>
## Version: $Id: Makefile,v 0.0 2015/03/21 13:12:00 gaspy Exp $
## Keywords: 
## X-URL: 

DESKTOP_NOTIFICATION=-DDESKTOP_NOTIFICATION

CC=g++
CFLAGS=-O4 -std=c++11
LIBS=-lpthread -lssl -lcrypto

SRCFILES =  sermon_app.cpp 	\
	glove/glove.cpp		\
	notify.cpp		\
	notifymail.cpp		\
	notifycli.cpp		\
	service_fail.cpp	\
	lib/cfileutils.cpp 	\
	lib/timeutils.cpp	\
	lib/mailer.cpp		\
	lib/tcolor.cpp

ifeq ($(DESKTOP_NOTIFICATION),-DDESKTOP_NOTIFICATION)
	SRCFILES := $(SRCFILES) notifydesktop.cpp
	CFLAGS := $(CFLAGS) `pkg-config --cflags libnotify`
	LIBS := $(LIBS) `pkg-config --libs libnotify`
endif
SOURCES=sermon.cpp $(SRCFILES)
OBJECTS=$(SOURCES:.cpp=.o)
INCLUDES=

EXECUTABLE=main

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJECTS) $(DESKTOP_NOTIFICATION) $(LIBS) -o $@

.cpp.o:
	$(CC) -c $(CFLAGS) $(DESKTOP_NOTIFICATION) $(INCLUDES) $< -o $@

clean:
	rm -rf $(OBJECTS) 
	rm -rf $(EXECUTABLE)
### Makefile ends here
