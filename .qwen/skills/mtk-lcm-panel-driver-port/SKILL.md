---
name: mtk-lcm-panel-driver-port
description: Porting a MediaTek LCM (display panel) driver from one kernel version/device to another — finding compatible drivers, adapting 4.9 API to 4.19, stripping vendor-specific code, and integrating into the MTK lcm framework
source: auto-skill
extracted_at: '2026-06-04T06:37:15.225Z'
---

# MediaTek LCM Panel Driver Port

## When to Use This

A MediaTek kernel (MT67xx/MT68xx) compiles and boots, hardware drivers initialize, but the
**screen stays dark** (no display output). The kernel config's `CONFIG_CUSTOM_KERNEL_LCM` is
empty or set to a panel name whose driver source doesn't exist in the tree. The target panel
is a Novatek NT36525 (or similar NT3652x), ILI9881, HX83112, or JD9365 — any DSI video-mode
LCD panel common in budget MT6761/MT6765/MT6771 phones.

**Root cause:** The LCM panel driver lives in `drivers/misc/mediatek/lcm/<panel_name>/`
and was part of the device OEM's kernel fork. It was never upstreamed and is absent from
generic SoC kernel trees (e.g., `bengris32/` lineage kernels).

## Finding a Compatible LCM Driver

### Step 1: Identify the target panel name
From the stock kernel config (extracted via `extract-ikconfig`):
```
CONFIG_CUSTOM_KERNEL_LCM="nt36525b_hdp_dsi_vdo_txd_mdt_x657b ..."
```
Parse: `nt36525b` = Novatek NT36525B controller, `hdp` = HD+ resolution (720×1600),
`dsi_vdo` = DSI video mode, `txd` = TXD panel vendor, `mdt` = MDT backlight IC,
`x657b` = device model.

### Step 2: Search for the panel controller name in GitHub code
```bash
# Search for nt36525 in C source (NOT ikconfig dumps)
gh search code "nt36525" language:c

# Search for the panel name in Kconfig/Makefile
gh search code "nt36525" filename:Makefile

# Also search for similar panel controllers
gh search code "nt3652" language:c
```

Key search terms: `<controller> dsi vdo hdp`, `<controller> mediatek lcm`, `CONFIG_CUSTOM_KERNEL_LCM`.

### Step 3: Prioritize MTK framework drivers
The driver MUST be in the MTK LCM framework (`drivers/misc/mediatek/lcm/`). Drivers in:
- `drivers/video/fbdev/exynos/panel/` — Samsung Exynos framework, NOT compatible
- `drivers/gpu/drm/` — DRM framework, NOT compatible with legacy MTK fbdev
- `drivers/devkit/lcdkit/` — Huawei Qcom framework, NOT compatible
- `drivers/input/touchscreen/` — Touchscreen driver (NT36525 is also a touch IC), NOT LCM

### Known good sources (as of 2026-06):
- **Vivo Y81** (`DavidWithTuxedo/android_kernel_vivo_y81`, 4.9.77): Multiple nt36525 variants
  (txd, tianma, boe, truly) + lm3697 backlight. MT6761 platform.
- **Nokia MT6771** (`nokia-dev/android_kernel_nokia_mt6771`): `nt36525_hdplus_dsi_vdo_tianma_rt5081`
- **Oppo A3** (`oppo-source/A3-8.1-kernel-source`, 4.4.95): `oppo17101_boe_nt36525_720p_dsi_vdo`
- **Vivo Y3s** (`vivo-source-mirror/Y3s`, 4.19.127): Many LCM drivers on same kernel version

## API Adaptation: 4.9 → 4.19

The MTK LCM framework (`lcm_drv.h`, `LCM_UTIL_FUNCS`, `LCM_DRIVER` struct) changed
between kernel versions. Here's what to adapt:

### Functions removed from LCM_UTIL_FUNCS in 4.19

| 4.9 Function | 4.19 Replacement | Notes |
|---|---|---|
| `lcm_vddi_setting(n)` | Remove | Bootloader/regulator handles VDDI |
| `lcm_enp_setting(n)` | Remove | Bootloader handles positive voltage |
| `lcm_enn_setting(n)` | Remove | Bootloader handles negative voltage |
| `lcm_reset_setting(n)` | `SET_RESET_PIN(n)` → `lcm_util.set_reset_pin()` | Still in LCM_UTIL_FUNCS |
| `lcm_bkg_setting(n)` | Remove | Backlight controlled via DCS/PWM |
| `dsi_set_hs(n)` | Remove | HS mode auto-negotiated in 4.19 DSI driver |

