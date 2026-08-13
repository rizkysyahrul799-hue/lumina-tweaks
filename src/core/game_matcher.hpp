#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class GameMatcher {
public:
    GameMatcher(const std::string& path) : json_path_(path) {
        load();
    }

    void load() {
        std::lock_guard<std::mutex> lock(mutex_);
        game_list_.clear();
        lite_modes_.clear();

        std::ifstream file(json_path_);
        if (!file.is_open()) return;

        try {
            json data = json::parse(file);
            for (auto& [pkg, config] : data.items()) {
                game_list_.insert(pkg);
                if (config.contains("lite_mode"))
                    lite_modes_[pkg] = config["lite_mode"].get<std::string>();
            }
        } catch (...) {}
    }

    void reload() { load(); }

    bool is_game(const std::string& pkg) {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_list_.count(pkg) > 0;
    }

    bool get_lite_mode(const std::string& pkg, bool global_default) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lite_modes_.find(pkg);
        if (it != lite_modes_.end()) {
            if (it->second == "on")  return true;
            if (it->second == "off") return false;
        }
        return global_default;
    }

private:
    std::string json_path_;
    std::unordered_set<std::string>         game_list_;
    std::unordered_map<std::string, std::string> lite_modes_;
    std::mutex mutex_;
};
