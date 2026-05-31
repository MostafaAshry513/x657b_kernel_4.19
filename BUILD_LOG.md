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
