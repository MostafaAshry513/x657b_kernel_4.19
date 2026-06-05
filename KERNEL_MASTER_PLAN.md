# X657B From-Source Kernel — MASTER PLAN (long-horizon: weeks→months)

**GOAL:** a from-source MT6761 4.19 kernel that BOOTS the Infinix X657B all the way to a fully
working Android (`sys.boot_completed=1` + display + all peripherals), able to replace the stock
`57e6` boot. The user has committed weeks/months. This plan is built to **survive agent restarts and
long timelines**: all state lives in files, progress is incremental and logged, nothing is re-derived.

## OPERATING RULES (re-read these EVERY session / after every restart)
1. **On any start/restart, BEFORE doing anything:** read this plan, then `KERNEL_STATE.md`, then
   `KERNEL_KB.md`, then `tail KERNEL_TESTLOG.md`. Resume from `KERNEL_STATE.md` → "NEXT ACTION".
2. **After every meaningful step:** update `KERNEL_STATE.md` (current phase / blocker / next action /
   last result / pending test request). Append new findings to `KERNEL_KB.md`. `git commit && push`
   (GitHub `MostafaAshry513/x657b_kernel_4.19`) and `mega-put` to `/X657B-build/kernel/`.
3. **NEVER flash.** When you need a device test, write a precise TEST REQUEST in `KERNEL_STATE.md`; the
   orchestrator/user flashes and pastes back the result (boot state + pstore/last_kmsg). Keep doing
   build-side work meanwhile — there is ALWAYS analysis/porting to do between tests; never sit idle.
4. **Never endanger the working ROM.** Stock boot `57e6` (`/root/android/flash_build9/boot_orig_backup.img`)
   is always restorable. Test candidates are flashed to `boot` (mmcblk0p28) and reverted if they fail.
5. **One variable at a time** where possible; always diff candidates against the known-good stock boot.
6. **Don't stop early.** Only a booting, stable kernel ends this. "Partially works" is not done.

## PHASES (each has a hard acceptance criterion)
- **Phase 1 — Pass the bootloader.** [DONE pending confirm] cmdline-overflow fixed (candidate D1 = no DTB,
  header v1 → LK uses the dtb partition like stock). *Accept:* LK loads the kernel, no "cmdline overflow"/abort.
- **Phase 2 — DIAGNOSTIC VISIBILITY (highest leverage — DO THIS FIRST).** Configure **pstore/ramoops
  console** at the EXACT reserved-memory address the stock kernel/DTB uses, so after ANY failed boot we
  reboot to TWRP and read `/sys/fs/pstore/console-ramoops` = the kernel log. Without this we are blind
  (candidates A/B "stalled" with zero logs). With it, every test yields the next concrete blocker.
  Also enable `earlycon`/UART if a hardware test point exists. *Accept:* a deliberately-failed boot
  leaves a readable kernel log in pstore.
- **Phase 3 — Display under the from-source kernel.** (nt36525b LCM ported from Vivo Y81 in candidate C.)
  *Accept:* the panel lights from the KERNEL (not LK's own display).
- **Phase 4 — Reach boot_completed.** Loop: flash → read pstore → identify the stall (missing driver /
  CONFIG_TRAN_* hook / SELinux deny / panic) → port/fix from the stock binary + sibling sources → rebuild.
  Suspects: the 75 `CONFIG_TRAN_*` hooks, sensors, touch, charger/PMIC, clk/regulator. *Accept:* `sys.boot_completed=1`.
- **Phase 5 — Peripherals & stability.** WiFi, RIL/telephony, audio routing, sensors, camera, BT,
  fingerprint, thermal, charging. *Accept:* functional parity with the stock-boot ROM; survives reboots + a day of use.
- **Phase 6 — Integrate & ship.** Make the from-source kernel the ROM's default boot; ship in a build.

## STATE FILES (the memory that survives months — keep them current)
- `KERNEL_MASTER_PLAN.md` — this. Roadmap + rules. Rarely changes.
- `KERNEL_STATE.md` — CURRENT phase, blocker, **NEXT ACTION**, last test result, pending TEST REQUEST. Updated constantly.
- `KERNEL_KB.md` — accumulated knowledge: driver ports done (with source repo + commit), the CONFIG_TRAN_*
  → sibling-source mapping, the reserved-memory map, and **dead-ends** (so they are never retried).
- `KERNEL_TESTLOG.md` — every flash-test: candidate file+md5, what was flashed, boot result, pstore/last_kmsg findings, conclusion.

## SOURCE STRATEGY (there is NO official source)
Assemble from: (a) the **stock kernel** — pull `/proc/config.gz` from the running stock-boot ROM (ask
orchestrator), extract the embedded DTB and the driver/symbol list from the stock boot image; (b) **sibling
MT6761/MT6765 sources** already located — Vivo Y81 (4.9.77), Nokia MT6771, Oppo A3 — port their LCM,
TRAN_* hooks, sensor/touch/charger drivers; (c) the generic **mt6761 4.19** tree as the base. Reverse-engineer
from the stock binary only what no sibling provides.

## TEST PROTOCOL (make each human-in-loop test count)
Every TEST REQUEST in `KERNEL_STATE.md` must state: candidate path+md5; exactly what to flash (boot only);
what to observe (display? adb? boot_completed?); and the EXACT recovery + log-capture steps (reboot to TWRP,
`cat /sys/fs/pstore/console-ramoops`, `cat /proc/last_kmsg`, pull both). Because Phase 2 gives us logs, most
tests advance us by exactly one identified blocker — which is what makes a months-long effort converge.
