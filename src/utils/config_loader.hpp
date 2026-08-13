#ifndef LUMINA_CONFIG_LOADER_HPP
#define LUMINA_CONFIG_LOADER_HPP

#include <string>
#include <map>
#include <fstream>

struct AppConfig {
    std::string lite_mode = "default";
    bool enable_dnd = false;
};

inline bool parse_json_bool(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) pos = json.find("\"" + key + "\" :");
    if (pos == std::string::npos) return false;
    std::string sub = json.substr(pos + key.length() + 2, 10);
    return (sub.find("true") != std::string::npos);
}

inline std::string parse_json_string(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) pos = json.find("\"" + key + "\" :");
    if (pos == std::string::npos) return "";
    size_t start_quote = json.find("\"", pos + key.length() + 2);
    if (start_quote == std::string::npos) return "";
    size_t end_quote = json.find("\"", start_quote + 1);
    if (end_quote == std::string::npos) return "";
    return json.substr(start_quote + 1, end_quote - start_quote - 1);
}

inline bool get_global_lite_mode() {
    std::string path = "/data/adb/.config/lumina/config.json";
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_json_bool(content, "enforce_lite_mode");
}

inline bool get_disable_thermal_config() {
    std::string path = "/data/adb/.config/lumina/config.json";
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_json_bool(content, "disable_thermal");
}

inline std::map<std::string, AppConfig> load_gamelist_json(const std::string& path) {
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

inline bool evaluate_lite_mode(const std::string& package_name, const std::map<std::string, AppConfig>& gamelist) {
    auto it = gamelist.find(package_name);
    if (it != gamelist.end()) {
        if (it->second.lite_mode == "on") return true;
        if (it->second.lite_mode == "off") return false;
    }
    return get_global_lite_mode();
}

#endif
