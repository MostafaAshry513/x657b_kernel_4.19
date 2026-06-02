AUTONOMOUS KERNEL BUILD TASK — Infinix X657B (MT6761, Linux 4.19, 32-bit ARM). Work until done; iterate on
every error; do NOT stop at the first failure. Log every step and push to GitHub + Mega.

CONTEXT (read first):
- Read /root/android/kernel_x657b/BUILD_LOG.md fully. Source = bengris32/android_kernel_mediatek_4.19 (4.19.325),
  targeting the device's stock 4.19.127. Source fixes + config fixups already applied are listed there.
- The build is at /root/android/kernel_x657b (git remote origin = https://github.com/MostafaAshry513/x657b_kernel_4.19.git).
- Stock defconfig: /root/android/kernel_backup/x657b_kernel_defconfig (also /root/android/x657b_kernel_defconfig).
- Stock kernel was built with CLANG 11.0.1. Stock boot/ramdisk/dtb/dtbo in /root/android/working_ref/boot.emmc.win
  and /root/android/kernel_backup/{stock_kernel_zImage,dtb.img,dtbo.img}.

THE FIX TO APPLY (root cause of the GCC failure):
GCC's GNU `as` treats `@` as a comment, so `.type sym,@object` breaks on ARM ("unrecognized symbol type"),
failing kernel/bpf/verifier.o. SOLUTION: build with AOSP CLANG 11 (its integrated assembler has no @object
problem) — the same toolchain stock used. Clang is already here:
  /root/android/lineage/prebuilts/clang/host/linux-x86/clang-r383902b1/bin
  /root/android/lineage/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin (for CROSS_COMPILE binutils)

Build recipe to try (adjust as needed):
  export PATH=/root/android/lineage/prebuilts/clang/host/linux-x86/clang-r383902b1/bin:\
/root/android/lineage/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin:$PATH
  cd /root/android/kernel_x657b
  make O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- CROSS_COMPILE=arm-linux-androideabi- \
       <base_defconfig>      # seed config from the stock defconfig, then olddefconfig
  # re-apply the config fixups from build.sh (DEBUG_INFO_BTF off, TASKS_TRACE_RCU off, SYSTEM_TRUSTED_KEYS="",
  #   SECURITY_LOCKDOWN_LSM_EARLY off) to out/.config, then:
  make -j$(nproc) O=out ARCH=arm CC=clang CLANG_TRIPLE=arm-linux-gnueabi- CROSS_COMPILE=arm-linux-androideabi- zImage
Keep clang's INTEGRATED assembler (do NOT pass -fno-integrated-as). If a few host tools still need GNU as, that
is fine — the kernel objects must use clang IAS. Try LD=ld.lld if GNU ld errors. Keep all the documented source
fixes; only revert a source fix if clang makes it unnecessary.

GOALS (in order):
1. Build arch/arm/boot/zImage successfully with clang. Iterate through every compile error: apply minimal
   fixes (config or source), record each in BUILD_LOG.md, rebuild. This is the main objective.
2. Once zImage builds: package a flashable boot.img. Unpack the stock boot (/root/android/working_ref/boot.emmc.win)
   with the lineage mkbootimg/unpack_bootimg (out/host/linux-x86/bin) to get its ramdisk + header args + cmdline,
   swap in the new zImage (keep stock dtb/dtbo and the EXACT stock cmdline + base/offsets — MTK LK is picky),
   repack as boot_newkernel.img. Verify with unpack.
3. Save artifacts: out/arch/arm/boot/zImage and boot_newkernel.img.

LOGGING (do after EACH meaningful step — required):
- Append progress + errors + fixes to /root/android/kernel_x657b/BUILD_LOG.md.
- git: `gh auth setup-git` (gh is authed as MostafaAshry513); then commit & push source/log changes:
  git -C /root/android/kernel_x657b add -A && git -C /root/android/kernel_x657b commit -m "..." && git -C /root/android/kernel_x657b push origin HEAD
- Mega: `mega-put -c BUILD_LOG.md /X657B-build/kernel/` and upload zImage/boot_newkernel.img to /X657B-build/kernel/ when built.
- Do NOT upload the agentrouter/deepseek API keys anywhere.

RULES: This is fire-and-forget; nobody is watching. Keep iterating autonomously until zImage (and ideally
boot.img) is built, or until you have exhausted reasonable fixes and documented exactly what's blocking. Write
a clear final SUMMARY in BUILD_LOG.md (status, zImage path+md5, boot.img path+md5, or the precise remaining error).
