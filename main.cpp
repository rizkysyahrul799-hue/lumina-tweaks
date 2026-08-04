#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cmath>
#include <sstream>

namespace fs = std::filesystem;

struct AppConfig {
    std::string lite_mode = "default";
    bool enable_dnd = false;
};

bool write_sysfs(const std::string& path, const std::string& value) {
    std::string chmod_cmd = "chmod 666 " + path + " 2>/dev/null";
    system(chmod_cmd.c_str());
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << value;
    return true;
}

std::string read_sysfs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content, line;
    while (std::getline(file, line)) content += line + "\n";
    return content;
}

std::string exec_cmd(const std::string& cmd) {
    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);
    return result;
}

bool is_mediatek() {
    std::string vendor = read_sysfs("/sys/devices/soc0/vendor");
    if (vendor.find("MediaTek") != std::string::npos || vendor.find("MTK") != std::string::npos) return true;
    std::string platform = exec_cmd("getprop ro.board.platform 2>/dev/null");
    if (platform.find("mt") == 0 || platform.find("MT") == 0 || platform.find("mediatek") != std::string::npos) return true;
    return false;
}

void send_notification(const std::string& title, const std::string& message) {
    std::string cmd = "su 2000 -c \"cmd notification post -S bigtext -t '"
                      + title + "' 'lumina_notif' '" + message + "'\" 2>/dev/null";
    exec_cmd(cmd);
}

bool parse_json_bool(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) pos = json.find("\"" + key + "\" :");
    if (pos == std::string::npos) return false;
    std::string sub = json.substr(pos + key.length() + 2, 10);
    return (sub.find("true") != std::string::npos);
}

std::string parse_json_string(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) pos = json.find("\"" + key + "\" :");
    if (pos == std::string::npos) return "";
    size_t start_quote = json.find("\"", pos + key.length() + 2);
    if (start_quote == std::string::npos) return "";
    size_t end_quote = json.find("\"", start_quote + 1);
    if (end_quote == std::string::npos) return "";
    return json.substr(start_quote + 1, end_quote - start_quote - 1);
}

bool get_global_lite_mode() {
    std::string path = "/data/adb/.config/lumina/config.json";
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_json_bool(content, "enforce_lite_mode");
}

bool get_disable_thermal_config() {
    std::string path = "/data/adb/.config/lumina/config.json";
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_json_bool(content, "disable_thermal");
}

std::map<std::string, AppConfig> load_gamelist_json(const std::string& path) {
    std::map<std::string, AppConfig> apps;
    std::ifstream file(path);
    if (!file.is_open()) return apps;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        size_t end_pkg = content.find("\"", pos + 1);
        if (end_pkg == std::string::npos) break;
        std::string key = content.substr(pos + 1, end_pkg - pos - 1);
        if (key.find('.') != std::string::npos && key != "lite_mode" && key != "enable_dnd") {
            size_t block_start = content.find("{", end_pkg);
            size_t block_end = content.find("}", block_start);
            if (block_start != std::string::npos && block_end != std::string::npos) {
                std::string block = content.substr(block_start, block_end - block_start + 1);
                AppConfig cfg;
                if (block.find("\"lite_mode\": \"on\"") != std::string::npos || block.find("\"lite_mode\":\"on\"") != std::string::npos)
                    cfg.lite_mode = "on";
                else if (block.find("\"lite_mode\": \"off\"") != std::string::npos || block.find("\"lite_mode\":\"off\"") != std::string::npos)
                    cfg.lite_mode = "off";
                else
                    cfg.lite_mode = "default";
                if (block.find("\"enable_dnd\": true") != std::string::npos || block.find("\"enable_dnd\":true") != std::string::npos)
                    cfg.enable_dnd = true;
                apps[key] = cfg;
            }
        }
        pos = end_pkg + 1;
    }
    return apps;
}

bool evaluate_lite_mode(const std::string& package_name, const std::map<std::string, AppConfig>& gamelist) {
    auto it = gamelist.find(package_name);
    if (it != gamelist.end()) {
        if (it->second.lite_mode == "on") return true;
        if (it->second.lite_mode == "off") return false;
    }
    return get_global_lite_mode();
}

