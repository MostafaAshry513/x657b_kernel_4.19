#
# linux/arch/arm/vfp/Makefile
#
# Copyright (C) 2001 ARM Limited
#

# ccflags-y := -DDEBUG
# asflags-y := -DDEBUG

KBUILD_AFLAGS	:=$(filter-out -msoft-float, $(KBUILD_AFLAGS))
KBUILD_AFLAGS	+= -mfpu=neon -mfloat-abi=softfp
KBUILD_CFLAGS	:=$(filter-out -msoft-float, $(KBUILD_CFLAGS))
KBUILD_CFLAGS	+= -mfpu=neon -mfloat-abi=softfp

obj-y		+= vfpmodule.o entry.o vfphw.o vfpsingle.o vfpdouble.o
