#
# Makefile
#
# $Id: Makefile,v 1.1 2005/01/29 20:53:02 hos Exp $
#

PACK_BIN_DOC_FILES = CUSTOMIZE.html std.css manual.css
PACK_SRC_FILES = $(PACK_BIN_DOC_FILES) Makefile

INSTALL = install


all:

pack-bin:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(PACK_BIN_DOC_FILES) $(TARGET_DIR)

pack-src:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(PACK_SRC_FILES) $(TARGET_DIR)

clean:
