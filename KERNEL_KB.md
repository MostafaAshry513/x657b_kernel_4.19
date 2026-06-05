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
- **NT36572 (Novatek NT36xxx)** — ALREADY PRESENT in our tree
  - Source: `drivers/input/touchscreen/nt36572_common/` (nt36xxx.c, fw_update, ext_proc, mp_ctrlram)
  - Config: CONFIG_TOUCHSCREEN_MTK_NT36572_COMMON=y (matches stock)
  - Status: BUILT IN, object files confirmed in out/

### Sensors (sensors-1.0)
- **Accelerometer (BMI160)** — present in our tree (`drivers/misc/mediatek/sensors-1.0/accelerometer/`)
  - Config: CONFIG_CUSTOM_KERNEL_ACCELEROMETER=y (matches stock)
- **ALS/Proximity (CM36558)** — present in our tree (`drivers/misc/mediatek/sensors-1.0/alsps/`)
  - Config: CONFIG_CUSTOM_KERNEL_ALSPS=y (matches stock)
- Additional Vivo Y81 sensor sources available but not needed (sensors-1.0 already matches stock config)

### Charger / PMIC
- **MTK Charger** — present (CONFIG_MTK_CHARGER=y, matches stock)
- **MT6357 PMIC** — present in tree (`drivers/misc/mediatek/pmic/mt6357/`)
- Not needed: RT9465, MTK_PUMP_EXPRESS — not in stock config

### USB
- **USB_MTK_HDRC** — Kconfig EXISTS (`drivers/misc/mediatek/usb20/Kconfig`) but BUILD BROKEN
  - Error: `struct mt_usb_glue has no member named 'phy'` in `mt6765/usb20.c`
  - BLOCKED: needs struct definition fix or driver version port
- **MTK_MUSB_QMU_SUPPORT** — depends on USB_MTK_HDRC → blocked
- **MTK_MUSB_QMU_PURE_ZLP_SUPPORT** — depends on MTK_MUSB_QMU → blocked
- **DUAL_ROLE_USB_INTF** — Kconfig NOT in tree (exists in Vivo Y81: `drivers/usb/phy/Kconfig`)
  - But depends on USB_PHY which depends on USB_MTK_HDRC → blocked
- Status: ALL USB configs blocked behind USB_MTK_HDRC build failure

## CONFIG_TRAN_* Complete Map (39 options — all unportable)

### Category: Binder Monitoring (3 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_ANDROID_BINDER_BLOCK_MONITOR` | Monitors binder transactions for blocking calls | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_ANDROID_BINDER_BUFFER_EXHAUST_MONITOR` | Detects binder buffer exhaustion | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_ANDROID_BINDER_DEFER_WORK_BLOCK` | Defers work when binder blocks | NONE — proprietary | ❌ UNPORTABLE |

### Category: Display/Backlight (2 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_BACKLIGHT_OPT` | Optimizes backlight brightness curve for Transsion panels | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_BORDER_SUPPRESSION` | Border/notch area suppression for waterdrop display | NONE — proprietary | ❌ UNPORTABLE |

### Category: Battery/Charger (6 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_BATTERY_AGING` | Battery aging compensation algorithm | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CHARGER_AGING_SUPPORT` | Charger aging curve support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CHARGER_FLASHLIGHT_LED_SUPPORT` | Flashlight LED driver via charger IC | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CHARGER_FUNCS_ADD` | Additional charger functions | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_DUAL_LINEAR_CHARGER` | Dual linear charger support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_DUAL_LINEAR_2A_CHARGER` | 2A dual linear charger | NONE — proprietary | ❌ UNPORTABLE |

### Category: Camera (3 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_CAMERA_FAKE_STEREO_MAIN3` | Fake stereo camera using main3 sensor | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CAMERA_FAKE_STEREO_SAME_CLK_WITH_MAIN` | Shared clock for fake stereo main | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CAMERA_FAKE_STEREO_SAME_CLK_WITH_MAIN_SEC` | Shared clock for fake stereo main secondary | NONE — proprietary | ❌ UNPORTABLE |

### Category: Fingerprint (3 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_FINGERPRINT` | Fingerprint HAL driver routing | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_FP_DRV_MODE_SPI` | Fingerprint SPI driver mode | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_FP_VENDOR="chipone sunwave"` | Fingerprint vendor list | NONE — proprietary | ❌ UNPORTABLE |

### Category: Memory/Freeze (3 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_FREEZE` | Process freezer for power saving | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CUSTOM_LPR` | Custom low-power RAM mode | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_RM_SOLUTION` | RAM management solution | NONE — proprietary | ❌ UNPORTABLE |

### Category: IO/Storage (5 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_IOMANAGER_SUPPORT` | IO manager for storage scheduling | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_IO_LIMIT_SUPPORT` | IO bandwidth limiting | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_EXE_CQ_RT` | Execute CQ (Command Queue) in real-time | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_SPDK_SUPPORT` | SPDK (Storage Performance Dev Kit) support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_F2FS_DEFRAGMENT` | F2FS defragmentation support | NONE — proprietary | ❌ UNPORTABLE |

