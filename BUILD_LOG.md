# X657B Kernel Build Log
**Date:** $(date -u "+%Y-%m-%d %H:%M UTC")
**Device:** Infinix X657B (MT6761)
**Source:** bengris32/android_kernel_mediatek_4.19 (Linux 4.19.325)
**Target:** Stock kernel (Linux 4.19.127 originally, clang 11.0.1)
**Repo:** github.com/MostafaAshry513/x657b_kernel_4.19

---

## Source Code Fixes Applied

| File | Fix | Reason |
|------|-----|--------|
| `include/linux/ftrace.h:325` | Commented out `void stack_trace_print(void);` | Conflicts with `include/linux/stacktrace.h:12` — different signatures |
| `kernel/rcu/tasks.h:411` | Changed `#if defined(CONFIG_TASKS_RCU) \|\| defined(CONFIG_TASKS_TRACE_RCU)` → `#if defined(CONFIG_TASKS_RCU)` | `CONFIG_TASKS_TRACE_RCU=y` but `struct rcu_tasks` only exists when `CONFIG_TASKS_RCU=y` |
| `kernel/rcu/tasks.h:427+` | Wrapped `call_rcu_tasks_generic` and all `struct rcu_tasks` code in `#ifdef CONFIG_TASKS_RCU` / `#endif` | Prevents undefined `struct rcu_tasks` when only TRACE variant selected |
| `kernel/trace/trace_stack.c:45,190` | Renamed `stack_trace_print(void)` → `stack_trace_print_max(void)` | Avoids conflict with ftrace.h declaration |
| `kernel/trace/bpf_trace.c:363` | Removed `inline` from `static inline __printf(1,0) int bpf_do_trace_printk(...)` | GCC cannot inline varargs functions |
| `include/linux/keyslot-manager.h:101` | Removed `inline` keyword | `always_inline` on declaration without body |
| `include/linux/blk-crypto.h:33` | Removed `inline` keyword | `always_inline` on declaration without body |

## Config Fixups

| Option | Value | Reason |
|--------|-------|--------|
| `CONFIG_DEBUG_INFO_BTF` | `# ... is not set` | ARM assembler `@object` → empty type string |
| `CONFIG_TASKS_TRACE_RCU` | `# ... is not set` | Missing `struct rcu_tasks` support in 4.19 |
| `CONFIG_SYSTEM_TRUSTED_KEYS` | `""` | cert not present on build machine |
| `CONFIG_SECURITY_LOCKDOWN_LSM_EARLY` | `# ... is not set` | `DEFINE_EARLY_LSM` macro not in 4.19 |

## Toolchain Trials

| Toolchain | Result | Error |
|-----------|--------|-------|
| GCC 4.9.3 | Failed | Interactive Kconfig prompts, too old |
| clang 9.0.3 | Failed | Unknown warning options (`-Wno-enum-compare-conditional`) |
| GCC 13.3.0 | Failed | Many incompatibilities: BPF BTF, RCU, MTK net drivers, inline issues, missing backported symbols |
| GCC 10 | Failed | `kernel/bpf/verifier.o` — ARM assembler `.type sym, @object` empty type |

## Current Status: FAILED

**Last error (GCC 10):**
```
/tmp/cczKRF9w.s: Assembler messages:
/tmp/cczKRF9w.s:35: Error: unrecognized symbol type ""
/tmp/cczKRF9w.s:58: Error: unrecognized symbol type ""
/tmp/cczKRF9w.s:66: Error: unrecognized symbol type ""
/tmp/cczKRF9w.s:74: Error: unrecognized symbol type ""
make[3]: *** [../scripts/Makefile.build:335: kernel/bpf/verifier.o] Error 1
```

**Root cause:** ARM assembler uses `@` as comment character. Backported BPF code emits `.type symbol_name, @object` which gets truncated to `.type symbol_name, ` (empty type = unrecognized).

## Next Steps

1. Try disabling `CONFIG_BPF_SYSCALL` (and `CONFIG_CGROUP_BPF`) in defconfig
2. OR: switch to clang 11 (what stock kernel was built with)
3. OR: patch kernel/bpf/verifier.c to avoid `.type @object` on ARM