### Fields removed from LCM_DRIVER struct in 4.19

```c
// These fields exist in 4.9 LCM_DRIVER but NOT in 4.19:
.get_id         // Remove — not in 4.19 struct
.lcm_cabc_open  // Remove — not in 4.19 struct
.lcm_cabc_close // Remove — not in 4.19 struct
.lcm_reset      // Remove — not in 4.19 struct
```

### Macros that stay the same
```c
SET_RESET_PIN(v)       → lcm_util.set_reset_pin(v)
MDELAY(n)              → lcm_util.mdelay(n)
UDELAY(n)              → lcm_util.udelay(n)
dsi_set_cmdq_V2(cmd, count, ppara, force_update) → lcm_util.dsi_set_cmdq_V2()
dsi_set_cmdq(pdata, queue_size, force_update)     → lcm_util.dsi_set_cmdq()
write_cmd(cmd)         → lcm_util.dsi_write_cmd()
write_regs(addr, pdata, byte_nums) → lcm_util.dsi_write_regs()
read_reg(cmd)          → lcm_util.dsi_dcs_read_lcm_reg()
```

### Vendor-specific code to strip
- `CONFIG_VIVO_LCM_PD1732DF_CONTROL` blocks (Vivo hw revision logic)
- `CONFIG_MTK_LCM_MIPI_CLK_CONTROL` blocks (RF-dependent clock tuning)
- `product_hw_id`, `rf_hw_id` variable checks
- `panel_reset_state`, `panel_off_state`, `phone_shutdown_state` globals
- `lcm_cabc_vivo_open/close` functions
- `lcm_reset_for_touch` function
- CABC push tables (if not referenced from `set_backlight_cmdq`)
- `lcm_bias_set_avdd_n_avee`, `lcm_bias_set_control` (vendor PMIC bias)

## Porting Procedure

### 1. Clone the source kernel and copy the LCM driver
```bash
git clone --depth 1 <source-repo-url> /tmp/source_kernel
cp -r /tmp/source_kernel/drivers/misc/mediatek/lcm/<panel_name>/ \
      drivers/misc/mediatek/lcm/<new_panel_name>/
```

### 2. Rename identifiers
The driver filename, Makefile target, and `LCM_DRIVER` struct name must match
the directory name. Use sed:
```bash
sed -i 's/<old_panel_name>/<new_panel_name>/g' <new_panel_name>/<file>.c
```

### 3. Fix LCM_DRIVER struct initialization
Remove fields that don't exist in 4.19 (see table above). Keep only:
```c
struct LCM_DRIVER <name>_lcm_drv = {
    .name              = "<name>_drv",
    .set_util_funcs    = lcm_set_util_funcs,
    .get_params        = lcm_get_params,
    .init              = lcm_init,
    .suspend           = lcm_suspend,
    .resume            = lcm_resume,
    .compare_id        = lcm_compare_id,
    .init_power        = lcm_init_power,
    .resume_power      = lcm_init_power,
    .suspend_power     = lcm_suspend_power,
    .esd_check         = lcm_esd_check,
    .set_backlight_cmdq = lcm_setbacklight_cmdq,
    .ata_check         = lcm_ata_check,
    .update            = lcm_update,
    .switch_mode       = lcm_switch_mode,
};
```

### 4. Replace removed API calls
```bash
# Reset pin
sed -i 's/lcm_reset_setting(1)/SET_RESET_PIN(1)/g' <file>.c
sed -i 's/lcm_reset_setting(0)/SET_RESET_PIN(0)/g' <file>.c

# Voltage settings — replace with delays
sed -i 's/lcm_vddi_setting(1)/MDELAY(3)/g' <file>.c
sed -i 's/lcm_enp_setting(1)/MDELAY(3)/g' <file>.c
sed -i 's/lcm_enn_setting(1)/MDELAY(3)/g' <file>.c

# HS mode — remove, auto-negotiated
sed -i 's/dsi_set_hs_test(1);//g' <file>.c
sed -i 's/dsi_set_hs_test(0);//g' <file>.c

# Remove unused macro definitions
sed -i '/#define lcm_vddi_setting/,+1d' <file>.c
sed -i '/#define lcm_reset_setting/,+1d' <file>.c
sed -i '/#define lcm_enp_setting/,+1d' <file>.c
sed -i '/#define lcm_enn_setting/,+1d' <file>.c
sed -i '/#define lcm_bkg_setting/,+1d' <file>.c
sed -i '/#define dsi_set_hs_test/,+1d' <file>.c
```

