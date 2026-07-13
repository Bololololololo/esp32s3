#!/bin/bash
# Fix missing USB device node for ESP32-S3 built-in JTAG inside a dev container.
# Run this whenever OpenOCD fails with "LIBUSB_ERROR_NO_DEVICE" after
# plugging/replugging the board while the container is already running.

set -euo pipefail

VENDOR="303a"
PRODUCT="1001"

# Find the bus and devnum from sysfs
SYSFS_PATH=$(grep -rl "idVendor" /sys/bus/usb/devices/ 2>/dev/null | \
  while read f; do
    dir=$(dirname "$f")
    vid=$(cat "$dir/idVendor" 2>/dev/null)
    pid=$(cat "$dir/idProduct" 2>/dev/null)
    if [[ "$vid" == "$VENDOR" && "$pid" == "$PRODUCT" ]]; then
      echo "$dir"
      break
    fi
  done)

if [[ -z "$SYSFS_PATH" ]]; then
  echo "ERROR: ESP32-S3 USB JTAG device (${VENDOR}:${PRODUCT}) not found. Is the board plugged in?"
  exit 1
fi

BUSNUM=$(cat "$SYSFS_PATH/busnum")
DEVNUM=$(cat "$SYSFS_PATH/devnum")
MINOR=$(( (BUSNUM - 1) * 128 + (DEVNUM - 1) ))
NODE="/dev/bus/usb/$(printf '%03d' $BUSNUM)/$(printf '%03d' $DEVNUM)"

echo "Found ESP32-S3 JTAG at bus ${BUSNUM}, device ${DEVNUM} → ${NODE} (major 189, minor ${MINOR})"

if [[ -e "$NODE" ]]; then
  echo "Device node already exists: ${NODE}"
else
  mkdir -p "$(dirname "$NODE")"
  mknod "$NODE" c 189 "$MINOR"
  echo "Created device node: ${NODE}"
fi

chmod 666 "$NODE"
echo "Done. OpenOCD should now be able to open the device."
