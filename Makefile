#
# Makefile
#
# $Id: Makefile,v 1.5 2005/01/05 07:46:26 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE -DCOBJMACROS
INCLUDES = -I./util
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o scroll.o ie.o resource.o
EXE_HEADERS = main.h resource.h
EXE_LDLIBS = util/util.a -lole32 -loleaut32 -loleacc -luuid
EXE_LDFLAGS = $(LDFLAGS)

SUBDIRS = util
TARGET = $(EXE_NAME)

WINDRES = windres


all: all-rec $(TARGET)

%-rec:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d $*; \
	done

.SUFFIXES: .rc

$(EXE_NAME): $(EXE_OBJS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(EXE_OBJS) : $(EXE_HEADERS)

.rc.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


clean: clean-rec
	-$(RM) $(EXE_NAME) *.o *~
