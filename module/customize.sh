#!/system/bin/sh
# Copyright (C) 2026 LUMina Team

SKIPUNZIP=0

MODULE_CONFIG="/data/adb/.config/lumina"
BIN_DIR="$MODPATH/system/bin"
DAEMON_BIN="$BIN_DIR/luminad"

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

[ "$API" -lt 29 ] && abort_android_version

case "$ARCH" in
    "arm64") ARCH_TMP="arm64-v8a" ;;
    *) abort_unsupported_arch ;;
esac

ui_print "---------------------------------------------------------"
ui_print "             LUMina Tweaks Engine v1.1                   "
ui_print "                Modular C++ Backend                      "
ui_print "---------------------------------------------------------"

ui_print "- Extracting C++ native engine ($ARCH_TMP)..."
mkdir -p "$BIN_DIR"

if [ -d "$MODPATH/libs/$ARCH_TMP" ]; then
    cp -rf "$MODPATH/libs/$ARCH_TMP/"* "$BIN_DIR/" 2>/dev/null
    rm -rf "$MODPATH/libs"
elif [ -f "$MODPATH/luminad" ]; then
    mv -f "$MODPATH/luminad" "$DAEMON_BIN"
fi

if [ ! -f "$DAEMON_BIN" ]; then
    ui_print "! Critical Error: luminad binary missing from zip!"
    abort "---------------------------------------------------------"
fi

if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
    ui_print "- KernelSU / APatch environment detected"
    manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"
    for dir in $manager_paths; do
        if [ -d "$dir" ]; then
            ui_print "- Symlinking luminad to $dir"
            ln -sf "$DAEMON_BIN" "$dir/luminad"
        fi
    done
fi

ui_print "- Initializing system configurations..."
mkdir -p "$MODULE_CONFIG"

if [ ! -f "$MODULE_CONFIG/config.json" ]; then
    cat <<'EOF' > "$MODULE_CONFIG/config.json"
{
  "auto_mode": false,
  "disable_thermal": false,
  "ram_tweaks": false,
  "mtk_tweaks": false,
  "tcp_bbr": false,
  "disable_logd": false,
  "refresh_rate": "default",
  "render_engine": "default",
  "cpu_limit": 100
}
EOF
fi


if [ ! -f "$MODULE_CONFIG/gamelist.json" ]; then
    cat <<'EOF' > "$MODULE_CONFIG/gamelist.json"
{}
EOF
fi

ui_print "- Setting executable permissions..."
set_perm_recursive "$BIN_DIR" 0 0 0755 0755
chmod 755 "$DAEMON_BIN"

chcon u:object_r:system_file:s0 "$DAEMON_BIN" 2>/dev/null || chcon u:object_r:adb_data_file:s0 "$DAEMON_BIN" 2>/dev/null

[ -f "$MODPATH/service.sh" ] && set_perm "$MODPATH/service.sh" 0 0 0755

ui_print "---------------------------------------------------------"
ui_print "         LUMina Tweaks Installed Successfully!           "
ui_print "---------------------------------------------------------"
