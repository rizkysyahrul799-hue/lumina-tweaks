#pragma once

#include <string>
#include <unordered_set>
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
        std::ifstream file(json_path_);
        if (!file.is_open()) return;

        try {
            json data = json::parse(file);
            for (auto& [pkg, config] : data.items()) {
                game_list_.insert(pkg);
            }
        } catch (...) {}
    }

    bool is_game(const std::string& pkg) {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_list_.find(pkg) != game_list_.end();
    }

private:
    std::string json_path_;
    std::unordered_set<std::string> game_list_;
    std::mutex mutex_;
};
