#!/system/bin/sh
#
# Copyright (C) 2026 LUMina Team
#

export PATH="/system/bin:/system/xbin:/data/adb/ap/bin:/data/adb/ksu/bin:$PATH"

MODDIR="${0%/*}"
[ -z "$MODDIR" ] || [ "$MODDIR" = "." ] && MODDIR="/data/adb/modules/lumina"

# ─── 1. Tunggu Android Boot Selesai ───────────────────────────

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

# ─── 2. Setup Log dan Path Biner ──────────────────────────────

CONFIG_DIR="/data/adb/.config/lumina"
mkdir -p "$CONFIG_DIR"

BOOT_LOG="$CONFIG_DIR/daemon_boot.log"
echo "=== LUMina Boot Service Started [$(date '+%Y-%m-%d %H:%M:%S')] ===" > "$BOOT_LOG"

# Cari biner luminad secara presisi
BIN=""
for path in \
    "/data/adb/modules/lumina/system/bin/luminad" \
    "/data/adb/modules/lumina/luminad" \
    "$MODDIR/system/bin/luminad" \
    "$MODDIR/luminad"; do
    if [ -f "$path" ]; then
        BIN="$path"
        break
    fi
done

if [ -z "$BIN" ]; then
    echo "ERROR: Biner luminad tidak ditemukan di lokasi manapun!" >> "$BOOT_LOG"
    exit 1
fi

echo "Binary path: $BIN" >> "$BOOT_LOG"

# ─── 3. Set Permission & Clean Instance ───────────────────────

chmod 755 "$BIN"
chcon u:object_r:system_file:s0 "$BIN" 2>/dev/null

pkill -9 -f "luminad" 2>/dev/null

# ─── 4. Supervisor Loop (Menjaga Daemon Tetap Hidup) ─────────

echo "Supervisor active, monitoring luminad..." >> "$BOOT_LOG"

while true; do
    if ! pgrep -f "luminad" >/dev/null 2>&1; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Launching luminad daemon..." >> "$BOOT_LOG"
        "$BIN" daemon >> "$CONFIG_DIR/daemon.log" 2>&1
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Daemon stopped or crashed! Restarting in 5s..." >> "$BOOT_LOG"
    fi
    sleep 5
done
