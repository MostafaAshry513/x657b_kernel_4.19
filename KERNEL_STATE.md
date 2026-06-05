# KERNEL STATE — read first, UPDATE after every step

**Updated:** 2026-06-05 by orchestrator (initial).
**CURRENT PHASE:** 2 (Diagnostic visibility) — Phase 1 (cmdline overflow) is fixed but awaiting flash-confirmation.

## LAST RESULT
- Candidate C (`boot_C_lcm.img`) was flash-tested on the real device → MTK LK printed **"cmdline overflow"**
  and aborted **before** the kernel ran. Root cause: the from-source DTB `/chosen/bootargs` was 267 chars of
  debug flags + literal TABs; packed as header v2 (dtb_size>0) LK reads it, appends its own args, overflows
  the cmdline buffer. Stock boot works because dtb_size=0 (LK uses the separate dtb partition).
- Candidate D built to fix it: **D1** (no DTB, header v1 — LK falls back to dtb partition, RECOMMENDED),
  D2 (trimmed DTB, header v2), D3 (stock DTB, header v2). **D1 has NOT been flash-tested yet.**

## NEXT ACTION (do now — NO flashing; build-side work)
1. **Phase 2 (priority): get boot logs.** Extract the ramoops/pstore **reserved-memory** region from the
   stock boot's DTB (in `/root/android/flash_build9/boot_orig_backup.img`) — find the `ramoops`/`reserved-memory`
   node address+size. Configure the from-source kernel's defconfig (`CONFIG_PSTORE`, `CONFIG_PSTORE_CONSOLE`,
   `CONFIG_PSTORE_RAM`) and DTB so it reserves the SAME region. Rebuild D1 with ramoops on → call it **D1r**.
   Goal: the next flash-test, pass or fail, leaves a readable `/sys/fs/pstore/console-ramoops`.
2. Statically confirm this device's LK accepts **header v1 / no-dtb** (check the stock boot header version and
   how LK loads the dtb partition) — so D1's approach is sound before we spend a flash on it.
3. **Phase 4 prep:** get `/proc/config.gz` from the running stock-boot ROM (ask orchestrator to pull it via adb),
   extract the full `CONFIG_TRAN_*` list (75 options) and the stock driver list; in `KERNEL_KB.md` map each
   TRAN_* / missing driver to a sibling-source implementation (Vivo Y81 / Nokia / Oppo). This is the long tail
   of Phase 4 and can be worked entirely build-side in parallel.

## PENDING TEST REQUEST
(Fill once D1r is built.) Flash **D1r** (`boot_candidates/boot_D1r_*.img`, md5 TBD) to `boot`; observe display +
`adb` + `getprop sys.boot_completed`. Whatever happens: reboot to TWRP and capture
`/sys/fs/pstore/console-ramoops` + `/proc/last_kmsg`, pull both, paste back here. Then restore stock 57e6 if it didn't boot.
