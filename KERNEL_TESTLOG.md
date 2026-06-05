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
