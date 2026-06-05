---
name: mtk-pstore-ramoops-setup
description: Configuring pstore/ramoops on MediaTek MT6761 kernel builds to capture boot logs from failed boots — adding a ramoops reserved-memory DTS node at the MTK standard address and verifying PSTORE_RAM config
source: auto-skill
extracted_at: '2026-06-05T04:15:00.000Z'
---

# MTK Pstore/Ramoops Setup for Boot Diagnostics

## When to Use This

A from-source kernel builds and boots on a MediaTek (MT6761/MT6765/MT6771) device but fails
before reaching `sys.boot_completed`. The failure is invisible — no adb, no UART, no display
output. You need kernel boot logs that survive across reboots so the next flash-test yields
actionable diagnostic output instead of a blind "it stalled."

## Prerequisites

Verify these configs are already set (they typically are in MTK defconfigs):

```bash
grep -E "CONFIG_PSTORE_RAM|CONFIG_PSTORE_CONSOLE|CONFIG_PSTORE|CONFIG_PSTORE_DEFLATE" out/.config
# Should show:
# CONFIG_PSTORE=y
# CONFIG_PSTORE_CONSOLE=y
# CONFIG_PSTORE_RAM=y
# CONFIG_PSTORE_DEFLATE_COMPRESS=y
```

If any are missing, enable them:
```bash
make O=out ARCH=arm CC=clang ... menuconfig
# File systems → Pstore → enable PSTORE, PSTORE_CONSOLE, PSTORE_RAM, DEFLATE_COMPRESS
```

