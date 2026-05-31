#!/bin/bash
# Build script for X657B kernel (4.19.325)
# Fixes applied: DEBUG_INFO_BTF off, TASKS_TRACE_RCU off, certs path, lockdown early LSM off

set -e

KERNEL_DIR="/root/android/kernel_x657b"
JOBS=$(nproc)

cd "$KERNEL_DIR"

echo "=== Cleaning build directory ==="
rm -rf out
mkdir -p out

echo "=== Running olddefconfig ==="
make O=out ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- olddefconfig

echo "=== Applying config fixups ==="
# Disable DEBUG_INFO_BTF (breaks ARM asm with @object directives)
sed -i 's/^CONFIG_DEBUG_INFO_BTF=y$/# CONFIG_DEBUG_INFO_BTF is not set/' out/.config

# Disable TASKS_TRACE_RCU (struct rcu_tasks not defined without TASKS_RCU)
sed -i 's/^CONFIG_TASKS_TRACE_RCU=y$/# CONFIG_TASKS_TRACE_RCU is not set/' out/.config

# Fix certs path (debian canonical certs don't exist)
sed -i 's|^CONFIG_SYSTEM_TRUSTED_KEYS=.*|CONFIG_SYSTEM_TRUSTED_KEYS=""|' out/.config

# Disable lockdown early LSM (DEFINE_EARLY_LSM not defined in 4.19)
sed -i 's/^CONFIG_SECURITY_LOCKDOWN_LSM_EARLY=y$/# CONFIG_SECURITY_LOCKDOWN_LSM_EARLY is not set/' out/.config

echo "=== Building kernel (zImage) ==="
make -j"$JOBS" O=out ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- zImage

echo "=== Build complete ==="
ls -lh out/arch/arm/boot/zImage 2>/dev/null && echo "SUCCESS: zImage built!" || echo "FAILED: zImage not found"