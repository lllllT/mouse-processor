#
# util/Makefile
#
# $Id: Makefile,v 1.1 2005/01/05 07:46:30 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE -DCOBJMACROS
INCLUDES = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

OBJS = assq_pair.o automation.o
HEADERS = util.h automation.h
LIB_NAME = util.a

TARGET = $(LIB_NAME)

all: $(TARGET)

$(LIB_NAME): $(OBJS)

$(OBJS): $(HEADERS)

%.a:
	$(AR) crs $@ $^

clean:
	-$(RM) $(TARGET) $(OBJS) *.o
