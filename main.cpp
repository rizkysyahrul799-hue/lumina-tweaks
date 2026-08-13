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
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "src/core/game_matcher.hpp"
#include "src/core/watcher.cpp"
#include "src/utils/sysfs.hpp"
#include "src/utils/notifier.hpp"
#include "src/utils/config_loader.hpp"
#include "src/tweaks/system_tweaks.hpp"


std::atomic<bool> game_detected{false};
std::atomic<bool> daemon_running{true};
std::string current_game_pkg = "";
std::mutex game_mutex;

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

    GameMatcher matcher(gamelist_json_path);

    AppWatcher watcher([&](const std::string& pkg) {
        std::lock_guard<std::mutex> lock(game_mutex);
        if (matcher.is_game(pkg)) {
            game_detected = true;
            current_game_pkg = pkg;
            std::cout << "Game Detected: " << pkg << " -> Applying Performance Profile" << std::endl;
        } else {
            game_detected = false;
            current_game_pkg = "";
            std::cout << "Normal App: " << pkg << " -> Restoring Default Profile" << std::endl;
        }
    });

    watcher.start();

    while (daemon_running) {
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
            if (ram_en != last_ram_state) {
                last_ram_state = ram_en;
                if (!current_profile.empty()) {
                    apply_ram_tweaks_for_profile(current_profile);
                }
            }

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

        if (!is_screen_on()) {
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

        bool is_game = false;
        std::string active_game_pkg = "";

        {
            std::lock_guard<std::mutex> lock(game_mutex);
            is_game = game_detected;
            active_game_pkg = current_game_pkg;
        }

        if (active_game_pkg.empty()) {
            std::string top_app = exec_cmd("cmd activity get-top-app 2>/dev/null | grep -E 'TASK|ACTIVITY'");
            if (top_app.empty())
                top_app = exec_cmd("dumpsys window 2>/dev/null | grep -E 'mCurrentFocus|mFocusedApp'");

            auto gamelist = load_gamelist_json(gamelist_json_path);

            for (const auto& [pkg, cfg] : gamelist) {
                if (!pkg.empty() && top_app.find(pkg) != std::string::npos) {
                    is_game = true;
                    active_game_pkg = pkg;
                    break;
                }
            }
        }

        if (is_game && !active_game_pkg.empty()) {
            auto gamelist = load_gamelist_json(gamelist_json_path);
            bool target_lite = evaluate_lite_mode(active_game_pkg, gamelist);

            if (!game_session_active || locked_game_pkg != active_game_pkg) {
                game_session_active = true;
                locked_game_pkg = active_game_pkg;
                idle_counter = 0;
            } else {
                idle_counter = 0;
            }

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
                    auto gamelist = load_gamelist_json(gamelist_json_path);
                    bool target_lite = evaluate_lite_mode(locked_game_pkg, gamelist);
                    if (current_profile != "performance" || current_lite_state != target_lite) {
                        handle_apply_profile("performance", target_lite, locked_game_pkg);
                        current_profile = "performance";
                        current_lite_state = target_lite;
                    }
                } else {
                    game_session_active = false;
                    locked_game_pkg = "";
                    idle_counter = 0;
                    handle_apply_profile("balanced", current_global_lite, "", true);
                    current_profile = "balanced";
                    current_lite_state = current_global_lite;
                }
            } else {
                game_session_active = false;
                locked_game_pkg = "";
                idle_counter = 0;
                handle_apply_profile("balanced", current_global_lite, "", true);
                current_profile = "balanced";
                current_lite_state = current_global_lite;
            }

        } else {
            if (current_profile != "balanced" || current_lite_state != current_global_lite) {
                handle_apply_profile("balanced", current_global_lite);
                current_profile = "balanced";
                current_lite_state = current_global_lite;
            }
        }
    }

    watcher.stop();
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