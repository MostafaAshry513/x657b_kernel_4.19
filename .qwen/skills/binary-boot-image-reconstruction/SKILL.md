---
name: binary-boot-image-reconstruction
description: Manually reconstructing an Android boot image byte-by-byte with Python struct when mkbootimg.py has constraints (e.g., header v2 requires non-empty DTB but you need dtb_size=0) — also covers systematic cmdline overflow debugging for MTK LK
source: auto-skill
extracted_at: '2026-06-05T07:00:00.000Z'
---

# Binary Boot Image Reconstruction & LK Cmdline Overflow Debugging

## When to Use This

You need to produce a boot image that is byte-identical to a known-good stock boot image
except for the kernel (zImage), but `mkbootimg.py` prevents it:
- Header v2 **requires** a non-empty DTB (`--dtb` flag). You need `dtb_size=0`.
- You need to preserve stock header fields exactly (cmdline, os_version, name, dtbo_size).
- You're debugging an LK (Little Kernel) "cmdline overflow" where the DTB bootargs
  theory has been disproven by testing a candidate with no DTB.

The binary approach gives you full control over every header byte.

## Step 1: Read the Stock Header

```python
import struct

with open('stock_boot.img', 'rb') as f:
    stock_hdr = f.read(4096)  # header v2 = 4096 bytes

ks = struct.unpack_from('<I', stock_hdr, 8)[0]   # kernel_size
ra = struct.unpack_from('<I', stock_hdr, 16)[0]  # ramdisk_size
ps = struct.unpack_from('<I', stock_hdr, 36)[0]  # page_size
hv = struct.unpack_from('<I', stock_hdr, 40)[0]  # header_version
cmdline = stock_hdr[64:1600].split(b'\x00')[0]   # exact cmdline
dtb_size = struct.unpack_from('<I', stock_hdr, 1640)[0]
dtbo_size = struct.unpack_from('<I', stock_hdr, 1644)[0]
```

## Step 2: Build the Minimal-Diff Image

Start with the stock header as a bytearray, modify ONLY `kernel_size`:

```python
new_hdr = bytearray(stock_hdr)  # COPY EVERYTHING — cmdline, os_version, name, tail bytes
struct.pack_into('<I', new_hdr, 8, len(our_kernel))  # only change: kernel_size
struct.pack_into('<I', new_hdr, 1644, 0)              # dtbo_size=0 (if not copying dtbo)
```

Pad kernel and ramdisk to page boundaries:

```python
page_size = 2048
kp = (len(our_kernel) + page_size - 1) // page_size
kernel_padded = our_kernel + b'\x00' * (kp * page_size - len(our_kernel))
rp = (len(stock_ramdisk) + page_size - 1) // page_size
ramdisk_padded = stock_ramdisk + b'\x00' * (rp * page_size - len(stock_ramdisk))
```

Assemble and verify:

```python
boot_img = bytes(new_hdr) + kernel_padded + ramdisk_padded
assert boot_img[0:8] == b'ANDROID!'
assert struct.unpack_from('<I', boot_img, 40)[0] == 2  # header v2
```

## Common Pitfalls

### dtbo_size trap
If the stock header has `dtbo_size > 0`, the stock boot image has a device tree blob overlay
AFTER the ramdisk (at a page boundary). If you set `dtbo_size=0` in your image, LK won't
try to read it. If you keep `dtbo_size` but don't include the data, LK reads garbage.

**Solution:** Either set `dtbo_size=0`, or extract and splice in the stock DTBO bytes.

### Header tail bytes (offset 1648-4096)
The stock header v2 has non-zero metadata after the standard fields. COPY THEM when building
byte-for-byte — LK may read them.

### Kernel larger than stock
If your zImage is larger than the stock kernel_size, the ramdisk shifts to a later page.
This is fine — just update the header fields accordingly. The ramdisk_addr and tags_addr
stay the same (they're physical addresses, not image offsets).

## Systematic Cmdline Overflow Debugging

When LK prints "cmdline overflow" before the kernel runs:

### Hypothesis elimination protocol

| Candidate | What changed | What it tests |
|-----------|-------------|---------------|
| C (original) | DTB embedded in boot image | Baseline — overflow present |
| D1r | No DTB (header v1) | Is DTB bootargs the cause? |
| D1s | Stock header v2, ONLY kernel replaced | Is header version the cause? Is kernel image the cause? |

### Interpreting results

- **D1r overflows** → DTB bootargs is NOT the cause (DTB bootargs disproven)
- **D1s overflows** → Cause is IN the kernel image (size? appended data? decompressor?)
- **D1s boots past LK** → Header v1 was unsupported by this LK

### Next steps if kernel image is the cause
- Check zImage for appended FDT magic (`\xd0\x0d\xfe\xed`)
- Compare kernel sizes (stock vs yours) — LK may allocate cmdline buffer differently
- Check zImage decompressor format (gzip vs lzma vs xz vs none)
- Try padding/depadding the zImage to match stock size exactly
