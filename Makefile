#
# Makefile
#
# $Id: Makefile,v 1.1 2004/12/27 05:40:13 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0403 -DUNICODE -D_UNICODE
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o assq_pair.o resource.o
EXE_LDLIBS =
EXE_LDFLAGS = $(LDFLAGS)

TARGET = $(EXE_NAME)

WINDRES = windres


all: $(TARGET)

.SUFFIXES: .rc

$(EXE_NAME): $(EXE_OBJS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

.rc.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


clean:
	-$(RM) $(EXE_NAME) *.a *.o *~
