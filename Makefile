#
# Makefile
#
# $Id: Makefile,v 1.3 2004/12/31 18:55:50 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o scroll.o assq_pair.o resource.o
EXE_HEADERS = main.h util.h resource.h
EXE_LDLIBS =
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
