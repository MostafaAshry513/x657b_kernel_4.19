# KERNEL KNOWLEDGE BASE (append-only — driver ports, source refs, mappings, DEAD-ENDS)

## Confirmed facts
- Stock boot 57e6: header v2 but dtb_size=0 → LK loads DTB from the dtb partition, not the boot image.
- cmdline overflow on candidate C = from-source DTB /chosen/bootargs too long (debug args + literal TABs).
- nt36525b LCM panel driver ported from Vivo Y81 (DavidWithTuxedo/android_kernel_vivo_y81, 4.9.77).

## CONFIG_TRAN_* mapping (TODO: fill — option -> sibling source -> status)

## Reserved-memory / ramoops map (TODO: extract from stock DTB)

## DEAD-ENDS (never retry)
- (none yet)
