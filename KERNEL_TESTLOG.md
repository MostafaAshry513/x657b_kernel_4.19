# KERNEL TEST LOG — every flash-test result

## Test 1 — Candidate C (boot_C_lcm.img)
- **Date:** 2026-06-04
- **MD5:** 062294031caf668c0df8b1a962aae52c
- **Flash:** boot partition
- **Result:** MTK LK printed "cmdline overflow" and rebooted — kernel never ran
- **Root cause (INITIAL — LATER DISPROVED):** From-source DTB /chosen/bootargs (267 chars) overflowed LK cmdline buffer
- **Conclusion:** DTB-packed-in-boot-image path broken. Fix: no DTB in boot image.

## Test 2 — Candidate D1r (boot_D1r_ramoops_v1.img)
- **Date:** 2026-06-05
- **MD5:** 37f91c6dcb450edc82dd23c4f3456417
- **Flash:** boot partition
- **Result:** LK printed "cmdline overflow" — IDENTICAL to candidate C
- **Root cause REVISED:** D1r had NO DTB (header v1). This DISPROVES the DTB-bootargs theory.
  Cmdline overflow is NOT caused by the boot-image DTB. Source is elsewhere.
- **New hypothesis:** Header v1 not properly supported by MTK LK on mt6761 → LK reads wrong offsets.

## Test 3 — PENDING: D1s (boot_D1s_stock_swap.img)
- **MD5:** 9eb15d77073e41b4a4e59650a897e7f7
- **zImage MD5:** c5d5c65f9b6c2f0ee558ebb20d1ffb10
- **Features:** Stock-identical boot image (header v2, byte-copied cmdline/os_version/name,
  dtb_size=0, dtbo_size=0, stock ramdisk) with ONLY the kernel replaced by our from-source zImage.
  Absolute minimal diff from known-good stock boot.
- **Test:** If this overflows → trigger is inside our zImage, not the header.
  If this boots past LK → header v1 was the issue.

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