---

## CLANG 11 SUCCESSFUL BUILD — 2026-06-02

### Toolchain
- **CC:** clang 11.0.2 (r383902b1, AOSP)
- **CROSS_COMPILE:** arm-linux-androideabi- (GCC 4.9 binutils)
- **LD:** ld.lld
- **CLANG_TRIPLE:** arm-linux-gnueabi-

### Config Fixups (after olddefconfig)
| Option | Value | Reason |
|--------|-------|--------|
| CONFIG_DEBUG_INFO_BTF | already off | ARM assembler @object issue |
| CONFIG_TASKS_TRACE_RCU | off | missing struct rcu_tasks |
| CONFIG_SYSTEM_TRUSTED_KEYS | already "" | certs not present |
| CONFIG_CUSTOM_KERNEL_IMGSENSOR | "s5k3l6_mipi_raw" | missing source dirs |
| CONFIG_CUSTOM_KERNEL_LCM | "" | missing source dirs |
| CONFIG_MICROTRUST_TEE_VERSION | "400" (was "300") | source dir is v400 |
| CONFIG_USB_MTK_HDRC | off | mt_usb_is_device + struct issues |
| CONFIG_CPU_CRYPTO_CE options | guarded in Makefile | ARMv8 crypto not supported by clang IAS |

### Source Fixes for clang IAS (ULE — Unified Assembly Language)

