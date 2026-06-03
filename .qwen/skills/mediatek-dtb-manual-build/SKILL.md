---
name: mediatek-dtb-manual-build
description: How to compile MediaTek (MT67xx) device tree blobs outside the Android build system — make dtbs fails, use gcc -E + dtc with kernel include paths
source: auto-skill
extracted_at: '2026-06-03T04:52:07.738Z'
---

# MediaTek DTB Manual Build (Outside Android Build System)

## Problem

Running `make dtbs` or `make zImage dtbs` on a MediaTek (MT67xx) kernel tree produces **zero `.dtb` files** in `out/arch/arm/boot/dts/`. The directory is empty after a successful build.

**Why:** MediaTek kernels build DTBs via the **Android platform build system** (`device/mediatek/build/core/build_dtbimage.mk`), not via the upstream kernel's `arch/arm/boot/dts/Makefile`. The standard Makefile has no `dtb-$(CONFIG_MACH_MT6761)` entry — it only covers upstream (non-MTK) platforms.

Even though the DTS source files exist (`mt6761.dts`, `k61v1_32_bsp_1g.dts`, etc.) and `CONFIG_MACH_MT6761=y` is set, the upstream dtb build rules never fire.

## Detection

Before assuming dtbs built, check:
```bash
ls out/arch/arm/boot/dts/
# Empty? The DTB system is MTK-style, not upstream.
```

If `arch/arm/boot/dts/Makefile` has no `mt67xx` entries and `Android.mk` references `device/mediatek/build/core/build_dtbimage.mk`, this is a MediaTek kernel.

## Solution: Manual DTB Compilation

MediaTek base DTBs (like `mt6761.dts`) are standalone `/dts-v1/;` files (not DTBO overlays like the `k61v1_*.dts` which begin with `/plugin/;`). Compile the base DTS manually with the kernel's preprocessor + `dtc`.

### Step 1: Preprocess with `gcc -E`

The DTS includes `<generated/autoconf.h>` and kernel headers. Use the kernel build output's `include/` directories:

```bash
gcc -E -nostdinc -x assembler-with-cpp \
  -I out/include \
  -I arch/arm/boot/dts \
  -I arch/arm/boot/dts/include \
  -I include \
  -I arch/arm/include \
  -I . \
  -undef -D__DTS__ \
  -o /tmp/mt6761.dts.tmp \
  arch/arm/boot/dts/mt6761.dts
```

**Include paths explained:**
- `out/include` — for `generated/autoconf.h` and generated config headers
- `arch/arm/boot/dts` — for sibling `.dtsi` includes
- `arch/arm/boot/dts/include` — for dt-bindings headers shipped with the DTS
- `include` and `arch/arm/include` — for kernel-wide headers referenced from dt-bindings
- `.` (repo root) — for any absolute-path includes

### Step 2: Compile with `dtc`

```bash
dtc -I dts -O dtb -o out/arch/arm/boot/dts/mt6761.dtb /tmp/mt6761.dts.tmp
```

DTB warnings from `dtc` (unit_address_vs_reg, unique_unit_address, etc.) are expected — MediaTek DTS files have many duplicate unit addresses and missing reg properties. These do not prevent booting.

### Step 3: Verify

```bash
xxd out/arch/arm/boot/dts/mt6761.dtb | head -1
# Should show: 00000000: d00d feed ...
# FDT magic 0xd00dfeed confirms valid device tree blob
```

## Project DTBOs (Overlays)

Files like `k61v1_32_bsp_1g.dts` that begin with `/plugin/;` are DTBO overlays, not standalone DTBs. They modify nodes from the base `mt6761.dts`. The Android build system applies these at boot time. For manual builds:
- The base `mt6761.dtb` alone may be sufficient for initial boot testing
- Full project support requires applying the overlay with `fdtoverlay`, but the bootloader expects separate DTBO partitions

## Stock DTB Location

On MediaTek devices, the DTB typically lives in a **separate partition** (`/dev/block/platform/bootdevice/by-name/dtb`), not inside the boot image. The boot image header v2's `dtb_size` field will be **0** — the bootloader loads the DTB from the dtb partition, not from the boot image.

This is why building a boot image with `--dtb` (Candidate A) or appending the DTB to the kernel (Candidate B) are both valid approaches — they bypass the separate dtb partition entirely.

## Config Notes

Ensure these are set in `.config` (they typically are in MTK defconfigs):
```
CONFIG_MACH_MT6761=y
CONFIG_MTK_PLATFORM="mt6761"
CONFIG_ARCH_MTK_PROJECT="x657b_h6117"
```
