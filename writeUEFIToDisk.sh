#!/usr/bin/env sh
set -eu

usage() {
  cat <<EOF
Usage: $0 /dev/sdX
Writes bin/os-uefi.img to the whole-disk device. WARNING: This will destroy all data.

Options:
  FORCE=1    skip confirmation prompt (use with care)

Examples:
  sudo ./writeUEFIToDisk /dev/sdb
EOF
}

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

DEV=$1
IMG="bin/os-uefi.img"

if [ ! -e "$DEV" ]; then
  echo "Device not found: $DEV"
  exit 1
fi

# Ensure device is a whole-disk block device
TYPE=$(lsblk -n -o TYPE "$DEV" 2>/dev/null || true)
if [ "$TYPE" != "disk" ]; then
  echo "$DEV does not appear to be a whole-disk block device (type=$TYPE). Aborting."
  exit 1
fi

# Check for mounted partitions
MOUNTPOINTS=$(lsblk -nr -o MOUNTPOINT --paths "$DEV" | sed '/^$/d' || true)
if [ -n "$(echo "$MOUNTPOINTS" | sed -n '1p')" ]; then
  echo "Device has mounted partitions. Unmount them first and retry:"
  lsblk "$DEV"
  exit 1
fi

if [ ! -f "$IMG" ]; then
  echo "UEFI image not found: $IMG"
  exit 1
fi

# Require root; if not, re-exec with sudo
if [ "$(id -u)" -ne 0 ]; then
  echo "Root privileges are required to write to $DEV. Re-running with sudo..."
  exec sudo sh -c "exec \"$0\" \"$DEV\""
fi

if [ "${FORCE:-0}" != "1" ]; then
  echo "About to write $IMG to $DEV"
  echo "THIS WILL OVERWRITE THE ENTIRE DISK AND DESTROY ALL DATA ON IT."
  printf "Type YES to continue: "
  read ans || true
  if [ "$ans" != "YES" ]; then
    echo "Aborted by user."
    exit 1
  fi
fi

echo "Writing $IMG -> $DEV ..."
# Use dd with reasonable block size and fsync to ensure completion
if command -v dd >/dev/null 2>&1; then
  dd if="$IMG" of="$DEV" bs=4M status=progress conv=fsync || { echo "dd failed"; exit 1; }
else
  cat "$IMG" > "$DEV" || { echo "write failed"; exit 1; }
fi

sync
sleep 1

echo "Write complete."
exit 0
