#include "display.hpp"
#include <iostream>
#include <array>
#include <memory>
#include <sstream>
#include <regex>

std::string exec_cmd(const std::string& cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int get_sf_index_by_fps(float target_fps) {
    std::string dump = exec_cmd("dumpsys display 2>/dev/null");
    if (dump.empty()) return -1;

    std::istringstream stream(dump);
    std::string line;
    int target_int = static_cast<int>(target_fps);
    std::regex id_regex(R"(id=([0-9]+))");

    while (std::getline(stream, line)) {
        if (line.find("id=") != std::string::npos) {
            bool match_fps = (line.find("peakRefreshRate=" + std::to_string(target_int)) != std::string::npos) ||
                             (line.find("fps=" + std::to_string(target_int)) != std::string::npos);

            if (match_fps) {
                std::smatch match;
                if (std::regex_search(line, match, id_regex) && match.size() > 1) {
                    return std::stoi(match[1].str());
                }
            }
        }
    }
    return -1;
}

bool set_refresh_rate(float target_fps) {
    int idx = get_sf_index_by_fps(target_fps);

    if (idx >= 0) {
        std::string sf_cmd = "service call SurfaceFlinger 1035 i32 " + std::to_string(idx) + " >/dev/null 2>&1";
        exec_cmd(sf_cmd);
        std::cout << "[Display] " << target_fps << "Hz -> SF index " << idx << "\n";
        return true;
    } else {
        std::string fallback_cmd = "cmd display set-user-preferred-display-mode 0 0 " + std::to_string(target_fps) + ".0 2>/dev/null";
        exec_cmd(fallback_cmd);
        std::cout << "[Display] SF index not found for " << target_fps << "Hz, fallback applied.\n";
        return false;
    }
}
