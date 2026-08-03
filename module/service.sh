#!/system/bin/sh
#
# Copyright (C) 2024-2026 Rem01Gaming
# Copyright (C) 2026 LUMina Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MODDIR=${0%/*}
[ -z "$MODDIR" ] && MODDIR="/data/adb/modules/lumina"

# 1. Wait until Android system completely finished booting
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

# 2. Config directory initialization
CONFIG_DIR="/data/adb/.config/lumina"
mkdir -p "$CONFIG_DIR"

if [ ! -f "$CONFIG_DIR/auto_mode" ]; then
    echo "1" > "$CONFIG_DIR/auto_mode"
fi

BIN="$MODDIR/system/bin/luminad"
LOG="$CONFIG_DIR/daemon.log"

if [ -f "$LOG" ]; then
    tail -n 500 "$LOG" > "$LOG.tmp" 2>/dev/null && mv "$LOG.tmp" "$LOG"
else
    echo "=== LUMina Tweaks Daemon Log Started ===" > "$LOG"
fi

# 3. Ensure single instance & apply execution context
if [ -f "$BIN" ]; then
    killall -9 luminad 2>/dev/null
    
    chmod 755 "$BIN"
    chcon u:object_r:system_file:s0 "$BIN" 2>/dev/null
    
    # 4. Background daemon supervisor loop
    (
        while true; do
            echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting luminad daemon..." >> "$LOG"
            "$BIN" --daemon >> "$LOG" 2>&1
            
            echo "[$(date '+%Y-%m-%d %H:%M:%S')] luminad stopped or crashed! Restarting in 5s..." >> "$LOG"
            sleep 5
        done
    ) &
fi