int get_sf_index_by_fps(float target_fps) {
    std::string dump = exec_cmd("dumpsys display 2>/dev/null | grep -E 'id=[0-9]+.*fps=[0-9]'");
    std::istringstream stream(dump);
    std::string line;
    while (std::getline(stream, line)) {
        int sf_id = -1;
        float fps = 0;
        if (sscanf(line.c_str(), " {id=%d, width=%*d, height=%*d, fps=%f", &sf_id, &fps) == 2) {
            if (sf_id >= 0 && std::fabs(fps - target_fps) < 1.0f) return sf_id;
        }
        if (sscanf(line.c_str(), " id=%d, width=%*d, height=%*d, fps=%f", &sf_id, &fps) == 2) {
            if (sf_id >= 0 && std::fabs(fps - target_fps) < 1.0f) return sf_id;
        }
    }
    return -1;
}

int get_default_sf_index() {
    std::string dump = exec_cmd("dumpsys display 2>/dev/null");
    int default_id = -1;
    size_t pos = dump.find("defaultMode ");
    if (pos != std::string::npos)
        sscanf(dump.c_str() + pos, "defaultMode %d", &default_id);
    if (default_id < 0) default_id = get_sf_index_by_fps(60.0f);
    if (default_id < 0) default_id = 0;
    return default_id;
}

void apply_refresh_rate(const std::string& rate) {
    if (rate == "auto" || rate == "Auto" || rate == "0" || rate.empty()) {
        int idx = get_default_sf_index();
        exec_cmd("service call SurfaceFlinger 1035 i32 " + std::to_string(idx) + " 2>/dev/null");
        return;
    }

    float target_hz = 0;
    try { target_hz = std::stof(rate); } catch (...) { return; }

    int sf_id = get_sf_index_by_fps(target_hz);
    if (sf_id >= 0) {
        exec_cmd("service call SurfaceFlinger 1035 i32 " + std::to_string(sf_id) + " 2>/dev/null");
    }
}

