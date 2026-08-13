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
#include "src/core/watcher.hpp"
#include "src/utils/sysfs.hpp"
#include "src/utils/notifier.hpp"
#include "src/utils/config_loader.hpp"
#include "src/tweaks/system_tweaks.hpp"

namespace fs = std::filesystem;

struct CachedConfig {
    bool global_lite_mode = false;
    bool disable_thermal = false;
    bool ram_tweaks = false;
    bool mtk_tweaks = false;
    bool tcp_bbr = false;
    bool logd_killer = false;
    int cpu_limit = 100;
    std::string render_engine = "";
};

static std::mutex g_cfg_mutex;
static CachedConfig g_cfg;
static std::atomic<bool> g_cfg_dirty{true};

static void reload_config(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string s((std::istreambuf_iterator<char>(f)), {});

    std::lock_guard<std::mutex> lk(g_cfg_mutex);
    g_cfg.global_lite_mode = parse_json_bool(s, "enforce_lite_mode");
    g_cfg.disable_thermal = parse_json_bool(s, "disable_thermal");
    g_cfg.ram_tweaks = parse_json_bool(s, "ram_tweaks");
    g_cfg.mtk_tweaks = parse_json_bool(s, "mtk_tweaks");
    g_cfg.tcp_bbr = parse_json_bool(s, "tcp_bbr");
    g_cfg.logd_killer = parse_json_bool(s, "disable_logd") ||
                        parse_json_bool(s, "logd_killer");
    g_cfg.render_engine = parse_json_string(s, "render_engine");

    size_t pos = s.find("\"cpu_limit\":");
    if (pos == std::string::npos) pos = s.find("\"cpu_limit_percent\":");
    if (pos != std::string::npos) {
        size_t col = s.find(":", pos);
        if (col != std::string::npos) {
            try {
                int v = std::stoi(s.substr(col + 1));
                if (v >= 50 && v <= 100) g_cfg.cpu_limit = v;
            } catch (...) {}
        }
    }

    g_cfg_dirty = false;
}

static CachedConfig get_cfg() {
    std::lock_guard<std::mutex> lk(g_cfg_mutex);
    return g_cfg;
}

static bool is_process_alive(const std::string& pkg) {
    for (const auto& entry : fs::directory_iterator("/proc")) {
        const auto& p = entry.path().filename().string();
        if (p.empty() || !std::isdigit(p[0])) continue;
        std::ifstream cmdline("/proc/" + p + "/cmdline");
        if (!cmdline) continue;
        std::string cmd;
        std::getline(cmdline, cmd, '\0');
        auto colon = cmd.find(':');
        if (colon != std::string::npos) cmd = cmd.substr(0, colon);
        if (cmd == pkg) return true;
    }
    return false;
}

static std::atomic<bool> g_game_detected{false};
static std::atomic<bool> g_daemon_running{true};
static std::string g_current_game_pkg;
static std::mutex g_game_mutex;

