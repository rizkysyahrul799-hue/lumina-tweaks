#ifndef LUMINA_SYSTEM_TWEAKS_HPP
#define LUMINA_SYSTEM_TWEAKS_HPP

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "src/utils/sysfs.hpp"
#include "src/utils/notifier.hpp"
#include "src/utils/config_loader.hpp"

inline void apply_render_engine(const std::string& engine) {
    if (engine == "Vulkan" || engine == "vulkan") {
        exec_cmd("setprop debug.hwui.renderer vulkan 2>/dev/null");
        exec_cmd("setprop debug.renderengine.backend vulkan 2>/dev/null");
    } else if (engine == "OpenGL" || engine == "opengl") {
        exec_cmd("setprop debug.hwui.renderer opengl 2>/dev/null");
        exec_cmd("setprop debug.renderengine.backend skiagl 2>/dev/null");
    } else if (engine == "Skia" || engine == "skia") {
        exec_cmd("setprop debug.hwui.renderer skiavk 2>/dev/null");
        exec_cmd("setprop debug.renderengine.backend skia 2>/dev/null");
    } else {
        exec_cmd("setprop debug.hwui.renderer \"\" 2>/dev/null");
        exec_cmd("setprop debug.renderengine.backend \"\" 2>/dev/null");
    }
}

inline void disable_all_thermal() {
    exec_cmd("stop thermal 2>/dev/null");
    exec_cmd("stop thermald 2>/dev/null");
    exec_cmd("stop vendor.thermal-hal-2-0 2>/dev/null");
    exec_cmd("stop vendor.thermal-hal-1-0 2>/dev/null");
    exec_cmd("stop mi_thermald 2>/dev/null");
    exec_cmd("stop thermal_monitor 2>/dev/null");
    for (int i = 0; i < 30; i++) {
        std::string zone = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/mode";
        if (fs::exists(zone)) write_sysfs(zone, "disabled");
    }
}

inline void enable_all_thermal() {
    exec_cmd("start thermal 2>/dev/null");
    exec_cmd("start thermald 2>/dev/null");
    exec_cmd("start vendor.thermal-hal-2-0 2>/dev/null");
    exec_cmd("start vendor.thermal-hal-1-0 2>/dev/null");
    exec_cmd("start mi_thermald 2>/dev/null");
    exec_cmd("start thermal_monitor 2>/dev/null");
    for (int i = 0; i < 30; i++) {
        std::string zone = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/mode";
        if (fs::exists(zone)) write_sysfs(zone, "enabled");
    }
}

inline void apply_ram_tweaks_for_profile(const std::string& profile) {
    std::string config_path = "/data/adb/.config/lumina/config.json";
    std::ifstream cfg_file(config_path);
    bool ram_enabled = false;

    if (cfg_file.is_open()) {
        std::string cfg_str((std::istreambuf_iterator<char>(cfg_file)),
                            std::istreambuf_iterator<char>());
        ram_enabled = parse_json_bool(cfg_str, "ram_tweaks");
    }

    if (!ram_enabled) {
        write_sysfs("/proc/sys/vm/swappiness", "60");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "100");
        write_sysfs("/proc/sys/vm/dirty_ratio", "20");
        return;
    }

    if (profile == "performance") {
        write_sysfs("/proc/sys/vm/swappiness", "10");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "50");
        write_sysfs("/proc/sys/vm/dirty_ratio", "10");
    } else if (profile == "balanced") {
        write_sysfs("/proc/sys/vm/swappiness", "30");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "60");
        write_sysfs("/proc/sys/vm/dirty_ratio", "15");
    } else if (profile == "eco" || profile == "powersave") {
        write_sysfs("/proc/sys/vm/swappiness", "60");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "100");
        write_sysfs("/proc/sys/vm/dirty_ratio", "20");
    }
}

inline void apply_ram_tweaks(bool enable) {
    if (enable) {
        write_sysfs("/proc/sys/vm/swappiness", "10");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "50");
        write_sysfs("/proc/sys/vm/dirty_ratio", "15");
    } else {
        write_sysfs("/proc/sys/vm/swappiness", "60");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "100");
        write_sysfs("/proc/sys/vm/dirty_ratio", "20");
    }
}

inline void apply_mtk_tweaks(bool enable) {
    if (!is_mediatek()) return;
    std::string val = enable ? "0" : "1";
    std::vector<std::string> paths = {
        "/sys/kernel/ged/hal/ged_kpi",
        "/sys/kernel/ged/hal/ged_kpi_enable",
        "/sys/module/ged/parameters/ged_log_perf_trace_enable",
        "/sys/module/pbm/parameters/pbm_disable",
        "/sys/module/mtk_pbm/parameters/pbm_disable"
    };
    for (const auto& p : paths) {
        if (fs::exists(p)) {
            write_sysfs(p, p.find("pbm_disable") != std::string::npos ? (enable ? "1" : "0") : val);
        }
    }
}

inline void apply_gpu_boost(int level) {
    if (!is_mediatek()) return;
    std::string path = "/sys/kernel/ged/hal/gpu_boost_level";
    if (fs::exists(path)) write_sysfs(path, std::to_string(level));
}

inline void apply_governor_all(const std::string& governor) {
    for (int i = 0; i < 8; i++) {
        std::string p = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
        if (fs::exists(p)) write_sysfs(p, governor);
    }
}

