# KERNEL TEST LOG (every flash-test: candidate, md5, what flashed, result, pstore findings, conclusion)

## 2026-06-04 — candidate C boot_C_lcm.img (md5 062294031caf...)
Flashed to boot. Result: LK "cmdline overflow", aborted before kernel. Conclusion: DTB bootargs too long → led to candidate D.