int handle_daemon() {
    send_notification("LUMina Tweaks", "Daemon Aktif");

    const std::string gamelist_path = "/data/adb/.config/lumina/gamelist.json";
    const std::string config_path = "/data/adb/.config/lumina/config.json";
    const std::string auto_flag = "/data/adb/.config/lumina/auto_mode";

    fs::create_directories("/data/adb/.config/lumina");

    reload_config(config_path);

    CachedConfig prev_cfg = get_cfg();
    apply_ram_tweaks(prev_cfg.ram_tweaks);
    apply_mtk_tweaks(prev_cfg.mtk_tweaks);
    apply_tcp_bbr(prev_cfg.tcp_bbr);
    apply_logd_killer(prev_cfg.logd_killer);
    if (!prev_cfg.render_engine.empty())
        apply_render_engine(prev_cfg.render_engine);

    std::string current_profile = "";
    bool current_lite = false;
    bool game_session = false;
    std::string locked_pkg = "";
    int idle_counter = 0;
    const int IDLE_TIMEOUT = 7;

    GameMatcher matcher(gamelist_path);

    AppWatcher watcher([&](const std::string& pkg) {
        std::lock_guard<std::mutex> lk(g_game_mutex);
        if (matcher.is_game(pkg)) {
            g_game_detected = true;
            g_current_game_pkg = pkg;
        } else {
            g_game_detected = false;
            g_current_game_pkg = "";
        }
    });

    watcher.start();

    while (g_daemon_running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (g_cfg_dirty) {
            reload_config(config_path);
        }

        auto cfg = get_cfg();

        if (cfg.ram_tweaks != prev_cfg.ram_tweaks) {
            apply_ram_tweaks(cfg.ram_tweaks);
            prev_cfg.ram_tweaks = cfg.ram_tweaks;
        }
        if (cfg.mtk_tweaks != prev_cfg.mtk_tweaks) {
            apply_mtk_tweaks(cfg.mtk_tweaks);
            prev_cfg.mtk_tweaks = cfg.mtk_tweaks;
        }
        if (cfg.tcp_bbr != prev_cfg.tcp_bbr) {
            apply_tcp_bbr(cfg.tcp_bbr);
            prev_cfg.tcp_bbr = cfg.tcp_bbr;
        }
        if (cfg.logd_killer != prev_cfg.logd_killer) {
            apply_logd_killer(cfg.logd_killer);
            prev_cfg.logd_killer = cfg.logd_killer;
        }
        if (cfg.render_engine != prev_cfg.render_engine) {
            apply_render_engine(cfg.render_engine);
            prev_cfg.render_engine = cfg.render_engine;
        }
        if (cfg.global_lite_mode != prev_cfg.global_lite_mode) {
            prev_cfg.global_lite_mode = cfg.global_lite_mode;
            current_profile = "";
        }
        if (cfg.disable_thermal != prev_cfg.disable_thermal) {
            prev_cfg.disable_thermal = cfg.disable_thermal;
            if (current_profile == "performance") {
                cfg.disable_thermal ? disable_all_thermal() : enable_all_thermal();
            }
        }

        if (fs::exists(auto_flag)) {
            std::string flag = read_sysfs(auto_flag);
            if (flag.find("0") != std::string::npos ||
                flag.find("false") != std::string::npos) {
                current_profile = "";
                game_session = false;
                continue;
            }
        }

        if (!is_screen_on()) {
            if (current_profile != "eco") {
                handle_apply_profile("eco");
                current_profile = "eco";
                current_lite = false;
                game_session = false;
                locked_pkg = "";
                idle_counter = 0;
            }
            continue;
        }

        bool is_game = false;
        std::string active_pkg = "";

        {
            std::lock_guard<std::mutex> lk(g_game_mutex);
            is_game = g_game_detected.load();
            active_pkg = g_current_game_pkg;
        }

        if (is_game && !active_pkg.empty()) {
            bool target_lite = matcher.get_lite_mode(active_pkg, cfg.global_lite_mode);

            if (!game_session || locked_pkg != active_pkg) {
                game_session = true;
                locked_pkg = active_pkg;
                idle_counter = 0;
            } else {
                idle_counter = 0;
            }

            if (current_profile != "performance" || current_lite != target_lite) {
                handle_apply_profile("performance", target_lite, active_pkg);
                current_profile = "performance";
                current_lite = target_lite;
            }

        } else if (game_session) {
            if (is_process_alive(locked_pkg)) {
                idle_counter++;
                if (idle_counter < IDLE_TIMEOUT) {
                    bool target_lite = matcher.get_lite_mode(locked_pkg, cfg.global_lite_mode);
                    if (current_profile != "performance" || current_lite != target_lite) {
                        handle_apply_profile("performance", target_lite, locked_pkg);
                        current_profile = "performance";
                        current_lite = target_lite;
                    }
                } else {
                    game_session = false;
                    locked_pkg = "";
                    idle_counter = 0;
                    handle_apply_profile("balanced", cfg.global_lite_mode, "", true);
                    current_profile = "balanced";
                    current_lite = cfg.global_lite_mode;
                }
            } else {
                game_session = false;
                locked_pkg = "";
                idle_counter = 0;
                handle_apply_profile("balanced", cfg.global_lite_mode, "", true);
                current_profile = "balanced";
                current_lite = cfg.global_lite_mode;
            }

        } else {
            if (current_profile != "balanced" || current_lite != cfg.global_lite_mode) {
                handle_apply_profile("balanced", cfg.global_lite_mode);
                current_profile = "balanced";
                current_lite = cfg.global_lite_mode;
            }
        }
    }

    watcher.stop();
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string cmd = argv[1];

    if (cmd == "daemon" || cmd == "--daemon") return handle_daemon();

    if (cmd == "apply_tcp_bbr" && argc >= 3) { apply_tcp_bbr(std::string(argv[2]) == "1"); return 0; }
    if (cmd == "apply_ram_tweaks" && argc >= 3) { apply_ram_tweaks(std::string(argv[2]) == "1"); return 0; }
    if (cmd == "apply_mtk_tweaks" && argc >= 3) { apply_mtk_tweaks(std::string(argv[2]) == "1"); return 0; }
    if (cmd == "apply_logd_killer" && argc >= 3) { apply_logd_killer(std::string(argv[2]) == "1"); return 0; }
    if (cmd == "apply_gpu_boost" && argc >= 3) { apply_gpu_boost(std::stoi(argv[2])); return 0; }
    if (cmd == "apply_disable_thermal" && argc >= 3) { std::string(argv[2]) == "1" ? disable_all_thermal() : enable_all_thermal(); return 0; }
    if (cmd == "set_render_engine" && argc >= 3) { apply_render_engine(argv[2]); return 0; }
    if (cmd == "apply_cpu_limit" && argc >= 3) { apply_cpu_limit_percent(std::stoi(argv[2])); return 0; }
    if (cmd == "apply_profile" && argc >= 3) { return handle_apply_profile(argv[2], get_cfg().global_lite_mode); }
    if (cmd == "set_io_scheduler" && argc >= 3) { apply_io_scheduler_all(argv[2]); return 0; }
    if (cmd == "change_cpu_gov" && argc >= 3) { apply_governor_all(argv[2]); return 0; }
    if (cmd == "reload_config") { g_cfg_dirty = true; return 0; }

    if (cmd == "set_cpu_max" && argc >= 4) {
        std::string path = "/sys/devices/system/cpu/cpufreq/policy"
                         + std::string(argv[2]) + "/scaling_max_freq";
        return write_sysfs(path, argv[3]) ? 0 : 1;
    }

    return 0;
}