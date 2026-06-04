---
name: clang-ias-arm32-kernel-build
description: Methodology for building ARM32 (4.19) kernels with clang integrated assembler — VFP UAL conversion, legacy mnemonic fixes, config persistence, MTK quirks
source: auto-skill
extracted_at: '2026-06-02T23:24:14.701Z'
---

# Building ARM32 Kernels with clang Integrated Assembler

## Overview

clang's integrated assembler (IAS) for ARM has strict UAL (Unified Assembly Language) requirements. Pre-UAL ARM mnemonics — common in 4.19-era kernel source — cause `invalid instruction` errors. Additionally, clang IAS rejects VFP coprocessor instructions (p10/p11 MRC/MCR/MCRR/MRRC) and ARMv8 Crypto Extensions from perlasm output. This skill covers the systematic approach used to build an MT6761 (4.19.325) kernel with AOSP clang 11.

## Toolchain Setup

Use clang as `CC` with integrated assembler, GCC binutils for `CROSS_COMPILE`, and lld for linking:

```
export PATH=/path/to/clang-r383902b1/bin:/path/to/arm-linux-androideabi-4.9/bin:$PATH
make O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- \
     CROSS_COMPILE=arm-linux-androideabi- LD=ld.lld zImage
```

**Do NOT pass `-fno-integrated-as` globally** — use it only for directories with too many legacy .S files (see Makefile guards below).

## VFP Coprocessor → UAL Conversion

clang IAS does not support `mrc p10` / `mcr p10` / `mcrr p11` / `mrrc p11` instructions. Convert to UAL mnemonics:

### System Register Access (fmrx/fmxr)
- **Pre-UAL:** `mrc p10, 7, Rd, crN, cr0, 0` / `mcr p10, 7, Rd, crN, cr0, 0`
- **UAL:** `vmrs Rd, <regname>` / `vmsr <regname>, Rd`
- Map coprocessor registers to names in `arch/arm/include/asm/vfp.h`:
  - `cr0`→`fpsid`, `cr1`→`fpscr`, `cr6`→`mvfr1`, `cr7`→`mvfr0`, `cr8`→`fpexc`, `cr9`→`fpinst`, `cr10`→`fpinst2`
- Add `.fpu neon` directive inside inline asm blocks for files outside `arch/arm/vfp/`

### Data Register Access (float/double transfers)
- **Pre-UAL:** `mrc p10, 0, Rd, cN, c0, 0` (fmrs) / `mcr p10, 0, Rd, cN, c0, 0` (fmsr)
- **UAL:** `vmov Rd, sN` / `vmov sN, Rd`
- For floats, iterate `.irp` over 0..31 directly with `vmov r0, s\dr`

- **Pre-UAL:** `mrrc p11, 3, Rd, Rd2, cN` (fmrrd) / `mcrr p11, 3, Rd, Rd2, cN` (fmdrr)
- **UAL:** `fmrrd Rd, Rd2, dN` / `fmdrr dN, Rd, Rd2`
- For d16-d31: use `.irp dr,16,17,...31` with `fmrrd r0, r1, d\dr`

### VFP Load/Store Multiple
- **Pre-UAL:** `LDC p11, cr0, [Rn],#32*4` / `STC p11, cr0, [Rn],#32*4`
- **UAL:** `VLDM Rn!, {d0-d15}` / `VSTM Rn!, {d0-d15}`
- Conditional forms: `ldclne p11...` → `vldmne Rn!, {d16-d31}`