### Category: Connectivity/Peripheral (4 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_SD_TRAY_SUPPORTED` | SD card tray detection | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_MMC_DEBUG` | MMC/SD debug logging | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_OPEN_HEADSET_MIC_TEST` | Headset microphone test mode | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_LED_ISINK_REVERT` | LED ISINK current control revert | NONE — proprietary | ❌ UNPORTABLE |

### Category: System/Platform (10 options)
| Config | Function | Source | Status |
|--------|----------|--------|--------|
| `TRAN_AFTER_SALE_TOOL` | After-sales diagnostic tool support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_PERF_LOG` | Performance logging framework | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_TRANLOG` | Transsion system logger | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_PSI_DEBUG` | Pressure Stall Information debugging | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_PNP_SCREEN_CTL` | Plug-and-play screen control | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_DOUBLE_KEY_TRIGGER` | Double key press trigger (power+vol) | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_EXTENSION_WHITELIST_SUPPORT` | Extension whitelist | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_TKV_SUPPORTED` | TKV security framework support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_TNEK_SUPPORT` | TNEK kernel extension support | NONE — proprietary | ❌ UNPORTABLE |
| `TRAN_CHIP_PLAT="MTK"` | Chip platform identifier | NONE — proprietary | ❌ UNPORTABLE |

## RAM stock CONFIG_TRAN_* summary
- **39 total TRAN_* configs in stock kernel** (verified via `zcat stock_config.gz`)
- **0 exist as Kconfig or C source in any public repo** (verified: Vivo Y81, Nokia MT6771, Oppo A3, GitHub code search)
- **0 can be ported** — all are proprietary Transsion/Infinix additions

## Missing Non-TRAN Device Configs (from stock, not in our tree)

| Config | Stock | Our Kconfig | Vivo Y81 Kconfig | Status |
|--------|-------|-------------|------------------|--------|
| `USB_MTK_HDRC` | y | EXISTS | EXISTS | ❌ BUILD BROKEN (struct miss) |
| `MTK_MUSB_QMU_SUPPORT` | y | EXISTS | EXISTS | ❌ BLOCKED (dep USB_MTK_HDRC) |
| `MTK_MUSB_QMU_PURE_ZLP_SUPPORT` | y | EXISTS | EXISTS | ❌ BLOCKED (dep QMU) |
| `DUAL_ROLE_USB_INTF` | y | MISSING | EXISTS (drivers/usb/phy/) | ❌ BLOCKED (dep USB_PHY/MTK_HDRC) |
| `MTK_SENSORS_1_0` | y | MISSING | MISSING | ⚠️ NOT NEEDED (sensors work without) |
| `MTK_MEM` | y | MISSING | EXISTS (misc/mediatek/Kconfig) | ⚠️ LOW PRIORITY |
| `MTK_FLASHLIGHT_OCP81373` | y | MISSING | MISSING | ❌ UNPORTABLE (no Kconfig) |
| `MTK_LENS_GT9772AF_SUPPORT` | y | MISSING | MISSING | ❌ UNPORTABLE (no Kconfig) |
| `MTK_JEITA_STANDARD_SUPPORT` | y | MISSING | MISSING | ❌ UNPORTABLE (no Kconfig) |
| `MTK_GET_BATTERY_ID_BY_AUXADC` | y | MISSING | MISSING | ❌ UNPORTABLE (no Kconfig) |
| `MTK_CAM_MAX_NUMBER_OF_CAMERA=4` | 4 | MISSING | MISSING | ⚠️ LOW PRIORITY (Kconfig addition) |

## Ramoops/Pstore
- Ramoops DTS node added: `ramoops@47E80000` (1MB, 128KB per sub-buffer)
- Address: 0x47E80000 (standard MT6761 ram_console address)
- Config: PSTORE_RAM=y, PSTORE_CONSOLE=y (already set)

## DEAD-ENDS (never retry)
- **CONFIG_TRAN_* hooks from source** — no public source exists; all 39 are proprietary.
- **stock DTB from boot partition dump** — has identical long debug bootargs, won't fix cmdline overflow.
- **Header v2 without DTB with mkbootimg.py** — tool enforces dtb_size > 0 for v2.
- **CONFIG_USB_MTK_HDRC=y build** — struct mt_usb_glue missing .phy member in 4.19 tree.
- **CONFIG_MTK_SENSORS_1_0** — Kconfig doesn't exist in any public tree.
- **CONFIG_MTK_FLASHLIGHT_OCP81373** — Kconfig doesn't exist in any public tree.
- **CONFIG_MTK_LENS_GT9772AF_SUPPORT** — Kconfig doesn't exist in any public tree.
- **CONFIG_MTK_JEITA_STANDARD_SUPPORT** — Kconfig doesn't exist in any public tree.
