#!/system/bin/sh
#
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

MODULE_DIR="/data/adb/modules/lumina"
CONFIG_DIR="/data/adb/.config/lumina"
CONFIG_FILE="$CONFIG_DIR/config.json"
DAEMON_BIN="$MODULE_DIR/luminad"

# Fallback path jika dijalankan dari system bin
[ ! -f "$DAEMON_BIN" ] && DAEMON_BIN="/system/bin/luminad"

# ─── 1. Daemon Status & Auto-Recovery ─────────────────────────

DAEMON_PID=$(pgrep -f "luminad daemon" || pgrep -f "luminad")
RECOVERY_MSG=""

# Jika daemon mati, coba jalankan kembali secara otomatis
if [ -z "$DAEMON_PID" ] && [ -f "$DAEMON_BIN" ]; then
    nohup "$DAEMON_BIN" daemon >/dev/null 2>&1 &
    sleep 1
    DAEMON_PID=$(pgrep -f "luminad daemon" || pgrep -f "luminad")
    [ -n "$DAEMON_PID" ] && RECOVERY_MSG=" (Auto-Recovered! 🔄)"
fi

DAEMON_STATUS="MATI / CRASH ❌"
[ -n "$DAEMON_PID" ] && DAEMON_STATUS="AKTIF ✅ (PID: $DAEMON_PID)$RECOVERY_MSG"

# ─── 2. System Information Reader ─────────────────────────────

BRAND=$(getprop ro.product.brand)
MODEL=$(getprop ro.product.model)
DEVICE=$(getprop ro.product.device)
ANDROID_VER=$(getprop ro.build.version.release)
SDK_VER=$(getprop ro.build.version.sdk)
KERNEL_VER=$(uname -r)
SELINUX=$(getenforce)

# ─── 3. Real-Time Hardware & System State ─────────────────────

TCP_REAL=$(cat /proc/sys/net/ipv4/tcp_congestion_control 2>/dev/null || echo "N/A")
SW_REAL=$(cat /proc/sys/vm/swappiness 2>/dev/null || echo "N/A")
LOGD_STATUS=$(pgrep -x logd >/dev/null && echo "Active (Logd Killer Off)" || echo "Killed (Logd Killer On)")
THERM_MODE=$(cat /sys/class/thermal/thermal_zone0/mode 2>/dev/null || echo "N/A")
PEAK_HZ=$(settings get system peak_refresh_rate 2>/dev/null || echo "N/A")
HWUI_PROP=$(getprop debug.hwui.renderer 2>/dev/null)
[ -z "$HWUI_PROP" ] && HWUI_PROP="default"
CPU0_MAX=$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq 2>/dev/null || echo "N/A")

# ─── 4. Output Report Print ───────────────────────────────────

cat << EOF
==========================================
        LUMINA TWEAKS DIAGNOSTICS         
==========================================
Waktu         : $(date)
Brand / Model : $BRAND $MODEL ($DEVICE)
Android Ver   : $ANDROID_VER (SDK $SDK_VER)
Kernel Ver    : $KERNEL_VER
SELinux       : $SELINUX
==========================================
            STATUS DAEMON & CORE          
==========================================
Status Daemon   : $DAEMON_STATUS

==========================================
          REAL-TIME SYSTEM STATE          
==========================================
TCP Congestion  : $TCP_REAL
VM Swappiness   : $SW_REAL
Logd Service    : $LOGD_STATUS
Thermal Zone0   : $THERM_MODE
Peak Refresh Hz : $PEAK_HZ
Render Engine   : $HWUI_PROP
CPU Policy0 Max : $CPU0_MAX KHz

==========================================
              CONFIG JSON                 
==========================================
$(cat "$CONFIG_FILE" 2>/dev/null || echo "File config.json tidak ditemukan!")
==========================================
EOF
