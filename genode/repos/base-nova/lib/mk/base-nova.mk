include $(BASE_DIR)/lib/mk/base.inc

LIBS   += base-nova-common cxx timeout
SRC_CC += thread_start.cc
SRC_CC += cache.cc
SRC_CC += signal.cc
SRC_CC += platform.cc
