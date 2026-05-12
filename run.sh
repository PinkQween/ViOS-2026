qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
  -drive if=pflash,format=raw,file=bin/OVMF_VARS.fd \
  -drive file=bin/os.bin,format=raw,if=ide \
  -m 512M \
  -serial stdio