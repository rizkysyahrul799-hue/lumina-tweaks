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

SKIPUNZIP=0

MODULE_CONFIG="/data/adb/.config/lumina"
BIN_DIR="$MODPATH/system/bin"
DAEMON_BIN="$BIN_DIR/luminad"

# ─── Helper Functions ─────────────────────────────────────────

abort_unsupported_arch() {
	ui_print "*********************************************************"
	ui_print "! Unsupported Architecture: $ARCH"
	ui_print "! LUMina Tweaks C++ Engine requires 64-bit (arm64-v8a)."
	abort "*********************************************************"
}

abort_android_version() {
	ui_print "*********************************************************"
	ui_print "! Unsupported Android Version: API $API"
	ui_print "! LUMina Tweaks requires Android 10 (API 29) or higher."
	abort "*********************************************************"
}

# ─── 1. Pre-Installation Checks ───────────────────────────────

[ "$API" -lt 29 ] && abort_android_version

case "$ARCH" in
    "arm64") ARCH_TMP="arm64-v8a" ;;
    *) abort_unsupported_arch ;;
esac

ui_print "---------------------------------------------------------"
ui_print "             LUMina Tweaks Engine v1.1                   "
ui_print "                Modular C++ Backend                      "
ui_print "---------------------------------------------------------"

# ─── 2. Extract Core Binary ───────────────────────────────────

ui_print "- Extracting C++ native engine ($ARCH_TMP)..."
mkdir -p "$BIN_DIR"

if [ -d "$TMPDIR/libs/$ARCH_TMP" ]; then
    cp -rf "$TMPDIR/libs/$ARCH_TMP/"* "$BIN_DIR/" 2>/dev/null
    rm -rf "$TMPDIR/libs"
elif [ -f "$TMPDIR/luminad" ]; then
    cp -f "$TMPDIR/luminad" "$DAEMON_BIN"
fi

# Fallback check
if [ ! -f "$DAEMON_BIN" ]; then
    ui_print "! Critical Error: luminad binary missing from zip!"
    abort "---------------------------------------------------------"
fi

# ─── 3. Environment Flags ─────────────────────────────────────

touch "$MODPATH/skip_mountify"

# ─── 4. KernelSU / APatch Integration ─────────────────────────

if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
    ui_print "- KernelSU / APatch environment detected"
    touch "$MODPATH/skip_mount"

    manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"
    for dir in $manager_paths; do
        if [ -d "$dir" ]; then
            ui_print "- Symlinking luminad to $dir"
            ln -sf "$DAEMON_BIN" "$dir/luminad"
        fi
    done
fi

# ─── 5. Deploy WebUI ──────────────────────────────────────────

if unzip -l "$ZIPFILE" "webroot/*" >/dev/null 2>&1; then
    ui_print "- Deploying WebUI interface..."
    unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" >/dev/null 2>&1
fi

# ─── 6. Initialize Config Directory & Defaults ────────────────

ui_print "- Initializing system configurations..."
mkdir -p "$MODULE_CONFIG"

# Default config.json
if [ ! -f "$MODULE_CONFIG/config.json" ]; then
    ui_print "- Creating default config.json..."
    cat <<'EOF' > "$MODULE_CONFIG/config.json"
{
  "auto_mode": true,
  "disable_thermal": true,
  "ram_tweaks": true,
  "mtk_tweaks": true,
  "tcp_bbr": true,
  "disable_logd": true,
  "refresh_rate": "auto",
  "render_engine": "default",
  "cpu_limit": 100
}
EOF
fi

# Default gamelist.json
if [ ! -f "$MODULE_CONFIG/gamelist.json" ]; then
    ui_print "- Creating default gamelist.json..."
    cat <<'EOF' > "$MODULE_CONFIG/gamelist.json"
{
  "com.mobile.legends": {
    "perf_lite_mode": "default",
    "dnd_on_gaming": "false",
    "app_priority": "default",
    "game_preload": "default",
    "refresh_rate": "default",
    "renderer": "default",
    "resolution_downscale": "default",
    "resolution_fps": "60"
  },
  "com.dts.freefireth": {
    "perf_lite_mode": "default",
    "dnd_on_gaming": "false",
    "app_priority": "default",
    "game_preload": "default",
    "refresh_rate": "default",
    "renderer": "default",
    "resolution_downscale": "default",
    "resolution_fps": "60"
  }
}
EOF
fi

# ─── 7. Set Permissions ───────────────────────────────────────

ui_print "- Setting executable permissions..."
set_perm_recursive "$BIN_DIR" 0 0 0755 0755
chmod 755 "$DAEMON_BIN"

ui_print "---------------------------------------------------------"
ui_print "         LUMina Tweaks Installed Successfully!           "
ui_print "---------------------------------------------------------"
