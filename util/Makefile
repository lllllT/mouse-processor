#
# util/Makefile
#

DEFINES = 
INCLUDES =
OPT_CFLAGS = 
CFLAGS = -Wall -g -mwindows $(DEFINES) $(INCLUDES) $(OPT_CFLAGS)
LDFLAGS = -Wall -g -mwindows

SRCS = assq_pair.c rassq_pair.c \
       automation_idispatch.c \
       automation_create_instance_from_progid.c \
       automation_get_active_object.c \
       read_s_exp.c write_s_exp.c write_s_exp_u8s.c \
       s_exp_assq.c s_exp_massq.c s_exp_nth.c s_exp_nth_cdr.c s_exp_length.c \
       ucs.c
OBJS = $(SRCS:%.c=%.o)
HEADERS = util.h automation.h s_exp.h
LIB_NAME = util.a

PACK_SRC_FILES = $(SRCS) $(HEADERS) $(RSRC) Makefile

TARGET = $(LIB_NAME)

CC = gcc -b i686-w64-mingw32
INSTALL = install -p

all: $(TARGET)

$(LIB_NAME): $(OBJS)

$(OBJS): $(HEADERS)

%.a:
	$(AR) crs $@ $^

pack-bin:

pack-src:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(PACK_SRC_FILES) $(TARGET_DIR)

clean:
	-$(RM) $(TARGET) $(OBJS) *.o
