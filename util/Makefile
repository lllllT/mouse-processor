#
# util/Makefile
#
# $Id: Makefile,v 1.6 2005/01/12 09:39:48 hos Exp $
#

DEFINES = 
INCLUDES = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

OBJS = assq_pair.o rassq_pair.o \
       automation_idispatch.o automation_get_active_object.o \
       read_s_exp.o write_s_exp.o \
       s_exp_assq.o s_exp_massq.o s_exp_nth.o s_exp_length.o \
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
