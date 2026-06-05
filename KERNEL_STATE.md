# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 09:30 UTC
**CURRENT PHASE:** 1 (cmdline overflow) — D1s PASSED LK but kernel crashed. D2 candidates built.

## LAST RESULT
- **D1s (stock-swap) PASSED LK** — no "cmdline overflow" message. Kernel loaded and crashed before
  pstore/ramoops init → black screen, no kernel log.
- **Root cause refined:** Our kernel DOES boot past LK when using header v2. It CRASHES because D1s used
  the stock dtb partition DTB which has different peripheral HW nodes than our kernel expects.
  Candidate A (our from-source DTB) previously booted to userspace with the same kernel.

## Theory
- Header v2 with our DTB = kernel matches HW nodes → boots (candidate A evidence)
- Header v2 with stock DTB = HW node mismatch → kernel panics early (D1s evidence)
- Header v1 with no DTB = LK treats v1 differently → cmdline overflow (D1r evidence)

## Fix
Use header v2 + our from-source DTB with TRIMMED bootargs (99 chars, no debug flags or TABs).
The trimmed DTB includes ramoops at 0x47E80000 for boot diagnostics.

## Candidates Ready for Test

### D2s_gz — GZIP kernel + trimmed DTB + stock ramdisk
- **File:** `boot_candidates/boot_D2s_gz.img`
- **MD5:** `111d8765452685fc520a04ab41b39c0a`
- **zImage:** GZIP, 10.7 MB (MD5: `6c718669e2b3167f4e79887c1a0d248c`)
- **DTB:** trimmed (99-char bootargs, ramoops at 0x47E80000)
- **Ramdisk:** stock from boot_orig_backup.img
- **Features:** LCM panel driver + ramoops

### D2s_xz — XZ kernel + trimmed DTB + stock ramdisk
- **File:** `boot_candidates/boot_D2s_xz.img`
- **MD5:** `670616eb583d749e27607ea8ca384b6d`
- **zImage:** XZ, 7.0 MB (MD5: `7a51e946309655bb2d41696936b50271`)
- **DTB:** same trimmed DTB
- **Ramdisk:** stock from boot_orig_backup.img
- **Features:** LCM panel driver + ramoops, 38% smaller than stock kernel

## PENDING TEST REQUEST
Flash D2s_gz first (GZIP is standard, matches candidate A which booted to userspace).
If D2s_gz fails, flash D2s_xz.

**Both candidates:** header v2, our DTB (compatible HW nodes), trimmed bootargs (no overflow risk),
stock ramdisk, ramoops at 0x47E80000.

**Observe:**
1. Does LK pass? (no "cmdline overflow")
2. Does kernel boot? Display? ADB?
3. If stall/fail: capture `/sys/fs/pstore/console-ramoops` from TWRP — this is the critical log
4. If boot_completed: ADB shell available, kernel is working

**Recovery:** flash stock boot_orig_backup.img
