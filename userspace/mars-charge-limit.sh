#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
battery=/sys/class/power_supply/qcom-battmgr-bat
control=$battery/device/mars_input_suspend
state_dir=/var/lib/mars-charge-limit
mkdir -p "$state_dir"
exec 9>/run/lock/mars-charge-limit.lock
flock -n 9 || exit 0
start=${START_PERCENT:-80}
stop=${STOP_PERCENT:-85}
[[ $start =~ ^[0-9]+$ && $stop =~ ^[0-9]+$ ]] || exit 1
((start>=5 && stop<=95 && stop>=50 && start<stop)) || exit 1
if test -e "$state_dir/fault"; then exit 0; fi
capacity=$(cat "$battery/capacity")
actual=$(cat "$control")
[[ $capacity =~ ^[0-9]+$ ]] && ((capacity<=100)) || exit 1
[[ $actual = 0 || $actual = 1 ]] || exit 1
previous=$(cat "$state_dir/paused" 2>/dev/null || echo "$actual")
[[ $previous = 0 || $previous = 1 ]] || previous=$actual
desired=$previous
if ((capacity>=stop)); then desired=1
elif ((capacity<=start)); then desired=0
fi
if test "$actual" != "$desired"; then
    if ! echo "$desired" >"$control" || test "$(cat "$control")" != "$desired"; then
        echo 'Control write/readback failed; further writes disabled' >&2
        echo 1 >"$state_dir/fault"
        exit 1
    fi
    echo "Battery $capacity%: input_suspend=$desired (resume $start%, stop $stop%)"
fi
echo "$desired" >"$state_dir/paused"
