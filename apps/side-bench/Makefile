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

#include directory

#asm files
ASMFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.S))
ifdef CONFIG_ARCH_X86
ASMFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/mastik/*.S))
endif
ifdef CONFIG_ARCH_ARM 
ASMFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/mastik_arm/*.S))
endif 

#c files
CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/arch/$(ARCH)/*.c))

ifdef CONFIG_ARCH_X86
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/aes/*.c))
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/mastik/*.c))
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/MPI/*.c))
endif
ifdef CONFIG_ARCH_ARM 
CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/mastik_arm/*.c))
endif 

CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/mastik_common/*.c))

#splash begin

ifdef CONFIG_BENCH_SPLASH_FFT 
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/fft/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_CHOLESKY
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/cholesky/*.c))
SPLASH_DATAFILES := $(SOURCE_DIR)/data/cholesky_tk14_data
endif 


ifdef CONFIG_BENCH_SPLASH_LU
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/lu/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_RADIX
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/radix/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_BARNES
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/barnes/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_FMM
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/fmm/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_OCEAN
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/ocean/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_RADIOSITY
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/radiosity/*.c))
endif 

ifdef CONFIG_BENCH_SPLASH_RAYTRACE
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/raytrace/*.c))
SPLASH_DATAFILES := $(SOURCE_DIR)/data/teapot.env
SPLASH_DATAFILES += $(SOURCE_DIR)/data/teapot.geo
endif 

ifdef CONFIG_BENCH_SPLASH_WATER_NSQUARED
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/water-nsquared/*.c))
SPLASH_DATAFILES := $(SOURCE_DIR)/data/random.in
endif 


ifdef CONFIG_BENCH_SPLASH_WATER_SPATIAL
SPLASH_CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/water-spatial/*.c))
SPLASH_DATAFILES := $(SOURCE_DIR)/data/random.in
endif 

#stub
SPLASH_CFILES +=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/splash/*.c))

SPLASH_CFLAG := -D_POSIX_C_SOURCE=200113 -static -msoft-float 
SPLASH_CFLAG += -Wno-format -W -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wdisabled-optimization
SPLASH_CFLAG += -Wpadded -Winline -Wpointer-arith -Wsign-compare -Wendif-labels 

SPLASH_LDFLAG := -lm
#splash end 

CFLAGS += -DDBG_MEMORY=0 -I $(SOURCE_DIR)/src/ -I $(SOURCE_DIR)/src/mastik_common/ -I $(SOURCE_DIR)/../

ifdef CONFIG_ARCH_ARM 
CFLAGS += -I $(SOURCE_DIR)/src/mastik_arm/
endif 

ifdef CONFIG_ARCH_X86
CFLAGS += -I $(SOURCE_DIR)/src/mastik/
endif 

ifdef CONFIG_ARCH_AARCH32
CFLAGS += -I $(SOURCE_DIR)/arch_include/aarch32/
endif

ifdef CONFIG_ARCH_X86_64
CFLAGS += -I $(SOURCE_DIR)/arch_include/x86_64/
endif


ifdef CONFIG_BENCH_SPLASH 
CFILES += $(SPLASH_CFILES) 
CFLAGS += $(SPLASH_CFLAGS) 
LDFLAGS += $(SPLASH_LDFLAGS)
endif 


# Libraries
LIBS = c sel4 cpio elf sel4muslcsys sel4vka sel4allocman \
platsupport sel4platsupport  sel4vspace \
 sel4utils sel4debug sel4simple utils  sel4bench           

ifdef CONFIG_KERNEL_STABLE 
LIBS += sel4simple-stable 
else 
LIBS += sel4simple-default 
endif 

#macros for openssl compile
CFLAGS += -DOPENSSL_SYSNAME_LINUX

# CPIO archive                                                             
ifneq ($(SPLASH_DATAFILES), )
OFILES := archive.o     
endif 


include $(SEL4_COMMON)/common.mk

ifneq  ($(SPLASH_DATAFILES), )
archive.o: $(SPLASH_DATAFILES)
	${COMMON_PATH}/files_to_obj.sh $@ _cpio_archive $^
endif 
