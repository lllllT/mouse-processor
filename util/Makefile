#
# util/Makefile
#
# $Id: Makefile,v 1.5 2005/01/12 02:25:13 hos Exp $
#

DEFINES = 
INCLUDES = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

OBJS = assq_pair.o rassq_pair.o \
       automation_idispatch.o automation_get_active_object.o \
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
