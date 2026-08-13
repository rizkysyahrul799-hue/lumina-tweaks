#ifndef LUMINA_SYSTEM_TWEAKS_HPP
#define LUMINA_SYSTEM_TWEAKS_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cctype>

#include "src/utils/sysfs.hpp"
#include "src/utils/notifier.hpp"
#include "src/utils/config_loader.hpp"

namespace fs = std::filesystem;

inline std::vector<std::string> g_thermal_snapshot;

static std::string _get_max_freq(const std::string& policy) {
    std::ifstream avail(policy + "/scaling_available_frequencies");
    if (avail) {
        std::string line;
        std::getline(avail, line);
        std::istringstream ss(line);
        std::string tok, best;
        long best_val = 0;
        while (ss >> tok) {
            try {
                long v = std::stol(tok);
                if (v > best_val) { best_val = v; best = tok; }
            } catch (...) {}
        }
        if (!best.empty()) return best;
    }
    std::ifstream f(policy + "/cpuinfo_max_freq");
    if (!f) return "";
    std::string val;
    std::getline(f, val);
    while (!val.empty() && (val.back() == '\n' || val.back() == '\r' || val.back() == ' '))
        val.pop_back();
    return val;
}

static std::string _trim(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    return s;
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

inline void snapshot_thermal_zones() {
    g_thermal_snapshot.clear();
    for (int i = 0; i < 30; i++) {
        std::string zone = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/mode";
        if (fs::exists(zone)) {
            std::string val = read_sysfs(zone);
            val = _trim(val);
            g_thermal_snapshot.push_back(val);
        } else {
            g_thermal_snapshot.push_back("");
        }
    }
}

inline void restore_thermal_zones() {
    if (g_thermal_snapshot.empty()) {
        enable_all_thermal();
        return;
    }
    for (size_t i = 0; i < 30 && i < g_thermal_snapshot.size(); i++) {
        if (g_thermal_snapshot[i].empty()) continue;
        std::string zone = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/mode";
        if (fs::exists(zone)) {
            write_sysfs(zone, g_thermal_snapshot[i]);
        }
    }
    g_thermal_snapshot.clear();
}

static bool _lock_policy_max(const std::string& policy) {
    auto max_f = _get_max_freq(policy);
    if (max_f.empty()) return false;

    auto max_path = policy + "/scaling_max_freq";
    auto min_path = policy + "/scaling_min_freq";

    chmod(max_path.c_str(), 0644);
    chmod(min_path.c_str(), 0644);

    for (int attempt = 0; attempt < 5; attempt++) {
        write_sysfs(max_path, max_f);
        write_sysfs(min_path, max_f);

        std::string vm, vn;
        { std::ifstream f(max_path); std::getline(f, vm); }
        { std::ifstream f(min_path); std::getline(f, vn); }

        if (_trim(vm) == max_f && _trim(vn) == max_f) {
            chmod(max_path.c_str(), 0444);
            chmod(min_path.c_str(), 0444);

            const char* dbg = "/proc/cpudvfs/cpufreq_debug";
            if (access(dbg, W_OK) == 0) {
                std::string cid = policy.substr(policy.rfind("policy") + 6);
                chmod(dbg, 0644);
                write_sysfs(dbg, cid + " " + max_f + " " + max_f);
                chmod(dbg, 0444);
            }
            return true;
        }
        usleep(200000);
    }
    return false;
}

inline void lock_all_clusters_max() {
    const std::string base = "/sys/devices/system/cpu/cpufreq/";
    DIR* dir = opendir(base.c_str());
    if (!dir) return;
    dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name(entry->d_name);
        if (name.rfind("policy", 0) != 0) continue;
        _lock_policy_max(base + name);
    }
    closedir(dir);
}

inline void restore_all_clusters() {
    const std::string base = "/sys/devices/system/cpu/cpufreq/";
    DIR* dir = opendir(base.c_str());
    if (!dir) return;
    dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name(entry->d_name);
        if (name.rfind("policy", 0) != 0) continue;
        std::string pol = base + name;

        auto max_path = pol + "/scaling_max_freq";
        auto min_path = pol + "/scaling_min_freq";

        chmod(max_path.c_str(), 0644);
        chmod(min_path.c_str(), 0644);

        std::string mx = _trim(read_sysfs(pol + "/cpuinfo_max_freq"));
        std::string mn = _trim(read_sysfs(pol + "/cpuinfo_min_freq"));
        if (!mx.empty()) write_sysfs(max_path, mx);
        if (!mn.empty()) write_sysfs(min_path, mn);
    }
    closedir(dir);
}

inline void apply_touch_boost(int level) {
    if (level < 0 || level > 5) level = 5;
    std::string touch_ms, idle_ms;
    switch (level) {
        case 0:
            exec_cmd("setprop debug.sf.touch_timer_ms \"\" 2>/dev/null");
            exec_cmd("setprop debug.sf.idle_timer_ms \"\" 2>/dev/null");
            return;
        case 1: touch_ms = "200"; idle_ms = "100"; break;
        case 2: touch_ms = "400"; idle_ms = "200"; break;
        case 3: touch_ms = "600"; idle_ms = "300"; break;
        case 4: touch_ms = "800"; idle_ms = "400"; break;
        case 5: touch_ms = "999999"; idle_ms = "999999"; break;
    }
    exec_cmd("setprop debug.sf.touch_timer_ms " + touch_ms + " 2>/dev/null");
    exec_cmd("setprop debug.sf.idle_timer_ms " + idle_ms + " 2>/dev/null");
}

