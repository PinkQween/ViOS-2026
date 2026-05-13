#!/usr/bin/env sh
set -eu

ovmf_code="/usr/share/edk2/x64/OVMF_CODE.4m.fd"
ovmf_vars_template="/usr/share/edk2/x64/OVMF_VARS.4m.fd"
ovmf_vars_runtime="bin/OVMF_VARS.fd"

if [ ! -f "$ovmf_vars_runtime" ]; then
  cp "$ovmf_vars_template" "$ovmf_vars_runtime"
fi

qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file="$ovmf_code" \
  -drive if=pflash,format=raw,file="$ovmf_vars_runtime" \
  -drive file=bin/os-uefi.img,format=raw,if=ide \
  -m 512M \
  -serial stdio