inline void apply_io_scheduler_all(const std::string& scheduler) {
    for (const auto& dev : std::vector<std::string>{"sda","sdb","sdc","sdd","mmcblk0","dm-0"}) {
        std::string p = "/sys/block/" + dev + "/queue/scheduler";
        if (fs::exists(p)) write_sysfs(p, scheduler);
    }
}

inline void apply_tcp_bbr(bool enable) {
    if (enable) {
        std::string avail = read_sysfs("/proc/sys/net/ipv4/tcp_available_congestion_control");
        write_sysfs("/proc/sys/net/ipv4/tcp_fastopen", "3");
        write_sysfs("/proc/sys/net/ipv4/tcp_autocorking", "1");
        if (avail.find("bbr") != std::string::npos) {
            write_sysfs("/proc/sys/net/ipv4/tcp_congestion_control", "bbr");
            if (fs::exists("/proc/sys/net/core/default_qdisc"))
                write_sysfs("/proc/sys/net/core/default_qdisc", "fq_codel");
        } else if (avail.find("westwood") != std::string::npos) {
            write_sysfs("/proc/sys/net/ipv4/tcp_congestion_control", "westwood");
        }
    } else {
        write_sysfs("/proc/sys/net/ipv4/tcp_congestion_control", "cubic");
        write_sysfs("/proc/sys/net/ipv4/tcp_fastopen", "1");
        write_sysfs("/proc/sys/net/ipv4/tcp_autocorking", "1");
    }
}

inline void apply_logd_killer(bool enable) {
    exec_cmd(enable ? "stop logd 2>/dev/null" : "start logd 2>/dev/null");
}

inline void lock_all_clusters_max() {
    for (int i = 0; i < 8; i++) {
        std::string pol = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i);
        if (!fs::exists(pol)) continue;
        std::string max = read_sysfs(pol + "/cpuinfo_max_freq");
        if (max.empty()) continue;
        max.erase(max.find_last_not_of(" \n\r\t") + 1);
        write_sysfs(pol + "/scaling_min_freq", max);
        write_sysfs(pol + "/scaling_max_freq", max);
    }
}

inline void restore_all_clusters() {
    for (int i = 0; i < 8; i++) {
        std::string pol = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i);
        if (!fs::exists(pol)) continue;
        std::string mn = read_sysfs(pol + "/cpuinfo_min_freq");
        std::string mx = read_sysfs(pol + "/cpuinfo_max_freq");
        if (mn.empty() || mx.empty()) continue;
        mn.erase(mn.find_last_not_of(" \n\r\t") + 1);
        mx.erase(mx.find_last_not_of(" \n\r\t") + 1);
        write_sysfs(pol + "/scaling_min_freq", mn);
        write_sysfs(pol + "/scaling_max_freq", mx);
    }
}

inline void apply_cpu_limit_percent(int percent) {
    if (percent >= 100) { lock_all_clusters_max(); return; }
    for (int i = 0; i < 8; i++) {
        std::string pol = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i);
        if (!fs::exists(pol)) continue;
        std::string mx = read_sysfs(pol + "/cpuinfo_max_freq");
        std::string mn = read_sysfs(pol + "/cpuinfo_min_freq");
        if (mx.empty() || mn.empty()) continue;
        mx.erase(mx.find_last_not_of(" \n\r\t") + 1);
        mn.erase(mn.find_last_not_of(" \n\r\t") + 1);
        long max_f = std::stol(mx), min_f = std::stol(mn);
        long target = min_f + ((max_f - min_f) * percent / 100);
        write_sysfs(pol + "/scaling_min_freq", mn);
        write_sysfs(pol + "/scaling_max_freq", std::to_string(target));
    }
}

inline int handle_apply_profile(const std::string& profile, bool is_lite_mode = false,
                                const std::string& game_pkg = "", bool session_ended = false) {
    std::string governor, io_scheduler;
    bool disable_thermal_toggle = get_disable_thermal_config();

    if (profile == "performance") {
        io_scheduler = "mq-deadline";
        std::string notif_msg = is_lite_mode ? "Profil Performance Lite" : "Profil Performance";
        send_notification("LUMina Tweaks", notif_msg);

        if (is_lite_mode) {
            governor = "performance";
            restore_all_clusters();
            apply_cpu_limit_percent(80);
            if (is_mediatek()) apply_gpu_boost(50);
            if (disable_thermal_toggle) disable_all_thermal();
        } else {
            governor = "performance";
            apply_cpu_limit_percent(100);
            if (is_mediatek()) apply_gpu_boost(100);
            if (disable_thermal_toggle) disable_all_thermal();
        }
        apply_logd_killer(true);

    } else if (profile == "balanced") {
        governor = "schedutil";
        io_scheduler = "bfq";
        restore_all_clusters();
        enable_all_thermal();

        send_notification("LUMina Tweaks", "Profil Balance");

        if (is_lite_mode) {
            apply_cpu_limit_percent(80);
        } else {
            if (is_mediatek()) apply_gpu_boost(0);
        }
        apply_logd_killer(false);

    } else if (profile == "eco" || profile == "powersave") {
        governor = "powersave";
        io_scheduler = "noop";
        restore_all_clusters();
        enable_all_thermal();
        if (is_mediatek()) apply_gpu_boost(0);
        apply_logd_killer(false);

        send_notification("LUMina Tweaks", "Profil Eco");

    } else {
        return 1;
    }

    apply_governor_all(governor);
    apply_io_scheduler_all(io_scheduler);
    apply_ram_tweaks_for_profile(profile);

    return 0;
}

#endif
