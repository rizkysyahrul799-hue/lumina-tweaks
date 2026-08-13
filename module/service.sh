#!/system/bin/sh
# Copyright (C) 2026 LUMina Team

MODDIR="$(dirname "$(readlink -f "$0")")"
DAEMON="$MODDIR/system/bin/luminad"

until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 3
done

[ ! -f "$DAEMON" ] && exit 1

chmod 755 "$DAEMON" 2>/dev/null
chcon u:object_r:system_file:s0 "$DAEMON" 2>/dev/null || chcon u:object_r:adb_data_file:s0 "$DAEMON" 2>/dev/null

"$DAEMON" daemon >/dev/null 2>&1 &

exit 0
