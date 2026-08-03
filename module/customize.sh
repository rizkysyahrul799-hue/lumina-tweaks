#!/system/bin/sh
#
# Copyright (C) 2024-2026 LUMina Team
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

# shellcheck disable=SC1091,SC2034,SC2317
SKIPUNZIP=0
SOC=0

MODULE_CONFIG="/data/adb/.config/lumina"
BIN_DIR="$MODPATH/system/bin"
DAEMON_BIN="$BIN_DIR/luminad"

make_dir() {
	[ ! -d "$1" ] && mkdir -p "$1"
}

abort_unsupported_arch() {
	ui_print "*********************************************************"
	ui_print "! Unsupported Architecture: $ARCH"
	ui_print "! Your CPU architecture is not supported by LUMina Tweaks."
	abort "*********************************************************"
}

abort_gamelist_error() {
	ui_print "*********************************************************"
	ui_print "! Failed to initialize gamelist!"
	ui_print "! Installation aborted."
	abort "*********************************************************"
}

abort_android_version() {
	ui_print "*********************************************************"
	ui_print "! Your Android Version is not supported!"
	ui_print "! Please use Android 10 (API 29) or higher."
	abort "*********************************************************"
}

soc_recognition_extra() {
	[ -d /sys/class/kgsl/kgsl-3d0/devfreq ] || [ -d /sys/devices/platform/kgsl-2d0.0/kgsl ] && {
		SOC=2
		ui_print "- SoC Target: Qualcomm Snapdragon"
		return 0
	}

	[ -d /sys/kernel/ged/hal ] && {
		SOC=1
		ui_print "- SoC Target: MediaTek"
		return 0
	}

	[ -d /sys/kernel/tegra_gpu ] && {
		SOC=6
		ui_print "- SoC Target: Nvidia Tegra"
		return 0
	}

	return 1
}

get_soc_getprop() {
	SOC_PROP="
ro.board.platform
ro.soc.model
ro.hardware
ro.chipname
ro.hardware.chipname
ro.vendor.soc.model.external_name
ro.vendor.qti.soc_name
ro.vendor.soc.model.part_name
ro.vendor.soc.model
"
	for prop in $SOC_PROP; do
		getprop "$prop"
	done
}

recognize_soc() {
	case "$1" in
	*mt* | *MT*) SOC=1 ;;
	*sm* | *qcom* | *SM* | *QCOM* | *Qualcomm*) SOC=2 ;;
	*exynos* | *Exynos* | *EXYNOS* | *universal* | *samsung* | *erd* | *s5e*) SOC=3 ;;
	*Unisoc* | *unisoc* | *ums* | *UNISOC* | *sp* | *SC*) SOC=4 ;;
	*gs* | *Tensor* | *tensor*) SOC=5 ;;
	*kirin*) SOC=7 ;;
	esac

	case "$SOC" in
	1) ui_print "- SoC Target: MediaTek" ;;
	2) ui_print "- SoC Target: Qualcomm Snapdragon" ;;
	3) ui_print "- SoC Target: Samsung Exynos" ;;
	4) ui_print "- SoC Target: UNISOC" ;;
	5) ui_print "- SoC Target: Google Tensor" ;;
	6) ui_print "- SoC Target: Nvidia Tegra" ;;
	7) ui_print "- SoC Target: HiSilicon Kirin" ;;
	0) return 1 ;;
	esac
}

generate_gamelist() {
  if [ -f "$DAEMON_BIN" ]; then
    "$DAEMON_BIN" setup_gamelist "$MODULE_CONFIG/gamelist.txt"
    exit_code=$?
  else
    exit_code=0
  fi

  rm -f "$MODULE_CONFIG/gamelist.txt"
  # Fallback safe
}

# 1. Compatibility Check
[ "$API" -lt 29 ] && abort_android_version

ui_print "---------------------------------------------------------"
ui_print "             LUMina Tweaks Engine v1.0                   "
ui_print "---------------------------------------------------------"

