#
# Makefile
#
# $Id: Makefile,v 1.26 2005/02/01 11:28:21 hos Exp $
#

DEFINES = 
INCLUDES = -I./util
OPT_CFLAGS = -O3 -fomit-frame-pointer
CFLAGS = -Wall -g -mwindows -mno-cygwin $(DEFINES) $(INCLUDES) $(OPT_CFLAGS)
LDFLAGS = -Wall -g -mwindows -mno-cygwin

UTIL_LIBS = util/util.a

TARGET_NAME = mp

EXE_NAME = $(TARGET_NAME).exe
EXE_SRCS = main.c hook.c \
           scroll.c scroll_op.c \
           scroll_op_scrollbar.c scroll_op_trackbar.c \
           scroll_op_ie.c scroll_op_wheel.c \
           log.c regexp.c conf.c window.c shmem.c
EXE_RSRC = resource.rc
EXE_OBJS = $(EXE_SRCS:%.c=%.o) $(EXE_RSRC:%.rc=%.o)
EXE_HEADERS = main.h operator.h resource.h \
              scroll_op_utils.h scroll_op_scrollbar.h shmem.h
EXE_LDLIBS = $(UTIL_LIBS) -lpsapi -lole32 -loleaut32 -loleacc -luuid
EXE_LDFLAGS = $(LDFLAGS)

DLL_NAME = $(TARGET_NAME)sup.dll
DLL_SRCS = dllmain.c dllinj.c shmem.c
DLL_RSRC = 
DLL_OBJS = $(DLL_SRCS:%.c=%.o) $(DLL_RSRC:%.rc=%.o)
DLL_HEADERS = dllinj.h scroll_op_scrollbar.h shmem.h
DLL_LDLIBS = -lpsapi -lkernel32
DLL_LDFLAGS = $(LDFLAGS) -mdll -nostartfiles -nostdlib -e _DllMain@12

PACK_BIN_FILES = $(EXE_NAME)
PACK_BIN_ADD_FILES = README.txt VERSION default.mprc
PACK_SRC_FILES = $(EXE_SRCS) $(EXE_RSRC) $(EXE_HEADERS) icon.ico Makefile

SUBDIRS = util doc
TARGET = $(DLL_NAME) $(EXE_NAME)
VERSION = `cat VERSION`

RC = rc
WINDRES = windres
ZIP = zip
GTAR = tar
INSTALL = install


.SUFFIXES: .rc .res

all: all-rec $(TARGET)

%-rec:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d $* OPT_CFLAGS="$(OPT_CFLAGS)"; \
	done

.SUFFIXES: .rc

$(EXE_NAME): $(EXE_OBJS) $(UTIL_LIBS)
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(DLL_NAME): $(DLL_OBJS)
	$(CC) $(DLL_LDFLAGS) $(DLL_OBJS) $(DLL_LDLIBS) -o $@

$(EXE_OBJS) : $(EXE_HEADERS)

$(DLL_OBJS) : $(DLL_HEADERS)

.rc.res:
	$(RC) /fo$@ $<

.res.o:
	$(WINDRES) -o $@ $<

resource.o: icon.ico


pack: pack-bin pack-src

pack-bin: all
	-$(RM) -r $(TARGET_NAME)-$(VERSION)
	$(INSTALL) -m 755 -d $(TARGET_NAME)-$(VERSION)
	VERSION=$(VERSION) ; \
	TARGET_DIR=`pwd`/$(TARGET_NAME)-$$VERSION ; \
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d pack-bin TARGET_DIR="$$TARGET_DIR/$$d" ; \
	done
	$(INSTALL) -m 755 -s $(PACK_BIN_FILES) $(TARGET_NAME)-$(VERSION)
	$(INSTALL) -m 644 $(PACK_BIN_ADD_FILES) $(TARGET_NAME)-$(VERSION)
	$(ZIP) -r $(TARGET_NAME)-$(VERSION).zip $(TARGET_NAME)-$(VERSION)
	-$(RM) -r $(TARGET_NAME)-$(VERSION)

pack-src:
	-$(RM) -r $(TARGET_NAME)_src-$(VERSION)
	$(INSTALL) -m 755 -d $(TARGET_NAME)_src-$(VERSION)
	VERSION=$(VERSION) ; \
	TARGET_DIR=`pwd`/$(TARGET_NAME)_src-$$VERSION ; \
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d pack-src TARGET_DIR="$$TARGET_DIR/$$d" ; \
	done
	$(INSTALL) -m 644 $(PACK_SRC_FILES) $(TARGET_NAME)_src-$(VERSION)
	$(GTAR) -zcf $(TARGET_NAME)_src-$(VERSION).tar.gz \
	             $(TARGET_NAME)_src-$(VERSION)
	-$(RM) -r $(TARGET_NAME)_src-$(VERSION)

pack-clean:
	-$(RM) $(TARGET_NAME)-*.zip $(TARGET_NAME)_src-*.tar.gz
	-$(RM) -r $(TARGET_NAME)-$(VERSION) $(TARGET_NAME)_src-$(VERSION)

clean: clean-rec pack-clean
	-$(RM) $(EXE_NAME) *.o *.res *~
