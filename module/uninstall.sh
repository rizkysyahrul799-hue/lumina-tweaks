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

# ─── 1. Terminate Active Daemon & Supervisor ─────────────────

pkill -9 -f "luminad" 2>/dev/null
killall -9 luminad 2>/dev/null

# ─── 2. Restore System Thermal & Unmount Battery Spoof ───────


for temp_node in \
    "/sys/class/power_supply/battery/temp" \
    "/sys/class/power_supply/bms/temp" \
    "/sys/class/power_supply/battery/batt_temp" \
    "/sys/class/power_supply/main/temp"; do
    if grep -qs "$temp_node" /proc/mounts; then
        umount -f "$temp_node" 2>/dev/null
    fi
done


start thermal 2>/dev/null
start thermald 2>/dev/null
start vendor.thermal-hal-2-0 2>/dev/null
start vendor.thermal-hal-1-0 2>/dev/null
start mi_thermald 2>/dev/null
start thermal_monitor 2>/dev/null

# ─── 3. Remove Configurations & System Scripts ────────────────

rm -rf /data/adb/.config/lumina
rm -f /data/adb/service.d/.lumina_cleanup.sh

# ─── 4. Remove KernelSU / APatch Binaries & Symlinks ──────────

need_gone="luminad"
manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"

for dir in $manager_paths; do
    if [ -d "$dir" ]; then
        for bin in $need_gone; do
            rm -f "$dir/$bin"
        done
    fi
done
