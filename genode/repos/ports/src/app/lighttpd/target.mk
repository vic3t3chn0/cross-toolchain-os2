TARGET = lighttpd

include $(REP_DIR)/src/app/lighttpd/target.inc

LIBS += libc libm

CC_CXX_WARN_STRICT =
