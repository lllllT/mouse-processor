#
# Makefile
#
# $Id: Makefile,v 1.12 2005/01/13 09:39:54 hos Exp $
#

DEFINES = 
INCLUDES = -I./util
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

UTIL_LIBS = util/util.a

EXE_NAME = mp.exe
EXE_OBJS = main.o hook.o \
           scroll.o scroll_op.o \
           scroll_op_scrollbar.o scroll_op_trackbar.o \
           scroll_op_ie.o scroll_op_tab.o scroll_op_wheel.o \
           regexp.o \
           ie.o conf.o resource.o
EXE_HEADERS = main.h scroll_op.h scroll_op_utils.h resource.h
EXE_LDLIBS = $(UTIL_LIBS) -lpsapi -lole32 -loleaut32 -loleacc -luuid
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

$(EXE_NAME): $(EXE_OBJS) $(UTIL_LIBS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(EXE_OBJS) : $(EXE_HEADERS)

.rc.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


clean: clean-rec
	-$(RM) $(EXE_NAME) *.o *~
