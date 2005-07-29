#
# Makefile
#
# $Id: Makefile,v 1.1 2005/07/29 06:44:06 hos Exp $
#

PACK_BIN_DOC_FILES = 2button.mprc default.mprc dot.mprc
PACK_SRC_FILES = $(PACK_BIN_DOC_FILES) Makefile

INSTALL = install -p


all:

pack-bin:
	$(INSTALL) -m 644 $(PACK_BIN_DOC_FILES) $(TARGET_DIR)/..

pack-src:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(PACK_SRC_FILES) $(TARGET_DIR)

clean:
