#
# Makefile
#
# $Id: Makefile,v 1.4 2005/01/04 09:36:10 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE -DCOBJMACROS
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o scroll.o ie.o assq_pair.o automation.o resource.o
EXE_HEADERS = main.h util.h automation.h resource.h
EXE_LDLIBS = -lole32 -loleaut32 -loleacc -luuid
EXE_LDFLAGS = $(LDFLAGS)

TARGET = $(EXE_NAME)

WINDRES = windres


all: $(TARGET)

.SUFFIXES: .rc

$(EXE_NAME): $(EXE_OBJS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(EXE_OBJS) : $(EXE_HEADERS)

.rc.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


clean:
	-$(RM) $(EXE_NAME) *.a *.o *~
