### Makefile --- 

## Author: Gaspar Fernández <blakeyed@totaki.com>
## Version: $Id: Makefile,v 0.0 2015/03/21 13:12:00 gaspy Exp $
## Keywords: 
## X-URL: 

DESKTOP_NOTIFICATION=-DDESKTOP_NOTIFICATION
USE_SQLITE=yes

CC=g++
CFLAGS=-O4 -std=c++11
LIBS=-lpthread -lssl -lcrypto -lz -luuid

SRCFILES =  sermon_app.cpp 	\
	glove/glove.cpp						\
	glove/glovehttpcommon.cpp	\
	glove/glovehttpclient.cpp	\
	glove/glovehttpserver.cpp	\
	glove/glovewebsockets.cpp	\
	glove/glovecoding.cpp			\
	notify.cpp								\
	storage.cpp								\
	httpapi.cpp								\
	notifymail.cpp						\
	notifycli.cpp							\
	service_fail.cpp					\
	lib/cfileutils.cpp 				\
	lib/timeutils.cpp					\
	lib/gutils.cpp						\
	lib/mailer.cpp						\
	lib/tcolor.cpp

ifeq ($(DESKTOP_NOTIFICATION),-DDESKTOP_NOTIFICATION)
	SRCFILES := $(SRCFILES) notifydesktop.cpp
	CFLAGS := $(CFLAGS) `pkg-config --cflags libnotify`
	LIBS := $(LIBS) `pkg-config --libs libnotify`
endif

ifeq ($(USE_SQLITE),yes)
	SRCFILES := $(SRCFILES) storageEngineSqlite.cpp
	LIBS := $(LIBS) -lsqlite3
else
	EXTRASETTINGS := $(EXTRASETTINGS) -DNO_SQLITE_STORAGE
endif

SOURCES=sermon.cpp $(SRCFILES)
OBJECTS=$(SOURCES:.cpp=.o)
INCLUDES=

EXECUTABLE=main

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJECTS) $(DESKTOP_NOTIFICATION) $(EXTRASETTINGS) $(LIBS) -o $@

.cpp.o:
	$(CC) -c $(CFLAGS) $(DESKTOP_NOTIFICATION) $(EXTRASETTINGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(OBJECTS) 
	rm -rf $(EXECUTABLE)
### Makefile ends here
