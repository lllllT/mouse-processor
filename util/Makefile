#
# util/Makefile
#
# $Id: Makefile,v 1.3 2005/01/08 21:47:52 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE
INCLUDES = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

OBJS = assq_pair.o rassq_pair.o automation.o \
       read_s_exp.o write_s_exp.o s_exp_assq.o s_exp_massq.o s_exp_length.o \
       ucs.o
HEADERS = util.h automation.h s_exp.h
LIB_NAME = util.a

TARGET = $(LIB_NAME)

all: $(TARGET)

$(LIB_NAME): $(OBJS)

$(OBJS): $(HEADERS)

%.a:
	$(AR) crs $@ $^

clean:
	-$(RM) $(TARGET) $(OBJS) *.o