# Backup original module.prop
[ -f "$MODPATH/module.prop" ] && cp "$MODPATH/module.prop" "$MODPATH/module.prop.orig"

# 2. Extract Native Binaries
case $ARCH in
"arm64") ARCH_TMP="arm64-v8a" ;;
"arm") ARCH_TMP="armeabi-v7a" ;;
*) abort_unsupported_arch ;;
esac

ui_print "- Extracting core binaries ($ARCH)..."
if [ -d "$TMPDIR/libs/$ARCH_TMP" ]; then
  mkdir -p "$BIN_DIR"
  cp -rf "$TMPDIR"/libs/"$ARCH_TMP"/* "$BIN_DIR/" 2>/dev/null
  rm -rf "$TMPDIR/libs"
fi

# 3. Mountify & Environment Flags
touch "$MODPATH/skip_mountify"

# 4. KernelSU / APatch Symlink Handler
if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
  ui_print "- KernelSU / APatch environment detected"
	rm -f "$MODPATH/action.sh" 2>/dev/null
	touch "$MODPATH/skip_mount"

	manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"
	for dir in $manager_paths; do
		[ -d "$dir" ] && {
			ui_print "- Creating daemon symlink in $dir"
			[ -f "$DAEMON_BIN" ] && ln -sf "$DAEMON_BIN" "$dir/luminad"
			[ -f "$BIN_DIR/lumina_profiler" ] && ln -sf "$BIN_DIR/lumina_profiler" "$dir/lumina_profiler"
			[ -f "$BIN_DIR/lumina_utility" ] && ln -sf "$BIN_DIR/lumina_utility" "$dir/lumina_utility"
		}
	done
fi

# 5. Extract Webroot UI
if unzip -l "$ZIPFILE" "webroot/*" >/dev/null 2>&1; then
  ui_print "- Deploying WebUI interface..."
  unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" -x "*.sha256" >&2
fi

# 6. Environmental Cleanup
[ -d /data/encore ] && rm -rf /data/encore
[ -d /data/lumina ] && rm -rf /data/lumina

# 7. Configurations Setup
ui_print "- Initializing system configurations..."
make_dir "$MODULE_CONFIG"
if unzip -l "$ZIPFILE" "config/*" >/dev/null 2>&1; then
  unzip -o "$ZIPFILE" "config/*" -d "$MODULE_CONFIG" -x "*.sha256" >&2
  [ -d "$MODULE_CONFIG/config" ] && mv "$MODULE_CONFIG/config/"* "$MODULE_CONFIG/" 2>/dev/null
  rm -rf "$MODULE_CONFIG/config"
fi

# 8. Set Execution Permissions
ui_print "- Setting executable permissions..."
set_perm_recursive "$BIN_DIR" 0 0 0755 0755

# 9. Gamelist JSON Initialization
if [ ! -f "$MODULE_CONFIG/gamelist.json" ]; then
  ui_print "- Creating initial gamelist database..."
  generate_gamelist
else
  if [ -f "$DAEMON_BIN" ]; then
    "$DAEMON_BIN" check_gamelist
    if [ $? -gt 0 ]; then
      ui_print "! Gamelist database is malformed, re-generating..."
      generate_gamelist
    fi
  fi
fi

# 10. Hardware SoC Recognition
soc_recognition_extra
[ $SOC -eq 0 ] && recognize_soc "$(</proc/device-tree/model)"
[ $SOC -eq 0 ] && recognize_soc "$(get_soc_getprop)"
[ $SOC -eq 0 ] && recognize_soc "$(grep -E "Hardware|Processor" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')"
[ $SOC -eq 0 ] && recognize_soc "$(grep "model\sname" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')"
[ $SOC -eq 0 ] && {
	ui_print "! Warning: Hardware platform unidentified, running generic profile"
}

echo $SOC >"$MODULE_CONFIG/soc_recognition"

ui_print "---------------------------------------------------------"
ui_print "           LUMina Tweaks Installed Successfully!         "
ui_print "---------------------------------------------------------"
