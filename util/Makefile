#
# util/Makefile
#
# $Id: Makefile,v 1.8 2005/01/19 08:36:49 hos Exp $
#

DEFINES = 
INCLUDES = 
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

OBJS = assq_pair.o rassq_pair.o \
       automation_idispatch.o \
       automation_create_instance_from_progid.o \
       automation_get_active_object.o \
       read_s_exp.o write_s_exp.o write_s_exp_u8s.o \
       s_exp_assq.o s_exp_massq.o s_exp_nth.o s_exp_nth_cdr.o s_exp_length.o \
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
