#ifndef LUMINA_SYSFS_HPP
#define LUMINA_SYSFS_HPP

#include <fstream>
#include <string>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

inline bool write_sysfs(const std::string& path, const std::string& value) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << value;
    return true;
}

inline std::string read_sysfs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content, line;
    while (std::getline(file, line)) content += line + "\n";
    return content;
}

inline std::string exec_cmd(const std::string& cmd) {
    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);
    return result;
}

inline bool is_mediatek() {
    std::string vendor = read_sysfs("/sys/devices/soc0/vendor");
    if (vendor.find("MediaTek") != std::string::npos || vendor.find("MTK") != std::string::npos) return true;
    std::string platform = exec_cmd("getprop ro.board.platform 2>/dev/null");
    if (platform.find("mt") == 0 || platform.find("MT") == 0 || platform.find("mediatek") != std::string::npos) return true;
    return false;
}

inline bool is_screen_on() {
    std::string bright = read_sysfs("/sys/class/backlight/panel0-backlight/brightness");
    if (!bright.empty() && bright != "0\n" && bright != "0") return true;

    std::string screen = exec_cmd("dumpsys display 2>/dev/null | grep -E 'mScreenState=ON|state=ON|mWakefulness=Awake'");
    return !screen.empty();
}

#endif