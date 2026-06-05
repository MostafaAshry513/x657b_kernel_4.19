# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 06:20 UTC
**CURRENT PHASE:** 1 (cmdline overflow) — RE-OPENED. D1r failed with same "cmdline overflow" as C.

## LAST RESULT
- **D1r FAILED** on real device: LK printed "cmdline overflow" — identical to candidate C.
  - D1r had NO DTB (header v1) and STILL overflowed.
  - **This disproves the DTB-bootargs theory.** The overflow source is elsewhere.
- **New hypothesis:** Header v1 may not be properly supported by MTK LK on mt6761.
  - Stock uses header v2, dtb_size=0 → works.
  - D1r uses header v1 → overflows (same 40-char cmdline).
  - Maybe LK reads v2-specific fields at wrong offsets in v1, gets garbage, causes overflow.

## NEXT ACTION
1. **Test D1s** — stock-identical boot image with ONLY the kernel replaced.
   - Header v2, byte-copied from stock (cmdline, os_version, name, offsets)
   - dtb_size=0, dtbo_size=0
   - Stock ramdisk
   - Our from-source zImage (with LCM panel driver + ramoops)
   - This is the ABSOLUTE MINIMAL DIFF from the known-good stock boot.
2. **If D1s also overflows:** The overflow is triggered by something IN the kernel image
   itself (not the header). Investigate: kernel size, decompressor format, appended data.
3. **If D1s boots past LK:** Header v1 was the issue. All future candidates use header v2.

## PENDING TEST REQUEST — D1s
**Candidate:** `boot_candidates/boot_D1s_stock_swap.img`
**MD5:** `9eb15d77073e41b4a4e59650a897e7f7`
**zImage MD5:** `c5d5c65f9b6c2f0ee558ebb20d1ffb10` (same LCM+ramoops kernel as D1r)
**Flash to:** boot partition
**Observe:**
1. Does LK boot past cmdline? Kernel starts loading?
2. If same "cmdline overflow": the trigger is inside our zImage — not header, not DTB.
3. If boots: header v1 was the problem. Proceed to Phase 3/4.
**Recovery:** flash stock boot_orig_backup.img
