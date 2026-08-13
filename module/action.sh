#!/system/bin/sh
# Copyright (C) 2026 LUMina Team

MODULE_DIR="/data/adb/modules/lumina"
CONFIG_DIR="/data/adb/.config/lumina"
CONFIG_FILE="$CONFIG_DIR/config.json"
DAEMON_BIN="$MODULE_DIR/system/bin/luminad"

[ ! -f "$DAEMON_BIN" ] && DAEMON_BIN="/system/bin/luminad"

DAEMON_PID=$(pgrep -f "luminad daemon" || pgrep -f "luminad")
RECOVERY_MSG=""

if [ -z "$DAEMON_PID" ] && [ -f "$DAEMON_BIN" ]; then
    nohup "$DAEMON_BIN" daemon >/dev/null 2>&1 &
    sleep 1
    DAEMON_PID=$(pgrep -f "luminad daemon" || pgrep -f "luminad")
    [ -n "$DAEMON_PID" ] && RECOVERY_MSG=" (Auto-Recovered)"
fi

DAEMON_STATUS="OFF / CRASH"
[ -n "$DAEMON_PID" ] && DAEMON_STATUS="ACTIVE (PID: $DAEMON_PID)$RECOVERY_MSG"

BRAND=$(getprop ro.product.brand)
MODEL=$(getprop ro.product.model)
DEVICE=$(getprop ro.product.device)
ANDROID_VER=$(getprop ro.build.version.release)
SDK_VER=$(getprop ro.build.version.sdk)
KERNEL_VER=$(uname -r)
SELINUX=$(getenforce)

TCP_REAL=$(cat /proc/sys/net/ipv4/tcp_congestion_control 2>/dev/null || echo "N/A")
SW_REAL=$(cat /proc/sys/vm/swappiness 2>/dev/null || echo "N/A")
LOGD_STATUS=$(pgrep -x logd >/dev/null && echo "Active" || echo "Killed")
THERM_MODE=$(cat /sys/class/thermal/thermal_zone0/mode 2>/dev/null || echo "N/A")
PEAK_HZ=$(settings get system peak_refresh_rate 2>/dev/null || echo "N/A")
HWUI_PROP=$(getprop debug.hwui.renderer 2>/dev/null)
[ -z "$HWUI_PROP" ] && HWUI_PROP="default"
CPU0_MAX=$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq 2>/dev/null || echo "N/A")

cat << EOF
==========================================
        LUMINA TWEAKS DIAGNOSTICS         
==========================================
Time          : $(date)
Brand / Model : $BRAND $MODEL ($DEVICE)
Android Ver   : $ANDROID_VER (SDK $SDK_VER)
Kernel Ver    : $KERNEL_VER
SELinux       : $SELINUX
==========================================
            DAEMON & CORE STATUS          
==========================================
Daemon Status : $DAEMON_STATUS

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
$(cat "$CONFIG_FILE" 2>/dev/null || echo "config.json not found")
==========================================
EOF
