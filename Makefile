#
# Makefile
#
# $Id: Makefile,v 1.32 2005/02/03 09:51:12 hos Exp $
#

DEFINES = -D_WIN32_WINNT=0x0500 -DUNICODE=1 -D_UNICODE=1
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
EXE_LDLIBS = $(UTIL_LIBS) -L. -l$(SBH_DLL_NAME) \
             -lpsapi -lole32 -loleaut32 -loleacc -luuid
EXE_LDFLAGS = $(LDFLAGS)

SBI_DLL_NAME = $(TARGET_NAME)sbi.dll
SBI_DLL_SRCS = sbi_dllmain.c dllinj.c shmem.c
SBI_DLL_RSRC = 
SBI_DLL_OBJS = $(SBI_DLL_SRCS:%.c=%.o) $(SBI_DLL_RSRC:%.rc=%.o)
SBI_DLL_HEADERS = dllinj.h scroll_op_scrollbar.h shmem.h
SBI_DLL_LDLIBS = -lpsapi -lkernel32
SBI_DLL_LDFLAGS = $(LDFLAGS) -shared -nostartfiles -nostdlib -e _DllMain@12

SBH_DLL_NAME = $(TARGET_NAME)sbh.dll
SBH_DLL_SRCS = sbh_dllmain.c shmem.c
SBH_DLL_RSRC = 
SBH_DLL_OBJS = $(SBH_DLL_SRCS:%.c=%.o) $(SBH_DLL_RSRC:%.rc=%.o)
SBH_DLL_HEADERS = scroll_op_scrollbar.h shmem.h
SBH_DLL_LDLIBS = -lkernel32 -luser32
#SBH_DLL_LDFLAGS = $(LDFLAGS) -shared -nostartfiles -nostdlib -e _DllMain@12 \
#                  -Wl,--out-implib,lib$(SBH_DLL_NAME).a
SBH_DLL_LDFLAGS = $(LDFLAGS) -shared \
                  -Wl,--out-implib,lib$(SBH_DLL_NAME).a

PACK_BIN_FILES = $(EXE_NAME) $(SBI_DLL_NAME)
PACK_BIN_ADD_FILES = README.txt VERSION default.mprc
PACK_SRC_FILES = $(EXE_SRCS) $(EXE_RSRC) $(EXE_HEADERS) icon.ico \
                 $(SBI_DLL_SRCS) $(SBI_DLL_RSRC) $(SBI_DLL_HEADERS) \
                 Makefile

SUBDIRS = util doc
TARGET = $(SBI_DLL_NAME) $(SBH_DLL_NAME) $(EXE_NAME)
VERSION = `cat VERSION`

RC = rc
WINDRES = windres
DLLTOOL = dlltool
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

$(EXE_NAME): $(EXE_OBJS) $(UTIL_LIBS) lib$(SBH_DLL_NAME).a
	$(CC) $(EXE_LDFLAGS) $(EXE_OBJS) $(EXE_LDLIBS) -o $@

$(SBI_DLL_NAME): $(SBI_DLL_OBJS)
	$(CC) $(SBI_DLL_LDFLAGS) $(SBI_DLL_OBJS) $(SBI_DLL_LDLIBS) -o $@

$(SBH_DLL_NAME): $(SBH_DLL_OBJS)
	$(CC) $(SBH_DLL_LDFLAGS) $(SBH_DLL_OBJS) $(SBH_DLL_LDLIBS) -o $@

lib$(SBH_DLL_NAME).a: $(SBH_DLL_NAME)

$(EXE_OBJS) : $(EXE_HEADERS)

$(SBI_DLL_OBJS) : $(SBI_DLL_HEADERS)

$(SBH_DLL_OBJS) : $(SBH_DLL_HEADERS)

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
	-$(RM) $(TARGET) *.a *.o *.res *.tmp *~