### 5. Remove vendor-specific code
```bash
# Vivo global variables
sed -i '/panel_reset_state/d' <file>.c
sed -i '/panel_off_state/d' <file>.c
sed -i '/phone_shutdown_state/d' <file>.c

# Vivo hardware revision checks
sed -i '/product_hw_id/d' <file>.c
sed -i '/rf_hw_id/d' <file>.c
sed -i '/CONFIG_VIVO/d' <file>.c
sed -i '/lcm_bias_set/d' <file>.c
```

### 6. Remove unused functions
Use Python regex to surgically remove function bodies that are no longer
referenced from the LCM_DRIVER struct:
```python
import re
# Remove functions that were dropped from LCM_DRIVER
content = re.sub(r'static unsigned int lcm_get_id\(.*?\n\}\n', '', content, flags=re.DOTALL)
content = re.sub(r'static void lcm_cabc_vivo_open\(.*?\n\}\n', '', content, flags=re.DOTALL)
content = re.sub(r'static void lcm_cabc_vivo_close\(.*?\n\}\n', '', content, flags=re.DOTALL)
content = re.sub(r'static void lcm_reset_for_touch\(.*?\n\}\n', '', content, flags=re.DOTALL)
content = re.sub(r'static void cabc_push_table\(.*?\n\}\n', '', content, flags=re.DOTALL)
# Remove CABC command table arrays
content = re.sub(r'static struct LCM_setting_table lcm_cmd_cabc_.*?\[\].*?\n};\n', '', content, flags=re.DOTALL)
```

### 7. Rewrite lcm_suspend_power()
The 4.9 suspend function often has complex Vivo-specific conditional logic. Replace with:
```c
static void lcm_suspend_power(void)
{
    pr_err("[LCM]nt36525- lcm_suspend power off begin\n");
    SET_RESET_PIN(0);
    MDELAY(10);
    MDELAY(5);
    MDELAY(5);
    pr_err("[LCM]nt36525- lcm_suspend power off end\n");
}
```

### 8. Create the Makefile
```makefile
obj-y += <panel_name>.o
ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc
```

### 9. Enable in kernel config
```bash
# In out/.config:
CONFIG_MTK_LCM=y
CONFIG_CUSTOM_KERNEL_LCM="<panel_name>"
```

### 10. Build and iterate
```bash
make O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- \
     CROSS_COMPILE=arm-linux-androideabi- LD=ld.lld zImage
```

Common build errors and fixes:
- `no member named 'lcm_vddi_setting'` → Replace with MDELAY (step 4)
- `no member named 'dsi_set_hs'` → Remove dsi_set_hs_test calls (step 4)
- `unused function 'cabc_push_table'` → Remove with regex (step 6)
- `undefined symbol: phone_shutdown_state` → Remove vendor globals (step 5)
- `error: use of logical '&&' with constant operand` → sizeof() in CABC code, remove those functions

## What the Driver MUST Preserve

These are the critical parts that must survive the port:
1. **Panel init sequence** — the `init_setting[]` arrays with DSI commands
2. **`lcm_get_params()`** — resolution (FRAME_WIDTH/HEIGHT), DSI lane count, PLL clock, timing
3. **`lcm_compare_id()`** — panel identification via DSI read
4. **`lcm_init()`** — reset toggle sequence + init command push
5. **`lcm_setbacklight_cmdq()`** — DCS backlight commands
6. **`lcm_update()`** — partial update for panel self-refresh

If any of these are damaged during porting, the panel won't initialize.

## Resolution Adjustment

If the donor panel has a different resolution from the target:
- Adjust `FRAME_WIDTH` and `FRAME_HEIGHT` defines
- Adjust `LCM_PHYSICAL_WIDTH` and `LCM_PHYSICAL_HEIGHT`
- The init sequence (`init_setting[]`) may need adjustment for the resolution registers
- DSI timing parameters (`horizontal_backporch`, etc.) are resolution-dependent

For X657B (HDP): 720×1600, physical ~68mm×151mm, PLL clock ~362 MHz.

## Limitations

This port provides the panel **initialization sequence** and **power management**.
It typically does NOT provide:
- Panel-specific CABC/content-adaptive backlight (proprietary algorithms)
- ESD recovery beyond basic check
- Panel aging compensation
- Factory calibration data loading

These are usually handled by vendor HALs in userspace anyway.
