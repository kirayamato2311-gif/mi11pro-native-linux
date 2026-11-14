#!/bin/bash
# SPDX-License-Identifier: MIT
set -u
exec >>/var/log/mars-managed-usb-test.log 2>&1
echo "=== Managed USB test $(date -Iseconds) ==="
if test -e /etc/mars-managed-usb-debug; then
    dmesg -w >>/var/log/mars-managed-usb-kernel.log &
    kernel_log_pid=$!
    trap 'kill "$kernel_log_pid" 2>/dev/null || true' EXIT
fi
modefile=/sys/kernel/debug/usb/a600000.usb/mode
gadget=/sys/kernel/config/usb_gadget/mars
previous=''; stable=0; changes=0; window=$SECONDS
for driver in usbnet cdc_ether r8152 asix ax88179_178a; do modprobe "$driver" || true; done
bind_device() {
    for ((n=0;n<20;n++)); do
        if test -e /sys/class/udc/a600000.usb; then
            if test -z "$(cat "$gadget/UDC" 2>/dev/null)"; then
                echo a600000.usb >"$gadget/UDC" || return 1
            fi
            ip link set usb0 up || return 1
            ip address replace 192.168.77.2/24 dev usb0 || return 1
            return 0
        fi
        sleep 0.5
    done
    return 1
}
snapshot() {
    echo "USB snapshot: mode=$(cat "$modefile" 2>/dev/null) UDC=$(cat "$gadget/UDC" 2>/dev/null) state=$(cat /sys/class/udc/a600000.usb/state 2>/dev/null) speed=$(cat /sys/class/udc/a600000.usb/current_speed 2>/dev/null) power=$(cat /sys/class/typec/port0/power_role 2>/dev/null)"
}
for ((n=0;n<60;n++)); do test -e "$modefile" && break; sleep 0.5; done
if ! test -w "$modefile"; then echo 'DWC3 mode control missing'; exit 1; fi
bind_device || echo 'Initial gadget bind not ready yet'
while sleep 0.5; do
    if ! test -e /sys/class/typec/port0-partner; then previous=''; stable=0; continue; fi
    role=$(cat /sys/class/typec/port0/data_role 2>/dev/null)
    case "$role" in
        '[host] device') wanted=host ;;
        'host [device]') wanted=device ;;
        *) continue ;;
    esac
    if test "$wanted" != "$previous"; then previous=$wanted; stable=1; continue; fi
    stable=$((stable+1)); if ((stable<6)); then continue; fi
    current=$(cat "$modefile")
    if test "$current" = "$wanted"; then
        if test "$wanted" = device; then bind_device || true; fi
        continue
    fi
    if ((SECONDS-window>30)); then window=$SECONDS; changes=0; fi
    changes=$((changes+1))
    if ((changes>3)); then echo 'Too many role changes; stopping test'; exit 1; fi
    echo "$(date -Iseconds) transition $current -> $wanted; typec=$role"
    if test "$wanted" = host; then
        if test -n "$(cat "$gadget/UDC" 2>/dev/null)"; then
            echo '' >"$gadget/UDC" || exit 1
        fi
        sleep 1
    fi
    if test "$wanted" = device && test "$current" = host; then
        # Reprobe only the USB controller: fully tears down xHCI, DMA and PHY
        # state instead of relying on the previously unsuccessful DRD reset.
        driver=/sys/bus/platform/drivers/dwc3
        test -L "$driver/a600000.usb" || { echo 'Unexpected DWC3 driver binding'; exit 1; }
        snapshot
        echo a600000.usb >"$driver/unbind" || exit 1
        sleep 1
        echo a600000.usb >"$driver/bind" || exit 1
    else
        echo "$wanted" >"$modefile" || exit 1
    fi
    for ((n=0;n<20;n++)); do
        test "$(cat "$modefile")" = "$wanted" && break
        sleep 0.5
    done
    if test "$(cat "$modefile")" != "$wanted"; then echo 'Role transition timeout'; exit 1; fi
    if test "$wanted" = device; then bind_device || exit 1; fi
    echo "$(date -Iseconds) transition complete"
    snapshot
    if test "$wanted" = device; then
        for ((n=0;n<10;n++)); do
            state=$(cat /sys/class/udc/a600000.usb/state 2>/dev/null)
            test "$state" = configured && break
            sleep 1
        done
        snapshot
    fi
    sync
    sleep 2
    stable=0
done
