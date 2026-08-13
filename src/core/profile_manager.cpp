#include "profile_manager.hpp"
#include <cstdlib>

ProfileManager::ProfileManager() : current_profile_(Profile::NORMAL) {}

void ProfileManager::write_node(const std::string& path, const std::string& value) {

    std::string cmd = "echo " + value + " > " + path;
    system(cmd.c_str());
}

void ProfileManager::apply_profile(Profile profile) {


    if (profile == Profile::PERFORMANCE) {
        apply_performance();
    } else {
        apply_normal();
    }

    current_profile_ = profile;
}

void ProfileManager::apply_performance() {
    write_node("/proc/sys/vm/swappiness", "10");
    write_node("/proc/sys/vm/vfs_cache_pressure", "50");
}

void ProfileManager::apply_normal() {
    write_node("/proc/sys/vm/swappiness", "60");
    write_node("/proc/sys/vm/vfs_cache_pressure", "100");
}
