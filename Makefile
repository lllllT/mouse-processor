#
# Makefile
#
# $Id: Makefile,v 1.17 2005/01/17 07:41:50 hos Exp $
#

DEFINES = 
INCLUDES = -I./util
OPT_CFLAGS = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES) $(OPT_CFLAGS)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

UTIL_LIBS = util/util.a

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o \
           scroll.o scroll_op.o \
           scroll_op_scrollbar.o scroll_op_trackbar.o \
           scroll_op_ie.o scroll_op_wheel.o \
           log.o regexp.o conf.o resource.o
EXE_HEADERS = main.h scroll_op.h scroll_op_utils.h resource.h
EXE_LDLIBS = $(UTIL_LIBS) -lpsapi -lole32 -loleaut32 -loleacc -luuid
EXE_LDFLAGS = $(LDFLAGS)

SUBDIRS = util
TARGET = $(EXE_NAME)

RC = rc
WINDRES = windres


.SUFFIXES: .rc .res

all: all-rec $(TARGET)

%-rec:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d $*; \
	done

.SUFFIXES: .rc

$(EXE_NAME): $(EXE_OBJS) $(UTIL_LIBS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(EXE_OBJS) : $(EXE_HEADERS)

.rc.res:
	$(RC) /fo$@ $<

.res.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


clean: clean-rec
	-$(RM) $(EXE_NAME) *.o *.res *~
