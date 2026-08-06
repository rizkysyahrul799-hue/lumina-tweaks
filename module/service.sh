#!/system/bin/sh
# Copyright (C) 2026 LUMina Team

# Resolving path presisi
MODDIR="$(dirname "$(readlink -f "$0")")"
DAEMON="$MODDIR/system/bin/luminad"


[ ! -f "$DAEMON" ] && exit 1


chmod 755 "$DAEMON" 2>/dev/null
chcon u:object_r:system_file:s0 "$DAEMON" 2>/dev/null || chcon u:object_r:adb_data_file:s0 "$DAEMON" 2>/dev/null


"$DAEMON" daemon >/dev/null 2>&1 &

exit 0