inline void apply_game_priority(const std::string& game_pkg) {
    if (game_pkg.empty()) return;
    std::string pid;
    DIR* dir = opendir("/proc");
    if (!dir) return;
    dirent* entry;
    while ((entry = readdir(dir))) {
        std::string p = entry->d_name;
        if (p.empty() || !std::isdigit(p[0])) continue;
        std::ifstream cmdline("/proc/" + p + "/cmdline");
        if (!cmdline) continue;
        std::string cmd;
        std::getline(cmdline, cmd, '\0');
        if (cmd.find(game_pkg) != std::string::npos) {
            pid = p;
            break;
        }
    }
    closedir(dir);
    if (pid.empty()) return;

    setpriority(PRIO_PROCESS, std::stoi(pid), -10);
    exec_cmd("ionice -c 2 -n 0 -p " + pid + " 2>/dev/null");

    for (const auto& cg : std::vector<std::string>{
            "/dev/cpuctl/top-app/tasks",
            "/dev/cgroup/top-app/tasks"}) {
        if (fs::exists(cg)) {
            write_sysfs(cg, pid);
            break;
        }
    }

    std::string oom_path = "/proc/" + pid + "/oom_score_adj";
    write_sysfs(oom_path, "-500");
}

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

inline void apply_ram_tweaks_for_profile(const std::string& profile) {
    std::ifstream cfg_file("/data/adb/.config/lumina/config.json");
    bool ram_enabled = false;
    if (cfg_file.is_open()) {
        std::string s((std::istreambuf_iterator<char>(cfg_file)), {});
        ram_enabled = parse_json_bool(s, "ram_tweaks");
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
    } else {
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
        if (fs::exists(p))
            write_sysfs(p, p.find("pbm_disable") != std::string::npos
                           ? (enable ? "1" : "0") : val);
    }
}

inline void apply_gpu_boost(int level) {
    if (!is_mediatek()) return;
    std::string path = "/sys/kernel/ged/hal/gpu_boost_level";
    if (fs::exists(path)) write_sysfs(path, std::to_string(level));
}

inline void apply_governor_all(const std::string& governor) {
    for (int i = 0; i < 8; i++) {
        std::string p = "/sys/devices/system/cpu/cpufreq/policy"
                      + std::to_string(i) + "/scaling_governor";
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

inline void apply_cpu_limit_percent(int percent) {
    if (percent >= 100) { lock_all_clusters_max(); return; }
    const std::string base = "/sys/devices/system/cpu/cpufreq/";
    DIR* dir = opendir(base.c_str());
    if (!dir) return;
    dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name(entry->d_name);
        if (name.rfind("policy", 0) != 0) continue;
        std::string pol = base + name;
        std::string mx = _trim(read_sysfs(pol + "/cpuinfo_max_freq"));
        std::string mn = _trim(read_sysfs(pol + "/cpuinfo_min_freq"));
        if (mx.empty() || mn.empty()) continue;
        long max_f = std::stol(mx), min_f = std::stol(mn);
        long target = min_f + ((max_f - min_f) * percent / 100);
        write_sysfs(pol + "/scaling_min_freq", mn);
        write_sysfs(pol + "/scaling_max_freq", std::to_string(target));
    }
    closedir(dir);
}

inline int handle_apply_profile(const std::string& profile, bool is_lite_mode = false,
                                const std::string& game_pkg = "", bool session_ended = false) {
    bool disable_thermal_toggle = get_disable_thermal_config();

    if (profile == "performance") {
        send_notification("LUMina Tweaks",
            is_lite_mode ? "Profil Performance Lite" : "Profil Performance");

        apply_governor_all("performance");
        apply_io_scheduler_all("mq-deadline");

        if (is_lite_mode) {
            restore_all_clusters();
            apply_cpu_limit_percent(80);
            if (is_mediatek()) apply_gpu_boost(50);
        } else {
            lock_all_clusters_max();
            if (is_mediatek()) apply_gpu_boost(100);
        }

        apply_touch_boost(5);

        if (!game_pkg.empty()) {
            apply_game_priority(game_pkg);
        }

        snapshot_thermal_zones();
        if (disable_thermal_toggle) {
            disable_all_thermal();
        }

        apply_logd_killer(true);

    } else if (profile == "balanced") {
        send_notification("LUMina Tweaks", "Profil Balance");

        apply_governor_all("schedutil");
        apply_io_scheduler_all("bfq");
        restore_all_clusters();

        restore_thermal_zones();

        if (is_lite_mode) apply_cpu_limit_percent(80);
        else if (is_mediatek()) apply_gpu_boost(0);

        apply_touch_boost(0);
        apply_logd_killer(false);

    } else if (profile == "eco" || profile == "powersave") {
        send_notification("LUMina Tweaks", "Profil Eco");

        apply_governor_all("powersave");
        apply_io_scheduler_all("noop");
        restore_all_clusters();

        restore_thermal_zones();

        if (is_mediatek()) apply_gpu_boost(0);
        apply_touch_boost(0);
        apply_logd_killer(false);

    } else {
        return 1;
    }

    apply_ram_tweaks_for_profile(profile);
    return 0;
}

#endif // LUMINA_SYSTEM_TWEAKS_HPP