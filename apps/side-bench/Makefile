#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Targets
TARGETS := $(notdir $(SOURCE_DIR)).bin

# Make sure this symbol stays around as we don't reference this, but
# whoever loads us will
LDFLAGS += -u __vsyscall_ptr

# Source files required to build the target     
ASMFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.S))
CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/aes/*.c))
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
# Libraries
LIBS = c sel4 cpio elf sel4muslcsys sel4vka sel4allocman \
platsupport sel4platsupport sel4test sel4vspace \
sel4utils sel4simple utils sel4cachecolor            

#macros for openssl compile
CFLAGS += -DOPENSSL_SYSNAME_LINUX


include $(SEL4_COMMON)/common.mk