MTK devices also have `CONFIG_MTK_RAM_CONSOLE` (MediaTek's own ram console driver), but
`CONFIG_PSTORE_RAM` (the upstream Linux driver) can work independently with a proper DTS node.

## Step 1: Determine the Ramoops Memory Address

The key is using the **same reserved memory address** that LK (the MTK bootloader) already
carves out for the ram console. On MT6761 this is standardized:

```
Address: 0x47E80000
Size:    0x00100000 (1 MB)
```

This address is defined in MTK's LK source at `platform/mt6761/include/platform/ram_console.h`:
```c
#define RAM_CONSOLE_START_ADDR   0x47E80000
#define RAM_CONSOLE_LENGTH       0x100000
```

**Why this address:** It sits in the first 512MB DRAM region (0x40000000–0x60000000), below
the ATAGS/tags_addr (0x47880000) but above the kernel load address. LK already marks this
region as reserved before jumping to the kernel, so adding a DTS `no-map` node here is safe
and matches what LK expects.

**Checking your device's actual address:** If you have the stock DTB:
```bash
dtc -I dtb -O dts stock.dtb | grep -B2 -A10 "ramoops\|pstore\|ram_console\|ram-console"
```

If nothing appears (common — MTK devices may pass ramoops via LK runtime parameters, not DTS),
use the standard MT6761 address: `0x47E80000`.

## Step 2: Add Ramoops Node to DTS

Edit `arch/arm/boot/dts/mt6761.dts` (or your platform DTS). Add inside the `reserved-memory` node:

```dts
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;

    ramoops_mem: ramoops@47E80000 {
        compatible = "ramoops";
        reg = <0 0x47E80000 0 0x00100000>;
        record-size = <0x00020000>;   /* 128 KB per panic record */
        console-size = <0x00020000>;  /* 128 KB console log */
        ftrace-size = <0x00020000>;   /* 128 KB ftrace */
        pmsg-size = <0x00020000>;     /* 128 KB userspace messages */
        no-map;
    };

    /* ... other reserved-memory nodes follow ... */
};
```

**Buffer sizing:** With 1 MB total, 128 KB per sub-buffer uses 4×128KB = 512 KB, leaving
headroom. Adjust based on available RAM. For a 2GB device with 512MB declared in DTS,
1 MB is negligible.

## Step 3: Rebuild the DTB

Since `make dtbs` doesn't work on MTK kernels, rebuild manually:

```bash
gcc -E -nostdinc -x assembler-with-cpp \
  -I out/include -I arch/arm/boot/dts -I arch/arm/boot/dts/include \
  -I include -I arch/arm/include -I . -undef -D__DTS__ \
  -o /tmp/mt6761.dts.tmp arch/arm/boot/dts/mt6761.dts

dtc -I dts -O dtb -o out/arch/arm/boot/dts/mt6761_ramoops.dtb /tmp/mt6761.dts.tmp
```

Verify the node is present:
```bash
dtc -I dtb -O dts out/arch/arm/boot/dts/mt6761_ramoops.dtb | grep -B1 -A10 'ramoops'
```

## Step 4: Build and Pack

Rebuild zImage (no config change needed if PSTORE_RAM was already set):

```bash
make O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- \
     CROSS_COMPILE=arm-linux-androideabi- LD=ld.lld zImage
```

Pack the new DTB with the boot image (approach A — separate DTB):

```bash
python3 mkbootimg.py \
  --header_version 2 --pagesize 2048 --base 0x40000000 \
  --kernel_offset 0x00008000 --ramdisk_offset 0x11b00000 \
  --tags_offset 0x07880000 --dtb_offset 0x07880000 \
  --kernel out/arch/arm/boot/zImage \
  --ramdisk /tmp/bu/ramdisk \
  --dtb out/arch/arm/boot/dts/mt6761_ramoops.dtb \
  -o boot_candidates/boot_ramoops.img
```

## Step 5: Read Logs After Boot (Pass or Fail)

After flashing the candidate, whether it boots or fails:

1. **Boot to TWRP or any custom recovery with pstore support**
2. **Check pstore:**
```bash
# From TWRP or adb shell:
cat /sys/fs/pstore/console-ramoops       # THE critical one — full kernel log
cat /sys/fs/pstore/dmesg-ramoops-0        # Panic/oops messages
cat /proc/last_kmsg                       # Last kernel message buffer (legacy MTK)
```
3. **If /sys/fs/pstore is empty:** The ramoops address may be wrong, or the kernel never
   reached pstore initialization before crashing.

## Troubleshooting

### Empty /sys/fs/pstore
- **Wrong address:** LK may use a different address. Try `grep -ri "0x47E\|0x444\|0x5FE\|0xBF0"` on the stock kernel config.
- **PSTORE not built-in:** Check `zcat /proc/config.gz | grep PSTORE` on the stock ROM. If PSTORE_RAM=m, it must be loaded from vendor modules.
- **Overwritten by init:** Some init.rc scripts clear pstore on boot. Check the ramdisk init.

### Kernel panics before pstore init
- `CONFIG_PSTORE_RAM` must be **built-in** (y), not a module (m)
- If the crash is very early (before `fs/pstore` initcall), pstore never gets set up
- In this case, `CONFIG_MTK_RAM_CONSOLE=y` + LK's ram_console may be more reliable (uses a pre-initialized buffer)

### Alternative: Use MTK RAM Console Instead
If pstore proves unreliable, MediaTek's own ram console driver (`CONFIG_MTK_RAM_CONSOLE=y`)
initializes earlier (in the LK-to-kernel handoff) and doesn't require a DTS node:

```bash
# Enable in .config
sed -i 's/# CONFIG_MTK_RAM_CONSOLE is not set/CONFIG_MTK_RAM_CONSOLE=y/' out/.config
make olddefconfig && make zImage
```

This uses the same 0x47E80000 buffer that LK sets up, read via `/proc/last_kmsg`.

## Key Insight

Adding pstore/ramoops is a **one-time investment** that pays off in every subsequent flash-test.
Without it, each failed boot is a black box. With it, each test yields a kernel log pointing to
the exact stall/crash. This is the highest-leverage step in any kernel bringup campaign.
