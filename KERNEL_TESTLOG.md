# KERNEL TEST LOG — every flash-test result

## Test 1 — Candidate C (boot_C_lcm.img)
- **Date:** 2026-06-04
- **MD5:** 062294031caf668c0df8b1a962aae52c
- **Flash:** boot partition
- **Result:** MTK LK printed "cmdline overflow" and rebooted — kernel never ran
- **Root cause:** From-source DTB /chosen/bootargs (267 chars debug + TABs) overflowed LK cmdline buffer
- **Conclusion:** DTB-packed-in-boot-image path broken. Fix: no DTB in boot image (header v1) or trimmed DTB.

## Test 2 — PENDING: D1r (boot_D1r_ramoops_v1.img)
- **MD5:** 37f91c6dcb450edc82dd23c4f3456417
- **zImage MD5:** c5d5c65f9b6c2f0ee558ebb20d1ffb10
- **Features:** Header v1 (no DTB), ramoops at 0x47E80000, LCM panel driver, stock ramdisk
- **Test steps:** Flash to boot. Observe: LK acceptance? display? adb? boot_completed?
- **If fail:** Reboot to TWRP, capture /sys/fs/pstore/console-ramoops, paste result.

## 2026-06-05 — candidate D1r boot_D1r_ramoops_v1.img (md5 37f91c6) — FAILED, KEY FINDING
TWRP-flashed (verified boot=D1r before reboot). Result: LK "cmdline overflow" AGAIN — same as candidate C.
CRITICAL: D1r has NO DTB (header v1), yet LK still overflows. => the cmdline overflow is NOT caused by the
boot-image DTB bootargs (candidate-D theory was WRONG). Source is elsewhere: the dtb-PARTITION bootargs LK
reads+appends, or how LK assembles the cmdline for any non-stock-structured boot image. Since it's a pre-kernel
LK error, ramoops captured nothing (kernel never ran). Also learned: rooted-Android `dd` to boot does NOT stick
(Magisk namespace) — ALWAYS flash test kernels via TWRP. Stock boot restored.
