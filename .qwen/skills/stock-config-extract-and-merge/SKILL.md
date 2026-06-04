---
name: stock-config-extract-and-merge
description: Extracting a running kernel's config via extract-ikconfig and merging it into a different kernel tree via olddefconfig — used when a source-built kernel boots but fails to reach sys.boot_completed due to config mismatches
source: auto-skill
extracted_at: '2026-06-03T05:08:00.000Z'
---

# Stock Config Extraction and Merge

## When to Use This

A source-built kernel compiles and boots — all drivers load, services start — but the Android
framework never reaches `sys.boot_completed`. A boot-failure watchdog reboots the device at ~116s.
The kernel is missing config options the Android framework or init requires.

The fix: extract the *running* stock kernel's `.config` and merge it into the source tree's
`.config`, then rebuild.

## Step 1: Extract Stock Config

Every Linux kernel built with `CONFIG_IKCONFIG=y` and `CONFIG_IKCONFIG_PROC=y` embeds its
`.config` in the kernel binary. Use the kernel's own extraction script:

```bash
scripts/extract-ikconfig /path/to/stock_kernel_zImage > /tmp/stock.config
```

Verify:
```bash
head -5 /tmp/stock.config
# Should show: "Automatically generated file; DO NOT EDIT."
#              "Linux/arm 4.19.127 Kernel Configuration"
wc -l /tmp/stock.config
# Should be several thousand lines
```

If `extract-ikconfig` returns empty output, the stock kernel was not built with embedded config.
In that case, try `/proc/config.gz` on a running device, or find the defconfig in the stock
kernel source if available.

## Step 2: Diff First (Understand What's Different)

Before merging, understand the gap between stock config and the current build config:

```bash
# Options ENABLED in stock that are NOT in our config
comm -23 <(grep -E "^(CONFIG_[A-Za-z0-9_]+)=[ym]" /tmp/stock.config | sort) \
         <(grep -E "^(CONFIG_[A-Za-z0-9_]+)=[ym]" out/.config | sort)
```

Filter out architecture noise (HAVE_*, GENERIC_*, CPU_HAS_*, etc.) to find the real differences.
Pay special attention to:
- Android-specific: `CONFIG_ANDROID_*`, `CONFIG_BINDER_*`, `CONFIG_ASHMEM`
- Cgroups: `CONFIG_CGROUP_*`, `CONFIG_CPUSETS`, `CONFIG_MEMCG`, `CONFIG_BLK_CGROUP`
- Filesystems: `CONFIG_FUSE_FS`, `CONFIG_OVERLAY_FS`, `CONFIG_SDCARD_FS`, `CONFIG_QUOTA`
- Security: `CONFIG_SECURITY_SELINUX`, `CONFIG_DM_VERITY`
- Device-specific string configs: `CONFIG_CUSTOM_KERNEL_LCM`, `CONFIG_CUSTOM_KERNEL_IMGSENSOR`

## Step 3: Merge via olddefconfig

```bash
# Save current config as backup
cp out/.config /tmp/our_old.config

# Replace with stock config
cp /tmp/stock.config out/.config

# Merge — fills in new options from our kernel version with defaults
make O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- \
     CROSS_COMPILE=arm-linux-androideabi- LD=ld.lld olddefconfig
```

**What olddefconfig does:**
- Keeps stock options whose Kconfig definitions exist in our tree
- Adds new options (present in 4.19.325 but not 4.19.127) with their default values
- **Silently drops** stock options whose Kconfig entries don't exist in our tree
- Preserves string values for existing Kconfig options

**Critical insight:** Many vendor-specific options (TranSSION `CONFIG_TRAN_*`, `CONFIG_CUSTOM_TRAN_*`,
`CONFIG_F2FS_TRAN_*`, etc.) will be silently dropped because their Kconfig definitions were
added by the vendor to their kernel tree and don't exist in a mainline/upstream tree.
These options cannot be enabled without the corresponding source code.

## Step 4: Fix Build-Breaking Options

Stock configs often enable drivers that cause build errors in a different kernel tree. Common fixes:

### a) ARMv8 Crypto Extensions (clang IAS incompatible)
```bash
sed -i 's/^CONFIG_CRYPTO_AES_ARM_CE=y$/# CONFIG_CRYPTO_AES_ARM_CE is not set/' out/.config
sed -i 's/^CONFIG_CRYPTO_SHA2_ARM_CE=y$/# CONFIG_CRYPTO_SHA2_ARM_CE is not set/' out/.config
sed -i 's/^CONFIG_CRYPTO_SHA256_ARM=y$/# CONFIG_CRYPTO_SHA256_ARM is not set/' out/.config
```

### b) USB Driver (struct mismatches in MTK trees)
```bash
# CONFIG_USB_MTK_HDRC often fails with struct mt_usb_glue member mismatches
sed -i 's/^CONFIG_USB_MTK_HDRC=y$/# CONFIG_USB_MTK_HDRC is not set/' out/.config
```

### c) TEEI/Microtrust version
```bash
# Stock may have version "300" but only "400" source exists
grep "CONFIG_MICROTRUST_TEE_VERSION" out/.config
sed -i 's/CONFIG_MICROTRUST_TEE_VERSION="300"/CONFIG_MICROTRUST_TEE_VERSION="400"/' out/.config
```