void apply_render_engine(const std::string& engine) {
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

void disable_all_thermal() {
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

void enable_all_thermal() {
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

void apply_ram_tweaks(bool enable) {
    if (enable) {
        write_sysfs("/proc/sys/vm/swappiness", "30");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "60");
        write_sysfs("/proc/sys/vm/dirty_ratio", "15");
    } else {
        write_sysfs("/proc/sys/vm/swappiness", "60");
        write_sysfs("/proc/sys/vm/vfs_cache_pressure", "100");
        write_sysfs("/proc/sys/vm/dirty_ratio", "20");
    }
}

void apply_mtk_tweaks(bool enable) {
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

void apply_gpu_boost(int level) {
    if (!is_mediatek()) return;
    std::string path = "/sys/kernel/ged/hal/gpu_boost_level";
    if (fs::exists(path)) write_sysfs(path, std::to_string(level));
}

void apply_governor_all(const std::string& governor) {
    for (int i = 0; i < 8; i++) {
        std::string p = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
        if (fs::exists(p)) write_sysfs(p, governor);
    }
}

void apply_io_scheduler_all(const std::string& scheduler) {
    for (const auto& dev : std::vector<std::string>{"sda","sdb","sdc","sdd","mmcblk0","dm-0"}) {
        std::string p = "/sys/block/" + dev + "/queue/scheduler";
        if (fs::exists(p)) write_sysfs(p, scheduler);
    }
}

void apply_tcp_bbr(bool enable) {
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

void apply_logd_killer(bool enable) {
    exec_cmd(enable ? "stop logd 2>/dev/null" : "start logd 2>/dev/null");
}

void lock_all_clusters_max() {
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

void restore_all_clusters() {
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

void apply_cpu_limit_percent(int percent) {
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

int handle_apply_profile(const std::string& profile, bool is_lite_mode = false,
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
    return 0;
}

int handle_daemon() {
    send_notification("LUMina Tweaks", "Daemon Aktif");

    std::string current_profile = "";
    bool current_lite_state = false;

    bool last_ram_state = false;
    bool last_mtk_state = false;
    bool last_tcp_state = false;
    bool last_logd_state = false;
    bool last_global_lite_state = false;
    bool last_disable_thermal_state = false;
    std::string last_refresh_rate = "";
    std::string last_render_engine = "";
    int last_cpu_limit = -1;

    bool game_session_active = false;
    std::string locked_game_pkg = "";
    int idle_counter = 0;
    const int IDLE_TIMEOUT = 7;

    std::string gamelist_json_path = "/data/adb/.config/lumina/gamelist.json";
    std::string config_json_path   = "/data/adb/.config/lumina/config.json";
    std::string auto_flag_path     = "/data/adb/.config/lumina/auto_mode";
    fs::create_directories("/data/adb/.config/lumina");

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        bool current_global_lite = get_global_lite_mode();
        std::ifstream cfg_file(config_json_path);
        if (cfg_file.is_open()) {
            std::string cfg_str((std::istreambuf_iterator<char>(cfg_file)), std::istreambuf_iterator<char>());

            if (current_global_lite != last_global_lite_state) {
                last_global_lite_state = current_global_lite;
                current_profile = "";
            }

            bool ram_en = parse_json_bool(cfg_str, "ram_tweaks");
            if (ram_en != last_ram_state) { apply_ram_tweaks(ram_en); last_ram_state = ram_en; }

            bool mtk_en = parse_json_bool(cfg_str, "mtk_tweaks");
            if (mtk_en != last_mtk_state) { apply_mtk_tweaks(mtk_en); last_mtk_state = mtk_en; }

            bool tcp_en = parse_json_bool(cfg_str, "tcp_bbr");
            if (tcp_en != last_tcp_state) { apply_tcp_bbr(tcp_en); last_tcp_state = tcp_en; }

            bool logd_en = parse_json_bool(cfg_str, "disable_logd") || parse_json_bool(cfg_str, "logd_killer");
            if (logd_en != last_logd_state) { apply_logd_killer(logd_en); last_logd_state = logd_en; }

            bool thermal_dis = parse_json_bool(cfg_str, "disable_thermal");
            if (thermal_dis != last_disable_thermal_state) {
                last_disable_thermal_state = thermal_dis;
                if (current_profile == "performance") {
                    thermal_dis ? disable_all_thermal() : enable_all_thermal();
                }
            }

            std::string cur_hz = parse_json_string(cfg_str, "refresh_rate");
            if (!cur_hz.empty() && cur_hz != last_refresh_rate) {
                apply_refresh_rate(cur_hz);
                last_refresh_rate = cur_hz;
            }

            std::string cur_render = parse_json_string(cfg_str, "render_engine");
            if (!cur_render.empty() && cur_render != last_render_engine) {
                apply_render_engine(cur_render);
                last_render_engine = cur_render;
            }

            size_t pos = cfg_str.find("\"cpu_limit\":");
            if (pos == std::string::npos) pos = cfg_str.find("\"cpu_limit_percent\":");
            if (pos != std::string::npos) {
                size_t colon_pos = cfg_str.find(":", pos);
                if (colon_pos != std::string::npos) {
                    try {
                        int val = std::stoi(cfg_str.substr(colon_pos + 1));
                        if (val >= 50 && val <= 100 && val != last_cpu_limit) {
                            apply_cpu_limit_percent(val);
                            last_cpu_limit = val;
                        }
                    } catch (...) {}
                }
            }
        }

        if (fs::exists(auto_flag_path)) {
            std::string flag = read_sysfs(auto_flag_path);
            if (flag.find("0") != std::string::npos || flag.find("false") != std::string::npos) {
                current_profile = "";
                game_session_active = false;
                continue;
            }
        }

        std::string screen = exec_cmd("dumpsys power 2>/dev/null | grep -E 'mHoldingDisplaySuspendBlocker=true|Display Power: state=ON'");
        if (screen.empty()) {
            if (current_profile != "eco") {
                handle_apply_profile("eco");
                current_profile = "eco";
                current_lite_state = false;
                game_session_active = false;
                locked_game_pkg = "";
                idle_counter = 0;
            }
            continue;
        }

        std::string top_app = exec_cmd("cmd activity get-top-app 2>/dev/null | grep -E 'TASK|ACTIVITY'");
        if (top_app.empty())
            top_app = exec_cmd("dumpsys window 2>/dev/null | grep -E 'mCurrentFocus|mFocusedApp'");

        auto gamelist = load_gamelist_json(gamelist_json_path);
        bool is_game = false;
        std::string active_game_pkg = "";

        for (const auto& [pkg, cfg] : gamelist) {
            if (!pkg.empty() && top_app.find(pkg) != std::string::npos) {
                is_game = true; active_game_pkg = pkg; break;
            }
        }

        if (is_game) {
            bool target_lite = evaluate_lite_mode(active_game_pkg, gamelist);
            if (!game_session_active || locked_game_pkg != active_game_pkg) {
                game_session_active = true;
                locked_game_pkg = active_game_pkg;
                idle_counter = 0;
            } else idle_counter = 0;

            if (current_profile != "performance" || current_lite_state != target_lite) {
                handle_apply_profile("performance", target_lite, active_game_pkg);
                current_profile = "performance";
                current_lite_state = target_lite;
            }

        } else if (game_session_active) {
            std::string check = exec_cmd("pgrep -f " + locked_game_pkg + " 2>/dev/null");
            if (!check.empty()) {
                idle_counter++;
                if (idle_counter < IDLE_TIMEOUT) {
                    bool target_lite = evaluate_lite_mode(locked_game_pkg, gamelist);
                    if (current_profile != "performance" || current_lite_state != target_lite) {
                        handle_apply_profile("performance", target_lite, locked_game_pkg);
                        current_profile = "performance";
                        current_lite_state = target_lite;
                    }
                } else {
                    game_session_active = false; locked_game_pkg = ""; idle_counter = 0;
                    handle_apply_profile("balanced", current_global_lite, "", true);
                    current_profile = "balanced"; current_lite_state = current_global_lite;
                }
            } else {
                game_session_active = false; locked_game_pkg = ""; idle_counter = 0;
                handle_apply_profile("balanced", current_global_lite, "", true);
                current_profile = "balanced"; current_lite_state = current_global_lite;
            }

        } else {
            if (current_profile != "balanced" || current_lite_state != current_global_lite) {
                handle_apply_profile("balanced", current_global_lite);
                current_profile = "balanced"; current_lite_state = current_global_lite;
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string command = argv[1];

    if (command == "daemon" || command == "--daemon") return handle_daemon();
    if (command == "apply_tcp_bbr" && argc >= 3)         { apply_tcp_bbr(std::string(argv[2]) == "1"); return 0; }
    if (command == "apply_ram_tweaks" && argc >= 3)      { apply_ram_tweaks(std::string(argv[2]) == "1"); return 0; }
    if (command == "apply_mtk_tweaks" && argc >= 3)      { apply_mtk_tweaks(std::string(argv[2]) == "1"); return 0; }
    if (command == "apply_logd_killer" && argc >= 3)     { apply_logd_killer(std::string(argv[2]) == "1"); return 0; }
    if (command == "apply_gpu_boost" && argc >= 3)       { apply_gpu_boost(std::stoi(argv[2])); return 0; }
    if (command == "apply_disable_thermal" && argc >= 3) { std::string(argv[2]) == "1" ? disable_all_thermal() : enable_all_thermal(); return 0; }
    if ((command == "set_refresh_rate" || command == "set-rr") && argc >= 3) { apply_refresh_rate(argv[2]); return 0; }
    if (command == "set_render_engine" && argc >= 3)     { apply_render_engine(argv[2]); return 0; }

    if (command == "apply_cpu_limit" && argc >= 3) {
        apply_cpu_limit_percent(std::stoi(argv[2])); return 0;
    }
    if (command == "apply_profile" && argc >= 3) {
        return handle_apply_profile(argv[2], get_global_lite_mode());
    }
    if (command == "set_io_scheduler" && argc >= 3) {
        apply_io_scheduler_all(argv[2]); return 0;
    }
    if (command == "change_cpu_gov" && argc >= 3) {
        apply_governor_all(argv[2]); return 0;
    }
    if (command == "set_cpu_max" && argc >= 4) {
        std::string path = "/sys/devices/system/cpu/cpufreq/policy" + std::string(argv[2]) + "/scaling_max_freq";
        if (write_sysfs(path, argv[3])) { return 0; }
        else { return 1; }
    }

    return 0;
}
