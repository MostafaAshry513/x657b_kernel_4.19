# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 04:10 UTC
**CURRENT PHASE:** 2 complete, awaiting flash test of D1r. Phase 4 prep done (KERNEL_READINESS.md written).

## LAST RESULT
- **D1r** built and pushed: `boot_D1r_ramoops_v1.img` (MD5: `37f91c6dcb450edc82dd23c4f3456417`)
- **KERNEL_READINESS.md** written — comprehensive 12-point risk assessment.
  - ✅ Mitigated: LK acceptance, DTB compat, ramdisk, SELinux, ramoops
  - ✅ Already present: touchscreen (NT36572), sensors, charger/PMIC
  - ⚠️ Ported untested: LCM panel driver
  - ❌ Cannot fix: TRAN_* vendor hooks (proprietary, no source), USB QMU (build broken)
- **Touch/sensor/config gaps analyzed:** Touchscreen NT36572 already compiled in. Sensor configs match stock.
  Charger/PMIC configs match. USB_MTK_HDRC build-broken, MUSB_QMU depends on it. No remaining
  config options from stock can be enabled without Kconfig/source additions.

## NEXT ACTION (build-side work — do NOT flash)
1. **Await D1r flash test result.** The orchestrator will flash D1r and report:
   - LK acceptance? display? adb? boot_completed?
   - If fail: pstore console-ramoops output (this is the key — it tells us exactly where it stalls)
2. **While waiting, continue porting:** Camera sensor drivers from Vivo Y81 (s5k3l6xx → s5k3l6ext2 adaptation).
   Low priority (camera not needed for boot) but moves us toward Phase 5.
3. **Investigate USB_MTK_HDRC struct fix** — can we add the missing .phy member to mt_usb_glue?
   Search for the struct definition and see if it's a simple addition or a major refactor.

## PENDING TEST REQUEST — D1r
**Candidate:** `boot_candidates/boot_D1r_ramoops_v1.img`
**MD5:** `37f91c6dcb450edc82dd23c4f3456417`
**zImage MD5:** `c5d5c65f9b6c2f0ee558ebb20d1ffb10`
**Flash to:** boot partition
**Observe:**
1. Does LK accept header v1? (no "cmdline overflow", no abort → kernel starts loading)
2. Does display light up? (kernel LCM driver initializes the panel)
3. Does adb come up? (`adb devices`, `adb shell`)
4. If adb: `getprop sys.boot_completed` → 1?
5. If boot fails/stalls: reboot to TWRP, capture:
   - `cat /sys/fs/pstore/console-ramoops` (THE CRITICAL ONE)
   - `cat /proc/last_kmsg`
   - Paste both outputs here
**Recovery:** flash stock boot_orig_backup.img if it fails