| File | Change | Reason |
|------|--------|--------|
| `arch/arm/vfp/vfpinstr.h` | `fmrx`/`fmxr`: MRC/MCR p10 → VMRS/VMSR + `.fpu neon` | clang IAS rejects p10 coprocessor |
| `arch/arm/vfp/vfphw.S` | vfp_get/put_float: MRC/MCR→VMOV; vfp_get/put_double: MCRR/MRRC→FMDRR/FMRRD with d16-d31 `.irp` | clang IAS rejects p10/p11 coprocessor |
| `arch/arm/include/asm/vfpmacros.h` | MRC/MCR p10→VMRS/VMSR; LDC/STC p11→VLDM/VSTM | clang IAS coprocessor support |
| `arch/arm/include/asm/vfp.h` | Register defines: cr0→fpsid, cr1→fpscr, etc. | Required for VMRS/VMSR names |
| `arch/arm/include/asm/uaccess.h` | `sbcccs` → `sbccs` | Pre-UAL→UAL SBC conditional |
| `arch/arm/include/asm/spinlock.h` | `rsbpls` → `rsbspl` | Pre-UAL→UAL RSB conditional |
| `arch/arm/kernel/entry-header.S` | `ldmccia`→`ldmiacc`, `stmccia`→`stmiacc` | Pre-UAL→UAL LDM/STM |
| `arch/arm/kernel/entry-common.S` | `stmloia`→`stmialo` | Pre-UAL→UAL STM |
| `arch/arm/kernel/entry-armv.S` | `ldrht`→`.inst`; `eoreqs`→`eorseq` | clang IAS limitations |
| `arch/arm/kernel/perf_event_v7.c` | p10 c11 MRC/MCR→`.inst`; fmrx/fmxr | clang IAS rejects p10 |
| `arch/arm/include/asm/assembler.h` | ALT_UP/ALT_UP_B: skip .if checks for clang; usracc/usraccoff: remove TUSER() default; LDM/STM condition order | Multiple clang IAS incompatibilities |
| `arch/arm/lib/clear_user.S` | `strnebt`→`strbtne`; removed `it ne` | Pre-UAL→UAL; IT invalid in ARM mode |
| `arch/arm/lib/copy_from_user.S` | `str\cond\()b`→`strb\cond` | Pre-UAL→UAL order |
| `arch/arm/lib/copy_to_user.S` | `ldr\cond\()b`→`ldrb\cond` | Pre-UAL→UAL order |
| `arch/arm/lib/memcpy.S` | `ldr\cond\()b`→`ldrb\cond`; `str\cond\()b`→`strb\cond` | Pre-UAL→UAL order |
| `arch/arm/lib/copy_page.S` | `ldmgtia`→`ldmiagt`; `ldmeqia`→`ldmiaeq` | Pre-UAL→UAL LDM |
| `arch/arm/lib/copy_template.S` | Removed double comma `, ,` | Too many positional args |
| `arch/arm/lib/csumpartial.S` | `ldrneb`→`ldrbne`; `adcnes`→`adcsne`; `ldrneh`→`ldrhne` | Pre-UAL→UAL |
| `arch/arm/lib/csumpartialcopygeneric.S` | `strneb`→`strbne`; `adcnes`→`adcsne` | Pre-UAL→UAL |
| `arch/arm/lib/csumpartialcopyuser.S` | `strneb`→`strbne` | Pre-UAL→UAL |
| `arch/arm/lib/div64.S` | `subcss`→`subshs`; `movnes`→`movsne` | clang CS cond parsing |
| `arch/arm/lib/io-readsb.S` | `ldrgeb`→`ldrbge` etc; `ldmeqfd`→`ldmfdeq` | Pre-UAL→UAL |
| `arch/arm/boot/compressed/head.S` | `mrc p15, 0, r15,...`→use r12 + tst | clang IAS rejects r15 dest |
| `arch/arm/vfp/Makefile` | Replace `-Wa,-mfpu=softvfp+vfp` with `-mfpu=neon -mfloat-abi=softfp` | clang doesn't use -Wa flags |
| `arch/arm/lib/Makefile` | Added `asflags-y += -fno-integrated-as` | Legacy ARM asm in lib |
| `arch/arm/boot/compressed/Makefile` | Added `-fno-integrated-as` to `asflags-y` | Legacy ARM asm in decompressor |
| `arch/arm/crypto/Makefile` | Guard CE (Crypto Extensions) obj lines with `ifneq clang` | ARMv8 crypto perlasm not clang-IAS compatible |
| `arch/arm/crypto/Kconfig` | Added `depends on BROKEN` to AES_ARM_CE, SHA2_ARM_CE | Prevent select chain enabling crypto .S |
| `kernel/usermode_driver.c` | `task_tgid`→`task_pid(group_leader)` | Backported API not in 4.19 |
| `net/core/filter.c` | Added `#define compat_ptr(ptr) (ptr)` | compat_ptr not in 4.19 |
| `drivers/misc/mediatek/lcm/mt65xx_lcm_list.c` | Commented out LCM_COMPILE_ASSERT | No LCMs configured |
| `drivers/misc/mediatek/usb20/musb_debugfs.c` | Guarded otg_sx code with `#ifdef CONFIG_MTK_MUSB_DUAL_ROLE` | Struct member absent |
| `drivers/misc/mediatek/usb20/mtk_musb.h` | Added stub `mt_usb_is_device()` for `!CONFIG_USB_MTK_HDRC` | Linker error |
| `drivers/misc/mediatek/base/power/spm/Makefile` | Fixed `include` path | TEEI header |

### Build Result
- **zImage:** SUCCESS (11 MB, MD5: 1a9f4fb3f0da778922d6c8048fd123b0)
- **Kernel image:** [out/arch/arm/boot/zImage]
- **Boot image:** boot_newkernel.img built from stock ramdisk + dtb + new zImage

### Build Command
```
export PATH=/root/android/lineage/prebuilts/clang/host/linux-x86/clang-r383902b1/bin:\
/root/android/lineage/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin:$PATH
make -j$(nproc) O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- \
     CROSS_COMPILE=arm-linux-androideabi- LD=ld.lld zImage
```

### Key Design Decisions
1. Used clang's integrated assembler for kernel objects (the goal); GNU as (`-fno-integrated-as`) for arch/arm/lib and arch/arm/boot/compressed where legacy ARM mnemonics are too numerous.
2. Disabled ARMv8 Crypto Extensions (*.ce) at Makefile level — these perlasm-generated .S files use instructions clang IAS doesn't support.
3. Converted VFP coprocessor instructions (MRC/MCR p10, MCRR/MRRC p11) to UAL VMRS/VMSR/VMOV/VLDM/VSTM throughout.
