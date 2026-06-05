# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 03:40 UTC
**CURRENT PHASE:** 2 (Diagnostic visibility) — D1r built with pstore/ramoops.

## LAST RESULT
- D1r (`boot_D1r_ramoops_v1.img`) BUILT SUCCESSFULLY.
  - Header v1, no DTB → LK falls back to dtb partition (no cmdline overflow risk)
  - Ramoops at 0x47E80000, 1MB (128KB console + 128KB record + 128KB ftrace + 128KB pmsg)
  - PSTORE_RAM=y, PSTORE_CONSOLE=y already present in config
  - Stock ramdisk from boot_orig_backup.img
  - LCM panel driver (nt36525b TXD) ported from Vivo Y81
  - zImage MD5: c5d5c65f9b6c2f0ee558ebb20d1ffb10
  - D1r MD5: 37f91c6dcb450edc82dd23c4f3456417

## NEXT ACTION (build-side work — do NOT flash)
1. **Map CONFIG_TRAN_* hooks** from stock config to sibling sources in KERNEL_KB.md.
   The 75 TRAN_* hooks in stock config need driver modules. Search Vivo Y81/Y3s/Nokia/Oppo for
   matching drivers. Document each: option name → what it does → sibling source path → status.
2. **Port available drivers** from sibling sources. Priority: touchscreen (focaltech/Novatek NT36xxx
   from Vivo Y81), sensors (BMI160/CM36558 from Vivo Y81), charger (RT9465 from Vivo Y81).
3. **Ask orchestrator for /proc/config.gz** from running stock-boot ROM for exact config comparison.
4. Continue building candidates as gaps are closed.

## PENDING TEST REQUEST — D1r
**Candidate:** `boot_candidates/boot_D1r_ramoops_v1.img`
**MD5:** 37f91c6dcb450edc82dd23c4f3456417
**Flash to:** boot partition
**Observe:**
- Does LK accept header v1? (no "cmdline overflow", no abort)
- Does kernel load and reach Android? (display? adb? `getprop sys.boot_completed`)
- If boot fails/stalls: reboot to TWRP, capture `/sys/fs/pstore/console-ramoops` and `/proc/last_kmsg`
- Paste pstore output + boot result back here
**Recovery:** flash stock 57e6 (`boot_orig_backup.img`) if it fails