### VFP Makefile
Change the VFP Makefile to pass `-mfpu=neon -mfloat-abi=softfp` instead of `-Wa,-mfpu=softvfp+vfp` (clang doesn't use `-Wa` flags for IAS):

```
KBUILD_AFLAGS :=$(filter-out -msoft-float, $(KBUILD_AFLAGS))
KBUILD_AFLAGS += -mfpu=neon -mfloat-abi=softfp
KBUILD_CFLAGS :=$(filter-out -msoft-float, $(KBUILD_CFLAGS))
KBUILD_CFLAGS += -mfpu=neon -mfloat-abi=softfp
```

## Legacy ARM Mnemonic Patterns → UAL

### Condition Ordering
Pre-UAL puts condition before size suffix; UAL reverses:
| Pre-UAL | UAL | Pattern |
|---------|-----|---------|
| `strneb` | `strbne` | `{instr}{cond}b` → `{instr}b{cond}` |
| `ldrneb` | `ldrbne` | Same for ldr |
| `ldrgtb` | `ldrbgt` | Any condition |
| `adcnes` | `adcsne` | S-flag before condition |
| `subcss`  | `subshs`  | CS condition → HS for clang |
| `movnes`  | `movsne`  | S-flag before condition |
| `rsbpls`  | `rsbspl`  | S-flag before condition |
| `sbcccs`  | `sbccs`   | Drop one `c` (UAL SBC+CC+S) |
| `eoreqs`  | `eorseq`  | S before condition |

### LDM/STM Addressing Mode + Condition Ordering
| Pre-UAL | UAL |
|---------|-----|
| `ldmccia`  | `ldmiacc` |
| `stmccia`  | `stmiacc` |
| `stmloia`  | `stmialo` |
| `ldmgtia`  | `ldmiagt` |
| `ldmeqfd`  | `ldmfdeq` |
| `ldmnefd`  | `ldmfdne` |

### Thumb2 Instructions in ARM Mode
- `ldrht` → encode with `.inst 0xe0bNN0b0` (ARM LDRHT encoding)
- `it ne` block → remove; ARM mode doesn't use IT blocks

### r15 as Destination
- `mrc p15, 0, r15, c7, c14, 3` → use scratch register: `mrc p15, 0, r12, c7, c14, 3` + `tst r12, r12`

## ALT_UP Macro Fix

The `ALT_UP` macro in `arch/arm/include/asm/assembler.h` uses `.if . - 9997b` checks that clang IAS cannot evaluate. Use `#ifdef __clang__` at the CPP level (since `.S` files go through CPP):

```
#ifdef __clang__
#define ALT_UP(instr...) \
    .pushsection ".alt.smp.init", "a"; .long 9998b; 9997: instr; .popsection
#else
#define ALT_UP(instr...) \
    /* original with .if checks */
#endif
```

Similarly for `ALT_UP_B`: skip the `.equ up_b_offset` for clang, just emit a direct branch.

## `usracc`/`usraccoff` Macros — TUSER() Issue

The macros use `t=TUSER()` as default parameter. `TUSER()` is a CPP function-like macro that requires 1 argument; called with 0 args it doesn't expand in clang CPP, leaving literal `TUSER()` in assembly. Fix: remove the default value and pass empty `t` explicitly from callers (strusr/ldrusr).

## ARMv8 Crypto Extensions

Perlasm-generated .S files (`aes-ce-core.S`, `sha256-core.S`, `sha512-core.S`) use ARMv8 Crypto Extension instructions (`aese.8`, `aesd.8`, `aesimc.8`, `sha256h.32`, etc.) that clang IAS rejects. 

**Fix:** Guard the CE object lines in `arch/arm/crypto/Makefile`:

```
ifneq ($(CONFIG_CC_IS_CLANG),y)
ce-obj-$(CONFIG_CRYPTO_AES_ARM_CE) += aes-arm-ce.o
ce-obj-$(CONFIG_CRYPTO_SHA2_ARM_CE) += sha2-arm-ce.o
# ... etc
endif
```

Keep non-CE crypto objects (blake2s, chacha, poly1305, curve25519) unguarded — their .S files compile fine and wireguard needs them.

## Config Persistence

The kernel build system (`syncconfig`) re-enables options via `select` chains. When you need to disable an option that something else selects:
1. Edit `.config` with `sed` AFTER `olddefconfig`, immediately before `make`
2. Remove `out/include/config/auto.conf` before building (forces regeneration from edited `.config`)
3. Or break the `select` chain in Kconfig with `depends on BROKEN`

## GNU as Fallback Directories

When a directory has too many legacy .S files, add `asflags-y += -fno-integrated-as` to its Makefile:
- `arch/arm/lib/Makefile` — csumpartial, copy_template, memset, div64, io-read/write, etc.
- `arch/arm/boot/compressed/Makefile` — lib1funcs.S, head.S, etc.
  - Note: this Makefile uses `asflags-y := -DZIMAGE` (override, not append) later; add `-fno-integrated-as` to that line.

## MTK-Specific Quirks

- **TEEI version mismatch:** Config says `MICROTRUST_TEE_VERSION="300"` but source is `drivers/tee/teei/400/`. Fix: `sed -i 's/"300"/"400"/'` in `.config`.
- **Missing camera sensor dirs:** `CUSTOM_KERNEL_IMGSENSOR` lists sensors whose source dirs don't exist. Trim to only those present.
- **Missing LCM dirs:** `CUSTOM_KERNEL_LCM` — all may be missing. Set to `""` and comment out `LCM_COMPILE_ASSERT` in `mt65xx_lcm_list.c`.
- **USB struct members (`otg_sx`):** Behind `#ifdef CONFIG_MTK_MUSB_DUAL_ROLE` but debugfs code accesses it unconditionally. Guard with same `#ifdef`.
- **USB debugfs unused variable (`proc_dr_files`):** In `drivers/misc/mediatek/usb20/musb_debugfs.c`, the `proc_dr_files` array is declared outside the `#ifdef CONFIG_MTK_MUSB_DUAL_ROLE` guard but only used inside it. When the config is disabled, `-Werror,-Wunused-variable` kills the build. Fix: add `__maybe_unused` to the declaration:
  ```c
  static struct proc_dir_entry *proc_dr_files[PROC_FILE_DR_NUM] __maybe_unused = {NULL, NULL};
  ```
- **`mt_usb_is_device` stub:** When `CONFIG_USB_MTK_HDRC` is disabled, provide `static inline bool mt_usb_is_device(void) { return false; }` in `mtk_musb.h`.

## Backport API Gaps (4.19 vs newer)

- `task_tgid(tsk)` → `task_pid(tsk->group_leader)` — returns `struct pid *`
- `compat_ptr(x)` → `#define compat_ptr(ptr) (ptr)` — 32-bit identity; not defined in 4.19

## Build Iteration Strategy

1. Seed defconfig from stock, run `olddefconfig`
2. Apply known config fixups (DEBUG_INFO_BTF, TASKS_TRACE_RCU, etc.)
3. Build; for each error batch:
   a. Fix source (prefer UAL conversion over disabling features)
   b. Re-run `make` (syncconfig may re-enable things)
   c. If config keeps reverting, edit `.config` just before the `make` command
4. When assembly errors become too numerous in a directory, add `-fno-integrated-as` there
