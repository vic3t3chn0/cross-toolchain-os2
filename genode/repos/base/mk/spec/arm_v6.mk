SPECS       += arm
REP_INC_DIR += include/spec/arm_v6
CC_MARCH    ?= -march=armv6k -mfpu=vfp -mfloat-abi=softfp

include $(BASE_DIR)/mk/spec/arm.mk

