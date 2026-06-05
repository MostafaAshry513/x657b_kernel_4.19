# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 07:15 UTC
**CURRENT PHASE:** 1 (cmdline overflow) — D1s and D1s_xz built, awaiting flash test.

## LAST RESULT
- **D1r FAILED** — cmdline overflow with NO DTB → DTB-bootargs theory DISPROVED.
- **D1s built** — stock-identical boot image, ONLY kernel replaced (header v2, dtb_size=0, stock ramdisk).
  - MD5: `9eb15d77073e41b4a4e59650a897e7f7`
  - zImage: 10.7 MB (GZIP), same LCM+ramoops kernel
- **D1s_xz built** — same as D1s but kernel compressed with XZ (7.0 MB, SMALLER than stock's 10.5 MB).
  - MD5: `87a8afed80bfe30e1a7a7e519e2ec828`
  - zImage MD5: `6cf907e77f7d5c4867a4f28e9f3d64c8`
  - XZ decompressor: CONFIG_XZ_DEC_ARM=y, CONFIG_XZ_DEC_ARMTHUMB=y — fully configured
  - Hypothesis: if cmdline overflow is triggered by kernel size (larger than stock = LK memory layout change),
    XZ compression (38% smaller than stock) should fix it.

## Theory: Cmdline overflow may be kernel-size-dependent
- Stock kernel: 10.5 MB → works
- D1r kernel: 10.7 MB (GZIP) → overflowed (header v1)
- D1s kernel: 10.7 MB (GZIP) → to test (header v2, if smaller header → might work)
- D1s_xz kernel: 7.0 MB (XZ) → to test (38% smaller than stock)
- If D1s overflows but D1s_xz doesn't → kernel size affects LK memory layout → overflow
- If both overflow → something else in kernel content triggers it
- If D1s works → header v1 was the issue (not kernel size)

## NEXT ACTIONS
1. **Flash D1s first.** If it works → header v1 was unsupported. Proceed to Phase 3.
2. **If D1s overflows:** Flash D1s_xz. If it works → kernel size was the issue.
3. **If both overflow:** The trigger is inside the kernel binary content (not size, not header).
   Investigate kernel configuration differences that LK might scan.

## PENDING TEST REQUESTS

### Test A — D1s
**File:** `boot_candidates/boot_D1s_stock_swap.img`
**MD5:** `9eb15d77073e41b4a4e59650a897e7f7`
**Test:** Does header v2 with dtb_size=0 pass LK when everything else is stock?

### Test B — D1s_xz (if D1s fails)
**File:** `boot_candidates/boot_D1s_xz.img`
**MD5:** `87a8afed80bfe30e1a7a7e519e2ec828`
**Test:** Does a smaller kernel (XZ, 7.0 MB) avoid the cmdline overflow?
