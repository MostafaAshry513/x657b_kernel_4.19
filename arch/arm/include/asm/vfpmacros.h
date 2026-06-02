/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/arm/include/asm/vfpmacros.h
 *
 * Assembler-only file containing VFP macros and register definitions.
 */
#include <asm/hwcap.h>

#include <asm/vfp.h>

@ Macros to allow building with old toolkits (with no VFP support)
	.macro	VFPFMRX, rd, sysreg, cond
	VMRS\cond	\rd, \sysreg		@ FMRX	\rd, \sysreg
	.endm

	.macro	VFPFMXR, sysreg, rd, cond
	VMSR\cond	\sysreg, \rd		@ FMXR	\sysreg, \rd
	.endm

	@ read all the working registers back into the VFP
	.macro	VFPFLDMIA, base, tmp
#if __LINUX_ARM_ARCH__ < 6
	VLDM	\base!, {d0-d15}
#else
	VLDM	\base!, {d0-d15}
#endif
#ifdef CONFIG_VFPv3
#if __LINUX_ARM_ARCH__ <= 6
	ldr	\tmp, =elf_hwcap		    @ may not have MVFR regs
	ldr	\tmp, [\tmp, #0]
	tst	\tmp, #HWCAP_VFPD32
	vldmne	\base!, {d16-d31}
	addeq	\base, \base, #32*4		    @ step over unused register space
#else
	VFPFMRX	\tmp, MVFR0			    @ Media and VFP Feature Register 0
	and	\tmp, \tmp, #MVFR0_A_SIMD_MASK	    @ A_SIMD field
	cmp	\tmp, #2			    @ 32 x 64bit registers?
	vldmeq	\base!, {d16-d31}
	addne	\base, \base, #32*4		    @ step over unused register space
#endif
#endif
	.endm

	@ write all the working registers out of the VFP
	.macro	VFPFSTMIA, base, tmp
#if __LINUX_ARM_ARCH__ < 6
	VSTM	\base!, {d0-d15}
#else
	VSTM	\base!, {d0-d15}
#endif
#ifdef CONFIG_VFPv3
#if __LINUX_ARM_ARCH__ <= 6
	ldr	\tmp, =elf_hwcap		    @ may not have MVFR regs
	ldr	\tmp, [\tmp, #0]
	tst	\tmp, #HWCAP_VFPD32
	vstmne	\base!, {d16-d31}
	addeq	\base, \base, #32*4		    @ step over unused register space
#else
	VFPFMRX	\tmp, MVFR0			    @ Media and VFP Feature Register 0
	and	\tmp, \tmp, #MVFR0_A_SIMD_MASK	    @ A_SIMD field
	cmp	\tmp, #2			    @ 32 x 64bit registers?
	vstmeq	\base!, {d16-d31}
	addne	\base, \base, #32*4		    @ step over unused register space
#endif
#endif
	.endm
