---
name: android-device-kernel-source-hunt
description: Systematic protocol for finding a device-specific Android kernel source tree (LCM drivers, vendor hooks) when a generic SoC kernel partially boots but stalls — GitHub orgs, code search, distinguishing prebuilt dumps from real source
source: auto-skill
extracted_at: '2026-06-03T05:30:00.000Z'
---

# Android Device-Specific Kernel Source Hunt

## When to Use

A generic SoC kernel (e.g., mt6761 from `bengris32/`) compiles and partially boots, but the
Android framework never reaches `sys.boot_completed`. Config analysis shows 50+ vendor symbols
(`CONFIG_TRAN_*`, `CONFIG_MTK_SENSORS_*`, etc.) exist in the stock kernel config but have
**no Kconfig entries** in the source tree. The source tree is a generic kernel — you need the
device manufacturer's actual kernel release.

## Step 1: Official OEM Open Source Portals

Check these in order:

| Manufacturer | URL / Org |
|-------------|-----------|
| Transsion (Infinix/Tecno/Itel) | `https://github.com/Transsion-Community-OpenSource` |
| Transsion (Infinix) | `https://github.com/OpenSource-Infinix` (54+ repos) |
| Infinix Devices Series | `https://github.com/Infinix-Devices-Series` (24+ repos) |
| Xiaomi (MediaTek) | `https://github.com/MiCode` |
| Realme/OPPO | Their opensource portals |
| Samsung | `opensource.samsung.com` |

**Important:** Many OEM GitHub orgs list repos by **device codename**, not model number. Infinix
X657B might be under a codename. Check the `BoardConfig.mk` or build.prop for the codename.

## Step 2: GitHub Repository Search

### By naming convention
```
gh search repos "android_kernel_infinix" --limit 50
gh search repos "android_kernel_transsion" --limit 50
gh search repos "android_device_infinix X657" --limit 20
```

### By vendor-specific symbols
If the stock config has unique vendor symbols (TRAN_ hooks for Transsion devices), search for
those symbols in actual source code (not just config dumps):

```bash
gh search code "CONFIG_TRAN_BACKLIGHT_OPT"      # Transsion backlight hook
gh search code "CONFIG_TRAN_FREEZE"              # Transsion freeze management
gh search code "CONFIG_MTK_MUSB_QMU_SUPPORT"     # MediaTek USB QMU
```

**Critical filter:** When searching code, differentiate between:
- **Real source:** `.c` files, `Makefile`, `Kconfig` — contains actual driver code
- **Config dumps:** `ikconfig` files, `bootRE/ikconfig` — just extracted kernel configs, not source

If results are all `ikconfig` files, the vendor code was never publicly released.

## Step 3: Check Prebuilt Repos (Usually Dead Ends)

`android_device_infinix_*` repos are **prebuilt kernel images + headers**, not full source.
Verify with:

```bash
gh api repos/<org>/<repo>/contents --jq '.[].name'
# If output is: Image.gz, dtb, kernel-headers, modules → PREBUILT (not source)
# If output has: Makefile, arch/, drivers/, fs/ → FULL SOURCE
```

Prebuilt repos can still be useful:
- Extract the stock `Image.gz` for config extraction (`extract-ikconfig`)
- Get the exact DTB for the device
- Get the stock kernel modules

## Step 4: Community Repos and Forks

Search for device model + kernel:
- `github.com/search?q="X657B"+kernel&type=repositories`
- `github.com/search?q="mt6761"+kernel+"infinix"&type=repositories`
- XDA Developers forums for the device

Third-party TWRP device trees (like `twrp_device_infinix_X657B`) may have:
- A `prebuilt/` folder with the stock kernel (extract config with `extract-ikconfig`)
- The exact device codename and defconfig name

## Step 5: Recognize a Dead End

The search is a **dead end** when ALL of these are true:

1. **No OEM org has a repo for this device** — checked Transsion-Community-OpenSource, OpenSource-Infinix, Infinix-Devices-Series
2. **Vendor symbols (TRAN_*) appear only in `ikconfig` dumps** — no `.c` or `Kconfig` files in any public repo
3. **All `android_device_*` repos are prebuilt-only** — `Image.gz`, `dtb`, `kernel-headers`, no source
4. **`gh search repos` for `android_kernel_<vendor>`** returns only unrelated devices or your own fork
5. **Web search for device-specific LCM driver name** (e.g., `nt36525b_hdp_dsi_vdo_txd_mdt_x657b`) returns zero results

When a dead end is reached:
- **Verdict:** "From-source kernel NOT FEASIBLE — vendor drivers never publicly released"
- **Fallback:** Use stock kernel from device, or file a GPL request with the OEM
- **Document:** Save the stock config + prebuilt kernel for future reference

## Step 6: If Found — Clone and Verify

```bash
git clone <url> /root/android/kernel_device
cd /root/android/kernel_device

# Quick verification: check for vendor-specific code
grep -r "CONFIG_TRAN_" arch/arm/configs/ drivers/ | head -5
ls drivers/misc/mediatek/lcm/ | grep -i "nt36525b\|hdp_dsi_vdo"
```

If vendor symbols and device drivers are present → build from this tree with the same clang
toolchain and mkbootimg flow.

## Recognized Patterns

### Transsion/Infinix/Tecno
- Vendor hooks: `CONFIG_TRAN_*` (75+ options)
- LCM drivers: `nt36525b_hdp_dsi_vdo_*_x657b`, `nt36672d_hdp_dsi_vdo_*_x657b`
- Sensor configs: `CONFIG_MTK_SENSORS_1_0`, `CONFIG_MTK_LENS_GT9772AF_SUPPORT`
- Custom strings: `CONFIG_CUSTOM_TRAN_PROJECT`, `CONFIG_CUSTOM_TRAN_BOM`, `CONFIG_CUSTOM_TRAN_PCBA`
- F2FS extensions: `CONFIG_F2FS_TRAN_GC`, `CONFIG_F2FS_TRAN_RHI`, `CONFIG_F2FS_FS_EP01`

### MediaTek QMU USB
- `CONFIG_MTK_MUSB_QMU_SUPPORT` — exists in Kconfig but may need `CONFIG_USB_MTK_HDRC` which has struct compatibility issues across kernel versions

### Key Repo Naming
- `android_kernel_<vendor>_<model>` → full kernel source (what you want)
- `android_device_<vendor>_<model>-kernel` → prebuilt images + modules (NOT source)
- `<vendor>-graveyard/android_device_*` → prebuilt dumps from dead devices