### d) Vendor-specific driver lists
```bash
# LCM and camera sensor lists reference drivers not in our tree
# Check what's available:
ls drivers/misc/mediatek/lcm/*/
ls drivers/misc/mediatek/imgsensor/src/common/v1/*/

# Trim to available entries or set empty:
sed -i 's|CONFIG_CUSTOM_KERNEL_LCM=".*"|CONFIG_CUSTOM_KERNEL_LCM=""|' out/.config
```

### e) Known 4.19 incompatibilities
```bash
sed -i 's/^CONFIG_TASKS_TRACE_RCU=y$/# CONFIG_TASKS_TRACE_RCU is not set/' out/.config
```

## Step 5: Build and Handle Incremental Errors

```bash
make -j$(nproc) O=out ARCH=arm CC=clang ...
```

Each build error is a new incompatibility between the stock config and our tree. Fix iteratively:
1. Identify the failing file/driver
2. Either fix the source (preferred) or disable the config option
3. Re-run `make`

Common error types and fixes:
- **Missing headers** (`fatal error: 'teei_client_main.h' file not found`): Wrong TEE version. Fix config string.
- **Missing source directories** (`No such file or directory: .../gc6133_serial_yuv/Makefile`): Trim `CUSTOM_KERNEL_IMGSENSOR`.
- **`error: unused variable`** (`-Werror,-Wunused-variable`): Add `__maybe_unused` to the declaration.
- **`error: no member named 'phy'`**: Struct changed between kernel versions. Disable driver.

## Step 6: Verify Critical Configs Survived

After build succeeds, verify the Android-critical options are present:
```bash
for opt in CONFIG_CGROUPS CONFIG_CGROUP_FREEZER CONFIG_CPUSETS CONFIG_MEMCG \
           CONFIG_BLK_CGROUP CONFIG_FAIR_GROUP_SCHED CONFIG_UCLAMP_TASK \
           CONFIG_ANDROID_BINDER_IPC CONFIG_ASHMEM CONFIG_FUSE_FS \
           CONFIG_OVERLAY_FS CONFIG_SDCARD_FS CONFIG_SECURITY_SELINUX \
           CONFIG_DM_VERITY CONFIG_PSI; do
  grep "^$opt=" out/.config || echo "MISSING: $opt"
done
```

## Understanding What Did NOT Carry Over

After `olddefconfig`, diff again to see what was lost:
```bash
comm -13 <(grep -E "^(CONFIG_[A-Za-z0-9_]+)=" out/.config | sort) \
         <(grep -E "^(CONFIG_[A-Za-z0-9_]+)=" /tmp/stock.config | sort)
```

Options lost fall into three categories:
1. **Kconfig not present** — vendor hooks, custom features. Can't enable without source code.
2. **Auto-detected features** — `CONFIG_HAVE_*`, `CONFIG_CC_HAS_*`, etc. Handled by Kbuild.
3. **Options we intentionally disabled** — crypto CE, USB_MTK_HDRC, etc.

### Detecting Fatal Vendor Gaps (Category 1)

When vendor options were dropped, check whether they actually exist in the source tree's Kconfig
files. Missing Kconfig = missing source code = the feature is completely absent:

```bash
# Programmatic check: which stock CONFIG_TRAN_* / CONFIG_MTK_* symbols
# actually have Kconfig definitions in this source tree?
for cfg in $(grep "^CONFIG_TRAN_\|^CONFIG_CUSTOM_TRAN\|^CONFIG_F2FS_TRAN\|^CONFIG_MTK_SENSORS\|^CONFIG_MTK_LENS\|^CONFIG_MTK_FLASH\|^CONFIG_MTK_MUSB_QMU\|^CONFIG_PROCESS_RECLAIM\|^CONFIG_SPECULATIVE_PAGE_FAULT" /tmp/stock.config | grep -v "^#\|is not set" | cut -d= -f1 | sort -u); do
  if grep -rq "^config $cfg\b" drivers/ arch/ fs/ mm/ net/ sound/ 2>/dev/null; then
    echo "EXISTS (enable this): $cfg"
  else
    echo "MISSING (vendor code absent): $cfg"
  fi
done
```

**Red flag:** If 50+ vendor symbols report `MISSING` — especially `CONFIG_TRAN_*` hooks and
device LCM/sensor drivers — the source tree is a **generic SoC kernel**, not a device-specific
one. Config changes alone will never produce a fully booting kernel. You need the device's
actual kernel source release from the OEM.

### Boot-Stall Diagnosis Pattern

When a source-built kernel boots but never reaches `sys.boot_completed`:

| Symptom | Likely Cause |
|---------|-------------|
| All HW drivers load, adbd/surfaceflinger/zygote start | Kernel+DTB are correct |
| No display output | Missing LCM panel driver |
| `sys.boot_completed` never set, ~116s watchdog reboot | Framework can't finish init sequence |
| Config diff shows no Android-critical gaps | NOT a config problem |

**Conclusion when all four symptoms are present:** The source tree is missing vendor device
drivers (LCM panel, sensors, TRAN_ hooks). Config merging won't fix this — you need the
device-specific kernel source or must use the stock kernel.

## Key Insight

**If the stock and source configs are nearly identical after merge** (which is often the case
when building from a newer mainline/LTS tree — the stock was 4.19.127, the source is 4.19.325,
and all the Android-critical options were already set), then the boot issue is NOT a config
problem. It's a vendor driver/source compatibility problem. The vendor hooks that were dropped
by olddefconfig need their source code ported from the stock kernel tree.
