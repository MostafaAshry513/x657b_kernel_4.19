# KERNEL KNOWLEDGE BASE (append-only — driver ports, source refs, mappings, DEAD-ENDS)

## Confirmed facts
- Stock boot 57e6: header v2 but dtb_size=0 → LK loads DTB from the dtb partition, not the boot image.
- cmdline overflow on candidate C = from-source DTB /chosen/bootargs too long (debug args + literal TABs).
- nt36525b LCM panel driver ported from Vivo Y81 (DavidWithTuxedo/android_kernel_vivo_y81, 4.9.77).
- CONFIG_PSTORE_RAM + CONFIG_PSTORE_CONSOLE already y in both stock and our config — just needed DTS node.
- MT6761 standard ramoops address: 0x47E80000, size 0x100000 (1MB).
- LK header v2 without --dtb requires DTB in mkbootimg.py → header v1 avoids this.
- Stock RAM: 512MB declared at 0x40000000 in DTS, alloc-ranges extend to 0xA0000000.

## Driver port status (from sibling sources)

### LCM Panel
- **nt36525b_hdp_dsi_vdo_txd_mdt_x657b** — PORTED from Vivo Y81 (4.9.77)
  - Source: `DavidWithTuxedo/android_kernel_vivo_y81` → `drivers/misc/mediatek/lcm/nt36525_hdp_dsi_vdo_txd_lm3697_622/`
  - Status: BUILDS, UNTESTED on device
  - Adaptations: lcm_reset_setting→SET_RESET_PIN, removed Vivo globals, removed 4.9-only LCM_DRIVER fields

### Touchscreen (TPD)
- **Focaltech FTxxxx** — Vivo Y81 has `drivers/input/touchscreen/mediatek/focaltech_touch/` (focaltech_core + flash + gesture)
- **Novatek NT36xxx** — Vivo Y81 has `drivers/input/touchscreen/mediatek/NT36xxx/`
- **Goodix GT1151** — Vivo Y81 has `drivers/input/touchscreen/mediatek/GT1151/`
- **Novatek NT36525 touch** — Vivo Y81 has `drivers/input/touchscreen/nt36525_1732/` (panel-specific touch driver)
- Status: NOT PORTED yet

### Sensors (sensors-1.0)
- **BMI160** (accel+gyro) — Vivo Y81: `drivers/misc/mediatek/sensors-1.0/accelerometer/bmi160-i2c/`
- **CM36558** (ALS/proximity) — Vivo Y81: `drivers/misc/mediatek/sensors-1.0/alsps/cm36558/`
- **AKM09911/15/18** (magnetometer) — Vivo Y81: `drivers/misc/mediatek/sensors-1.0/magnetometer/akm09911/`
- **BMP280** (barometer) — Vivo Y81: `drivers/misc/mediatek/sensors-1.0/barometer/BMP280-new/`
- Status: NOT PORTED yet

### Camera Sensors (imgsensor)
- **s5k3l6xx** — Vivo Y81: `drivers/misc/mediatek/imgsensor/src/common/v1/s5k3l6xx/`
- **imx258** — Vivo Y81: `drivers/misc/mediatek/imgsensor/src/common/v1/imx258_mipi_raw/`
- Status: NOT PORTED yet

### Charger / PMIC
- **RT9465** switch charger — Vivo Y81: `drivers/power/supply/mediatek/charger/rt9465.c`
- **MT6357** PMIC — Vivo Y81: `drivers/misc/mediatek/pmic/mt6357/v1/`
- Status: NOT PORTED yet

### USB QMU
- **mtk_qmu + musb_qmu** — Vivo Y81: `drivers/misc/mediatek/usb20/mtk_qmu.c`, `musb_qmu.c`
- **MTU3 QMU** — Vivo Y81: `drivers/usb/mtu3/mtu3_qmu.c`
- Status: NOT PORTED yet. USB_MTK_HDRC fails to build due to struct mismatch.

## CONFIG_TRAN_* mapping (75 options in stock config)
- NONE of the TRAN_* options exist as Kconfig symbols in ANY public sibling source (Vivo Y81, Nokia MT6771, Oppo A3).
- They ONLY appear in ikconfig dumps (kernel config extracts) — never in actual Kconfig or C source.
- TRAN_* hooks are proprietary Transsion/Infinix additions not released under GPL.
- Known TRAN_* categories from stock config:
  - Binder monitoring: `TRAN_ANDROID_BINDER_BLOCK_MONITOR`, `TRAN_ANDROID_BINDER_BUFFER_EXHAUST_MONITOR`
  - Display: `TRAN_BACKLIGHT_OPT`, `TRAN_GAMMA_CORRECTION`
  - Battery: `TRAN_CHARGER_JEITA`, `TRAN_BATTERY_AGING`
  - Camera: `TRAN_CAMERA_FLASHLIGHT`, `TRAN_CAMERA_AF`
  - Fingerprint: `TRAN_FINGERPRINT_NAVIGATION`
  - Memory: `TRAN_FREEZE`, `TRAN_PROCESS_RECLAIM`
  - Storage: `TRAN_SD_TRAY_SUPPORTED`, `TRAN_EXFAT_SUPPORT`
  - Audio: `TRAN_AUDIO_SMARTPA`
  - Network: `TRAN_NETWORK_BOOST`
- Status: CANNOT BE PORTED — no source exists in any public repo.

## Ramoops/Pstore
- Ramoops DTS node added: `ramoops@47E80000` (1MB, 128KB per sub-buffer)
- Address: 0x47E80000 (standard MT6761 ram_console address)
- Config: PSTORE_RAM=y, PSTORE_CONSOLE=y (already set)

## DEAD-ENDS (never retry)
- **CONFIG_TRAN_* hooks from source** — no public source exists; these are proprietary.
- **stock DTB from boot partition dump** — has identical long debug bootargs, won't fix cmdline overflow.
- **Header v2 without DTB with mkbootimg.py** — tool enforces dtb_size > 0 for v2.
- **CONFIG_USB_MTK_HDRC=y build** — struct mt_usb_glue missing .phy member in 4.19 tree.
